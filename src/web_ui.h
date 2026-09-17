// =============================================================================
//  web_ui.h - the dashboard, embedded in flash.
//  One file, no build step, no filesystem upload. Plain HTML/CSS/JS.
//  Tabs on wide screens, hamburger drawer on narrow ones.
// =============================================================================
#pragma once
#include <Arduino.h>

const char INDEX_HTML[] PROGMEM = R"HTMLDOC(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<meta name="color-scheme" content="dark">
<title>Mushroom Chamber</title>
<style>
:root{
  --ink:#0d1512; --panel:#141f1b; --raise:#1a2723; --line:#223029;
  --text:#e6ebe4; --dim:#7d9086; --fog:#9fd8c8; --warm:#e0a45c;
  --bad:#e0705c;
  --r:14px; --gap:12px; --pad:16px; --rowh:46px;
}
*{box-sizing:border-box}
html,body{margin:0;padding:0}
body{
  background:var(--ink); color:var(--text);
  font-family:ui-sans-serif,system-ui,-apple-system,"Segoe UI",Roboto,sans-serif;
  font-size:16px; line-height:1.45;
  padding:16px 14px 44px; margin:0 auto; max-width:780px;
  font-variant-numeric:tabular-nums;
}
h1{font-size:17px;font-weight:600;margin:0;letter-spacing:-.01em;
  white-space:nowrap}
h2{font-size:11px;font-weight:600;margin:0 0 8px;color:var(--dim);
  letter-spacing:.06em}
.panel{background:var(--panel);border:1px solid var(--line);
  border-radius:var(--r);padding:var(--pad)}

/* ---------- app bar ---------- */
.bar{display:flex;align-items:center;gap:12px;margin-bottom:var(--gap)}
.ver{font-size:11px;color:var(--dim);margin-left:7px;font-weight:400}
.spacer{flex:1}
.tabs{display:none;gap:3px;background:var(--panel);border:1px solid var(--line);
  border-radius:11px;padding:3px}
.tabs button{background:none;border:0;color:var(--dim);font-size:13px;
  padding:6px 13px;border-radius:8px;cursor:pointer}
.tabs button[aria-current=page]{background:var(--raise);color:var(--text)}
.burger{width:40px;height:40px;padding:0;display:grid;place-items:center;
  font-size:17px;line-height:1}
.drawer{display:none;flex-direction:column;margin-bottom:var(--gap)}
.drawer.on{display:flex}
.drawer button{text-align:left;border-radius:0;border-width:0 0 1px 0;
  background:var(--panel);padding:13px var(--pad)}
.drawer button:first-child{border-radius:var(--r) var(--r) 0 0;border-top-width:1px}
.drawer button:last-child{border-radius:0 0 var(--r) var(--r);border-bottom-width:1px}
.drawer button[aria-current=page]{color:var(--fog)}

