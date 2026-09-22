# Mushroom Fruiting Chamber Controller

An ESP32 that holds humidity and cycles fresh air in a small fruiting chamber.
Everything runs on the board. No Home Assistant, no server, no computer left
running. Open the board's address in a browser and that is the whole interface.

Home Assistant sensors are an optional bonus over MQTT.

**What this is for:** a controlled environment to hold bags or small shoebox
tubs while nobody is home. It is not a monotub and not a shotgun chamber. A
monotub with taped holes regulates itself and needs no controller; this exists
for the case where containers sit in a chamber for weeks and the environment
has to be actively held.

---

## The build

### Chamber

| Item | Notes |
| --- | --- |
| 54 qt clear tote | ~51 L total, ~30 L headspace once loaded |
| Ambient | 70–73 °F, no heater needed, no cooling needed |

Three holes:

1. **Fog inlet** — hose from the reptile fogger
2. **Intake** — polyfill packed
3. **Exhaust** — fan, polyfill packed

Intake and exhaust should be as far apart as possible, ideally diagonal (intake
low at one end, exhaust high at the other). Holes close together short-circuit:
air goes straight from one to the other and the rest of the chamber never turns
over.

### Electronics

| Part | Notes |
| --- | --- |
| ESP32 dev board | Any ESP-WROOM-32. **Mounted outside the tote.** |
| SHT31-D sensor | I2C, address 0x44, mid-height, out of the fog path |
| Thermalright TL-C12C | 120mm 4-pin PWM, 12 V, 0.20 A |
| 3 V relay module | `SRD-03VDC` coil, optocoupler isolated, 10 A 30 VDC |
| 12 V supply | For the fan |
| Reptile fogger | Switched by the board over HTTP or MQTT |

The ESP32 lives outside the chamber. It is the one component here that is
genuinely fragile in 90%+ RH — the fan and sensor tolerate it, the board does
not.

---

## Humidity

The reptile fogger works and stays. No standing water on the floor, no moisture
coating the walls — that combination means the chamber is holding at the right
level and the dew point is not pinned. Nothing about this system needs changing.

### Fog delivery

Introduce fog at the **top**. Fog-laden air is denser than clear air, so it
falls through the whole column and evaporates on the way down. Introduced at
the bottom it pools where it is born and never reaches the fruiting surface.

Terminate the hose into a length of wide pipe along the ceiling with small holes
drilled facing upward. Fog leaves with almost no velocity, hits the lid, and
descends as a slow curtain. Slope the hose so condensate drains back to the
reservoir rather than dripping into the chamber. The same slope keeps anything
growing in the line from ending up in the tub.

### Peroxide in the reservoir

Peroxide keeps the tank from going slimy. Two things to understand before
picking a number:

**It protects the tank, not the chamber.** Anything in the line downstream
still gets aerosolized. The sloped hose above matters more than the dose.

**Whatever you atomize lands on your pins.** This is the reason to dose low. A
reservoir additive that would be harmless sitting in a jar is not harmless when
it is being sprayed directly onto fruiting bodies. H₂O₂ is actively harmful to
mycelium at concentration.

**Starting dose: 1 teaspoon of 3% per quart of water — about 4 teaspoons
(20 mL) of 3% per gallon.**

That figure comes from R. Rush Wayne's peroxide cultivation work, which puts
the lower limit of effectiveness for 3% peroxide at roughly a teaspoon per
quart and the upper limit of mycelium tolerance at around a quarter cup per
quart. For an atomizing fogger, stay at the bottom of that range, not the
middle — the quarter-cup figure is for contact, not for spraying.

**Be aware the published numbers conflict badly.** House of Hydro (who make
ultrasonic mist makers) suggest roughly a 20:1 water-to-3%-peroxide ratio once
weekly, which is far stronger than the above. Their figure reads as a tank
cleaning regime rather than a continuous dose. Plenty of experienced growers
also consider routine peroxide in a fruiting chamber counterproductive and
reserve it for treating cobweb mold. I have given you the conservative number
because the failure mode of too much is damaged pins and the failure mode of
too little is a tank you have to scrub.

**Better pattern if you want the tank properly clean:** dose strong
periodically with the fog line disconnected from the chamber, run it out, then
refill with plain water. Strong cleaning and gentle running, rather than a
compromise dose doing both jobs badly.

