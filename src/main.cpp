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
#include <time.h>
#include <WiFiManager.h>
#include <ESPAsyncWebServer.h>
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

// Reconnect quietly in the background if WiFi drops or never came up.
// The chamber keeps running either way; this only restores the dashboard.
static void wifiWatch() {
  static uint32_t last = 0;
  static bool wasUp = false;
  bool up = WiFi.status() == WL_CONNECTED;

  if (up && !wasUp) {
    Serial.print("[wifi] connected, ip ");
    Serial.println(WiFi.localIP());
    if (MDNS.begin(HOST_NAME)) MDNS.addService("http", "tcp", 80);
  }
  wasUp = up;

  if (!up && millis() - last > 30000) {
    last = millis();
    Serial.printf("[wifi] retrying, status %d\n", WiFi.status());
    WiFi.disconnect();
    WiFi.begin();
  }
}

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
  d["dry"] = st.reservoirLow;
  if (isnan(st.lastRise)) d["lastRise"] = nullptr; else d["lastRise"] = st.lastRise;
  time_t now = time(nullptr);
  d["epoch"] = (now > 1700000000) ? (uint32_t)now : 0;   // 0 = clock not set
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
      r->beginResponse(200, "text/html", (const uint8_t*)INDEX_HTML,
                       strlen_P(INDEX_HTML));
    res->addHeader("Cache-Control", "no-store");
    r->send(res);
  });

  server.on("/api/state", HTTP_GET, sendState);
  server.on("/api/config", HTTP_GET, sendConfig);
  server.on("/api/history", HTTP_GET, sendHistory);

  server.on("/api/presets", HTTP_GET, [](AsyncWebServerRequest* r) {
    r->send(200, "application/json", presetsLoad());
  });

  // Save or delete a named preset. Body: {"name":"...","v":{...}} or
  // {"name":"...","delete":true}
  server.on("/api/presets", HTTP_POST,
    [](AsyncWebServerRequest* r) { /* answered from the body handler */ },
    NULL,
    [](AsyncWebServerRequest* r, uint8_t* data, size_t len,
       size_t index, size_t total) {
      static String body;
      if (index == 0) { body = ""; body.reserve(total + 1); }
      for (size_t i = 0; i < len; i++) body += (char)data[i];
      if (index + len < total) return;

      JsonDocument d;
      DeserializationError err = deserializeJson(d, body);
      body = "";
      if (err) {
        r->send(400, "application/json", "{\"ok\":false}");
        return;
      }
      String name = d["name"] | "";
      name.trim();
      if (!name.length() || name.length() > 28) {
        r->send(400, "application/json", "{\"ok\":false}");
        return;
      }
      bool del = d["delete"] | false;
      String out = presetsApply(name, d["v"].as<JsonObjectConst>(), del);
      r->send(200, "application/json", out);
    });

  // Settings come in as a JSON body. Parsed by hand rather than with
  // AsyncCallbackJsonWebHandler, whose constructor signature changes between
  // ESPAsyncWebServer releases.
  server.on("/api/config", HTTP_POST,
    [](AsyncWebServerRequest* r) { /* answered from the body handler */ },
    NULL,
    [](AsyncWebServerRequest* r, uint8_t* data, size_t len,
       size_t index, size_t total) {
      static String body;
      if (index == 0) { body = ""; body.reserve(total + 1); }
      for (size_t i = 0; i < len; i++) body += (char)data[i];
      if (index + len < total) return;   // more chunks still coming

      JsonDocument d;
      DeserializationError err = deserializeJson(d, body);
      body = "";
      if (err) {
        r->send(400, "application/json", "{\"ok\":false,\"error\":\"bad json\"}");
        return;
      }
      configFromJson(d.as<JsonObjectConst>());
      configSave();
      // A mode change should take effect immediately, not on the next burst.
      if (cfg.foggerMode == FOG_NONE) fog::allOff();
      sendConfig(r);
    });

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
  // The browser PUTs the raw .bin as the request body. No multipart parsing,
  // which is the part that tends to stall on the async server.
  // The old firmware keeps running from one flash slot while the new one is
  // written to the other, so a failed upload leaves the board as it was.
  server.on("/api/update", HTTP_POST,
    [](AsyncWebServerRequest* r) {
      bool ok = !Update.hasError() && Update.isFinished();
      AsyncWebServerResponse* res = r->beginResponse(
        ok ? 200 : 500, "application/json",
        ok ? "{\"ok\":true}" : "{\"ok\":false}");
      res->addHeader("Connection", "close");
      r->send(res);
      st.updating = false;
      if (ok) shouldReboot = true;
      else Serial.println("[ota] failed, still on the old firmware");
    },
    NULL,
    [](AsyncWebServerRequest* r, uint8_t* data, size_t len,
       size_t index, size_t total) {
      if (index == 0) {
        Serial.printf("[ota] start, %u bytes\n", (unsigned)total);
        st.updating = true;
        fog::allOff();
        detachInterrupt(digitalPinToInterrupt(PIN_FAN_TACH));
        if (!Update.begin(total ? total : UPDATE_SIZE_UNKNOWN)) {
          Update.printError(Serial);
          st.updating = false;
          return;
        }
      }
      if (!Update.isRunning()) return;

      if (Update.write(data, len) != len) {
        Update.printError(Serial);
        Update.abort();
        return;
      }
      if ((index / 65536) != ((index + len) / 65536))
        Serial.printf("[ota] %u / %u\n", (unsigned)(index + len), (unsigned)total);

      if (index + len >= total) {
        if (Update.end(true)) Serial.println("[ota] write complete");
        else Update.printError(Serial);
      }
    });

  server.onNotFound([](AsyncWebServerRequest* r) {
    r->send(404, "text/plain", "Not found");
  });
}

