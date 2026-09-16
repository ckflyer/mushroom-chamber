// =============================================================================
//  web_ui.h - the dashboard, embedded in flash.
//  Kept as one string so there is no filesystem upload step to get wrong.
//  Edit freely; it is plain HTML/CSS/JS and needs no build tooling.
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
  --ink:#0d1512; --panel:#141f1b; --panel2:#1a2723; --line:#223029;
  --text:#e6ebe4; --muted:#7d9086; --fog:#9fd8c8; --warm:#e0a45c;
  --alert:#e0705c;
  --r:14px;
}
*{box-sizing:border-box}
html,body{margin:0;padding:0}
body{
  background:var(--ink); color:var(--text);
  font-family:ui-sans-serif,system-ui,-apple-system,"Segoe UI",Roboto,sans-serif;
  font-size:16px; line-height:1.5;
  padding:20px 16px 48px; max-width:760px; margin:0 auto;
  font-variant-numeric:tabular-nums;
}
h1{font-size:19px;font-weight:600;margin:0;letter-spacing:-.01em}
h2{font-size:14px;font-weight:600;margin:0 0 12px;color:var(--muted)}
header{display:flex;align-items:center;justify-content:space-between;
  gap:16px;margin-bottom:22px}
.panel{background:var(--panel);border:1px solid var(--line);
  border-radius:var(--r);padding:18px;margin-bottom:14px}

/* ---- hero: the chamber itself ---- */
.hero{display:flex;gap:22px;align-items:stretch}
.gauge{flex:0 0 96px}
.gauge svg{display:block;width:96px;height:210px}
.readout{flex:1;min-width:0;display:flex;flex-direction:column}
.rhnum{font-size:52px;font-weight:600;line-height:1;letter-spacing:-.03em}
.rhnum span{font-size:22px;font-weight:400;color:var(--muted);margin-left:3px}
.status{margin-top:10px;font-size:15px;color:var(--fog)}
.status.warn{color:var(--warm)} .status.bad{color:var(--alert)}
.stats{display:flex;gap:20px;margin-top:auto;padding-top:16px;flex-wrap:wrap}
.stat b{display:block;font-size:17px;font-weight:600}
.stat i{font-style:normal;font-size:12px;color:var(--muted)}

/* ---- controls ---- */
.row{display:flex;align-items:center;justify-content:space-between;
  gap:12px;padding:11px 0;border-top:1px solid var(--line)}
.row:first-of-type{border-top:0}
.row label{flex:1;min-width:0}
.row .hint{display:block;font-size:12px;color:var(--muted);margin-top:1px}
input[type=number],input[type=text],select{
  background:var(--panel2);color:var(--text);border:1px solid var(--line);
  border-radius:9px;padding:8px 10px;font:inherit;font-size:15px;width:96px;
  text-align:right}
input[type=text],select{width:190px;text-align:left}
input:focus-visible,select:focus-visible,button:focus-visible,
.sw:focus-visible{outline:2px solid var(--fog);outline-offset:2px}
.wide{width:100%}
button{background:var(--panel2);color:var(--text);border:1px solid var(--line);
  border-radius:10px;padding:10px 15px;font:inherit;font-size:14px;cursor:pointer}
button:hover{border-color:var(--fog)}
button:disabled{opacity:.45;cursor:default}
.btns{display:flex;gap:10px;flex-wrap:wrap}

/* ---- switch ---- */
.sw{position:relative;width:50px;height:29px;border-radius:15px;
  background:var(--panel2);border:1px solid var(--line);cursor:pointer;
  flex:0 0 auto;padding:0}
.sw::after{content:"";position:absolute;top:3px;left:3px;width:21px;height:21px;
  border-radius:50%;background:var(--muted);transition:transform .16s,background .16s}
