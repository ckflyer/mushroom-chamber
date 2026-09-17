// =============================================================================
//  ha_mqtt.h - OPTIONAL. Two jobs:
//    1. Publishes read-only sensors to Home Assistant via MQTT discovery.
//    2. Publishes the fogger request, so an HA automation can mirror it onto
//       a smart plug the ESP cannot talk to directly (Kasa KP125M and friends).
//
//  Leave MQTT off and the chamber works exactly the same, minus the entities.
//  Nothing in the control loop depends on this file.
// =============================================================================
#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "config.h"
#include "control.h"

namespace ha {

inline WiFiClient net;
inline PubSubClient mqtt(net);
inline String base;          // chamber/<id>
inline bool discoverySent = false;
inline uint32_t lastTry = 0, lastPub = 0;
inline bool pendingFogger = false, pendingFoggerState = false;

inline String devId() {
  uint64_t m = ESP.getEfuseMac();
  char buf[13];
  snprintf(buf, sizeof(buf), "%04x%08x", (uint16_t)(m >> 32), (uint32_t)m);
  return String("chamber_") + buf;
}

inline void _sensor(const char* key, const char* name, const char* unit,
                    const char* devClass, const char* icon) {
  JsonDocument d;
  d["name"] = name;
  d["uniq_id"] = devId() + "_" + key;
  d["stat_t"] = base + "/state";
  d["val_tpl"] = String("{{ value_json.") + key + " }}";
  if (unit) d["unit_of_meas"] = unit;
  if (devClass) d["dev_cla"] = devClass;
  if (icon) d["ic"] = icon;
  d["avty_t"] = base + "/available";
  JsonObject dev = d["dev"].to<JsonObject>();
  dev["ids"][0] = devId();
  dev["name"] = "Mushroom Chamber";
  dev["mf"] = "DIY";
  dev["mdl"] = "ESP32 Fruiting Chamber";

  String topic = String("homeassistant/sensor/") + devId() + "/" + key + "/config";
  String payload;
  serializeJson(d, payload);
  mqtt.publish(topic.c_str(), payload.c_str(), true);
}

inline void _binary(const char* key, const char* name, const char* devClass) {
  JsonDocument d;
  d["name"] = name;
  d["uniq_id"] = devId() + "_" + key;
  d["stat_t"] = base + "/state";
  d["val_tpl"] = String("{{ value_json.") + key + " }}";
  d["pl_on"] = "1";
  d["pl_off"] = "0";
  if (devClass) d["dev_cla"] = devClass;
  d["avty_t"] = base + "/available";
  JsonObject dev = d["dev"].to<JsonObject>();
  dev["ids"][0] = devId();
  dev["name"] = "Mushroom Chamber";

  String topic = String("homeassistant/binary_sensor/") + devId() + "/" + key + "/config";
  String payload;
  serializeJson(d, payload);
  mqtt.publish(topic.c_str(), payload.c_str(), true);
}

inline void _sendDiscovery() {
  if (!cfg.haDiscovery) return;
  _sensor("rh", "Humidity", "%", "humidity", nullptr);
  _sensor("temp", "Temperature", "°C", "temperature", nullptr);
  _sensor("dew", "Dew Point", "°C", "temperature", "mdi:thermometer-water");
  _sensor("vpd", "VPD", "kPa", nullptr, "mdi:water-thermometer");
  _sensor("rpm", "Fan RPM", "RPM", nullptr, "mdi:fan");
  _sensor("fan", "Fan Speed", "%", nullptr, "mdi:fan");
  _sensor("status", "Status", nullptr, nullptr, "mdi:information-outline");
  _sensor("fogUsed", "Fog Used This Hour", "s", nullptr, "mdi:spray");
  _binary("ceiling", "At Humidity Ceiling", "problem");
  _binary("dry", "Reservoir Empty", "problem");
  _binary("fault", "Sensor Fault", "problem");

  // The fogger request. This is what an HA automation watches in order to
  // drive a plug the ESP cannot reach itself.
  JsonDocument d;
  d["name"] = "Fogger Request";
  d["uniq_id"] = devId() + "_fogreq";
  d["stat_t"] = base + "/fogger";
  d["pl_on"] = "ON";
  d["pl_off"] = "OFF";
  d["dev_cla"] = "running";
  d["avty_t"] = base + "/available";
  JsonObject dev = d["dev"].to<JsonObject>();
  dev["ids"][0] = devId();
  dev["name"] = "Mushroom Chamber";
  String topic = String("homeassistant/binary_sensor/") + devId() + "/fogreq/config";
  String payload;
  serializeJson(d, payload);
  mqtt.publish(topic.c_str(), payload.c_str(), true);

  discoverySent = true;
  Serial.println("[mqtt] discovery published");
}

inline void _publishState() {
  JsonDocument d;
  d["rh"] = isnan(st.rh) ? 0 : round(st.rh * 10) / 10.0;
  d["temp"] = isnan(st.temp) ? 0 : round(st.temp * 10) / 10.0;
  d["dew"] = isnan(st.dewPoint) ? 0 : round(st.dewPoint * 10) / 10.0;
  d["vpd"] = isnan(st.vpd) ? 0 : round(st.vpd * 100) / 100.0;
  d["rpm"] = st.rpm;
  d["fan"] = st.fanSpeed;
  d["status"] = ctrl::statusText();
  d["fogUsed"] = (int)(cfg.fogBudgetS - st.fogCredit);
  d["ceiling"] = st.atCeiling ? "1" : "0";
  d["dry"] = st.reservoirLow ? "1" : "0";
  d["fault"] = st.sensorFault ? "1" : "0";

  String payload;
  serializeJson(d, payload);
  mqtt.publish((base + "/state").c_str(), payload.c_str(), true);
}

inline void begin() {
  base = String("chamber/") + devId();
}

inline void loop() {
  if (!cfg.mqttEnabled || !cfg.mqttHost.length()) return;
  if (WiFi.status() != WL_CONNECTED) return;

  if (!mqtt.connected()) {
    if (millis() - lastTry < 5000) return;
    lastTry = millis();
    mqtt.setServer(cfg.mqttHost.c_str(), cfg.mqttPort);
    mqtt.setBufferSize(1024);

    String willTopic = base + "/available";
    bool ok = cfg.mqttUser.length()
      ? mqtt.connect(devId().c_str(), cfg.mqttUser.c_str(), cfg.mqttPass.c_str(),
                     willTopic.c_str(), 0, true, "offline")
      : mqtt.connect(devId().c_str(), willTopic.c_str(), 0, true, "offline");

    if (!ok) return;
    mqtt.publish(willTopic.c_str(), "online", true);
    discoverySent = false;
    Serial.println("[mqtt] connected");
  }

  mqtt.loop();
  if (!discoverySent) _sendDiscovery();

  if (pendingFogger) {
    mqtt.publish((base + "/fogger").c_str(),
                 pendingFoggerState ? "ON" : "OFF", true);
    pendingFogger = false;
  }

  if (millis() - lastPub >= 5000) {
    lastPub = millis();
    _publishState();
  }
}

} // namespace ha

// Queued rather than sent inline, so the control loop never blocks on network.
inline void mqttPublishFogger(bool on) {
  ha::pendingFogger = true;
  ha::pendingFoggerState = on;
}
