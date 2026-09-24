// =============================================================================
//  config.h - pins, tunable settings, and saving them to flash
// =============================================================================
#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <Preferences.h>

// Bump this on every release. Shown in the dashboard so people can tell
// whether they are running the latest build.
#define FW_VERSION "1.7.0"

// Reachable at http://<this>.local, and how the board names itself to your
// router. Letters, digits and hyphens only.
#define HOST_NAME "mushroom"


// ---- Pins (change these if your wiring differs) ----------------------------
#define PIN_SDA        21   // SHT31 data
#define PIN_SCL        22   // SHT31 clock
#define PIN_FAN_PWM    25   // 4-pin fan, blue wire
#define PIN_FAN_TACH   26   // 4-pin fan, green wire
#define PIN_FAN_POWER  27   // Relay IN. Switches +12V on the high side, so the
                            // fan's ground stays common with the board and the
                            // tach line never floats up to 12V.

// ---- How the fogger gets switched ------------------------------------------
// Chosen in the web UI at runtime, not at compile time, so swapping hardware
// later is a dropdown rather than a reflash.
enum FoggerMode : uint8_t {
  FOG_NONE = 0,   // nothing set up yet; the controller runs but never fogs
  FOG_MQTT = 1,   // publish ON/OFF; a Home Assistant automation drives the plug
  FOG_HTTP = 2,   // GET a URL to switch another device on or off
};

struct Config {
  // Humidity
  float targetRh      = 90.0f;
  float maxRh         = 97.0f;   // above this for purgeDelayMin -> dry down
  float deadband      = 3.0f;    // how far RH may sag before fogging
  float emergencyRh   = 98.0f;   // instant fogger cutoff

  // Fogging. See README for why these are so small.
  uint16_t fogBurstS  = 5;       // one shot of fog, never longer
  uint16_t fogSettleS = 180;     // blind window while fog becomes humidity
  uint16_t fogBudgetS = 300;     // max fogger seconds per hour (flood cap)

  // Fresh air
  uint16_t faeIntervalMin = 60;  // 0 = fresh air off
  uint16_t faeDurationS   = 45;
  uint8_t  faeFanSpeed    = 60;

  // Fan
  uint8_t  fanMinDuty   = 25;    // slowest duty the fan reliably turns at
  uint16_t mixDurationS = 0;     // post-fog stir; 0 unless a SEPARATE
                                 // internal circulation fan is fitted
  bool useF = false;             // display only. Everything internal, and
                                 // everything published to MQTT, stays Celsius.
  bool relayActiveLow = false;   // true if the relay module switches on when
                                 // IN is pulled LOW. Many modules do. Flip it
                                 // in the UI rather than rewiring.

  // Dry-down
  uint16_t purgeDelayMin  = 10;
  float    purgeDeadband  = 2.0f;
  uint8_t  purgeFanSpeed  = 100;

  // Safety
  uint16_t sensorFaultS = 60;

  // Fogger output
  uint8_t foggerMode = FOG_NONE;
  String  httpOnUrl  = "";
  String  httpOffUrl = "";

  // MQTT (optional - powers both the HA entities and FOG_MQTT)
  bool   mqttEnabled = false;
  String mqttHost    = "";
  uint16_t mqttPort  = 1883;
  String mqttUser    = "";
  String mqttPass    = "";
  bool   haDiscovery = true;

  // Clock. POSIX timezone string, used only to label the chart with real
  // times. Default is US Central.
  String tz = "CST6CDT,M3.2.0,M11.1.0";

  // Runtime
  bool autoMode = true;
};

extern Config cfg;

inline void configToJson(JsonObject o) {
  o["targetRh"] = cfg.targetRh;
  o["maxRh"] = cfg.maxRh;
  o["deadband"] = cfg.deadband;
  o["emergencyRh"] = cfg.emergencyRh;
  o["fogBurstS"] = cfg.fogBurstS;
  o["fogSettleS"] = cfg.fogSettleS;
  o["fogBudgetS"] = cfg.fogBudgetS;
  o["faeIntervalMin"] = cfg.faeIntervalMin;
  o["faeDurationS"] = cfg.faeDurationS;
  o["faeFanSpeed"] = cfg.faeFanSpeed;
  o["fanMinDuty"] = cfg.fanMinDuty;
  o["mixDurationS"] = cfg.mixDurationS;
  o["useF"] = cfg.useF;
  o["relayActiveLow"] = cfg.relayActiveLow;
  o["purgeDelayMin"] = cfg.purgeDelayMin;
  o["purgeDeadband"] = cfg.purgeDeadband;
  o["purgeFanSpeed"] = cfg.purgeFanSpeed;
  o["sensorFaultS"] = cfg.sensorFaultS;
  o["foggerMode"] = cfg.foggerMode;
  o["httpOnUrl"] = cfg.httpOnUrl;
  o["httpOffUrl"] = cfg.httpOffUrl;
  o["mqttEnabled"] = cfg.mqttEnabled;
  o["mqttHost"] = cfg.mqttHost;
  o["mqttPort"] = cfg.mqttPort;
  o["mqttUser"] = cfg.mqttUser;
  o["haDiscovery"] = cfg.haDiscovery;
  o["tz"] = cfg.tz;
  o["autoMode"] = cfg.autoMode;
  // mqttPass deliberately never sent to the browser
}