.sw[aria-checked=true]{background:#20403a;border-color:var(--fog)}
.sw[aria-checked=true]::after{transform:translateX(21px);background:var(--fog)}

/* ---- chart ---- */
.chart{width:100%;height:132px;display:block}
.chartfoot{display:flex;justify-content:space-between;font-size:12px;
  color:var(--muted);margin-top:6px}

details{margin-bottom:14px}
summary{cursor:pointer;padding:14px 18px;background:var(--panel);
  border:1px solid var(--line);border-radius:var(--r);font-size:14px;
  color:var(--muted);list-style:none}
summary::-webkit-details-marker{display:none}
summary::before{content:"+ ";font-weight:600}
details[open] summary{border-radius:var(--r) var(--r) 0 0;color:var(--text)}
details[open] summary::before{content:"\2212 "}
details .panel{border-radius:0 0 var(--r) var(--r);border-top:0;margin:0}
.note{font-size:13px;color:var(--muted);margin:14px 0 0}
.hide{display:none}
.drop{border:1.5px dashed var(--line);border-radius:12px;padding:26px 16px;
  text-align:center;color:var(--muted);font-size:14px;cursor:pointer;
  margin-top:4px}
.drop:hover,.drop.over{border-color:var(--fog);color:var(--text)}
.prog{height:8px;background:var(--panel2);border-radius:4px;margin-top:14px;
  overflow:hidden}
.prog i{display:block;height:100%;width:0;background:var(--fog);
  transition:width .15s}
#toast{position:fixed;left:50%;bottom:22px;transform:translateX(-50%);
  background:var(--panel2);border:1px solid var(--fog);color:var(--text);
  padding:9px 16px;border-radius:10px;font-size:14px;opacity:0;
  transition:opacity .2s;pointer-events:none}
#toast.on{opacity:1}
@media (max-width:430px){
  .hero{gap:16px}.gauge,.gauge svg{flex-basis:76px;width:76px}
  .rhnum{font-size:44px}
}
@media (prefers-reduced-motion:reduce){*{transition:none!important;
  animation:none!important}}
</style>
</head>
<body>

<header>
  <h1>Mushroom Chamber</h1>
  <div style="display:flex;align-items:center;gap:10px">
    <span id="modeLbl" style="font-size:14px;color:var(--muted)">Automatic</span>
    <button class="sw" id="autoSw" role="switch" aria-checked="true"
            aria-label="Automatic control"></button>
  </div>
</header>

<div class="panel">
  <div class="hero">
    <div class="gauge">
      <svg viewBox="0 0 96 210" aria-hidden="true">
        <defs>
          <clipPath id="box"><rect x="9" y="16" width="78" height="180" rx="8"/></clipPath>
        </defs>
        <rect x="9" y="16" width="78" height="180" rx="8" fill="#101a17"/>
        <g clip-path="url(#box)">
          <rect id="fill" x="9" y="196" width="78" height="0" fill="#9fd8c8"
                opacity=".22"/>
          <rect id="fillTop" x="9" y="196" width="78" height="2" fill="#9fd8c8"/>
        </g>
        <line id="tgtLine" x1="9" y1="100" x2="87" y2="100" stroke="#e6ebe4"
              stroke-width="1.5" stroke-dasharray="4 3"/>
        <line id="maxLine" x1="9" y1="60" x2="87" y2="60" stroke="#e0a45c"
              stroke-width="1.5" stroke-dasharray="2 3"/>
        <rect x="9" y="16" width="78" height="180" rx="8" fill="none"
              stroke="#223029" stroke-width="1.5"/>
      </svg>
    </div>
    <div class="readout">
      <div class="rhnum" id="rh">--<span>%</span></div>
      <div class="status" id="status">Connecting</div>
      <div class="stats">
        <div class="stat"><b id="temp">--</b><i>Temperature</i></div>
        <div class="stat"><b id="dew">--</b><i>Dew point</i></div>
        <div class="stat"><b id="vpd">--</b><i>VPD</i></div>
        <div class="stat"><b id="fan">--</b><i>Fan</i></div>
      </div>
    </div>
  </div>
</div>

<div class="panel">
  <div class="row">
    <label for="targetRh">Target humidity
      <span class="hint">What the chamber aims to hold.</span></label>
    <input type="number" id="targetRh" min="40" max="99" step="1">
  </div>
  <div class="row">
    <label for="maxRh">Maximum humidity
      <span class="hint">Above this for a while, the fan dries it down.
        Set 100 to switch that off.</span></label>
    <input type="number" id="maxRh" min="80" max="100" step="1">
  </div>
  <div class="row" id="manualFanRow" style="display:none">
    <label for="manualFan">Fan speed
      <span class="hint">Yours while Automatic is off.</span></label>
    <input type="number" id="manualFan" min="0" max="100" step="5">
  </div>