Peroxide also degrades quickly in light and heat, so a reservoir dose does not
persist long regardless.

### The fogger's piezo disc

Peroxide is hard on the piezo disc over time. You lose output before it fails
outright.

`reservoirLow` in the firmware trips after three bursts with no humidity rise.
That was written to catch an empty tank, but a degrading disc looks identical
to the controller. **When that flag fires, check the disc, not just the water
level.**

### Why the fog settings are so small

A 50 L tub at 20 °C holds about **0.87 g** of water vapour when completely
saturated from dry. Going from 80% to 90% RH takes about **0.09 g**. A reptile
fogger at full output makes roughly **0.08 g per second**.

So a five second burst already delivers several times what a ten point rise
needs. Fog is not vapour — it is liquid droplets that only become humidity as
they evaporate, and evaporation nearly stops as you approach saturation. Past a
point, more fog means water on the floor, and that water cools the floor by
evaporation, which lowers the maximum humidity the chamber can reach.

If humidity will not climb, it is never a fog shortage. It is air leaving the
box, or a cold surface capping you.

The controller works in **bursts**: a short shot of fog, then a blind settle
period where readings are ignored while droplets actually evaporate, then a
re-check. On top of that is an hourly budget, a leaky bucket that caps total
water entering the chamber no matter what else fails.

### Dew point tells you the ceiling

Air touching a cold surface cannot hold more vapour than that surface's
saturation point. When dew point sits within about 1 °C of chamber
temperature, you are as humid as this box can physically get, and the
dashboard says so. Warm the cold spot or drain standing water. Do not add fog.

At 70–73 °F ambient this should not come up, but it is the first thing to check
if the chamber is ever moved somewhere colder.

---

## Fresh air

Fan exhausts, passive intake, both packed with polyfill.

**The polyfill on the exhaust is a restrictor, not a filter.** Nothing
contaminating comes in through a hole with a fan pushing out of it. It is there
because without the airflow resistance, humidity bleeds out too fast between
cycles. Do not remove it.

Because the intake breathes continuously, there is slow passive gas exchange
all the time, with the scheduled fan doing the bulk work on top. This is why
CO₂ never runs away even if a fan cycle is missed, and why a CO₂ sensor is not
worth fitting. (Every NDIR CO₂ sensor on the market is rated non-condensing,
which this chamber is not. One would have to be mounted outside the wall
anyway.)

### Tuning the FAE duration

**The stock 45 seconds at 60% is far too much for this chamber.** It was a
default, not a measurement.

Mixing is exponential, not linear — pushing one headspace volume through only
exchanges about 63% of the air, because you are partly blowing out air you just
brought in. At 72 °F, saturated air holds ~19.1 g of water per m³; the chamber
at 92% carries ~17.5 g/m³; room air at 45% is ~8.6 g/m³.

For a ~30 L headspace:

| RH drop | Air exchanged | Air moved |
| --- | --- | --- |
| 5 points (92→87) | ~11% | ~3.4 L |
| 10 points (92→82) | ~21% | ~7.2 L |
| 20 points (92→72) | ~45% | ~18 L |
| 30 points (92→62) | ~63% | ~30 L (one full change) |

**Tune for a 5–10 point drop.** Enough exchange to matter for CO₂, small enough
that the fruiting surface never notices. Short and frequent beats long and
rare: same average CO₂ removal, gentler swings.

A 10 point drop needs ~7 L of air. The fan is rated 66 CFM free-air; through a
polyfill-packed hole, realistically 5–10 CFM. That makes 7 L about **one to
three seconds**. Even at a brutal 2 CFM it is under eight.

**Measure, do not trust the estimate.** The restriction is the unknown and it
is the whole ballgame.

1. Set `faeDurationS` to 10.
2. Fire one cycle.
3. Wait 60–90 seconds — the SHT31 lags the chamber.
4. Read the RH dip off the dashboard chart.
5. Scale. 10 s giving 20 points → use 5. 10 s giving 4 points → use 20.

Two things that will interfere: the fogger starts chasing as soon as RH falls
past `targetRh - deadband` (87 by default), so read the dip promptly or you are
measuring the recovery. And room RH changes the result — the same run time
drops less in a humid room than a dry one, so re-check seasonally.

