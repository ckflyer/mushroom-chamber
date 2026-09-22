// =============================================================================
//  control.h - sensors, the burst/settle state machine, and fan arbitration
//
//  WHY THE FOG NUMBERS ARE SO SMALL
//  A 50 L tub at 20 C holds ~0.87 g of water vapour when fully saturated.
//  Going 80% -> 90% RH takes ~0.09 g. A fogger at full output makes ~0.08 g/s.
//  So a 5 second burst already delivers several times what a 10-point rise
//  needs. If humidity will not climb, it is never a fog shortage - it is air
//  leaving the box or a cold surface capping you. More fog only makes puddles,
//  and puddles cool the floor, which lowers the ceiling further.
// =============================================================================
#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SHT31.h>
#include "config.h"
#include "fogger.h"

// Arduino core 3.x changed the LEDC API.
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  #define FAN_PWM_BEGIN() ledcAttach(PIN_FAN_PWM, 25000, 8)
  #define FAN_PWM_WRITE(d) ledcWrite(PIN_FAN_PWM, d)
#else
  #define FAN_PWM_BEGIN() do { ledcSetup(0, 25000, 8); \
                               ledcAttachPin(PIN_FAN_PWM, 0); } while (0)
  #define FAN_PWM_WRITE(d) ledcWrite(0, d)
#endif

enum FogState : uint8_t { FOG_IDLE = 0, FOG_BURST = 1, FOG_SETTLE = 2 };
enum FanReason : uint8_t {
  FAN_IDLE = 0, FAN_MANUAL, FAN_FRESH_AIR, FAN_DRYDOWN,
  FAN_HELD, FAN_MIXING, FAN_TEST
};

struct State {
  float rh = NAN, temp = NAN, dewPoint = NAN, vpd = NAN;
  uint16_t rpm = 0;

  uint8_t  fogState = FOG_IDLE;
  uint16_t fogTimer = 0;
  float    fogCredit = 300.0f;

  uint32_t faeTimer = 0, faeRun = 0;
  bool     faeActive = false;

  uint32_t highRhS = 0;
  bool     purgeActive = false;

  bool     mixActive = false;
  uint16_t mixS = 0;

  uint16_t sensorFailS = 0;
  bool     sensorFault = false;
  bool     atCeiling = false;

  uint8_t  fanSpeed = 0;
  uint8_t  fanReason = FAN_IDLE;
  uint8_t  manualFanSpeed = 0;

  uint16_t fanTestS = 0;
  bool     forceBurst = false;
  bool     updating = false;   // true while firmware is being written

  // Dry reservoir detection. A burst that produces no humidity rise, three
  // times running, means the tank is empty or the disc is fouled.
  float    burstStartRh = NAN;
  uint8_t  dryStrikes = 0;
  bool     reservoirLow = false;
  float    lastRise = NAN;
};

extern State st;

// ---- 24 h history, one sample per minute, kept in RAM -----------------------
#define HIST_LEN 1440
struct History {
  int16_t rh[HIST_LEN];    // RH * 10
  int16_t t[HIST_LEN];     // degC * 10
  uint16_t head = 0;
  uint16_t count = 0;
};
extern History hist;