</div>

<div class="panel">
  <h2>Last 24 hours</h2>
  <svg class="chart" id="chart" viewBox="0 0 700 132" preserveAspectRatio="none"
       role="img" aria-label="Humidity over the last 24 hours"></svg>
  <div class="chartfoot"><span id="chartLeft">24h ago</span><span>now</span></div>
</div>

<div class="panel">
  <div class="row">
    <label for="faeIntervalMin">Fresh air every
      <span class="hint">Minutes between cycles. Set 0 to switch fresh air off.</span></label>
    <input type="number" id="faeIntervalMin" min="0" max="360" step="5">
  </div>
  <div class="row">
    <label for="faeDurationS">Fresh air for
      <span class="hint">Seconds. 45 already swaps the air in a small tub
        several times.</span></label>
    <input type="number" id="faeDurationS" min="5" max="600" step="5">
  </div>
</div>

<details>
  <summary>Fogging</summary>
  <div class="panel">
    <div class="row">
      <label for="fogBurstS">Burst length
        <span class="hint">Seconds. The fogger never runs longer than this at
          a stretch.</span></label>
      <input type="number" id="fogBurstS" min="1" max="60" step="1">
    </div>
    <div class="row">
      <label for="fogSettleS">Settle time
        <span class="hint">Seconds of quiet while fog turns into humidity.
          Readings are ignored. Raise this if humidity overshoots.</span></label>
      <input type="number" id="fogSettleS" min="15" max="900" step="15">
    </div>
    <div class="row">
      <label for="fogBudgetS">Fog limit per hour
        <span class="hint">Seconds. A hard cap on how much water can enter the
          chamber, whatever else goes wrong.</span></label>
      <input type="number" id="fogBudgetS" min="30" max="1800" step="15">
    </div>
    <div class="row">
      <label for="deadband">Humidity deadband
        <span class="hint">How far below target it may sag before fogging
          starts.</span></label>
      <input type="number" id="deadband" min="0.5" max="15" step="0.5">
    </div>
    <p class="note">A five second burst already delivers several times the water
      a ten point humidity rise needs. If humidity will not climb, adding fog
      makes puddles rather than humidity. Check for leaks and watch the dew
      point instead.</p>
  </div>
</details>

<details>
  <summary>Fogger wiring</summary>
  <div class="panel">
    <div class="row">
      <label for="foggerMode">How the fogger is switched</label>
      <select id="foggerMode">
        <option value="0">Not set up yet</option>
        <option value="1">Home Assistant, over MQTT</option>
        <option value="2">Relay or SSR on the board</option>
        <option value="3">Another device, over HTTP</option>
      </select>
    </div>
    <div id="modeMqtt" class="hide">
      <p class="note">Publishes a Fogger Request entity. Make one Home
        Assistant automation that mirrors it onto your plug. Set up the broker
        under Network below.</p>
    </div>
    <div id="modeRelay" class="hide">
      <div class="row">
        <label for="relayActiveHigh">Relay switches on when the pin is high
          <span class="hint">Turn off for active-low relay boards.</span></label>
        <button class="sw" id="relayActiveHigh" role="switch"></button>
      </div>
      <p class="note">Wired to GPIO33. Mains wiring belongs in a closed,
        fused enclosure.</p>
    </div>
    <div id="modeHttp" class="hide">
      <div class="row"><label for="httpOnUrl">Turn on URL</label>
        <input type="text" id="httpOnUrl" placeholder="http://192.168.1.50/switch/fogger/turn_on"></div>
      <div class="row"><label for="httpOffUrl">Turn off URL</label>
        <input type="text" id="httpOffUrl" placeholder="http://192.168.1.50/switch/fogger/turn_off"></div>
    </div>
  </div>
</details>

