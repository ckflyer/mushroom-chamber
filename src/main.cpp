// =============================================================================
//  Mushroom Fruiting Chamber Controller
//
//  Everything runs on the ESP32. No Home Assistant required, no computer to
//  leave running. Open the board's IP in a browser and that is the whole
//  interface. Home Assistant sensors are an optional extra over MQTT.
//
//  First boot: join the "Chamber-Setup" WiFi network and pick your network.
// =============================================================================
#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiManager.h>
#include <ESPAsyncWebServer.h>
#include <AsyncJson.h>
#include <Update.h>
#include <esp_task_wdt.h>

#include "config.h"
#include "fogger.h"
#include "control.h"
#include "ha_mqtt.h"
#include "web_ui.h"

Config cfg;
State st;
History hist;

AsyncWebServer server(80);
uint32_t lastTick = 0;
volatile bool shouldReboot = false;

// ---- API -------------------------------------------------------------------

static void sendState(AsyncWebServerRequest* req) {
  JsonDocument d;
  if (isnan(st.rh)) d["rh"] = nullptr; else d["rh"] = st.rh;
  if (isnan(st.temp)) d["temp"] = nullptr; else d["temp"] = st.temp;
  if (isnan(st.dewPoint)) d["dew"] = nullptr; else d["dew"] = st.dewPoint;
  if (isnan(st.vpd)) d["vpd"] = nullptr; else d["vpd"] = st.vpd;
  d["rpm"] = st.rpm;
  d["fan"] = st.fanSpeed;
  d["fanReason"] = st.fanReason;
  d["status"] = ctrl::statusText();
  d["fogState"] = st.fogState;
  d["fogUsed"] = (int)(cfg.fogBudgetS - st.fogCredit);
  d["fault"] = st.sensorFault;
  d["ceiling"] = st.atCeiling;
  d["mqtt"] = ha::mqtt.connected();
  d["uptime"] = millis() / 1000;
  d["heap"] = ESP.getFreeHeap();
  d["version"] = FW_VERSION;

  String out;
  serializeJson(d, out);
  req->send(200, "application/json", out);
}

static void sendConfig(AsyncWebServerRequest* req) {
  JsonDocument d;
  JsonObject o = d.to<JsonObject>();
  configToJson(o);
  String out;
  serializeJson(d, out);
  req->send(200, "application/json", out);
}

static void sendHistory(AsyncWebServerRequest* req) {
  // Oldest first, so the browser can draw it left to right without thinking.
  String out = "{\"rh\":[";
  uint16_t start = (hist.count < HIST_LEN)
                   ? 0 : hist.head;
  for (uint16_t i = 0; i < hist.count; i++) {
    uint16_t idx = (start + i) % HIST_LEN;
    if (i) out += ',';
    out += String(hist.rh[idx] / 10.0f, 1);
  }
  out += "]}";
  req->send(200, "application/json", out);
}