namespace ctrl {

inline Adafruit_SHT31 sht;
inline bool shtOk = false;
inline uint32_t lastTach = 0, lastHist = 0;

// NOT inline. An inline function placed in IRAM trips an xtensa linker bug:
// "dangerous relocation: l32r: literal placed after use". Internal linkage
// in this single translation unit avoids it. IRAM_ATTR must stay, because
// the fan keeps spinning during a firmware write and an ISR living in flash
// would crash once the flash cache is disabled.
static volatile uint32_t tachPulses = 0;

static void IRAM_ATTR tachISR() { tachPulses++; }

// Three-sample median. One bad reading from the SHT31 should not be able to
// trigger a burst or trip the emergency cutoff.
inline float _median3(const float* v) {
  float a = v[0], b = v[1], c = v[2];
  if (a > b) { float t = a; a = b; b = t; }
  if (b > c) { float t = b; b = c; c = t; }
  if (a > b) { float t = a; a = b; b = t; }
  return b;
}
inline float _tBuf[3], _hBuf[3];
inline uint8_t _bufN = 0, _bufI = 0;

// The hourly fog cap is worthless if a reboot loop hands out a fresh budget
// every time, so it is kept in flash.
inline uint32_t _lastCreditSave = 0;

inline void saveCredit() {
  Preferences p;
  if (!p.begin("chamber", false)) return;
  p.putFloat("credit", st.fogCredit);
  p.end();
  _lastCreditSave = millis();
}

inline float loadCredit(float dflt) {
  Preferences p;
  if (!p.begin("chamber", true)) return dflt;
  float v = p.getFloat("credit", dflt);
  p.end();
  if (isnan(v) || v < 0) v = 0;
  if (v > dflt) v = dflt;
  return v;
}

// The relay module decides what "on" means. Active-low boards are common, so
// the polarity is a setting rather than something you rewire or reflash for.
inline void _fanPower(bool on) {
  digitalWrite(PIN_FAN_POWER, cfg.relayActiveLow ? (on ? LOW : HIGH)
                                                 : (on ? HIGH : LOW));
}

inline void begin() {
  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(50000);
  shtOk = sht.begin(0x44);

  pinMode(PIN_FAN_POWER, OUTPUT);
  _fanPower(false);
  FAN_PWM_BEGIN();
  FAN_PWM_WRITE(0);

  pinMode(PIN_FAN_TACH, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_FAN_TACH), tachISR, RISING);

  st.fogCredit = loadCredit(cfg.fogBudgetS);
}

inline void _applyFan(uint8_t pct) {
  if (pct == 0) {
    // PWM to zero FIRST, then drop the relay. Cutting power to a fan that is
    // still being commanded is what makes it grunt on shutdown.
    FAN_PWM_WRITE(0);
    _fanPower(false);
  } else {
    _fanPower(true);
    FAN_PWM_WRITE(map(pct, 0, 100, 0, 255));
  }
  st.fanSpeed = pct;
}

// Consecutive failed reads. A single dropped read is normal noise on a long
// I2C run; a run of them means the sensor is gone.
inline uint8_t _readFails = 0;
#define SENSOR_FAIL_LIMIT 3

inline void _readSensor() {
  if (!shtOk) {
    shtOk = sht.begin(0x44);
    if (!shtOk) return;   // readings already blanked below; fault timer runs
  }
  float t = sht.readTemperature();
  float h = sht.readHumidity();

  if (isnan(t) || isnan(h)) {
    // Returning here without blanking is what used to break fault detection:
    // st.rh kept its last good value forever, rhOk stayed true, sensorFailS
    // reset every tick, and the controller happily fogged against a frozen
    // reading. Blank the readings so the fault timer can actually run.
    if (_readFails < 255) _readFails++;
    if (_readFails >= SENSOR_FAIL_LIMIT) {
      st.temp = NAN;
      st.rh = NAN;
      st.dewPoint = NAN;
      st.vpd = NAN;
      st.atCeiling = false;
      _bufN = 0;          // the median buffer is stale too
      _bufI = 0;
      shtOk = false;      // retry begin() on the next tick
    }
    return;
  }
  _readFails = 0;

  _tBuf[_bufI] = t;
  _hBuf[_bufI] = h;
  _bufI = (_bufI + 1) % 3;
  if (_bufN < 3) _bufN++;

  if (_bufN < 3) { st.temp = t; st.rh = h; }
  else { st.temp = _median3(_tBuf); st.rh = _median3(_hBuf); }
  t = st.temp;
  h = st.rh;

  // Magnus. Any surface colder than this is condensing water.
  const float a = 17.62f, b = 243.12f;
  float g = (a * t) / (b + t) + logf(max(h, 1.0f) / 100.0f);
  st.dewPoint = (b * g) / (a - g);

  // Vapour pressure deficit - what actually drives evaporation off the pins.
  float svp = 0.61078f * expf((17.27f * t) / (t + 237.3f));
  st.vpd = svp * (1.0f - h / 100.0f);

  // Within 1 C of dew point means this box is as humid as it can physically
  // get. More fog would only make water on the floor.
  st.atCeiling = (t - st.dewPoint) < 1.0f;
}