<details>
  <summary>Network and Home Assistant</summary>
  <div class="panel">
    <div class="row">
      <label for="mqttEnabled">Connect to an MQTT broker
        <span class="hint">Optional. Adds Home Assistant sensors and enables
          the MQTT fogger mode.</span></label>
      <button class="sw" id="mqttEnabled" role="switch"></button>
    </div>
    <div class="row"><label for="mqttHost">Broker address</label>
      <input type="text" id="mqttHost" placeholder="192.168.1.10"></div>
    <div class="row"><label for="mqttPort">Port</label>
      <input type="number" id="mqttPort" min="1" max="65535" step="1"></div>
    <div class="row"><label for="mqttUser">Username</label>
      <input type="text" id="mqttUser"></div>
    <div class="row"><label for="mqttPass">Password
      <span class="hint">Leave blank to keep the saved one.</span></label>
      <input type="text" id="mqttPass" placeholder="unchanged"></div>
    <div class="row">
      <label for="haDiscovery">Publish Home Assistant sensors</label>
      <button class="sw" id="haDiscovery" role="switch"></button>
    </div>
    <p class="note" id="mqttState">Not connected.</p>
  </div>
</details>

<details>
  <summary>Setup and tools</summary>
  <div class="panel">
    <div class="row">
      <label for="fanMinDuty">Slowest fan speed
        <span class="hint">Turn Automatic off, lower the fan until it stalls,
          then put that number plus five here.</span></label>
      <input type="number" id="fanMinDuty" min="5" max="60" step="1">
    </div>
    <div class="row">
      <label for="mixDurationS">Stir after fogging
        <span class="hint">Seconds. Leave at 0 unless you fit a separate
          internal circulation fan. Stirring with an exhaust fan just vents
          the chamber.</span></label>
      <input type="number" id="mixDurationS" min="0" max="120" step="5">
    </div>
    <div class="row">
      <label>Fan reading now<span class="hint" id="rpm">0 RPM</span></label>
      <button id="btnFan">Spin the fan</button>
    </div>
    <div class="row">
      <label>Single burst<span class="hint">Runs one burst by hand. Still
        respects the hourly limit.</span></label>
      <button id="btnFog">Run a test burst</button>
    </div>
    <div class="row">
      <label>Restart the controller<span class="hint" id="uptime"></span></label>
      <button id="btnReboot">Restart</button>
    </div>
  </div>
</details>

<details>
  <summary>Firmware update</summary>
  <div class="panel">
    <div class="row">
      <label>Installed version
        <span class="hint" id="fwver">Checking</span></label>
    </div>
    <div class="drop" id="drop" tabindex="0" role="button"
         aria-label="Choose a firmware file">
      Drop a firmware file here, or click to choose one
    </div>
    <input type="file" id="fwfile" accept=".bin" class="hide">
    <div class="prog hide" id="progWrap"><i id="progBar"></i></div>
    <p class="note" id="otaNote">Download the latest chamber-firmware.bin from
      the project's releases page, then drop it here. The chamber keeps running
      on the current firmware while the new one is written, so a failed upload
      changes nothing. Your settings are kept.</p>
  </div>
</details>

<div id="toast"></div>

<script>
const $ = id => document.getElementById(id);
let cfg = {}, ready = false;

const NUMS = ["targetRh","maxRh","deadband","fogBurstS","fogSettleS",
  "fogBudgetS","faeIntervalMin","faeDurationS","fanMinDuty","mixDurationS",
  "mqttPort"];
const TEXTS = ["httpOnUrl","httpOffUrl","mqttHost","mqttUser"];
const SWS = ["relayActiveHigh","mqttEnabled","haDiscovery"];

function toast(msg){
  const t = $("toast"); t.textContent = msg; t.classList.add("on");
  clearTimeout(t._h); t._h = setTimeout(()=>t.classList.remove("on"), 1600);
}

async function patch(body){
  try{
    const r = await fetch("/api/config",{method:"POST",
      headers:{"Content-Type":"application/json"},body:JSON.stringify(body)});
    if(!r.ok) throw 0;
    cfg = await r.json();
    toast("Saved");
  }catch(e){ toast("Could not save. Check the connection."); }
}