// ---- Boot ------------------------------------------------------------------

void setup() {
  // Before anything else. An unconfigured pin floats: the relay can click on,
  // and a floating PWM line reads as 100% duty to the fan, so you get full
  // blast between reset and ledcAttach. Relay polarity is not known until
  // configLoad() runs a few lines down, so ctrl::begin() re-asserts the off
  // state once it is.
  pinMode(PIN_FAN_PWM, OUTPUT);
  digitalWrite(PIN_FAN_PWM, LOW);
  pinMode(PIN_FAN_POWER, OUTPUT);
  digitalWrite(PIN_FAN_POWER, LOW);

  Serial.begin(115200);
  delay(200);
  Serial.println("\n[boot] mushroom chamber");

  configLoad();

  // Outputs safe before anything else can go wrong.
  fog::begin();
  fog::allOff();
  ctrl::begin();

  WiFi.mode(WIFI_STA);
  WiFi.setHostname(HOST_NAME);   // must be set before connecting
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);

  // The driver knows exactly why a join failed. Ask it.
  WiFi.onEvent([](WiFiEvent_t, WiFiEventInfo_t info) {
    uint8_t r = info.wifi_sta_disconnected.reason;
    const char* why = "see the espressif reason code list";
    switch (r) {
      case 2:   why = "auth expired"; break;
      case 4:   why = "association expired"; break;
      case 15:  why = "handshake timeout - wrong password, or WPA3"; break;
      case 201: why = "network not found - 2.4GHz only, check the name"; break;
      case 202: why = "auth failed - wrong password"; break;
      case 203: why = "association failed"; break;
      case 204: why = "handshake timeout"; break;
      case 205: why = "connection failed"; break;
    }
    Serial.printf("[wifi] disconnected, reason %u: %s\n", r, why);
  }, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);

  // WiFiManager's portal owns port 80. It is scoped in a block so its
  // destructor runs and releases the port BEFORE our server binds it,
  // otherwise server.begin() fails with "bind error: -8" and there is no
  // dashboard.
  bool wifiOk = false;
  {
    WiFiManager wm;
    wm.setDebugOutput(true);
    wm.setConfigPortalTimeout(180);
    wm.setConnectTimeout(25);
    // One attempt only. WiFiManager fires its retries before the previous
    // attempt has finished, which the driver rejects with 0x3007. The
    // background watcher below retries properly, spaced out.
    wm.setConnectRetries(1);

    wifiOk = wm.autoConnect("Chamber-Setup");
    wm.stopConfigPortal();
    wm.stopWebPortal();
  }
  delay(300);   // let the sockets actually close

  if (!wifiOk) {
    // No WiFi is not a reason to stop growing mushrooms. Carry on headless;
    // the control loop does not need the network.
    Serial.printf("[wifi] not connected, status %d, running offline\n",
                  WiFi.status());
    Serial.println("[wifi]  3 = connected   4 = wrong password or rejected");
    Serial.println("[wifi]  1 = no such network (2.4GHz only, check the name)");
    Serial.println("[wifi]  6 = disconnected");
    WiFi.mode(WIFI_STA);
    WiFi.begin();                // keep retrying quietly in the background
  } else {
    Serial.print("[wifi] ip ");
    Serial.println(WiFi.localIP());
    if (MDNS.begin(HOST_NAME)) {
      MDNS.addService("http", "tcp", 80);
      Serial.println("[wifi] http://" HOST_NAME ".local");
    }
  }

  // Only used to put real times on the chart. Harmless if it never syncs.
  configTzTime(cfg.tz.c_str(), "pool.ntp.org", "time.nist.gov");

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
    wifiWatch();
    esp_task_wdt_reset();
  }
  ha::loop();
  delay(2);
}