// Applies only the keys present, so the UI can PATCH a single field.
inline void configFromJson(JsonObjectConst o) {
  #define F_NUM(k, dst, lo, hi) \
    if (o[#k].is<float>() || o[#k].is<int>()) { \
      float v = o[#k].as<float>(); \
      if (v < lo) v = lo; if (v > hi) v = hi; dst = v; }
  #define F_STR(k, dst) if (o[#k].is<const char*>()) dst = o[#k].as<const char*>();
  #define F_BOOL(k, dst) if (o[#k].is<bool>()) dst = o[#k].as<bool>();

  F_NUM(targetRh, cfg.targetRh, 40, 99)
  F_NUM(maxRh, cfg.maxRh, 80, 100)
  F_NUM(deadband, cfg.deadband, 0.5f, 15)
  F_NUM(emergencyRh, cfg.emergencyRh, 80, 100)
  F_NUM(fogBurstS, cfg.fogBurstS, 1, 60)
  F_NUM(fogSettleS, cfg.fogSettleS, 15, 900)
  F_NUM(fogBudgetS, cfg.fogBudgetS, 30, 1800)
  F_NUM(faeIntervalMin, cfg.faeIntervalMin, 0, 360)
  F_NUM(faeDurationS, cfg.faeDurationS, 5, 600)
  F_NUM(faeFanSpeed, cfg.faeFanSpeed, 1, 100)
  F_NUM(fanMinDuty, cfg.fanMinDuty, 5, 60)
  F_NUM(mixDurationS, cfg.mixDurationS, 0, 120)
  F_NUM(purgeDelayMin, cfg.purgeDelayMin, 1, 120)
  F_NUM(purgeDeadband, cfg.purgeDeadband, 0.5f, 10)
  F_NUM(purgeFanSpeed, cfg.purgeFanSpeed, 1, 100)
  F_NUM(sensorFaultS, cfg.sensorFaultS, 10, 600)
  F_NUM(foggerMode, cfg.foggerMode, 0, 2)
  F_NUM(mqttPort, cfg.mqttPort, 1, 65535)
  F_BOOL(useF, cfg.useF)
  F_BOOL(relayActiveLow, cfg.relayActiveLow)
  F_BOOL(mqttEnabled, cfg.mqttEnabled)
  F_BOOL(haDiscovery, cfg.haDiscovery)
  F_BOOL(autoMode, cfg.autoMode)
  F_STR(httpOnUrl, cfg.httpOnUrl)
  F_STR(httpOffUrl, cfg.httpOffUrl)
  F_STR(mqttHost, cfg.mqttHost)
  F_STR(mqttUser, cfg.mqttUser)
  F_STR(mqttPass, cfg.mqttPass)
  F_STR(tz, cfg.tz)

  // Max must stay above target or the two fight each other.
  if (cfg.maxRh <= cfg.targetRh) cfg.maxRh = min(100.0f, cfg.targetRh + 2.0f);

  #undef F_NUM
  #undef F_STR
  #undef F_BOOL
}

inline void configSave() {
  JsonDocument d;
  JsonObject o = d.to<JsonObject>();
  configToJson(o);
  o["mqttPass"] = cfg.mqttPass;
  String s;
  serializeJson(d, s);

  Preferences p;
  p.begin("chamber", false);
  p.putString("cfg", s);
  p.end();
}

// ---------------------------------------------------------------------------
// User presets. No built-in species list - people save their own once they
// find settings that work, which is the only kind of preset worth trusting.
// Stored as a JSON array: [{"name":"Blue oyster","v":{...}}, ...]
// ---------------------------------------------------------------------------
#define PRESET_MAX 12

inline String presetsLoad() {
  Preferences p;
  if (!p.begin("chamber", true)) return "[]";
  String s = p.getString("presets", "[]");
  p.end();
  return s.length() ? s : "[]";
}

inline void presetsStore(const String& json) {
  Preferences p;
  if (!p.begin("chamber", false)) return;
  p.putString("presets", json);
  p.end();
}

// Adds or replaces a preset by name, or removes it. Returns the new list.
inline String presetsApply(const String& name, JsonObjectConst values,
                           bool remove) {
  JsonDocument d;
  deserializeJson(d, presetsLoad());
  if (!d.is<JsonArray>()) d.to<JsonArray>();

  JsonDocument out;
  JsonArray arr = out.to<JsonArray>();
  for (JsonObjectConst it : d.as<JsonArrayConst>()) {
    if (String(it["name"] | "") == name) continue;   // replaced or removed
    arr.add(it);
  }
  if (!remove && name.length()) {
    JsonObject o = arr.add<JsonObject>();
    o["name"] = name;
    o["v"] = values;
  }
  while (arr.size() > PRESET_MAX) arr.remove(0);

  String s;
  serializeJson(arr, s);
  presetsStore(s);
  return s;
}

inline void configLoad() {
  Preferences p;
  p.begin("chamber", true);
  String s = p.getString("cfg", "");
  p.end();
  if (!s.length()) return;

  JsonDocument d;
  if (deserializeJson(d, s)) return;   // corrupt - keep defaults
  configFromJson(d.as<JsonObjectConst>());
}