function wire(){
  NUMS.forEach(k=>{
    const el = $(k); if(!el) return;
    el.addEventListener("change",()=>patch({[k]: parseFloat(el.value)}));
  });
  TEXTS.forEach(k=>{
    const el = $(k); if(!el) return;
    el.addEventListener("change",()=>patch({[k]: el.value}));
  });
  SWS.forEach(k=>{
    const el = $(k); if(!el) return;
    el.addEventListener("click",()=>{
      const v = el.getAttribute("aria-checked") !== "true";
      el.setAttribute("aria-checked", v); patch({[k]: v});
    });
  });
  $("mqttPass").addEventListener("change",e=>{
    if(e.target.value){ patch({mqttPass:e.target.value}); e.target.value=""; }
  });
  $("foggerMode").addEventListener("change",e=>{
    patch({foggerMode: parseInt(e.target.value)}); showMode(e.target.value);
  });
  $("autoSw").addEventListener("click",()=>{
    const v = $("autoSw").getAttribute("aria-checked") !== "true";
    $("autoSw").setAttribute("aria-checked", v);
    $("modeLbl").textContent = v ? "Automatic" : "Manual";
    $("manualFanRow").style.display = v ? "none" : "";
    patch({autoMode: v});
  });
  $("manualFan").addEventListener("change",e=>{
    fetch("/api/fan?speed="+parseInt(e.target.value),{method:"POST"});
  });
  $("btnFan").addEventListener("click",()=>{
    fetch("/api/fan-test",{method:"POST"}); toast("Fan running for 15 seconds");
  });
  $("btnFog").addEventListener("click",()=>{
    fetch("/api/fog-burst",{method:"POST"}); toast("Burst requested");
  });
  $("btnReboot").addEventListener("click",()=>{
    if(confirm("Restart the controller?")){
      fetch("/api/restart",{method:"POST"}); toast("Restarting");
    }
  });
}

function wireUpdate(){
  const drop = $("drop"), file = $("fwfile");
  const wrap = $("progWrap"), bar = $("progBar"), note = $("otaNote");

  drop.addEventListener("click", ()=>file.click());
  drop.addEventListener("keydown", e=>{
    if(e.key==="Enter"||e.key===" "){ e.preventDefault(); file.click(); }
  });
  file.addEventListener("change", ()=>{ if(file.files[0]) upload(file.files[0]); });

  ["dragenter","dragover"].forEach(ev=>drop.addEventListener(ev,e=>{
    e.preventDefault(); drop.classList.add("over");
  }));
  ["dragleave","drop"].forEach(ev=>drop.addEventListener(ev,e=>{
    e.preventDefault(); drop.classList.remove("over");
  }));
  drop.addEventListener("drop", e=>{
    const f = e.dataTransfer.files[0];
    if(f) upload(f);
  });

  function upload(f){
    if(!f.name.endsWith(".bin")){
      note.textContent = "That is not a firmware file. Look for one ending "+
        "in .bin on the releases page."; return;
    }
    wrap.classList.remove("hide");
    drop.textContent = "Uploading " + f.name;
    note.textContent = "Do not close this page or unplug the board.";

    const fd = new FormData(); fd.append("update", f);
    const xhr = new XMLHttpRequest();
    xhr.open("POST","/api/update");
    xhr.upload.onprogress = e=>{
      if(e.lengthComputable) bar.style.width = (e.loaded/e.total*100)+"%";
    };
    xhr.onload = ()=>{
      if(xhr.status === 200){
        bar.style.width = "100%";
        drop.textContent = "Installed";
        note.textContent = "The chamber is restarting. This page will "+
          "reconnect on its own in about fifteen seconds.";
        setTimeout(()=>location.reload(), 15000);
      } else {
        drop.textContent = "Drop a firmware file here, or click to choose one";
        note.textContent = "The update did not finish. The chamber is still "+
          "running the old firmware, so nothing is broken. Try again.";
        wrap.classList.add("hide"); bar.style.width = "0";
      }
    };
    xhr.onerror = ()=>{
      drop.textContent = "Drop a firmware file here, or click to choose one";
      note.textContent = "Lost the connection during upload. The chamber is "+
        "still on the old firmware. Try again.";
      wrap.classList.add("hide"); bar.style.width = "0";
    };
    xhr.send(fd);
  }
}

function showMode(m){
  $("modeMqtt").classList.toggle("hide", m != 1);
  $("modeRelay").classList.toggle("hide", m != 2);
  $("modeHttp").classList.toggle("hide", m != 3);
}

