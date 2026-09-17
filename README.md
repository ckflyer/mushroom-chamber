# Mushroom Fruiting Chamber Controller

An ESP32 that holds humidity and cycles fresh air in a small fruiting chamber.
Everything runs on the board. No Home Assistant, no server, no computer left
running. Open the board's address in a browser and that is the whole interface.

Home Assistant sensors are an optional bonus over MQTT.

## What you need

| Part | Notes |
| --- | --- |
| ESP32 dev board | Any ESP-WROOM-32 board |
| SHT31-D sensor | I2C, address 0x44 |
| 4-pin PWM fan | 12 V PC fan |
| N-channel MOSFET module | Cuts fan power so it can stop completely |
| 12 V supply | For the fan |
| A way to switch the fogger | See below |

### Wiring

| Signal | Pin |
| --- | --- |
| SHT31 SDA | GPIO21 |
| SHT31 SCL | GPIO22 |
| Fan PWM (blue) | GPIO25 |
| Fan tach (green) | GPIO26 |
| MOSFET gate | GPIO27 |
| Relay (optional) | GPIO33 |

## Flashing

```
pio run -t upload
pio device monitor
```

On first boot the board makes a WiFi network called **Chamber-Setup**. Join it,
pick your network, and the board reboots onto it. Then open `http://mushroom.local`
or the address printed on the serial monitor.

No credentials live in this repo, so it is safe to publish or share.

## Switching the fogger

Chosen in the web UI under *Fogger wiring*, not at compile time. Swapping
hardware later is a dropdown, not a reflash.

**Relay or SSR on the board.** The board switches the fogger's power directly.
Nothing else involved, works with the router unplugged. Mains wiring belongs in
a closed, fused enclosure.

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

## Why the fog settings look so small

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

## Setting it up

1. **Slowest fan speed.** Turn Automatic off, lower the fan until it stalls,
   add five, put that in *Slowest fan speed*.
2. **Fogger dial low, not maximum.** Lowest setting where fog still leaves the
   hose steadily, then one notch up.
3. **Sealed test.** Automatic off, fan off, press *Run a test burst*, wait ten
   minutes.
   - Rises and holds → the chamber is fine, turn Automatic back on
   - Rises then falls → air is leaking, find the gaps
   - Barely moves → check the dew point reading

### Dew point tells you the ceiling

Air touching a cold surface cannot hold more vapour than that surface's
saturation point. When dew point sits within about 1 °C of chamber
temperature, you are as humid as this box can physically get, and the
dashboard says so. Warm the cold spot or drain standing water. Do not add fog.

### Fog delivery

Introduce fog at the **top** if you can. Fog-laden air is denser than clear air,
so it falls through the whole column and evaporates on the way down. Introduced
at the bottom it pools where it is born and never reaches the fruiting surface.

To avoid spraying anything, terminate the hose into a length of wide pipe along
the ceiling with small holes drilled facing upward. Fog leaves with almost no
velocity, hits the lid, and descends as a slow curtain. Slope the hose so
condensate drains back to the reservoir rather than dripping into the chamber.

## Troubleshooting

| Symptom | Cause |
| --- | --- |
| Water pooling | Shorter burst, longer settle, lower the fogger dial |
| Never reaches target | Do the sealed test. Do not add fog. |
| Humidity swings | Longer settle time |
| Fan seems dead | Press *Spin the fan*. Idle between cycles is normal. |
| Fan will not spin slow | Raise *Slowest fan speed* |
| Fogger never fires | Check *Fogger wiring* is not still "Not set up yet" |

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
pauses. It resumes automatically after the restart.

## Releasing a new version (maintainers)

`.github/workflows/build.yml` builds the firmware for you on every push. To cut
a release people can download:

1. Bump `FW_VERSION` in `src/config.h`
2. Commit and push
3. Tag it and push the tag:

```
git tag v1.1.0
git push origin v1.1.0
```

GitHub builds it and creates a Release with `chamber-firmware.bin` attached.
Nothing to build locally.

Untagged pushes still build — the `.bin` shows up under the repository's
Actions tab for testing.

## Layout

```
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