The fruiting bodies are the slower but more honest signal. Long stems with
small caps means not enough air. Drying or cracking surfaces means too much.

---

## Fan wiring

### What changed and why

The original build switched the fan's **ground** with an IRLZ44N MOSFET. That
works for stopping the fan, but it is why the tach never read.

When a low-side switch opens, the fan's ground line floats up to 12 V. The tach
output is referenced to that same line, so every time the fan switched off,
roughly 12 V appeared on GPIO26 — a pin rated for 3.3. That either destroyed
the pin or made the signal garbage. Disconnecting the tach wire removed the
symptom; the cause was the switching side.

**Switching the +12 V instead leaves ground common.** When the relay opens the
fan is simply disconnected, nothing floats, and no stray voltage goes anywhere
near the ESP32.

A MOSFET can switch the high side too, but it needs a different type plus extra
parts. A relay module does it with three jumpers.

### Wiring

```
Relay VCC   → ESP32 3.3V
Relay GND   → common ground
Relay IN    → GPIO27          (replaces the MOSFET gate)
Relay COM   → +12V from supply
Relay NO    → fan yellow

Fan black   → common ground
Fan blue    → GPIO25          (PWM, 3.3 V drive is fine)
Fan green   → GPIO26          (tach — reconnect once relay is in)

SHT31 SDA   → GPIO21
SHT31 SCL   → GPIO22
```

**Grounds must be common.** The 12 V supply's negative, the fan's black, and
the ESP32's GND all tie together. Without that the PWM signal has no reference
and the fan does whatever it likes. This is the most common failure in this
kind of build.

**Check the VCC/JD-VCC jumper** on the relay module. Leave it fitted so the
coil runs off the same 3.3 V. If the board has separate pins and no jumper, tie
them together.

### Identify the tach by pin, not colour

Thermalright cables do not reliably follow the yellow/green/blue convention.
With the connector's retention tab facing you:

| Pin | Function |
| --- | --- |
| 1 | GND |
| 2 | +12 V |
| 3 | Tach |
| 4 | PWM |

### About RPM

Nothing in the firmware reads RPM. It is written once and displayed on the
dashboard and as an HA sensor. If it never comes back the chamber behaves
identically. Do not spend a third evening on it.

If it does not read after the relay is fitted, in order:

1. **Wrong pin** — go by position above, not wire colour.
2. **GPIO26 is damaged** from the 12 V. Move to GPIO33, change it in
   `config.h`, retest. If it works there, the old pin is cooked.
3. **Pull-up too weak.** The ESP32's internal pull-up is ~45 kΩ against a spec
   that expects 1–10 kΩ. Usually fine, but a tach line bundled alongside the
   25 kHz PWM wire can pick up enough interference to smear the edges. The fix
   is one 10 kΩ resistor from the tach wire to 3.3 V. There is no way around
   this one — a tach can only pull a line down, so something has to hold it up.
   The internal pull-up is already a resistor, just a weak one nobody had to
   buy.

---

## Temperature

70–73 °F ambient, year round, no heater and no cooling. Nothing to control and
nothing in the firmware does. Listed here so it is on the record that this was
checked and deliberately left alone.

---

## Build checklist

Do these in order once the relay arrives.

### 1. Test relay polarity before mounting anything

Many of these modules are active-LOW — pulling IN to ground turns the relay
**on**. The firmware currently assumes HIGH means on.

Toggle GPIO27 and listen for the click. Ten seconds, and it determines whether
the code needs inverting.

### 2. Fit the relay, remove the MOSFET

Wire per the table above. The IRLZ44N comes out entirely.

### 3. Reconnect the tach

Pin 3 to GPIO26. Then work the RPM list above if it reads zero.

### 4. Run the FAE measurement

`faeDurationS` to 10, fire a cycle, read the dip after 60–90 seconds, scale.

### 5. Set the remaining tuning values

- `faeDurationS` — from step 4
- `fanMinDuty` — automatic off, lower the fan until it stalls, add five
- Fogger dial — lowest setting where fog still leaves the hose steadily, then
  one notch up

### 6. Sealed test

