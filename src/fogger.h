// =============================================================================
//  fogger.h - the one place that knows HOW the fogger is switched
//
//  Everything above this file just calls foggerSet(true/false). Swapping a
//  smart plug for a relay later touches nothing but this.
// =============================================================================
#pragma once
#include <Arduino.h>
#include <HTTPClient.h>
#include "config.h"

// Provided by ha_mqtt.h - publishes the requested state for HA to mirror.
void mqttPublishFogger(bool on);

namespace fog {

inline bool _state = false;
inline uint32_t _lastAssert = 0;

inline void begin() {}

inline void _httpCall(const String& url) {
  if (!url.length() || WiFi.status() != WL_CONNECTED) return;
  HTTPClient http;
  http.setConnectTimeout(2000);
  http.setTimeout(2000);
  if (http.begin(url)) {
    http.GET();
    http.end();
  }
}

inline void _apply(bool on) {
  switch (cfg.foggerMode) {
    case FOG_HTTP:
      _httpCall(on ? cfg.httpOnUrl : cfg.httpOffUrl);
      break;
    case FOG_MQTT:
      mqttPublishFogger(on);
      break;
    case FOG_NONE:
    default:
      break;
  }
}

inline void set(bool on) {
  if (on == _state) return;
  _state = on;
  _lastAssert = millis();
  _apply(on);
}

inline bool state() { return _state; }

// Re-sends the current state periodically. If the broker reconnected, or a
// relay glitched, or an HTTP call was dropped, this is what recovers it.
// OFF is re-asserted more often than ON, because a stuck-on fogger is the
// failure that floods the tub.
inline void tick() {
  uint32_t period = _state ? 60000UL : 30000UL;
  if (millis() - _lastAssert > period) {
    _lastAssert = millis();
    _apply(_state);
  }
}

// Called on boot and on any fault. Forces OFF through every path at once,
// because we may not know which mode was active when things went wrong.
inline void allOff() {
  _state = false;
  _httpCall(cfg.httpOffUrl);
  mqttPublishFogger(false);
}

} // namespace fog