function fillConfig(c){
  cfg = c;
  NUMS.forEach(k=>{ if($(k) && c[k]!==undefined && document.activeElement!==$(k))
    $(k).value = c[k]; });
  TEXTS.forEach(k=>{ if($(k) && c[k]!==undefined && document.activeElement!==$(k))
    $(k).value = c[k]; });
  SWS.forEach(k=>{ if($(k) && c[k]!==undefined)
    $(k).setAttribute("aria-checked", !!c[k]); });
  $("foggerMode").value = c.foggerMode;
  showMode(c.foggerMode);
  $("autoSw").setAttribute("aria-checked", !!c.autoMode);
  $("modeLbl").textContent = c.autoMode ? "Automatic" : "Manual";
  $("manualFanRow").style.display = c.autoMode ? "none" : "";
  ready = true;
}

// Gauge spans 60-100% RH, which is the range this box actually lives in.
const GT=16, GH=180, LO=60, HI=100;
const yFor = v => GT + GH * (1 - (Math.min(HI,Math.max(LO,v))-LO)/(HI-LO));

function paint(s){
  $("rh").innerHTML = (s.rh==null?"--":s.rh.toFixed(1)) + "<span>%</span>";
  $("temp").textContent = s.temp==null?"--":s.temp.toFixed(1)+"\u00B0";
  $("dew").textContent  = s.dew==null?"--":s.dew.toFixed(1)+"\u00B0";
  $("vpd").textContent  = s.vpd==null?"--":s.vpd.toFixed(2);
  $("fan").textContent  = s.fan + "%";
  $("rpm").textContent  = s.rpm + " RPM";
  $("uptime").textContent = "Running " + Math.floor(s.uptime/3600) + "h " +
    (Math.floor(s.uptime/60)%60) + "m";
  $("fwver").textContent = "Version " + (s.version || "unknown");
  $("mqttState").textContent = s.mqtt ? "Connected to the broker."
                                      : "Not connected.";

  const st = $("status");
  st.textContent = s.status;
  st.className = "status" + (s.fault||s.ceiling ? " bad"
                 : /drying|limit|Below/.test(s.status) ? " warn" : "");
  if(s.ceiling) st.textContent = s.status + " \u00B7 at the humidity ceiling";

  if(s.rh!=null){
    const y = yFor(s.rh);
    $("fill").setAttribute("y", y);
    $("fill").setAttribute("height", Math.max(0, GT+GH-y));
    $("fillTop").setAttribute("y", y-1);
  }
  if(ready){
    $("tgtLine").setAttribute("y1", yFor(cfg.targetRh));
    $("tgtLine").setAttribute("y2", yFor(cfg.targetRh));
    const showMax = cfg.maxRh < 100;
    $("maxLine").style.display = showMax ? "" : "none";
    if(showMax){
      $("maxLine").setAttribute("y1", yFor(cfg.maxRh));
      $("maxLine").setAttribute("y2", yFor(cfg.maxRh));
    }
  }
}

function chart(vals){
  const svg = $("chart");
  if(!vals || vals.length < 2){
    svg.innerHTML = '<text x="350" y="70" fill="#7d9086" font-size="13" ' +
      'text-anchor="middle">Collecting readings</text>';
    return;
  }
  const W=700,H=132,P=6;
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
       'stroke-width="1" stroke-dasharray="4 3" opacity=".45"/>';
  }
  g+='<path d="'+d+'" fill="none" stroke="#9fd8c8" stroke-width="2" '+
     'stroke-linejoin="round"/>';
  g+='<text x="4" y="12" fill="#7d9086" font-size="11">'+hi.toFixed(0)+'%</text>';
  g+='<text x="4" y="128" fill="#7d9086" font-size="11">'+lo.toFixed(0)+'%</text>';
  svg.innerHTML=g;
  $("chartLeft").textContent = vals.length + " min ago";
}

async function tick(){
  try{
    const s = await (await fetch("/api/state")).json();
    paint(s);
  }catch(e){ $("status").textContent = "Lost connection to the chamber"; }
}

async function boot(){
  wire();
  wireUpdate();
  try{ fillConfig(await (await fetch("/api/config")).json()); }catch(e){}
  await tick();
  try{ chart((await (await fetch("/api/history")).json()).rh); }catch(e){}
  setInterval(tick, 2000);
  setInterval(async()=>{
    try{ chart((await (await fetch("/api/history")).json()).rh); }catch(e){}
  }, 60000);
}
boot();
</script>
</body>
</html>
)HTMLDOC";