static void setupRoutes() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* r) {
    AsyncWebServerResponse* res =
      r->beginResponse_P(200, "text/html", (const uint8_t*)INDEX_HTML,
                         strlen_P(INDEX_HTML));
    res->addHeader("Cache-Control", "no-store");
    r->send(res);
  });

  server.on("/api/state", HTTP_GET, sendState);
  server.on("/api/config", HTTP_GET, sendConfig);
  server.on("/api/history", HTTP_GET, sendHistory);

  auto* cfgHandler = new AsyncCallbackJsonWebHandler(
    "/api/config", [](AsyncWebServerRequest* req, JsonVariant& json) {
      configFromJson(json.as<JsonObjectConst>());
      configSave();
      // A mode change should take effect immediately, not on the next burst.
      if (cfg.foggerMode == FOG_NONE) fog::allOff();
      sendConfig(req);
    });
  server.addHandler(cfgHandler);

  server.on("/api/fog-burst", HTTP_POST, [](AsyncWebServerRequest* r) {
    st.forceBurst = true;
    r->send(200, "application/json", "{\"ok\":true}");
  });

  server.on("/api/fan-test", HTTP_POST, [](AsyncWebServerRequest* r) {
    st.fanTestS = 15;
    r->send(200, "application/json", "{\"ok\":true}");
  });

  server.on("/api/fan", HTTP_POST, [](AsyncWebServerRequest* r) {
    if (r->hasParam("speed")) {
      int v = r->getParam("speed")->value().toInt();
      st.manualFanSpeed = constrain(v, 0, 100);
    }
    r->send(200, "application/json", "{\"ok\":true}");
  });

  server.on("/api/restart", HTTP_POST, [](AsyncWebServerRequest* r) {
    r->send(200, "application/json", "{\"ok\":true}");
    fog::allOff();
    delay(200);
    ESP.restart();
  });

  // ---- Firmware update over the air -------------------------------------
  // The browser POSTs a .bin here. The old firmware keeps running from one
  // flash slot while the new one is written to the other, so a failed or
  // abandoned upload leaves the board exactly as it was.
  server.on("/api/update", HTTP_POST,
    [](AsyncWebServerRequest* r) {
      bool ok = !Update.hasError();
      AsyncWebServerResponse* res = r->beginResponse(
        ok ? 200 : 500, "application/json",
        ok ? "{\"ok\":true}" : "{\"ok\":false,\"error\":\"write failed\"}");
      res->addHeader("Connection", "close");
      r->send(res);
      st.updating = false;
      if (ok) shouldReboot = true;
    },
    [](AsyncWebServerRequest* r, String filename, size_t index,
       uint8_t* data, size_t len, bool final) {
      if (index == 0) {
        Serial.printf("[ota] receiving %s\n", filename.c_str());
        // Everything off before we touch flash. A half-written update must
        // never leave the fogger running.
        st.updating = true;
        fog::allOff();
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
          Update.printError(Serial);
          st.updating = false;
          return;
        }
      }
      if (Update.isRunning() && Update.write(data, len) != len) {
        Update.printError(Serial);
      }
      if (final) {
        if (Update.end(true)) Serial.printf("[ota] wrote %u bytes\n",
                                            (unsigned)(index + len));
        else Update.printError(Serial);
      }
    });

  server.onNotFound([](AsyncWebServerRequest* r) {
    r->send(404, "text/plain", "Not found");
  });
}

// ---- Boot ------------------------------------------------------------------

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n[boot] mushroom chamber");

  configLoad();

  // Outputs safe before anything else can go wrong.
  fog::begin();
  fog::allOff();
  ctrl::begin();

  WiFiManager wm;
  wm.setConfigPortalTimeout(180);
  if (!wm.autoConnect("Chamber-Setup")) {
    // No WiFi is not a reason to stop growing mushrooms. Carry on headless;
    // the control loop does not need the network.
    Serial.println("[wifi] no connection, running offline");
  } else {
    Serial.print("[wifi] ip ");
    Serial.println(WiFi.localIP());
    if (MDNS.begin("chamber")) Serial.println("[wifi] http://chamber.local");
  }

  ha::begin();
  setupRoutes();
  server.begin();

  // If the loop ever wedges, reboot rather than leave the fogger stuck on.
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  esp_task_wdt_config_t wdt = { .timeout_ms = 30000,
                                .idle_core_mask = 0, .trigger_panic = true };
  esp_task_wdt_reconfigure(&wdt);
#else
  esp_task_wdt_init(30, true);
#endif
  esp_task_wdt_add(NULL);

  Serial.println("[boot] ready");
}

void loop() {
  if (shouldReboot) {
    Serial.println("[ota] restarting into new firmware");
    fog::allOff();
    delay(600);
    ESP.restart();
  }

  uint32_t now = millis();
  if (now - lastTick >= 1000) {
    lastTick += 1000;
    if (now - lastTick > 5000) lastTick = now;  // recover from a long stall
    ctrl::tick();
    esp_task_wdt_reset();
  }
  ha::loop();
  delay(2);
}