/* ---------- alert ---------- */
.alert{display:none;align-items:flex-start;gap:10px;padding:12px 14px;
  border-radius:var(--r);margin-bottom:var(--gap);font-size:14px;
  border:1px solid var(--warm);background:#241d12;color:var(--warm)}
.alert.on{display:flex}
.alert.bad{border-color:var(--bad);background:#241513;color:var(--bad)}
.alert b{display:block;font-weight:600;margin-bottom:1px}
.alert span{color:var(--dim);font-weight:400}

/* ---------- views ---------- */
.view{display:none;flex-direction:column;gap:var(--gap)}
.view.on{display:flex}

/* ---------- hero ---------- */
.hero{display:flex;gap:16px;align-items:stretch}
.gauge{flex:0 0 78px;display:flex}
.gauge svg{display:block;width:78px;height:100%}
.readout{flex:1;min-width:0;display:flex;flex-direction:column;gap:10px}
.rhnum{font-size:48px;font-weight:600;line-height:1;letter-spacing:-.03em}
.rhnum span{font-size:19px;font-weight:400;color:var(--dim);margin-left:2px}
.state{display:flex;align-items:center;gap:8px;font-size:15px}
.dot{width:9px;height:9px;border-radius:50%;background:var(--fog);flex:0 0 auto}
.state.warn .dot{background:var(--warm)} .state.warn{color:var(--warm)}
.state.bad .dot{background:var(--bad)} .state.bad{color:var(--bad)}
.state.live .dot{animation:pulse 1.6s ease-in-out infinite}
@keyframes pulse{0%,100%{opacity:1}50%{opacity:.25}}
.tiles{display:grid;grid-template-columns:repeat(3,1fr);gap:1px;
  background:var(--line);border:1px solid var(--line);border-radius:11px;
  overflow:hidden;margin-top:auto}
.tile{background:var(--panel);padding:8px 10px}
.tile b{display:block;font-size:15px;font-weight:600;line-height:1.25}
.tile i{font-style:normal;font-size:10px;color:var(--dim);
  letter-spacing:.02em}

/* ---------- rows ---------- */
.row{display:flex;align-items:center;gap:10px;flex-wrap:wrap;
  min-height:var(--rowh);padding:5px 0;border-top:1px solid var(--line)}
.row:first-of-type{border-top:0}
.lbl{flex:1;min-width:110px;font-size:15px}
.ctl{display:flex;align-items:center;gap:7px;justify-content:flex-end}
.unit{font-size:13px;color:var(--dim);width:28px;text-align:left}
.ro{color:var(--dim);font-size:14px}
input[type=number],input[type=text],select{
  background:var(--raise);color:var(--text);border:1px solid var(--line);
  border-radius:9px;padding:8px 10px;font:inherit;font-size:15px;width:82px;
  text-align:right}
input[type=text],select{width:210px;text-align:left}
input:focus-visible,select:focus-visible,button:focus-visible{
  outline:2px solid var(--fog);outline-offset:2px}
button{background:var(--raise);color:var(--text);border:1px solid var(--line);
  border-radius:10px;padding:9px 14px;font:inherit;font-size:14px;cursor:pointer}
button:hover{border-color:var(--fog)}
.help{width:19px;height:19px;padding:0;border-radius:50%;font-size:12px;
  line-height:1;color:var(--dim);margin-left:6px;vertical-align:1px}
.help[aria-expanded=true]{background:var(--fog);color:var(--ink);
  border-color:var(--fog)}
.helptext{flex-basis:100%;font-size:13px;color:var(--dim);margin:0 0 6px;
  padding:9px 11px;background:var(--raise);border-radius:9px}
.sw{position:relative;width:48px;height:28px;border-radius:14px;
  background:var(--raise);border:1px solid var(--line);cursor:pointer;
  flex:0 0 auto;padding:0}
.sw::after{content:"";position:absolute;top:3px;left:3px;width:20px;height:20px;
  border-radius:50%;background:var(--dim);transition:transform .16s,background .16s}
.sw[aria-checked=true]{background:#20403a;border-color:var(--fog)}
.sw[aria-checked=true]::after{transform:translateX(20px);background:var(--fog)}

/* ---------- chart ---------- */
.chart{width:100%;height:150px;display:block}
.chartfoot{display:flex;justify-content:space-between;font-size:11px;
  color:var(--dim);margin-top:4px}

.note{font-size:13px;color:var(--dim);margin:10px 0 0}
.hide{display:none}
.btns{display:flex;gap:9px;flex-wrap:wrap}
.drop{border:1.5px dashed var(--line);border-radius:12px;padding:24px 14px;
  text-align:center;color:var(--dim);font-size:14px;cursor:pointer}
.drop:hover,.drop.over{border-color:var(--fog);color:var(--text)}
.prog{height:7px;background:var(--raise);border-radius:4px;margin-top:12px;
  overflow:hidden}
.prog i{display:block;height:100%;width:0;background:var(--fog);
  transition:width .15s}
#toast{position:fixed;left:50%;bottom:20px;transform:translateX(-50%);
  background:var(--raise);border:1px solid var(--fog);padding:8px 15px;
  border-radius:10px;font-size:14px;opacity:0;transition:opacity .2s;
  pointer-events:none;z-index:9}
#toast.on{opacity:1}

/* ---------- wide ---------- */
@media (min-width:760px){
  body{max-width:1060px;padding:24px 22px 56px}
  .tabs{display:flex}
  .burger{display:none}
  .drawer{display:none!important}
  .two{display:grid;grid-template-columns:1fr 1fr;gap:var(--gap);
    align-items:stretch}
  .two>*{margin:0}
  .gauge,.gauge svg{flex-basis:92px;width:92px}
  .rhnum{font-size:58px}
  .chart{height:190px}
  input[type=text],select{width:240px}
}
@media (max-width:400px){
  .gauge,.gauge svg{flex-basis:64px;width:64px}
  .rhnum{font-size:40px}
  .tiles{grid-template-columns:repeat(2,1fr)}
}
@media (prefers-reduced-motion:reduce){*{transition:none!important;
  animation:none!important}}
</style>
</head>
<body>

<div class="bar">
  <h1>Mushroom Chamber<span class="ver" id="fwver"></span></h1>
  <div class="spacer"></div>
  <div class="tabs" id="tabs">
    <button data-view="dash" aria-current="page">Dashboard</button>
    <button data-view="fog">Fogging</button>
    <button data-view="setup">Setup</button>
    <button data-view="net">Network</button>
    <button data-view="fw">Firmware</button>
  </div>
  <span id="modeLbl" style="font-size:13px;color:var(--dim)">Auto</span>
  <button class="sw" id="autoSw" role="switch" aria-checked="true"
          aria-label="Automatic control"></button>
  <button class="burger" id="burger" aria-label="Menu"
          aria-expanded="false">&#9776;</button>
</div>

<div class="drawer" id="drawer">
  <button data-view="dash" aria-current="page">Dashboard</button>
  <button data-view="fog">Fogging</button>
  <button data-view="setup">Setup</button>
  <button data-view="net">Network</button>
  <button data-view="fw">Firmware</button>
</div>

<div class="alert" id="alert">
  <span aria-hidden="true">&#9888;</span>
  <div><b id="alertTitle"></b><span id="alertBody"></span></div>
</div>

<!-- ==================== DASHBOARD ==================== -->
<section class="view on" id="v-dash">
  <div class="two">
    <div class="panel">
      <div class="hero">
        <div class="gauge">
          <svg viewBox="0 0 78 210" preserveAspectRatio="none" aria-hidden="true">
            <defs><clipPath id="box">
              <rect x="6" y="6" width="66" height="198" rx="8"/>
            </clipPath></defs>
            <rect x="6" y="6" width="66" height="198" rx="8" fill="#101a17"/>
            <g clip-path="url(#box)">
              <rect id="fill" x="6" y="204" width="66" height="0"
                    fill="#9fd8c8" opacity=".22"/>
              <rect id="fillTop" x="6" y="204" width="66" height="2"
                    fill="#9fd8c8"/>
            </g>
            <line id="tgtLine" x1="6" y1="110" x2="72" y2="110" stroke="#e6ebe4"
                  stroke-width="1.5" stroke-dasharray="4 3"/>
            <line id="maxLine" x1="6" y1="60" x2="72" y2="60" stroke="#e0a45c"
                  stroke-width="1.5" stroke-dasharray="2 3"/>
            <rect x="6" y="6" width="66" height="198" rx="8" fill="none"
                  stroke="#223029" stroke-width="1.5"/>
          </svg>
        </div>
        <div class="readout">
          <div>
            <div class="rhnum" id="rh">--<span>%</span></div>
            <div class="state" id="state"><span class="dot"></span>
              <span id="status">Connecting</span></div>
          </div>
          <div class="tiles">
            <div class="tile"><b id="temp">--</b><i>TEMP</i></div>
            <div class="tile"><b id="dew">--</b><i>DEW POINT</i></div>
            <div class="tile"><b id="vpd">--</b><i>VPD kPa</i></div>
            <div class="tile"><b id="fan">--</b><i>FAN</i></div>
            <div class="tile"><b id="rpm">--</b><i>RPM</i></div>
            <div class="tile"><b id="fogUsed">--</b><i>FOG THIS HR</i></div>
          </div>
        </div>
      </div>
    </div>

    <div class="panel">
      <h2>HUMIDITY</h2>
      <div class="row">
        <div class="lbl">Species
          <button class="help" aria-expanded="false" aria-label="About presets">?</button>
        </div>
        <div class="ctl"><select id="preset">
          <option value="">Custom</option>
        </select></div>
        <p class="helptext hide">Sets target humidity and fresh air timing to
          a sensible starting point. Tune from there. Changing any of those
          values by hand switches this back to Custom.</p>
      </div>
      <div class="row">
        <div class="lbl">Target</div>
        <div class="ctl"><input type="number" id="targetRh" min="40" max="99"
          step="1"><span class="unit">%</span></div>
      </div>
      <div class="row">
        <div class="lbl">Max
          <button class="help" aria-expanded="false" aria-label="About max">?</button>
        </div>
        <div class="ctl"><input type="number" id="maxRh" min="80" max="100"
          step="1"><span class="unit">%</span></div>
        <p class="helptext hide">Sit above this for 10 minutes and the fan runs
          full speed until it drops 2% below. Must be higher than Target.
          Set 100 to turn it off.</p>
      </div>
      <div class="row" id="manualFanRow" style="display:none">
        <div class="lbl">Fan</div>
        <div class="ctl"><input type="number" id="manualFan" min="0" max="100"
          step="5"><span class="unit">%</span></div>
      </div>

      <h2 style="margin-top:14px">FRESH AIR</h2>
      <div class="row">
        <div class="lbl">Every
          <button class="help" aria-expanded="false" aria-label="About interval">?</button>
        </div>
        <div class="ctl"><input type="number" id="faeIntervalMin" min="0"
          max="360" step="5"><span class="unit">min</span></div>
        <p class="helptext hide">Set 0 to turn fresh air off.</p>
      </div>
      <div class="row">
        <div class="lbl">For
          <button class="help" aria-expanded="false" aria-label="About duration">?</button>
        </div>
        <div class="ctl"><input type="number" id="faeDurationS" min="5" max="600"
          step="5"><span class="unit">sec</span></div>
        <p class="helptext hide">45 seconds swaps the air in a small tub several
          times over. Every second costs humidity.</p>
      </div>
      <div class="row">
        <div class="lbl">Fan speed</div>
        <div class="ctl"><input type="number" id="faeFanSpeed" min="1" max="100"
          step="5"><span class="unit">%</span></div>
      </div>
    </div>
  </div>

  <div class="panel">
    <h2>LAST 24 HOURS</h2>
    <svg class="chart" id="chart" viewBox="0 0 700 190"
         preserveAspectRatio="none" role="img"
         aria-label="Humidity over the last 24 hours"></svg>
    <div class="chartfoot"><span id="chartLeft">24h ago</span>
      <span id="chartRight">now</span></div>
  </div>
</section>

<!-- ==================== FOGGING ==================== -->
<section class="view" id="v-fog">
  <div class="two">
    <div class="panel">
      <h2>TIMING</h2>
      <div class="row">
        <div class="lbl">Burst
          <button class="help" aria-expanded="false" aria-label="About burst">?</button>
        </div>
        <div class="ctl"><input type="number" id="fogBurstS" min="1" max="60"
          step="1"><span class="unit">sec</span></div>
        <p class="helptext hide">One shot of fog. The fogger never runs longer
          than this at a stretch. A 50 L tub needs about 0.09 g of water to go
          from 80% to 90% humidity, and a fogger makes roughly 0.08 g per
          second, so 5 seconds is already several times over. Raise in 2 second
          steps.</p>
      </div>
      <div class="row">
        <div class="lbl">Settle
          <button class="help" aria-expanded="false" aria-label="About settle">?</button>
        </div>
        <div class="ctl"><input type="number" id="fogSettleS" min="15" max="900"
          step="15"><span class="unit">sec</span></div>
        <p class="helptext hide">Quiet time after a burst. Readings are ignored
          while droplets evaporate into actual humidity. Raise this first if
          humidity overshoots.</p>
      </div>
      <div class="row">
        <div class="lbl">Deadband
          <button class="help" aria-expanded="false" aria-label="About deadband">?</button>
        </div>
        <div class="ctl"><input type="number" id="deadband" min="0.5" max="15"
          step="0.5"><span class="unit">%</span></div>
        <p class="helptext hide">How far below Target it may drift before
          fogging starts.</p>
      </div>
    </div>

    <div class="panel">
      <h2>LIMITS</h2>
      <div class="row">
        <div class="lbl">Limit per hour
          <button class="help" aria-expanded="false" aria-label="About limit">?</button>
        </div>
        <div class="ctl"><input type="number" id="fogBudgetS" min="30"
          max="1800" step="15"><span class="unit">sec</span></div>
        <p class="helptext hide">Hard cap on total fogger run time per hour,
          whatever else goes wrong. Survives reboots. If you keep hitting it,
          look for leaks rather than raising it.</p>
      </div>
      <div class="row">
        <div class="lbl">Used this hour</div>
        <div class="ctl ro" id="fogUsed2">--</div>
      </div>
      <div class="row">
        <div class="lbl">Last burst gained
          <button class="help" aria-expanded="false" aria-label="About rise">?</button>
        </div>
        <div class="ctl ro" id="lastRise">--</div>
        <p class="helptext hide">Humidity gained by the most recent burst. Three
          in a row near zero and the chamber warns you the water is out.</p>
      </div>
      <div class="row">
        <div class="btns"><button id="btnFog">Test burst</button></div>
      </div>
      <p class="note">Burst plus Settle should divide into the hourly limit
        sensibly. At 5 second bursts and 180 second settles the chamber uses
        about 95 seconds an hour.</p>
    </div>
  </div>
</section>

<!-- ==================== SETUP ==================== -->
<section class="view" id="v-setup">
  <div class="two">
    <div class="panel">
      <h2>FOGGER WIRING</h2>
      <div class="row">
        <div class="lbl">Switched by</div>
        <div class="ctl"><select id="foggerMode">
          <option value="0">Nothing yet</option>
          <option value="1">Home Assistant (MQTT)</option>
          <option value="2">Relay on GPIO33</option>
          <option value="3">Another device (HTTP)</option>
        </select></div>
      </div>
      <div id="modeMqtt" class="hide">
        <p class="note">Publishes a Fogger Request entity. One Home Assistant
          automation mirrors it onto your plug. Set the broker up under
          Network.</p>
      </div>
      <div id="modeRelay" class="hide">
        <div class="row">
          <div class="lbl">Active high
            <button class="help" aria-expanded="false" aria-label="About polarity">?</button>
          </div>
          <button class="sw" id="relayActiveHigh" role="switch"
            aria-label="Relay active high"></button>
          <p class="helptext hide">On for most relay boards. Turn off for
            active-low boards, which switch when the pin goes low.</p>
        </div>
        <p class="note">Mains wiring belongs in a closed, fused enclosure.</p>
      </div>
      <div id="modeHttp" class="hide">
        <div class="row"><div class="lbl">On URL</div>
          <input type="text" id="httpOnUrl"
            placeholder="http://192.168.1.50/switch/fog/turn_on"></div>
        <div class="row"><div class="lbl">Off URL</div>
          <input type="text" id="httpOffUrl"
            placeholder="http://192.168.1.50/switch/fog/turn_off"></div>
      </div>
    </div>

    <div class="panel">
      <h2>FAN</h2>
      <div class="row">
        <div class="lbl">Min speed
          <button class="help" aria-expanded="false" aria-label="About min fan">?</button>
        </div>
        <div class="ctl"><input type="number" id="fanMinDuty" min="5" max="60"
          step="1"><span class="unit">%</span></div>
        <p class="helptext hide">The slowest your fan actually turns. Switch
          Auto off, lower the fan until it stalls, then enter that number
          plus five.</p>
      </div>
      <div class="row">
        <div class="lbl">Stir after fog
          <button class="help" aria-expanded="false" aria-label="About stir">?</button>
        </div>
        <div class="ctl"><input type="number" id="mixDurationS" min="0"
          max="120" step="5"><span class="unit">sec</span></div>
        <p class="helptext hide">Leave at 0 with one fan. Your fan vents the
          chamber, so stirring with it just blows the fog out. Only useful if
          you add a second fan inside that moves no air in or out.</p>
      </div>
      <div class="row">
        <div class="lbl">Running because</div>
        <div class="ctl ro" id="fanReason">--</div>
      </div>
      <div class="row">
        <div class="lbl">Uptime</div>
        <div class="ctl ro" id="uptime">--</div>
      </div>
      <div class="row">
        <div class="btns">
          <button id="btnFan">Spin fan</button>
          <button id="btnReboot">Restart</button>
        </div>
      </div>
    </div>
  </div>
</section>

<!-- ==================== NETWORK ==================== -->
<section class="view" id="v-net">
  <div class="two">
    <div class="panel">
      <h2>HOME ASSISTANT</h2>
      <div class="row">
        <div class="lbl">Use MQTT
          <button class="help" aria-expanded="false" aria-label="About MQTT">?</button>
        </div>
        <button class="sw" id="mqttEnabled" role="switch"
          aria-label="Use MQTT"></button>
        <p class="helptext hide">MQTT is a small messaging service that lets the
          chamber talk to Home Assistant. You need a broker running. In Home
          Assistant, install the Mosquitto broker add-on, then create a person
          under Settings, People with login enabled and use those details here.
          Leave this off and the chamber works exactly the same, just without
          Home Assistant entities.</p>
      </div>
      <div class="row"><div class="lbl">Broker
          <button class="help" aria-expanded="false" aria-label="About broker">?</button>
        </div>
        <input type="text" id="mqttHost" placeholder="192.168.1.10">
        <p class="helptext hide">The IP of the machine running Home Assistant.
          Find it under Settings, System, Network.</p>
      </div>
      <div class="row"><div class="lbl">Port</div>
        <div class="ctl"><input type="number" id="mqttPort" min="1" max="65535"
          step="1"></div></div>
      <div class="row"><div class="lbl">User</div>
        <input type="text" id="mqttUser"></div>
      <div class="row"><div class="lbl">Password</div>
        <input type="text" id="mqttPass" placeholder="unchanged"></div>
      <div class="row">
        <div class="lbl">Publish entities
          <button class="help" aria-expanded="false" aria-label="About entities">?</button>
        </div>
        <button class="sw" id="haDiscovery" role="switch"
          aria-label="Publish entities"></button>
        <p class="helptext hide">Creates the sensors in Home Assistant
          automatically. They appear under Settings, Devices, MQTT. Read only,
          so Home Assistant cannot change chamber settings.</p>
      </div>
      <p class="note" id="mqttState">Not connected.</p>
    </div>

    <div class="panel">
      <h2>CLOCK</h2>
      <div class="row">
        <div class="lbl">Timezone</div>
        <div class="ctl"><select id="tz"></select></div>
      </div>
      <div class="row">
        <div class="lbl">Chamber time</div>
        <div class="ctl ro" id="clock">--</div>
      </div>
      <p class="note">Only used to put real times on the chart. The chamber
        runs fine without it.</p>
    </div>
  </div>
</section>

<!-- ==================== FIRMWARE ==================== -->
<section class="view" id="v-fw">
  <div class="panel">
    <h2>UPDATE</h2>
    <div class="drop" id="drop" tabindex="0" role="button"
         aria-label="Choose a firmware file">
      Drop a .bin here, or click to choose
    </div>
    <input type="file" id="fwfile" accept=".bin" class="hide">
    <div class="prog hide" id="progWrap"><i id="progBar"></i></div>
    <p class="note" id="otaNote">Grab chamber-firmware.bin from the releases
      page. The chamber keeps running on the old firmware while the new one is
      written, so a failed upload changes nothing. Settings are kept.</p>
  </div>
</section>

<div id="toast"></div>

<script>
const $ = id => document.getElementById(id);
let cfg = {}, ready = false, epoch = 0, epochAt = 0;

const NUMS = ["targetRh","maxRh","deadband","fogBurstS","fogSettleS",
  "fogBudgetS","faeIntervalMin","faeDurationS","faeFanSpeed","fanMinDuty",
  "mixDurationS","mqttPort"];
const TEXTS = ["httpOnUrl","httpOffUrl","mqttHost","mqttUser"];
const SWS = ["relayActiveHigh","mqttEnabled","haDiscovery"];

// Starting points, not gospel. Every strain and every chamber differs.
const PRESETS = {
  "Oyster":       {targetRh:88, maxRh:96, faeIntervalMin:30, faeDurationS:60, faeFanSpeed:70},
  "King oyster":  {targetRh:88, maxRh:96, faeIntervalMin:30, faeDurationS:60, faeFanSpeed:70},
  "Lion's mane":  {targetRh:92, maxRh:98, faeIntervalMin:60, faeDurationS:45, faeFanSpeed:55},
  "Shiitake":     {targetRh:85, maxRh:94, faeIntervalMin:60, faeDurationS:45, faeFanSpeed:60},
  "Wine cap":     {targetRh:90, maxRh:97, faeIntervalMin:90, faeDurationS:40, faeFanSpeed:55},
  "Button":       {targetRh:88, maxRh:96, faeIntervalMin:90, faeDurationS:40, faeFanSpeed:50},
  "Reishi":       {targetRh:90, maxRh:97, faeIntervalMin:45, faeDurationS:50, faeFanSpeed:60}
};

// Plain names instead of POSIX strings nobody can read.
const ZONES = [
  ["UTC","UTC0"],
  ["US Eastern","EST5EDT,M3.2.0,M11.1.0"],
  ["US Central","CST6CDT,M3.2.0,M11.1.0"],
  ["US Mountain","MST7MDT,M3.2.0,M11.1.0"],
  ["US Arizona","MST7"],
  ["US Pacific","PST8PDT,M3.2.0,M11.1.0"],
  ["Alaska","AKST9AKDT,M3.2.0,M11.1.0"],
  ["Hawaii","HST10"],
  ["UK","GMT0BST,M3.5.0/1,M10.5.0"],
  ["Central Europe","CET-1CEST,M3.5.0,M10.5.0/3"],
  ["Eastern Europe","EET-2EEST,M3.5.0/3,M10.5.0/4"],
  ["India","IST-5:30"],
  ["China","CST-8"],
  ["Japan","JST-9"],
  ["Australia Eastern","AEST-10AEDT,M10.1.0,M4.1.0/3"],
  ["New Zealand","NZST-12NZDT,M9.5.0,M4.1.0/3"]
];

function toast(m){
  const t=$("toast"); t.textContent=m; t.classList.add("on");
  clearTimeout(t._h); t._h=setTimeout(()=>t.classList.remove("on"),1500);
}

async function patch(body){
  try{
    const r = await fetch("/api/config",{method:"POST",
      headers:{"Content-Type":"application/json"},body:JSON.stringify(body)});
    if(!r.ok) throw 0;
    cfg = await r.json();
    toast("Saved");
  }catch(e){ toast("Could not save"); }
}

/* ---------- navigation ---------- */
function show(view){
  document.querySelectorAll(".view").forEach(v=>
    v.classList.toggle("on", v.id === "v"+"-"+view));
  document.querySelectorAll("[data-view]").forEach(b=>{
    if(b.dataset.view===view) b.setAttribute("aria-current","page");
    else b.removeAttribute("aria-current");
  });
  $("drawer").classList.remove("on");
  $("burger").setAttribute("aria-expanded","false");
  window.scrollTo(0,0);
}

function wireNav(){
  document.querySelectorAll("[data-view]").forEach(b=>
    b.addEventListener("click",()=>show(b.dataset.view)));
  $("burger").addEventListener("click",()=>{
    const open = $("drawer").classList.toggle("on");
    $("burger").setAttribute("aria-expanded", String(open));
  });
}

function wireHelp(){
  document.querySelectorAll(".help").forEach(b=>{
    b.addEventListener("click",()=>{
      const t = b.closest(".row").querySelector(".helptext");
      if(!t) return;
      const closed = t.classList.toggle("hide");
      b.setAttribute("aria-expanded", String(!closed));
    });
  });
}

/* ---------- presets ---------- */
function buildPresets(){
  const sel = $("preset");
  Object.keys(PRESETS).forEach(n=>{
    const o=document.createElement("option"); o.value=n; o.textContent=n;
    sel.appendChild(o);
  });
  sel.addEventListener("change",()=>{
    const p = PRESETS[sel.value];
    if(!p) return;
    patch(p).then(()=>{ fillConfig(cfg); sel.value = matchPreset(); });
  });
}

// A preset is only "selected" while every one of its values still matches.
function matchPreset(){
  for(const [name,p] of Object.entries(PRESETS)){
    if(Object.keys(p).every(k=>Math.abs(cfg[k]-p[k])<0.01)) return name;
  }
  return "";
}

function buildZones(){
  const sel=$("tz");
  ZONES.forEach(([name,val])=>{
    const o=document.createElement("option"); o.value=val; o.textContent=name;
    sel.appendChild(o);
  });
  sel.addEventListener("change",()=>patch({tz:sel.value}));
}

function wire(){
  NUMS.forEach(k=>{ const el=$(k); if(el)
    el.addEventListener("change",()=>patch({[k]:parseFloat(el.value)})
      .then(()=>{ $("preset").value = matchPreset(); })); });
  TEXTS.forEach(k=>{ const el=$(k); if(el)
    el.addEventListener("change",()=>patch({[k]:el.value})); });
  SWS.forEach(k=>{ const el=$(k); if(el)
    el.addEventListener("click",()=>{
      const v = el.getAttribute("aria-checked")!=="true";
      el.setAttribute("aria-checked",v); patch({[k]:v});
    }); });

  $("mqttPass").addEventListener("change",e=>{
    if(e.target.value){ patch({mqttPass:e.target.value}); e.target.value=""; }
  });
  $("foggerMode").addEventListener("change",e=>{
    patch({foggerMode:parseInt(e.target.value)}); showMode(e.target.value);
  });
  $("autoSw").addEventListener("click",()=>{
    const v = $("autoSw").getAttribute("aria-checked")!=="true";
    $("autoSw").setAttribute("aria-checked",v);
    $("modeLbl").textContent = v?"Auto":"Manual";
    $("manualFanRow").style.display = v?"none":"";
    patch({autoMode:v});
  });
  $("manualFan").addEventListener("change",e=>{
    fetch("/api/fan?speed="+parseInt(e.target.value),{method:"POST"});
  });
  $("btnFan").addEventListener("click",()=>{
    fetch("/api/fan-test",{method:"POST"}); toast("Fan running 15s");
  });
  $("btnFog").addEventListener("click",()=>{
    fetch("/api/fog-burst",{method:"POST"}); toast("Burst requested");
  });
  $("btnReboot").addEventListener("click",()=>{
    if(confirm("Restart the chamber?")){
      fetch("/api/restart",{method:"POST"}); toast("Restarting");
    }
  });
  wireNav(); wireHelp(); buildPresets(); buildZones();
}

function showMode(m){
  $("modeMqtt").classList.toggle("hide", m!=1);
  $("modeRelay").classList.toggle("hide", m!=2);
  $("modeHttp").classList.toggle("hide", m!=3);
}

function fillConfig(c){
  cfg=c;
  NUMS.forEach(k=>{ if($(k)&&c[k]!==undefined&&document.activeElement!==$(k))
    $(k).value=c[k]; });
  TEXTS.forEach(k=>{ if($(k)&&c[k]!==undefined&&document.activeElement!==$(k))
    $(k).value=c[k]; });
  SWS.forEach(k=>{ if($(k)&&c[k]!==undefined)
    $(k).setAttribute("aria-checked",!!c[k]); });
  $("foggerMode").value=c.foggerMode; showMode(c.foggerMode);
  $("autoSw").setAttribute("aria-checked",!!c.autoMode);
  $("modeLbl").textContent = c.autoMode?"Auto":"Manual";
  $("manualFanRow").style.display = c.autoMode?"none":"";
  if(c.tz) $("tz").value = c.tz;
  ready=true;
  $("preset").value = matchPreset();
}

const GT=6, GH=198, LO=60, HI=100;
const yFor = v => GT + GH*(1-(Math.min(HI,Math.max(LO,v))-LO)/(HI-LO));
const REASONS=["Idle","Manual","Fresh air","Drying down","Held for fog",
               "Mixing","Test"];

function alertFor(s){
  if(s.fault) return ["bad","Sensor fault",
    " No readings from the SHT31. Fogging is stopped. Check the wiring on GPIO21 and 22."];
  if(s.dry) return ["bad","Check the water",
    " Three bursts in a row raised humidity by almost nothing. The reservoir is probably empty, or the disc needs cleaning."];
  if(s.ceiling) return ["","At the humidity ceiling",
    " Dew point has met air temperature, so this chamber cannot get any wetter. More fog will only pool. Warm the cold spot or drain standing water."];
  if(/Fog limit/.test(s.status)) return ["","Hourly fog limit reached",
    " No more fogging until the budget refills. If this keeps happening, look for leaks rather than raising the limit."];
  if(/No fogger/.test(s.status)) return ["","No fogger configured",
    " Pick how the fogger is switched under Setup."];
  return null;
}

function paint(s){
  const a=alertFor(s), box=$("alert");
  if(a){ box.className="alert on "+a[0];
    $("alertTitle").textContent=a[1]; $("alertBody").textContent=a[2]; }
  else box.className="alert";

  if(s.epoch){ epoch=s.epoch; epochAt=Date.now(); }

  $("rh").innerHTML = (s.rh==null?"--":s.rh.toFixed(1))+"<span>%</span>";
  $("temp").textContent = s.temp==null?"--":s.temp.toFixed(1)+"\u00B0";
  $("dew").textContent  = s.dew==null?"--":s.dew.toFixed(1)+"\u00B0";
  $("vpd").textContent  = s.vpd==null?"--":s.vpd.toFixed(2);
  $("fan").textContent  = s.fan+"%";
  $("rpm").textContent  = s.rpm;
  $("fogUsed").textContent = s.fogUsed+"s";
  $("fogUsed2").textContent = s.fogUsed+" of "+(cfg.fogBudgetS||"--")+" sec";
  $("lastRise").textContent = s.lastRise==null?"--":
    (s.lastRise>=0?"+":"")+s.lastRise.toFixed(1)+"%";
  $("fanReason").textContent = REASONS[s.fanReason]||"Idle";
  $("fwver").textContent = s.version?("v"+s.version):"";
  $("uptime").textContent = Math.floor(s.uptime/3600)+"h "+
    (Math.floor(s.uptime/60)%60)+"m";
  $("mqttState").textContent = s.mqtt?"Connected to broker.":"Not connected.";
  $("clock").textContent = s.epoch ?
    new Date(s.epoch*1000).toLocaleString() : "Not synced";

  let cls="state";
  if(s.fault) cls+=" bad";
  else if(s.ceiling||/Drying|limit|Below/.test(s.status)) cls+=" warn";
  if(s.fogState==1||s.fanReason==2||s.fanReason==3) cls+=" live";
  $("state").className=cls;
  $("status").textContent = s.ceiling ? s.status+" \u00B7 at ceiling" : s.status;

  if(s.rh!=null){
    const y=yFor(s.rh);
    $("fill").setAttribute("y",y);
    $("fill").setAttribute("height",Math.max(0,GT+GH-y));
    $("fillTop").setAttribute("y",y-1);
  }
  if(ready){
    $("tgtLine").setAttribute("y1",yFor(cfg.targetRh));
    $("tgtLine").setAttribute("y2",yFor(cfg.targetRh));
    const sm = cfg.maxRh<100;
    $("maxLine").style.display = sm?"":"none";
    if(sm){
      $("maxLine").setAttribute("y1",yFor(cfg.maxRh));
      $("maxLine").setAttribute("y2",yFor(cfg.maxRh));
    }
  }
}

function chart(vals){
  const svg=$("chart");
  if(!vals||vals.length<2){
    svg.innerHTML='<text x="350" y="95" fill="#7d9086" font-size="13" '+
      'text-anchor="middle">Collecting readings</text>';
    return;
  }
  const W=700,H=190,P=8;
  let lo=Math.min(...vals), hi=Math.max(...vals);
  if(ready){ lo=Math.min(lo,cfg.targetRh); hi=Math.max(hi,cfg.targetRh); }
  const pad=Math.max(2,(hi-lo)*0.15); lo-=pad; hi+=pad;
  const X=i=>P+i*(W-2*P)/(vals.length-1);
  const Y=v=>P+(H-2*P)*(1-(v-lo)/(hi-lo));
  let d="M"+X(0)+","+Y(vals[0]);
  for(let i=1;i<vals.length;i++) d+="L"+X(i).toFixed(1)+","+Y(vals[i]).toFixed(1);
  let g="";
  if(ready){
    const ty=Y(cfg.targetRh).toFixed(1);
    g+='<line x1="0" y1="'+ty+'" x2="700" y2="'+ty+'" stroke="#e6ebe4" '+
       'stroke-width="1" stroke-dasharray="4 3" opacity=".4"/>';
  }
  g+='<path d="'+d+'" fill="none" stroke="#9fd8c8" stroke-width="2" '+
     'stroke-linejoin="round"/>';
  g+='<text x="5" y="14" fill="#7d9086" font-size="11">'+hi.toFixed(0)+'%</text>';
  g+='<text x="5" y="185" fill="#7d9086" font-size="11">'+lo.toFixed(0)+'%</text>';
  svg.innerHTML=g;

  if(epoch){
    const now = epoch + (Date.now()-epochAt)/1000;
    $("chartLeft").textContent = new Date((now-vals.length*60)*1000)
      .toLocaleTimeString([],{hour:"numeric",minute:"2-digit"});
  } else {
    $("chartLeft").textContent = vals.length<60 ? vals.length+" min ago"
      : (vals.length/60).toFixed(0)+"h ago";
  }
}

function wireUpdate(){
  const drop=$("drop"), file=$("fwfile"), wrap=$("progWrap"),
        bar=$("progBar"), note=$("otaNote");
  drop.addEventListener("click",()=>file.click());
  drop.addEventListener("keydown",e=>{
    if(e.key==="Enter"||e.key===" "){ e.preventDefault(); file.click(); }});
  file.addEventListener("change",()=>{ if(file.files[0]) upload(file.files[0]); });
  ["dragenter","dragover"].forEach(ev=>drop.addEventListener(ev,e=>{
    e.preventDefault(); drop.classList.add("over"); }));
  ["dragleave","drop"].forEach(ev=>drop.addEventListener(ev,e=>{
    e.preventDefault(); drop.classList.remove("over"); }));
  drop.addEventListener("drop",e=>{
    const f=e.dataTransfer.files[0]; if(f) upload(f); });

  function upload(f){
    if(!f.name.endsWith(".bin")){
      note.textContent="Not a firmware file. Look for one ending in .bin.";
      return;
    }
    wrap.classList.remove("hide");
    drop.textContent="Uploading "+f.name;
    note.textContent="Do not close this page or unplug the board.";
    const fd=new FormData(); fd.append("update",f);
    const xhr=new XMLHttpRequest();
    xhr.open("POST","/api/update");
    xhr.upload.onprogress=e=>{
      if(e.lengthComputable) bar.style.width=(e.loaded/e.total*100)+"%"; };
    xhr.onload=()=>{
      if(xhr.status===200){
        bar.style.width="100%"; drop.textContent="Installed";
        note.textContent="Restarting. This page reloads in about 15 seconds.";
        setTimeout(()=>location.reload(),15000);
      } else fail("Update did not finish. Still on the old firmware.");
    };
    xhr.onerror=()=>fail("Lost connection. Still on the old firmware.");
    function fail(msg){
      drop.textContent="Drop a .bin here, or click to choose";
      note.textContent=msg; wrap.classList.add("hide"); bar.style.width="0";
    }
  }
}

async function tick(){
  try{ paint(await (await fetch("/api/state")).json()); }
  catch(e){ $("status").textContent="Lost connection"; }
}

async function boot(){
  wire(); wireUpdate();
  try{ fillConfig(await (await fetch("/api/config")).json()); }catch(e){}
  await tick();
  const draw = async()=>{
    try{ chart((await (await fetch("/api/history")).json()).rh); }catch(e){} };
  await draw();
  setInterval(tick,2000);
  setInterval(draw,60000);
}
boot();
</script>
</body>
</html>
)HTMLDOC";