Automatic off, fan off, press *Run a test burst*, wait ten minutes.

- Rises and holds → chamber is fine, turn Automatic back on
- Rises then falls → air is leaking, find the gaps
- Barely moves → check the dew point reading

---

## Firmware changes pending

None of these are written yet. Listed so nothing gets lost.

### Blocking — do before the next unattended run

**Sensor fault detection does not work.** In `_readSensor()`, a failed read
does `if (isnan(t) || isnan(h)) return;`, which leaves `st.rh` holding its last
good value. Back in `tick()`, `rhOk` is therefore still true, `sensorFailS`
resets every tick, and `sensorFault` never trips.

If the SHT31 dies or a wire works loose, the controller keeps fogging against a
frozen reading, with only the hourly budget holding it back. This is precisely
the failure that matters while away from home.

`shtOk` also latches true after the first successful `begin()`, so the sensor
is never re-initialised.

Fix: consecutive-failure counter inside `_readSensor()` that blanks `st.rh` and
`st.temp` to NAN after N failures.

### Required by the relay swap

- `_applyFan()` — drive GPIO27 for the relay, polarity per step 1
- `setup()` — drive GPIO27 to its **off** state as the first statement. An
  unconfigured pin floats and can click the relay on before the code takes over
- `setup()` — drive GPIO25 low as the first statement too. A floating PWM pin
  reads as 100% duty to the fan, so between reset and `ledcAttach` you get full
  blast
- `config.h` — `PIN_FAN_POWER` comment updated from MOSFET to relay
- Order of operations in `_applyFan()`: cut PWM to 0 **first**, then drop the
  relay. Powering down a fan that is still being commanded is how you get stall
  noise. Current code already does this — do not reorder it

### Defaults to change

- `faeDurationS` from 45 to the measured value (likely 3–20)
- `faeFanSpeed` — possibly down from 60 once duration is right

### Found in review, not yet fixed

Lower priority than the sensor fault, but all real:

- **OTA can wedge the board.** `st.updating` is set in the body handler but
  only cleared in the completion handler. Close the browser mid-upload and it
  stays true forever — control paused, status stuck on "Updating", tach
  interrupt detached and never re-attached. Needs a timeout.
- **Blocking work inside AsyncTCP callbacks.** `/api/config` calls
  `configSave()` (NVS write) and `fog::allOff()` (up to 4 s of HTTPClient);
  `/api/restart` adds `delay(200)`. The async task has a small stack and
  blocking it stalls all TCP. Should be flagged and done from `loop()`.
- **NVS wear.** `saveCredit()` writes a float every 30 s — roughly a million
  writes a year on one key, and the value always differs so NVS cannot skip it.
  Save on meaningful change instead.
- **Temperature history is collected and never served.** `hist.t[]` is filled
  in `_recordHistory()` but `/api/history` only emits `{"rh":[...]}`. 2.8 KB of
  RAM going nowhere and no temp chart.
- **Five config fields have no UI:** `emergencyRh`, `sensorFaultS`,
  `purgeDelayMin`, `purgeDeadband`, `purgeFanSpeed`. The first two are
  plausibly deliberate. The purge trio is behaviour rather than a safety limit,
  and `maxRh` is already exposed, so dry-down is half-configurable.
- **Fog boil-off after a crash.** `fog::allOff()` runs in `setup()` before WiFi
  is up, so the HTTP OFF is silently dropped, and `wm.autoConnect()` can block
  for up to 180 s. In HTTP mode the fogger can stay on that whole time.
- Minor: no auth on any endpoint including `/api/update`; `sendHistory`
  concatenates an ~8 KB String; `static String body` in the POST handlers is
  not reentrant; fan test overrides `FAN_HELD` so *Spin the fan* mid-burst
  blows the fog out; FAE can run during settle, working against the blind
  evaporation window; MQTT publishes `0` instead of null for NaN readings,
  which puts zeros in HA graphs.

---

## Phase 2 — multiple chambers (planned, not started)

**Do not start until the pending changes above are done, especially the sensor
fault.** Copying a design with a known flooding bug into three chambers triples
the risk.

### The principle