inline void _recordHistory() {
  if (isnan(st.rh)) return;
  hist.rh[hist.head] = (int16_t)(st.rh * 10);
  hist.t[hist.head] = (int16_t)(st.temp * 10);
  hist.head = (hist.head + 1) % HIST_LEN;
  if (hist.count < HIST_LEN) hist.count++;
}

// ---- The 1 Hz loop ---------------------------------------------------------
inline void tick() {
  _readSensor();

  bool rhOk = !isnan(st.rh);
  if (rhOk) st.sensorFailS = 0;
  else if (st.sensorFailS < 60000) st.sensorFailS++;
  st.sensorFault = st.sensorFailS > cfg.sensorFaultS;

  // ---- Fog budget, as a leaky bucket ----
  float budget = cfg.fogBudgetS;
  float refill = budget / 3600.0f;
  if (st.fogCredit > budget) st.fogCredit = budget;

  // ---- Burst / settle ----
  uint8_t s = st.fogState;
  if (st.sensorFault || !rhOk || cfg.foggerMode == FOG_NONE || st.updating) {
    s = FOG_IDLE;
    st.fogTimer = 0;
    st.forceBurst = false;
  } else if (s == FOG_IDLE) {
    bool below = st.rh < (cfg.targetRh - cfg.deadband);
    bool funded = st.fogCredit >= cfg.fogBurstS;
    if (((cfg.autoMode && below) || st.forceBurst) && funded) {
      s = FOG_BURST;
      st.fogTimer = 0;
      st.burstStartRh = st.rh;
      Serial.printf("[fog] burst at %.1f%%\n", st.rh);
    }
    st.forceBurst = false;
  } else if (s == FOG_BURST) {
    st.fogTimer++;
    if (st.fogTimer >= cfg.fogBurstS || st.rh >= cfg.targetRh ||
        st.fogCredit <= 0) {
      s = FOG_SETTLE;
      st.fogTimer = 0;
      if (cfg.mixDurationS >= 1) { st.mixActive = true; st.mixS = 0; }
    }
  } else {
    st.fogTimer++;
    if (st.fogTimer >= cfg.fogSettleS) {
      s = FOG_IDLE;
      st.fogTimer = 0;

      // Did that burst actually do anything? Only a fair question when the
      // chamber had room to rise and was not already capped by a cold
      // surface - otherwise a flat result says nothing about the reservoir.
      if (!isnan(st.burstStartRh) && !st.atCeiling &&
          st.burstStartRh < cfg.targetRh) {
        st.lastRise = st.rh - st.burstStartRh;
        if (st.lastRise < 0.3f) {
          if (st.dryStrikes < 200) st.dryStrikes++;
        } else {
          st.dryStrikes = 0;
        }
        bool low = st.dryStrikes >= 3;
        if (low && !st.reservoirLow)
          Serial.println("[fog] three bursts with no effect - check water");
        st.reservoirLow = low;
      }
      st.burstStartRh = NAN;
    }
  }

  if (rhOk && st.rh >= cfg.emergencyRh && s == FOG_BURST) {
    Serial.printf("[fog] emergency cutoff at %.1f%%\n", st.rh);
    s = FOG_SETTLE;
    st.fogTimer = 0;
  }
  st.fogState = s;

  bool wantFog = (s == FOG_BURST);
  if (wantFog) {
    st.fogCredit = max(0.0f, st.fogCredit - 1.0f);
  } else {
    st.fogCredit = min(budget, st.fogCredit + refill);
  }

  // Written after spending, rate limited so flash is not hammered.
  if (millis() - _lastCreditSave > 30000) saveCredit();

  fog::set(wantFog);
  fog::tick();

  // ---- Post-fog mixing ----
  if (st.mixActive) {
    st.mixS++;
    if (cfg.mixDurationS < 1 || st.mixS >= cfg.mixDurationS) {
      st.mixActive = false;
      st.mixS = 0;
    }
  }

  // ---- Fresh air ----
  uint32_t faeInterval = (uint32_t)cfg.faeIntervalMin * 60;
  if (cfg.autoMode && faeInterval > 0) {
    if (st.faeActive) {
      if (s != FOG_BURST) st.faeRun++;   // don't burn fresh air mid-burst
      if (st.faeRun >= cfg.faeDurationS) {
        st.faeActive = false; st.faeRun = 0; st.faeTimer = 0;
      }
    } else {
      st.faeTimer++;
      if (st.faeTimer >= faeInterval) {
        st.faeActive = true; st.faeRun = 0;
        Serial.println("[fae] cycle starting");
      }
    }
  } else {
    st.faeActive = false; st.faeRun = 0; st.faeTimer = 0;
  }

  // ---- Dry-down (maxRh 100 = off) ----
  bool purgeOn = cfg.autoMode && cfg.maxRh < 100.0f && rhOk && !st.sensorFault;
  if (purgeOn) {
    if (st.purgeActive) {
      if (st.rh <= cfg.maxRh - cfg.purgeDeadband) {
        st.purgeActive = false; st.highRhS = 0;
      }
    } else if (st.rh > cfg.maxRh) {
      st.highRhS++;
      if (st.highRhS >= (uint32_t)cfg.purgeDelayMin * 60) {
        st.purgeActive = true;
        Serial.println("[fan] drying down");
      }
    } else st.highRhS = 0;
  } else { st.highRhS = 0; st.purgeActive = false; }

  // ---- Fan arbitration, lowest priority first ----
  uint8_t speed = 0, reason = FAN_IDLE;
  if (st.mixActive)   { speed = cfg.fanMinDuty;     reason = FAN_MIXING; }
  if (st.faeActive)   { speed = cfg.faeFanSpeed;    reason = FAN_FRESH_AIR; }
  if (st.purgeActive) { speed = cfg.purgeFanSpeed;  reason = FAN_DRYDOWN; }

  // Never blow fog out of the chamber mid-burst.
  if (s == FOG_BURST && !st.purgeActive) { speed = 0; reason = FAN_HELD; }

  if (!cfg.autoMode) { speed = st.manualFanSpeed; reason = FAN_MANUAL; }

  if (st.fanTestS > 0) { st.fanTestS--; speed = 70; reason = FAN_TEST; }

  if (speed > 0 && speed < cfg.fanMinDuty) speed = cfg.fanMinDuty;
  if (speed > 100) speed = 100;

  _applyFan(speed);
  st.fanReason = reason;

  // ---- Tach + history ----
  uint32_t now = millis();
  if (now - lastTach >= 5000) {
    noInterrupts();
    uint32_t p = tachPulses;
    tachPulses = 0;
    interrupts();
    st.rpm = (uint16_t)(p * 6);   // 2 pulses/rev, 5 s window -> p/2*12
    lastTach = now;
  }
  if (now - lastHist >= 60000) { _recordHistory(); lastHist = now; }
}

inline const char* statusText() {
  if (st.updating)                  return "Updating";
  if (st.sensorFault)               return "Sensor fault";
  if (cfg.foggerMode == FOG_NONE)   return "No fogger set";
  if (!cfg.autoMode)                return "Manual";
  if (st.fogState == FOG_BURST)     return "Fogging";
  if (st.fanTestS > 0)              return "Fan test";
  if (st.purgeActive)               return "Drying down";
  if (st.faeActive)                 return "Fresh air";
  if (st.fogState == FOG_SETTLE)    return "Settling";
  if (st.fogCredit < cfg.fogBurstS) return "Fog limit reached";
  if (isnan(st.rh))                 return "No sensor";
  if (st.rh < cfg.targetRh - cfg.deadband) return "Below target";
  return "On target";
}

} // namespace ctrl