Every chamber is fully independent. Each board controls its own humidity and
fan with no dependence on the others or on the network. Connecting them is for
**viewing only, never for control**. If WiFi dies or one board crashes, every
other chamber keeps running exactly as before. There is no central hub and
there should never be one.

### One firmware, features switched on per chamber

Every board of the same chip runs the same `.bin`. In the web UI, tick what is
fitted: fan, lights. A chamber without a fan hides fan controls, skips FAE, and
does not publish fan sensors to HA.

One release, one update, flashed to everything. Maintaining separate firmware
per chamber is how side projects die.

**Humidity-only chamber hardware:** ESP32, SHT31, fogger switch. No fan, no
relay, no 12 V supply.

### Naming

The board name is currently hardcoded as `mushroom` — hostname, mDNS
(`mushroom.local`), and the HA device name "Mushroom Chamber". Two boards on
one network collide and are indistinguishable.

Add a `chamberName` setting that drives all of: hostname, mDNS name, HA device
name, dashboard title.

Minor: two boards in WiFi setup mode at once would both broadcast
"Chamber-Setup". Only matters during first-time setup.

### Home dashboard (for anyone without HA)

Every board shows it. No "main" board, so no single board whose failure takes
out the view.

- Each board advertises itself on the network and finds its peers
  automatically (mDNS service advertisement).
- Peer lookup runs periodically from `loop()` and is cached — **not** from a
  request handler, for the same blocking reasons listed in pending changes.
- The browser fetches each chamber's `/api/state` directly. The boards never
  fetch each other's data.
- `/api/state` needs an `Access-Control-Allow-Origin` header so a page served
  by one board is allowed to read another. One line.
- Home page shows a card per chamber: name, RH, temp, status. Tap a card for
  that chamber's full dashboard.

### Home Assistant

Already works for multiple boards. Each publishes under its own MAC-derived
`deviceId()`, so three boards appear as three separate devices with no code
changes.

Needed: publish only the entities for features that are fitted, and remove
stale ones when a feature is switched off (publish an empty retained discovery
config). Otherwise HA shows a dead RPM sensor on the humidity-only box.

### Different boards

**Different chip families** (ESP32, ESP32-S3, ESP32-C3, etc.) need separate
builds — one `.bin` cannot run on different processors.

- One PlatformIO environment per chip family
- CI builds every environment on each release
- Release assets named per chip: `chamber-firmware-esp32.bin`,
  `chamber-firmware-esp32s3.bin`, and so on
- Dashboard shows the board's chip next to the firmware update box, so the
  right file is obvious
- The OTA image header carries a chip ID and should be rejected on mismatch
  rather than bricking. Verify this once on real hardware before relying on it

**Same chip, wired differently:** pins configurable in the web UI.

- Stored in NVS per board, like every other setting
- Dropdown offers only pins safe for that role on that chip. Strapping pins and
  flash pins are never offered as outputs — a relay on a strapping pin can stop
  the board booting. Input-only pins are offered only for the tach.
- Changing a pin reboots the board to apply it (`ledcAttach`,
  `attachInterrupt` and `Wire.begin` all run once in `setup()`)
- **Safe mode:** a boot counter that reverts to default pins after several
  consecutive failed boots following a pin change. Otherwise a bad pin choice
  locks you out of the dashboard you need to fix it
- Each chip family ships sensible default pins

### Rough scope

Name setting, feature toggles, discovery, home page, CORS header, conditional
HA entities: roughly 300–400 lines across most files. Board profiles and
runtime pin configuration on top of that.

---

## Switching the fogger

Chosen in the web UI under *Fogger wiring*, not at compile time. Swapping
hardware later is a dropdown, not a reflash.

**Another device over HTTP.** Point it at any device with a URL-based API,
including another ESPHome node:
`http://192.168.1.50/switch/fogger/turn_on`.

**Home Assistant over MQTT.** For plugs the board cannot talk to directly, like
the Kasa KP125M, whose newer firmware needs TP-Link cloud credentials even
locally. The board publishes a *Fogger Request* entity and one automation
mirrors it onto the plug:

```yaml
alias: Mushroom fogger
mode: queued
trigger:
  - platform: state
    entity_id: binary_sensor.mushroom_chamber_fogger_request
  - platform: time_pattern
    minutes: "/1"
  - platform: state
    entity_id: switch.mushroom_fogger
    to: "on"
    for: "00:02:00"
    id: toolong
action:
  - choose:
      - conditions: "{{ trigger.id == 'toolong' }}"
        sequence:
          - service: switch.turn_off
            target: { entity_id: switch.mushroom_fogger }
    default:
      - service: >
          switch.turn_{{ 'on' if is_state('binary_sensor.mushroom_chamber_fogger_request','on') else 'off' }}
        target: { entity_id: switch.mushroom_fogger }
```

The time pattern re-asserts state every minute, so a missed message corrects
itself. The two minute rule is a backstop against a stuck plug.

---

## Flashing

```
pio run -t upload
pio device monitor
```

On first boot the board makes a WiFi network called **Chamber-Setup**. Join it,
pick your network, and the board reboots onto it. Then open `http://mushroom.local`
or the address printed on the serial monitor.

No credentials live in this repo, so it is safe to publish or share.

---

## Troubleshooting

| Symptom | Cause |
| --- | --- |
| Water pooling | Shorter burst, longer settle, lower the fogger dial |
| Never reaches target | Do the sealed test. Do not add fog. |
| Humidity swings | Longer settle time |
| Fan seems dead | Press *Spin the fan*. Idle between cycles is normal. |
| Fan will not spin slow | Raise *Slowest fan speed* |
| Fogger never fires | Check *Fogger wiring* is not still "Not set up yet" |
| `reservoirLow` fires | Check water **and** the piezo disc |
| RPM reads zero | Cosmetic. See the RPM section before spending time on it |
| Fan runs at boot | GPIO25 or GPIO27 floating — see pending changes |

---

## Updating

Open the dashboard, expand **Firmware update**, and drop in the
`chamber-firmware.bin` from the
[releases page](../../releases). A progress bar runs, the board restarts, and
the page reloads itself. No cables, no tools.

Your settings survive an update. They live in a separate area of flash that
firmware writes never touch.

The update is written to a spare flash slot while the current firmware keeps
running, so a failed or interrupted upload leaves the chamber exactly as it
was. If the upload dies halfway, nothing is broken — try again.

While an update is being written, the fogger is forced off and the controller
pauses. It resumes automatically after the restart. (See pending changes — an
abandoned upload currently leaves it paused indefinitely.)

---

## Publishing an update (maintainers)

Set the version on the first line, then paste the whole block into Git Bash.
It bumps the version the dashboard reports, commits, pushes, replaces the tag,
and triggers the build.

```bash
VER=1.2.0 && MSG="what changed" && \
cd ~/Desktop/mushroom-chamber && \
sed -i "s/#define FW_VERSION \".*\"/#define FW_VERSION \"$VER\"/" src/config.h && \
git add -A && \
git commit -m "$MSG" && \
git push && \
{ git tag -d "v$VER" || true; } && \
{ git push origin ":refs/tags/v$VER" || true; } && \
git tag "v$VER" && \
git push origin "v$VER" && \
echo "pushed v$VER - watch the Actions tab, then grab the .bin from Releases"
```

Or the same thing with the script:

```bash
./release.sh 1.2.0 "what changed"
```

**The build must go green before a `.bin` exists.** Open the Actions tab on the
repository. A red run means a compile error and the release will have only the
source zip attached. Click into the failed run, open the `build` step, and the
error is at the bottom.

Once it is green, the release page has `chamber-firmware.bin`. Download it and
drop it into the Firmware tab of the chamber dashboard.

Untagged pushes still build — that `.bin` shows up under Actions for testing,
without creating a release.

---

## Layout

```
release.sh      cut a release in one command
src/config.h    pins, settings, saving to flash
src/fogger.h    the only file that knows how the fogger is switched
src/control.h   sensors, burst state machine, fan arbitration
src/ha_mqtt.h   optional Home Assistant sensors
src/web_ui.h    the dashboard, plain HTML and JS
src/main.cpp    WiFi, web server, the 1 Hz loop
```

Safety limits live in `control.h` and hold regardless of the interface: burst
length, hourly budget, emergency cutoff, sensor fault detection, and a watchdog
that reboots rather than leaving the fogger on.

Note that sensor fault detection does not currently work — see pending changes.
