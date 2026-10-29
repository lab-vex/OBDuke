// OBDuke web page - served from the ESP32 at http://1.1.1.1
// Single file: HTML + CSS + JS. Edit freely, then re-upload the firmware.
#pragma once

const char ICON_SVG[] PROGMEM = R"SVG(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 512 512">
<rect width="512" height="512" rx="96" fill="#0a0a0a"/>
<path d="M150 400 L270 112 L362 112 L242 400 Z" fill="#ff6600"/>
<circle cx="156" cy="256" r="64" fill="none" stroke="#f2f2f2" stroke-width="28"/>
<path d="M330 192 h40 a64 64 0 0 1 0 128 h-40" fill="none" stroke="#f2f2f2" stroke-width="28"/>
</svg>)SVG";

const char WEB_UI[] PROGMEM = R"OBDUKE(<!doctype html>
<html lang="en"><head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<meta name="theme-color" content="#ff6600">
<link rel="manifest" href="/manifest.json">
<link rel="icon" href="/icon.svg">
<title>OBDuke</title>
<style>
:root{--o:#ff6600;--o2:#ff8a3d;--bg:#0a0a0a;--card:#151515;--card2:#1d1d1d;--line:#2b2b2b;--tx:#f2f2f2;--mut:#9a9a9a;--bad:#ff3b30;--ok:#30d158;--warn:#ffcc00}
*{box-sizing:border-box;margin:0;padding:0}
html,body{background:var(--bg);color:var(--tx);font-family:-apple-system,"Segoe UI",Roboto,Arial,sans-serif;-webkit-tap-highlight-color:transparent}
body{padding-bottom:calc(72px + env(safe-area-inset-bottom))}
header{position:sticky;top:0;z-index:5;display:flex;align-items:center;gap:12px;padding:12px 16px;background:#000;border-bottom:3px solid var(--o)}
.logo{font-weight:900;font-style:italic;font-size:24px;letter-spacing:1px;text-transform:uppercase}
.logo b{color:var(--o)}
.slash{width:14px;height:26px;background:var(--o);transform:skewX(-20deg)}
.pill{margin-left:auto;font-size:12px;font-weight:700;padding:5px 10px;border-radius:20px;background:var(--card2);color:var(--mut);text-transform:uppercase}
.pill.ok{background:rgba(48,209,88,.15);color:var(--ok)}
.pill.bad{background:rgba(255,59,48,.15);color:var(--bad)}
main{max-width:900px;margin:0 auto;padding:16px}
section{display:none}section.on{display:block}
h2{font-size:14px;font-weight:800;font-style:italic;text-transform:uppercase;letter-spacing:1px;color:var(--o);margin:18px 0 10px}
h2:first-child{margin-top:4px}
.grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(150px,1fr));gap:10px}
.tile{background:var(--card);border:1px solid var(--line);border-radius:12px;padding:12px 14px;position:relative;overflow:hidden}
.tile .k{font-size:11px;font-weight:700;color:var(--mut);text-transform:uppercase;letter-spacing:.5px}
.tile .v{font-size:30px;font-weight:800;font-variant-numeric:tabular-nums;margin-top:4px;line-height:1.1}
.tile .u{font-size:13px;color:var(--mut);font-weight:600;margin-left:3px}
.tile .bar{position:absolute;left:0;bottom:0;height:4px;background:var(--o);width:0;transition:width .3s}
.tile.big{grid-column:span 2}.tile.big .v{font-size:44px}
.tile.rpm{padding-bottom:10px}
.tile.rpm .bar{position:relative;width:100%!important;height:12px;border-radius:3px;margin-top:2px;background:linear-gradient(90deg,var(--o) 0 calc(var(--w,0)*1%),#262626 0)}
.scale{position:relative;height:26px;margin:6px 4px 0}
.scale em{position:absolute;right:0;top:-18px;font-size:10px;font-style:normal;color:var(--mut)}
.scale i{position:absolute;bottom:0;width:1px;height:8px;background:#555;font-style:normal}
.scale i b{position:absolute;bottom:10px;left:50%;transform:translateX(-50%);font-size:11px;font-weight:700;color:var(--mut)}
.scale i:nth-of-type(n+11){background:var(--bad)}.scale i:nth-of-type(n+11) b{color:var(--bad)}
.tile.warn{border-color:var(--bad)}.tile.warn .v{color:var(--bad)}.tile.warn .bar{background:var(--bad)}
.tile.na .v{color:#555}
.card{background:var(--card);border:1px solid var(--line);border-radius:12px;padding:14px;margin-bottom:10px}
.row{display:flex;gap:10px;flex-wrap:wrap}
button,.btn{appearance:none;border:0;border-radius:10px;padding:12px 16px;font-size:15px;font-weight:800;text-transform:uppercase;letter-spacing:.5px;font-style:italic;background:var(--card2);color:var(--tx);border:1px solid var(--line);cursor:pointer;text-decoration:none;display:inline-block}
button.p,.btn.p{background:var(--o);border-color:var(--o);color:#000}
button.d{background:var(--bad);border-color:var(--bad);color:#fff}
button:disabled{opacity:.4}
button:active{transform:scale(.97)}
.code{display:flex;gap:12px;align-items:center;padding:12px 0;border-bottom:1px solid var(--line)}
.code:last-child{border:0}
.code .c{font-family:ui-monospace,Menlo,monospace;font-weight:800;font-size:18px;color:var(--o);min-width:72px}
.code .t{font-size:11px;font-weight:700;padding:3px 8px;border-radius:6px;background:var(--card2);color:var(--mut);text-transform:uppercase}
.big-status{display:flex;align-items:center;gap:14px}
.lamp{width:46px;height:46px;border-radius:50%;background:#222;border:3px solid #333;flex:none}
.lamp.on{background:var(--warn);border-color:#ffe066;box-shadow:0 0 18px rgba(255,204,0,.6)}
.lamp.off{background:#1a3d22;border-color:var(--ok)}
.mut{color:var(--mut);font-size:13px}
pre{background:#000;border:1px solid var(--line);border-radius:10px;padding:12px;font-size:12px;overflow:auto;white-space:pre-wrap;word-break:break-all;max-height:55vh;color:#ddd}
input,select{width:100%;background:#000;color:var(--tx);border:1px solid var(--line);border-radius:8px;padding:11px;font-size:15px}
input:focus,select:focus{outline:2px solid var(--o);border-color:transparent}
label{display:block;font-size:13px;color:var(--mut);margin:12px 0 6px}
.sw{display:flex;align-items:center;justify-content:space-between;margin-top:12px;gap:12px}
.sw label{margin:0}
.sw input{width:auto;transform:scale(1.4);accent-color:var(--o)}
.small{font-size:12px;padding:6px 10px}
nav{position:fixed;left:0;right:0;bottom:0;z-index:5;display:flex;background:#000;border-top:1px solid var(--line);padding-bottom:env(safe-area-inset-bottom)}
nav button{flex:1;border:0;border-radius:0;background:none;padding:12px 4px 14px;font-size:11px;color:var(--mut);font-style:normal}
nav button.on{color:var(--o);box-shadow:inset 0 3px 0 var(--o)}
nav svg{display:block;margin:0 auto 4px;width:22px;height:22px;fill:none;stroke:currentColor;stroke-width:2}
#toast{position:fixed;left:50%;bottom:90px;transform:translateX(-50%) translateY(20px);opacity:0;background:var(--o);color:#000;font-weight:800;padding:12px 18px;border-radius:10px;transition:.25s;z-index:9;max-width:90vw;text-align:center}
#toast.show{opacity:1;transform:translateX(-50%)}
.kv{display:grid;grid-template-columns:auto 1fr;gap:6px 14px;font-size:14px}
.kv div:nth-child(odd){color:var(--mut)}
.spin{display:inline-block;width:14px;height:14px;border:2px solid #000;border-top-color:transparent;border-radius:50%;animation:s 1s linear infinite;vertical-align:-2px}
@keyframes s{to{transform:rotate(360deg)}}
.tile .pk{font-size:11px;color:var(--mut);font-weight:700;margin-top:2px;min-height:13px}
.mono{font-family:ui-monospace,Menlo,monospace;font-size:13px}
.sn{background:#000;border:1px solid var(--line);border-radius:10px;padding:8px 10px;max-height:60vh;overflow:auto}
.sr{padding:5px 0;border-bottom:1px solid #1a1a1a}
.sr:last-child{border:0}
.sr .id{display:inline-block;min-width:44px;color:var(--o);font-weight:800}
.sr .hz{display:inline-block;min-width:58px;color:var(--mut);font-size:11px}
.sr i{font-style:normal;padding:1px 2px;border-radius:3px;margin-right:2px}
.sr i.m{color:var(--o)}
.sr i.c{background:var(--o);color:#000}
.sr .rg{font-size:11px;color:var(--mut);margin:2px 0 0 44px}
.sr.stale{opacity:.45}
.sr .kxl{margin:2px 0 0 44px;font-size:12px;color:var(--o);cursor:pointer}
.kxd{margin:4px 0 2px 44px;padding:6px 8px;background:#0d0d0d;border-radius:8px;font-size:12px}
.kxf{padding:3px 0;border-bottom:1px solid #1a1a1a}
.kxf:last-child{border:0}
.kxf.bad{opacity:.5}
.kxv{font-family:ui-monospace,Menlo,monospace;color:var(--tx);font-weight:700}
.kxs{font-size:11px;color:var(--mut);word-break:break-word}
.mod{display:flex;gap:10px;align-items:center;flex-wrap:wrap;padding:10px 0;border-bottom:1px solid var(--line)}
.mod:last-child{border:0}
.mod .c{font-family:ui-monospace,Menlo,monospace;font-weight:800;color:var(--o)}
.chk{display:flex;align-items:center;gap:8px;font-size:13px;color:var(--mut);margin:10px 0 0}
.chk input{width:auto;accent-color:var(--o);transform:scale(1.2)}
</style></head><body>
<header><div class="slash"></div><div class="logo">OB<b>DUKE</b></div><div id="conn" class="pill">…</div></header>
<main>

<section id="dash" class="on">
  <div class="grid" id="tiles"></div>
  <p class="mut" style="margin-top:12px" id="dashnote"></p>
  <div class="row" style="margin-top:10px"><button class="small" onclick="cmd('peak reset').then(r=>toast(r.msg))">Reset peaks</button></div>
</section>

<section id="codes">
  <div class="card big-status">
    <div class="lamp" id="lamp"></div>
    <div><div style="font-weight:800;font-size:18px" id="fitext">FI light: unknown</div>
    <div class="mut" id="fisub">Tap Read codes</div></div>
  </div>
  <div class="row" style="margin:12px 0">
    <button class="p" onclick="readCodes()">Read codes</button>
    <button onclick="freeze()">Freeze frame</button>
    <button class="d" onclick="clearCodes()">Clear codes</button>
  </div>
  <div class="card" id="codelist"><p class="mut">No codes read yet.</p></div>
  <div class="card" id="freezebox" style="display:none"></div>
  <h2>ABS &amp; other modules</h2>
  <div class="row"><button onclick="findModules()">Find modules</button></div>
  <p class="mut" style="margin-top:6px">Key on, engine off. Pings every diagnostic address (about 10 s), then you can read or clear each module's codes.</p>
  <div class="card" id="modlist" style="margin-top:10px;display:none"></div>
  <div class="card" id="modcodes" style="display:none"></div>
</section>

<section id="tools">
  <h2>Info</h2>
  <div class="row">
    <button onclick="tool('pids')">Supported PIDs</button>
    <button onclick="tool('vin')">VIN</button>
    <button onclick="tool('ident')">ECU identity</button>
    <button onclick="tool('discover')">Service check</button>
  </div>
  <h2>KTM / reverse engineering</h2>
  <div class="row">
    <button onclick="tool('ktm')">KTM extended</button>
    <button onclick="tool('scan')">Scan local IDs</button>
    <button onclick="tool('monitor 3000')">Bus monitor 3s</button>
  </div>
  <h2>Bus sniffer</h2>
  <div class="row">
    <button class="p" id="snbtn" onclick="sniffToggle()">Start</button>
    <button onclick="cmd('sniff mark').then(()=>toast('Marked. Now spin a wheel / move a lever.'))">Mark</button>
    <button onclick="cmd('sniff clear')">Clear</button>
  </div>
  <label class="chk"><input type="checkbox" id="snhide" checked onchange="sniffDraw()">Hide diagnostic replies (7xx)</label>
  <label class="chk"><input type="checkbox" id="snchg" onchange="sniffDraw()">Only IDs changed since mark</label>
  <label class="chk"><input type="checkbox" id="snunk" onchange="sniffDraw()">Only IDs not in the KTM list</label>
  <p class="mut" style="margin:8px 0">Everything the bike broadcasts. Orange box = changed just now, orange text = changed since Mark (with its range below). Tap Mark, spin the front wheel by hand, and watch which bytes light up. An orange name under an ID means other KTMs use it: tap the name for the byte map, what to test, and a Custom signals line.</p>
  <div class="sn mono" id="sniff"><span class="mut">Not running.</span></div>
  <div class="row" style="margin-top:8px"><button class="small" onclick="sniffCopy()">Copy capture report</button></div>
  <textarea id="snrep" class="mono" readonly style="display:none;width:100%;height:150px;margin-top:8px;background:#000;color:#ddd;border:1px solid var(--line);border-radius:8px"></textarea>
  <p class="mut" style="margin-top:8px">Found one? Add it in Settings &gt; Custom signals, e.g. <span class="mono">12B:0:2:b:0.05627:0:Front wheel:km/h</span></p>

  <h2>Watch a request</h2>
  <div class="row"><input id="watchin" class="mono" placeholder="21 22   or   @7B0 21 05" style="flex:1;min-width:160px"><button class="p" id="wabtn" onclick="watchToggle()">Watch</button><button onclick="watchMark()">Mark</button></div>
  <p class="mut" style="margin:8px 0">Reads the same ECU value over and over and highlights bytes that move. Good for finding what's inside the KTM local IDs (21xx). Read-only requests only.</p>
  <div class="sn mono" id="watch"><span class="mut">Not running.</span></div>

  <h2>Gear calibration</h2>
  <p class="mut" style="margin-bottom:8px">Ride steady above 15 km/h, clutch out, and tap the gear you're in. Only needed if the gear shown on the Dash is wrong. Saved straight away.</p>
  <div class="row" id="gearbtns"></div>
  <p class="mut mono" style="margin-top:8px" id="gearinfo"></p>

  <h2>Raw command</h2>
  <div class="row"><input id="rawin" placeholder="e.g. 22 F1 90" style="flex:1;min-width:160px"><button onclick="tool('raw '+$('#rawin').value)">Send</button></div>
  <p class="mut" style="margin-top:6px">Needs "Allow raw ECU commands" in Settings.</p>
  <h2>Output</h2>
  <pre id="out">Pick a tool.</pre>
</section>

<section id="log">
  <div class="card">
    <div class="kv" id="logkv"></div>
  </div>
  <div class="row">
    <button class="p" onclick="logc('start')">Start</button>
    <button onclick="logc('stop')">Stop</button>
    <button onclick="logc('clear')">Clear</button>
    <a class="btn" href="/log.csv">Download CSV</a>
  </div>
  <p class="mut" style="margin-top:12px">Logs RPM, throttle, MAP, speed, temps, advance, fuel trims, O2, battery, gear and your custom signals. Stored in memory until power off. Interval is in Settings.</p>
</section>

<section id="set">
  <div id="setform"><p class="mut">Loading…</p></div>
  <div class="row" style="margin-top:16px">
    <button class="p" onclick="saveSettings()">Save</button>
    <button onclick="cmd('reboot').then(r=>toast(r.msg))">Reboot</button>
    <button class="d" onclick="factory()">Factory reset</button>
  </div>
  <h2>About</h2>
  <div class="card"><div class="kv" id="about"></div></div>
</section>

</main>

<nav>
  <button class="on" data-s="dash"><svg viewBox="0 0 24 24"><path d="M4 18a8 8 0 1 1 16 0"/><path d="M12 18l4-6"/></svg>Dash</button>
  <button data-s="codes"><svg viewBox="0 0 24 24"><path d="M12 3l9 16H3z"/><path d="M12 10v4M12 17v.5"/></svg>Codes</button>
  <button data-s="tools"><svg viewBox="0 0 24 24"><path d="M14 7l3-3 3 3-3 3M17 4L8 13l-4 7 7-4 9-9"/></svg>Tools</button>
  <button data-s="log"><svg viewBox="0 0 24 24"><path d="M4 20V4M4 20h16M8 16l4-5 3 3 5-7"/></svg>Log</button>
  <button data-s="set"><svg viewBox="0 0 24 24"><circle cx="12" cy="12" r="3"/><path d="M12 2v3M12 19v3M2 12h3M19 12h3M5 5l2 2M17 17l2 2M5 19l2-2M17 7l2-2"/></svg>Settings</button>
</nav>
<div id="toast"></div>

<script>
const $=s=>document.querySelector(s);
let S={units:0,live_ms:500,temp_warn:105,rpm_warn:9500,vbat_low:11.8},cur='dash',liveT=null,fields=[];
const imp=()=>S.units==1;
const T=[
 {k:'rpm',l:'RPM',u:'',max:15000,big:1,dp:0},
 {k:'tps',l:'Throttle',u:'%',max:100,dp:1},
 {k:'spd',l:'Speed',u:'km/h',max:180,dp:0,c:v=>imp()?v*0.6214:v,cu:'mph'},
 {k:'ect',l:'Coolant',u:'°C',max:120,dp:0,c:v=>imp()?v*1.8+32:v,cu:'°F'},
 {k:'iat',l:'Intake air',u:'°C',max:80,dp:0,c:v=>imp()?v*1.8+32:v,cu:'°F'},
 {k:'map',l:'MAP',u:'kPa',max:110,dp:0},
 {k:'adv',l:'Ign advance',u:'°',max:50,dp:1},
 {k:'stft',l:'Short trim',u:'%',max:25,dp:1,abs:1},
 {k:'ltft',l:'Long trim',u:'%',max:25,dp:1,abs:1},
 {k:'o2',l:'O2 sensor',u:'V',max:1,dp:2},
 {k:'vbat',l:'Battery',u:'V',max:15,dp:2},
 {k:'gear',l:'Gear',u:'',max:6,dp:0}
];
const PK={rpm:'rpm',spd:'spd',ect:'ect'};
function tilesList(){return T.concat((S.sigs||[]).map(g=>({k:g.k,l:g.n,u:g.u,max:0,dp:2,sig:1})))}
function toast(m){const t=$('#toast');t.textContent=m;t.classList.add('show');clearTimeout(t._t);t._t=setTimeout(()=>t.classList.remove('show'),2600)}
async function cmd(c){
  const r=await fetch('/api',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'c='+encodeURIComponent(c)});
  return r.json();
}
function busy(b,on){if(!b)return;if(on){b._h=b.innerHTML;b.innerHTML='<span class="spin"></span>';b.disabled=true}else{b.innerHTML=b._h;b.disabled=false}}
async function act(fn){const b=document.activeElement&&document.activeElement.tagName=='BUTTON'?document.activeElement:null;busy(b,1);try{await fn()}catch(e){toast('Lost connection to OBDuke')}busy(b,0)}

// ---------- tabs ----------
document.querySelectorAll('nav button').forEach(b=>b.onclick=()=>{
  document.querySelectorAll('nav button').forEach(x=>x.classList.toggle('on',x==b));
  document.querySelectorAll('section').forEach(s=>s.classList.toggle('on',s.id==b.dataset.s));
  cur=b.dataset.s;
  if(cur=='set')loadSettings();
  if(cur=='tools')cmd('status').then(s=>{Object.assign(S,s);gearUI(s.gears)});
  if(cur=='log')logc('status');
  live();
});

// ---------- dashboard ----------
function buildTiles(){
  const scale='<div class="scale"><em>×1000 rpm</em>'+Array.from({length:16},(_,i)=>`<i style="left:${i/15*100}%"><b>${i}</b></i>`).join('')+'</div>';
  $('#tiles').innerHTML=tilesList().map(t=>`<div class="tile na${t.big?' big':''}${t.k=='rpm'?' rpm':''}" id="t_${t.k}"><div class="k">${esc(t.l)}</div><div class="v"><span>--</span><span class="u">${esc(imp()&&t.cu?t.cu:t.u)}</span></div>${t.k=='rpm'?scale:''}${PK[t.k]?'<div class="pk"></div>':''}<div class="bar"></div></div>`).join('');
}
function showLive(d){
  setConn(d.ecu);
  tilesList().forEach(t=>{
    const el=$('#t_'+t.k);if(!el)return;
    let v=d[t.k];
    if(v===null||v===undefined){el.classList.add('na');el.querySelector('span').textContent='--';const b=el.querySelector('.bar');b.style.width='0';b.style.setProperty('--w',0);return}
    el.classList.remove('na');
    const raw=v; if(t.c)v=t.c(v);
    el.querySelector('span').textContent=t.k=='gear'?(v?v:'–'):v.toFixed(t.dp);
    const pk=el.querySelector('.pk');
    if(pk&&d.pk&&d.pk[t.k]!=null){let p=d.pk[t.k];if(t.c)p=t.c(p);pk.textContent='max '+Math.round(p)}
    const pct=t.max?Math.min(100,Math.max(0,(t.abs?Math.abs(raw):raw)/t.max*100)):0;
    if(t.k=='rpm')el.querySelector('.bar').style.setProperty('--w',pct);else el.querySelector('.bar').style.width=pct+'%';
    let w=false;
    if(t.k=='ect')w=raw>=S.temp_warn;
    if(t.k=='rpm')w=raw>=S.rpm_warn;
    if(t.k=='vbat')w=raw<S.vbat_low;
    el.classList.toggle('warn',w);
  });
  if(d.fi==1)$('#dashnote').innerHTML='<b style="color:var(--warn)">FI light is ON</b> — '+d.dtc+' code(s). See Codes.';
  else $('#dashnote').textContent=d.ecu?'':'Waiting for ECU — turn the key on.';
}
function setConn(ecu){const c=$('#conn');c.textContent=ecu?'ECU OK':'No ECU';c.className='pill '+(ecu?'ok':'bad')}
async function live(){
  clearTimeout(liveT);
  if(cur!='dash'||document.hidden)return;
  const t0=Date.now();
  try{showLive(await (await fetch('/live')).json())}catch(e){$('#conn').textContent='Offline';$('#conn').className='pill bad'}
  liveT=setTimeout(live,Math.max(10,Math.max(100,S.live_ms)-(Date.now()-t0)));
}
document.addEventListener('visibilitychange',live);

// ---------- codes ----------
function codeRows(list,tag){return (list||[]).map(c=>`<div class="code"><div class="c">${c.code}</div><div style="flex:1">${c.desc||'Unknown code'}</div><div class="t">${tag}</div></div>`).join('')}
function readCodes(){act(async()=>{
  const r=await cmd('codes');
  if(!r.ok){toast(r.msg);return}
  const lamp=$('#lamp');lamp.className='lamp '+(r.fi?'on':'off');
  $('#fitext').textContent='FI light: '+(r.fi?'ON':'off');
  let sub=[];if(r.count!==undefined)sub.push(r.count+' code(s) reported');
  if(r.km_fi!==undefined)sub.push(r.km_fi+' km with light on');
  if(r.km_clr!==undefined)sub.push(r.km_clr+' km since clear');
  $('#fisub').textContent=sub.join(' · ');
  const h=codeRows(r.stored,'Stored')+codeRows(r.pending,'Pending');
  $('#codelist').innerHTML=h||'<p style="color:var(--ok);font-weight:800">No stored or pending codes.</p>';
})}
function clearCodes(){
  if(!confirm('Clear all codes and turn the FI light off?\n\nKey on, engine off works best.'))return;
  act(async()=>{const r=await cmd('clear yes');toast(r.ok?'Codes nuked. Hail to the king, baby.':r.msg);if(r.ok)readCodes()})
}
function freeze(){act(async()=>{
  const r=await cmd('freeze');const b=$('#freezebox');b.style.display='block';
  if(!r.code){b.innerHTML='<p class="mut">'+(r.msg||'No freeze frame')+'</p>';return}
  const names={ect:'Coolant °C',iat:'Intake °C',tps:'Throttle %',map:'MAP kPa',spd:'Speed km/h',adv:'Advance °',stft:'Short trim %',ltft:'Long trim %',o2:'O2 V'};
  b.innerHTML='<h2 style="margin-top:0">Freeze frame — '+r.code+'</h2><p class="mut" style="margin-bottom:10px">'+(r.desc||'')+'</p><div class="kv">'+
    Object.entries(r.vals).map(([k,v])=>`<div>${names[k]||k}</div><div>${v}</div>`).join('')+'</div>';
})}

// ---------- tools ----------
function fmtTool(r){
  if(!r.ok)return 'Error: '+r.msg;
  switch(r.t){
    case 'pids':return 'Supported mode 01 PIDs:\n'+r.list.join(' ')+'\n\nKTM extended RPM (2121): '+(r.ktm_rpm?'yes':'no');
    case 'vin':return 'VIN: '+r.vin;
    case 'ident':case 'ktm':case 'scan':case 'discover':
      return (r.items.length?r.items.map(i=>i.k.padEnd(28)+(i.msg||i.hex+'\n'+' '.repeat(28)+'|'+i.ascii+'|')).join('\n'):'Nothing answered.')+(r.rpm!==undefined?'\n\nRPM: '+r.rpm:'');
    case 'monitor':return 'ID   count  data                      changed\n'+r.ids.map(i=>i.id.padEnd(5)+String(i.n).padEnd(7)+i.data.padEnd(26)+(i.chg?'*':'')).join('\n')+(r.ids.length?'':'(no broadcast traffic)');
    case 'raw':return 'Sent: '+r.req+'\nGot:  '+r.hex+'\n      |'+r.ascii+'|';
  }
  return JSON.stringify(r,null,2);
}
function tool(c){act(async()=>{$('#out').textContent='Working… (scan takes about a minute)';const r=await cmd(c);$('#out').textContent=fmtTool(r)})}

// ---------- modules (ABS etc) ----------
const esc=x=>String(x==null?'':x).replace(/[&<>"]/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;'}[c]));
function findModules(){act(async()=>{
  const b=$('#modlist');b.style.display='block';b.innerHTML='<p class="mut">Scanning… about 10 seconds.</p>';
  const r=await cmd('modules');
  if(!r.ok){b.innerHTML='<p class="mut">'+esc(r.msg)+'</p>';return}
  if(!r.list.length){b.innerHTML='<p class="mut">Nothing answered. Is the key on?</p>';return}
  b.innerHTML=r.list.map(m=>`<div class="mod"><div class="c">${m.tx}</div><div style="flex:1;min-width:140px"><b>${esc(m.name)}</b><div class="mut">${esc(m.info)||'replies on '+m.rx}</div></div><div class="row" style="gap:6px;flex-wrap:nowrap"><button class="small" onclick="modCodes('${m.tx}')">Read codes</button><button class="small d" onclick="modClear('${m.tx}')">Clear</button></div></div>`).join('');
})}
function modCodes(tx){act(async()=>{
  const b=$('#modcodes');b.style.display='block';
  const r=await cmd('mcodes '+tx);
  if(!r.ok){b.innerHTML='<p class="mut">'+esc(r.msg)+'</p>';return}
  b.innerHTML='<h2 style="margin-top:0">Module '+r.tx+' codes ('+r.proto+')</h2>'+(r.list.length?r.list.map(c=>`<div class="code"><div class="c">${c.code}</div><div style="flex:1">${esc(c.desc)||'See the KTM manual for this module'}</div><div class="t">${c.st?'status '+c.st:''}</div></div>`).join(''):'<p style="color:var(--ok);font-weight:800">No codes stored.</p>');
})}
function modClear(tx){
  if(!confirm('Clear all codes in module '+tx+'?\n\nKey on, engine off.'))return;
  act(async()=>{const r=await cmd('mclear '+tx+' yes');toast(r.msg);if(r.ok)modCodes(tx)})
}

// ---------- known KTM CAN IDs (table generated from can_ids.json by tools/gen_can_ids.py) ----------
/*KIDS-BEGIN*/const KIDS={"bikes":{"690":"KTM 690 Enduro R, 2020","Adv390":"KTM 390 Adventure (Gen 2), 2026","701":"Husqvarna 701 Enduro (Euro 5), 2023","1290":"KTM 1290 Super Duke R, 2017","890":"KTM 890 Duke (790 Duke forum posts)","1390":"KTM 1390 Super Duke R Evo, 2024"},"ids":[{"id":"120","name":"Engine: rpm, throttle, switches","ms":20,"from":"engine ECU","try":"Key on, then rev the engine or twist the grip. Flip the kill switch. Press start.","fields":[{"name":"Engine speed","b":0,"n":2,"unit":"rpm","min":0,"max":14000,"bikes":{"690":"confirmed","Adv390":"confirmed","1390":"confirmed"}},{"name":"Throttle grip (commanded)","b":2,"mul":0.3953,"dp":0,"unit":"%","min":0,"max":101,"bikes":{"690":"confirmed","Adv390":"confirmed","890":"reported","1390":"capture"},"note":"raw 0 = closed, 253-255 = fully open. 1390 Evo: 0 at idle, 24-44 on the blips in the start-up log"},{"name":"Kill switch","b":3,"shift":4,"bits":1,"yes":"RUN","no":"STOP","bikes":{"690":"confirmed"}},{"name":"Kill switch","b":3,"shift":3,"bits":1,"yes":"RUN","no":"STOP","bikes":{"Adv390":"confirmed","1390":"confirmed"},"note":"the same bit the 690 uses for the kill switch is the start button here. 1390 Evo: 08 with the switch on RUN, 00 on STOP"},{"name":"Start button","b":3,"shift":4,"bits":1,"yes":"PRESSED","no":"-","bikes":{"Adv390":"confirmed","1390":"confirmed"},"note":"1390 Evo: D3 = 18 while cranking"},{"name":"Throttle map in use","b":4,"shift":0,"bits":1,"map":{"0":"map 1","1":"map 2"},"bikes":{"690":"confirmed"}},{"name":"Ride mode","b":4,"map":{"0":"Street","1":"Offroad","2":"Rain"},"bikes":{"Adv390":"capture"},"note":"names assumed from the menu order"},{"b":7,"text":"alive counter, steps by 0x20 (30, 50, 70, 90, B0, D0)"}]},{"id":"121","name":"Throttle grip sensors","ms":20,"from":"engine ECU","try":"Twist the grip with the key on. Two bytes should rise together.","note":"D1 and D3 rise with the grip: probably the two redundant ride-by-wire sensors. 1390 Evo: D5 tracked engine speed closely in the start-up log (correlation 0.8), so the layout may differ there.","fields":[{"name":"Grip sensor A?","b":1,"bikes":{"690":"candidate"}},{"name":"Grip sensor B?","b":3,"bikes":{"690":"candidate"}},{"b":7,"text":"rolling counter"}]},{"id":"128","name":"Throttle closed switch","ms":20,"from":"key-on","try":"Key on: twist the grip off its stop and let go.","note":"690: reads 7F FF ... until the ECU wakes, then D2 toggles 02/03 with the engine running. 1390 Evo: D0-D1 stay 13 88 and D2 steps 01, 02, 03 through a start-up, so there it is a state, not a throttle switch (the repo README says gear and clutch live here, but its clutch log shows them in 129).","fields":[{"name":"Throttle","b":2,"shift":0,"bits":1,"yes":"closed","no":"open","bikes":{"Adv390":"confirmed","690":"candidate"}}]},{"id":"129","name":"Gear and clutch","ms":20,"from":"engine ECU","try":"Key on, engine off: roll the bike and change gear. Pull the clutch lever.","note":"701 Enduro: the 701display firmware watches D0 bit 3 going LOW to spot the off-road ABS button being held while stopped. 1390 Evo: the clutch bit toggled in its lever-pull log; gear stayed 0 (neutral) in every log, so gear is unchecked there.","fields":[{"name":"Gear","b":0,"shift":4,"bits":4,"map":{"0":"N"},"min":0,"max":6,"bikes":{"690":"confirmed","Adv390":"confirmed","890":"reported"}},{"name":"Clutch","b":0,"shift":3,"bits":1,"yes":"PULLED","no":"released","bikes":{"690":"confirmed","Adv390":"confirmed","890":"reported","1390":"confirmed"}},{"b":7,"text":"alive counter, steps by 0x20 (20, 40, 60, 80, A0, C0)"}]},{"id":"12A","name":"Ride mode, traction and ABS states","ms":50,"from":"key-on","try":"Bike stopped: change ride mode, press the TC button, switch ABS mode.","note":"The bit positions differ between models, so try each variant against what you change.","fields":[{"name":"Throttle","b":0,"shift":1,"bits":1,"yes":"closed","no":"open","bikes":{"690":"capture"}},{"name":"Requested throttle map","b":1,"shift":6,"bits":1,"map":{"0":"map 1","1":"map 2"},"bikes":{"690":"confirmed"}},{"name":"Traction control","b":1,"shift":4,"bits":1,"yes":"OFF","no":"ON","bikes":{"Adv390":"confirmed"},"note":"D1 = 28 on, 18 off. Offroad ride mode also turns it off."},{"name":"Off-road ABS mode","b":2,"shift":0,"bits":1,"yes":"OFF-ROAD","no":"road","bikes":{"701":"capture"}},{"name":"Traction control (second bit)","b":4,"shift":5,"bits":1,"yes":"set","no":"clear","bikes":{"Adv390":"capture"}},{"name":"ABS mode (second bit)","b":4,"shift":3,"bits":1,"yes":"set","no":"clear","bikes":{"Adv390":"capture"}},{"name":"ABS mode","b":6,"map":{"2":"mode A","4":"mode B"},"bikes":{"Adv390":"capture"},"note":"names not known"},{"name":"Slip adjuster (MTC) level","b":3,"mul":0.25,"dp":0,"min":0,"max":15,"bikes":{"1390":"confirmed"},"note":"1C = 7, 20 = 8, 24 = 9 while stepping the slip adjuster 7, 8, 9, 8, 7"},{"b":2,"text":"1390 Evo: D2 high nibble is 7 in Performance and 1 in Rain (anti-wheelie level, per the repo). D4 moved with the ride mode (60, 44, 04, 24 seen)."}]},{"id":"12B","name":"ABS wheel speeds and lean","ms":10,"from":"ABS","try":"Bike on a stand, key on: spin the front wheel by hand, then the rear. Lean the bike on the stand for tilt and lean.","note":"Not sent at all on the 390 Adventure (Gen 2) or the 1390 Evo (no 12B in any of its 17 logs); a RaceChrono commenter says 0x12B does not carry wheel speed on the 890 R. Scale: 701 Enduro fitted 17.77 counts per km/h against its OBD speed; the 690 speedometer gave about 17.0. Lean: 0 is upright, 1 and up lean right, FFF and down lean left (1290 thread). A 790 forum post (no ID given) says speed is (D0*256+D1)/16, unverified. One RaceChrono commenter (hedged with 'if I remember correctly') put front and rear wheel speed at PID 299 (= 0x12B) as bit ranges, calling them ABS pulse frequencies, which fits the layout here; another says 299 carries no wheel speed on the 890 R.","fields":[{"name":"Front wheel speed","b":0,"n":2,"mul":0.05627,"dp":1,"unit":"km/h","min":0,"max":280,"bikes":{"690":"confirmed","701":"confirmed"}},{"name":"Rear wheel speed","b":2,"n":2,"mul":0.05627,"dp":1,"unit":"km/h","min":0,"max":280,"bikes":{"690":"confirmed"},"note":"agrees with the front within about 3%"},{"name":"Tilt (raw)","b":5,"n":2,"shift":4,"bits":12,"signed":true,"bikes":{"1290":"candidate","690":"candidate"},"note":"12-bit signed, not validated"},{"name":"Lean angle (raw)","b":6,"n":2,"shift":0,"bits":12,"signed":true,"bikes":{"1290":"confirmed","690":"candidate"},"note":"12-bit signed; degrees per count not published"}]},{"id":"12C","name":"Slow wheel-speed copy?","ms":1000,"from":"key-on","bikes":{"690":"candidate"},"fields":[{"b":0,"text":"all zero when stopped; two 12-bit values follow wheel speed (slower, different scale)"}]},{"id":"12D","name":"Rear wheel speed (390 Adventure)","from":"ABS?","try":"Bike on a stand: spin the rear wheel by hand. Compare with the dash on a ride.","note":"Also sent on the 1390 Evo (about every 10 ms); with the wheels still only the counter in D7 changed.","fields":[{"name":"Rear wheel speed","b":1,"n":2,"mul":0.1,"dp":1,"unit":"km/h?","min":0,"max":280,"bikes":{"Adv390":"capture"},"note":"a per-pulse count; the unit (0.1 km/h per count) is a guess, calibrate against the dash"}]},{"id":"12E","name":"Smoothed speed (390 Adventure)","from":"ABS?","note":"690: D0 is a status flag (00/40) and D7 a counter. Also sent on the 1390 Evo; with the bike still D6 and D7 changed there.","fields":[{"name":"Speed, smoothed","b":1,"n":2,"mul":0.1,"dp":1,"unit":"km/h?","min":0,"max":280,"bikes":{"Adv390":"capture"},"note":"about 0.7 x the 12D value, fades out after the wheel stops"}]},{"id":"12F","name":"Lean angle (rear brake?)","ms":10,"from":"key-on","try":"Tilt the bike on its stand. Then press only the rear pedal. See which one moves D0-D1.","note":"Evidence for lean: on the 1390 Evo D0-D1 sat at F83x (-125 counts) with the bike upright on its stand and ignored the rear pedal, and a RaceChrono commenter guesses lean/tilt is at PID 303 (= 0x12F) on the 890 R, which has the newer Super Duke ABS (a guess, no capture). The 390 Adventure project claims both uses for D0-D1 and has not settled it. D1 bit 0x20 also flips in bursts there (a status bit, not identified).","fields":[{"name":"Lean angle (raw)","b":0,"n":2,"shift":4,"bits":12,"signed":true,"bikes":{"Adv390":"capture","1390":"capture"},"note":"tilted by hand on the stand: negative left, positive right. 1390 Evo upright on its stand: -125, jittering by 1-2 counts"},{"name":"Rear brake (raw)","b":0,"n":2,"unit":"raw","bikes":{"Adv390":"capture"},"note":"rests near 304, rises 50-90 with the pedal (390 Adventure). The 1390 Evo's D0-D1 did not move with the pedal"}]},{"id":"140","name":"Handlebar buttons (1390 Evo)","ms":20,"from":"switchgear","try":"With the key on, use the turn-signal switch, the cancel button and the high-beam flash and watch which bit pulses.","bikes":{"1390":"confirmed"},"note":"Each button sets its own bit while held, and D2 rests at 03. The four below were checked against the repo's turn-signal and high-beam logs. The repo README counts bits from the left (its B0 is 0x80); everything here uses bit 0 = 0x01. Newer switchgear: whether a Gen 1 sends this is unknown.","fields":[{"name":"Left turn signal button","b":0,"shift":4,"bits":1,"yes":"PRESSED","no":"-","bikes":{"1390":"confirmed"}},{"name":"Turn signal cancel button","b":0,"shift":3,"bits":1,"yes":"PRESSED","no":"-","bikes":{"1390":"confirmed"}},{"name":"Right turn signal button","b":0,"shift":2,"bits":1,"yes":"PRESSED","no":"-","bikes":{"1390":"confirmed"}},{"name":"High beam / flash button","b":1,"shift":0,"bits":1,"yes":"PRESSED","no":"-","bikes":{"1390":"confirmed"}},{"b":0,"text":"Other bits, from the repo README only (not checked one by one): D0 bit 1 = C1, bit 0 = C2; D1 bit 7 = cruise, bit 4 = +/resume, bit 3 = -/set, bit 1 = horn; D2 bit 7 right, 6 left, 5 up, 4 down, 3 centre of the d-pad. D2 bit 2 also pulses in the button log."}]},{"id":"174","name":"Motion sensor (IMU)?","ms":10,"from":"key-on","bikes":{"690":"candidate","1390":"candidate"},"try":"Lean the bike on the stand and watch which of 174, 178 and 17C swings.","note":"Also on the 1390 Evo, where almost every byte jitters even with the bike upright on its stand.","fields":[{"b":0,"text":"two 16-bit values centred near 0x8000 (one offset by gravity); counter in D6 low nibble; checksum in D7"}]},{"id":"176","name":"Motion or ABS data? (390 Adventure)","ms":10,"from":"key-on","bikes":{"Adv390":"candidate"},"fields":[{"b":0,"text":"D0 and D4 jitter at rest; D6 low nibble counts 0-F; D7 looks like a checksum. Large erratic jumps = raw sensor data, not a clean speed."}]},{"id":"177","name":"Motion or ABS data? (390 Adventure)","ms":10,"from":"key-on","bikes":{"Adv390":"candidate"},"fields":[{"b":4,"text":"D4 jitters at rest; D6 low nibble counts 0-F; D7 looks like a checksum."}]},{"id":"178","name":"Motion sensor (IMU)?","ms":10,"from":"key-on","bikes":{"1390":"candidate"},"try":"Lean the bike on the stand: D0-D1 should swing.","fields":[{"name":"Lean candidate","b":0,"n":2,"unit":"raw","bikes":{"690":"candidate"},"note":"swings most when the bike leans"},{"b":6,"text":"counter in D6 low nibble; checksum in D7"}]},{"id":"17C","name":"Motion sensor (IMU)?","ms":10,"from":"key-on","bikes":{"1390":"candidate"},"try":"Lean the bike on the stand: D4-D5 should swing.","fields":[{"name":"Lean candidate","b":4,"n":2,"unit":"raw","bikes":{"690":"candidate"},"note":"swings most when the bike leans"},{"b":6,"text":"counter in D6 low nibble; checksum in D7"}]},{"id":"290","name":"Brake pressure and brake light","ms":10,"from":"ABS?","try":"Key on: squeeze the front lever, then press the rear pedal.","note":"690: D2-D7 stay 0, so there is no brake-light bit there.","fields":[{"name":"Front brake pressure (raw)","b":0,"n":2,"unit":"raw","min":0,"max":40000,"bikes":{"690":"confirmed","Adv390":"confirmed","890":"reported","1390":"confirmed"},"note":"390 Adventure: rest noise up to about 48, firm stop 15,000-20,000. 1390 Evo: 0 at rest, about 8,500 at the peak of a hard squeeze on the stand"},{"name":"Front brake lever switch","b":3,"shift":3,"bits":1,"yes":"PRESSED","no":"-","bikes":{"1390":"confirmed"},"note":"D3 = 02 at rest, 0A with the lever"},{"name":"Rear brake pedal switch","b":3,"shift":2,"bits":1,"yes":"PRESSED","no":"-","bikes":{"1390":"confirmed"},"note":"D3 = 06 with the pedal. Rear pedal pressure did not show in D0-D1 (peak 32)"},{"b":3,"text":"390 Adventure: D3 goes 02 to 0E when the brake light is on (both bits at once), unlike the 1390 Evo where each switch has its own bit"}]},{"id":"300","name":"Constant 0F... (off-road ABS dongle?)","ms":200,"from":"key-on","bikes":{"690":"candidate"},"fields":[{"b":0,"text":"always 0F... on the 690; maybe the off-road ABS dongle, untested with it unplugged"}]},{"id":"310","name":"Not decoded (1390 Evo)","ms":100,"from":"?","bikes":{"1390":"candidate"},"fields":[{"b":0,"text":"D0-D3 are either 71 7A 4C 54 or FF FF FF FF (looks like four sensor values that read FF when invalid); D4-D5 stay 7D 7D. Meaning not known."}]},{"id":"400","name":"Not decoded (1390 Evo)","ms":10,"from":"?","bikes":{"1390":"candidate"},"fields":[{"b":0,"text":"D0 reads 28 at rest and swings while the engine cranks (down to 0B, then back up); D1 is 80, sometimes C0; D7 is a counter. Meaning not known."}]},{"id":"410","name":"Not decoded (1390 Evo)","ms":10,"from":"?","bikes":{"1390":"candidate"},"fields":[{"b":0,"text":"D0, D3 and D6 follow engine speed loosely (correlation about 0.7 in the start-up log); D1, D4 and D7 vary too. Meaning not known."}]},{"id":"450","name":"Buttons and ABS mode","ms":50,"from":"key-on","try":"Press the TC button and the map button. Change ABS mode with the bike stopped.","note":"390 Adventure: D2 sits at 0x80 at rest, so the 690's TC-button bit may not apply. 1390 Evo: the top bits of D2 step through 20, 40, 60 ... between logs (a slow counter), and D4 changed (1B, 13, 0B, 03) in the logs where the ride mode was switched.","fields":[{"name":"TC button","b":2,"shift":0,"bits":1,"yes":"PRESSED","no":"-","bikes":{"690":"confirmed"}},{"name":"Map button / request","b":4,"map":{"0":"map 0","9":"map 1"},"bikes":{"690":"confirmed"}},{"name":"ABS mode","b":3,"map":{"1":"Road","2":"Offroad"},"bikes":{"Adv390":"capture"},"note":"names assumed"},{"name":"Traction (second bit)","b":4,"shift":3,"bits":1,"yes":"set","no":"clear","bikes":{"Adv390":"capture"}}]},{"id":"4E8","to":"4E9","name":"Start-up exchange (1390 Evo)","from":"?","bikes":{"1390":"candidate"},"note":"Only 12 frames in the whole set, right after key-on: 4E8 = 55 01 ..., 55 02 ..., 55 03 ... with random-looking bytes, 4E9 = 5A 01 01, 5A 02 01, 5A 03 01. Looks like a key or security exchange, so there is nothing to decode.","fields":[]},{"id":"4EA","name":"Dashboard warning light?","ms":100,"from":"?","bikes":{"1390":"capture"},"fields":[{"name":"Orange warning light","b":4,"shift":0,"bits":1,"yes":"ON","no":"off","bikes":{"1390":"capture"},"note":"D4 = 01 in the log taken with the kill switch off, all zero in every other log; the repo README names it as the dashboard's orange light"}]},{"id":"540","name":"Slow status: coolant, gear, stand, fuel","ms":100,"from":"engine ECU","try":"Key on: raise and lower the side stand. Warm the engine and compare coolant with the dash.","note":"The 690 and 390 Adventure put coolant and the side stand one byte apart, so the variants disagree. Whichever gives a believable number is the one for your bike. The 1390 Evo agrees with the 390 Adventure for coolant.","fields":[{"name":"Engine speed (slow copy)","b":1,"n":2,"unit":"rpm","min":0,"max":14000,"bikes":{"690":"confirmed"}},{"name":"Gear (slow copy)","b":3,"shift":0,"bits":4,"map":{"0":"N","7":"shifting"},"min":0,"max":7,"bikes":{"690":"confirmed","701":"capture"}},{"name":"Side stand","b":4,"shift":0,"bits":1,"yes":"UP","no":"DOWN","bikes":{"690":"confirmed","701":"capture"}},{"name":"Side stand fault","b":4,"shift":7,"bits":1,"yes":"FAULT","no":"ok","bikes":{"690":"capture"}},{"name":"Coolant","b":6,"n":2,"mul":0.1,"dp":1,"unit":"\u00b0C","min":-40,"max":150,"bikes":{"690":"confirmed"}},{"name":"Side stand","b":3,"shift":0,"bits":1,"yes":"UP","no":"DOWN","bikes":{"Adv390":"confirmed"}},{"name":"Coolant","b":5,"n":2,"mul":0.1,"dp":1,"unit":"\u00b0C","min":-40,"max":150,"bikes":{"Adv390":"capture","1390":"capture"},"note":"96.2 after a ride (390 Adventure). 1390 Evo: 95.8 in a warm-engine log, 15.4 on a cold key-on, climbing through a start-up; check against the dash"},{"name":"Fuel flow (raw per second)","b":1,"n":2,"le":true,"unit":"raw","bikes":{"Adv390":"capture"},"note":"0 off, about 27 just after start, 19-22 idling; litres per hour not calibrated"},{"b":3,"text":"1390 Evo: D3 reads 10 before the engine starts and 00 once it is running (meaning not known). D1 follows engine speed loosely but is not a clean copy."}]},{"id":"541","name":"Quickshifter and timers","from":"engine ECU","note":"690: always 02 80. 390 Adventure: a byte counts seconds since start (D4 or D6 in the two captures), so it is a timer, not a sensor.","fields":[{"name":"Quickshifter","b":0,"shift":2,"bits":1,"yes":"ON","no":"OFF","bikes":{"Adv390":"confirmed"}},{"b":4,"text":"seconds-since-start style counter"}]},{"id":"550","name":"Lights and status flags","ms":200,"from":"key-on","bikes":{"690":"candidate","1390":"confirmed"},"try":"Switch the indicators, the hazards and the high beam on and off with the key on.","fields":[{"b":0,"text":"690: D0 flips between 00 and 10; D7 is a counter"},{"name":"Left indicator lit","b":2,"shift":0,"bits":1,"yes":"ON","no":"off","bikes":{"1390":"confirmed"},"note":"toggles about every 0.4 s while flashing; both lit = hazards"},{"name":"Right indicator lit","b":2,"shift":1,"bits":1,"yes":"ON","no":"off","bikes":{"1390":"confirmed"}},{"name":"High beam lit","b":4,"shift":4,"bits":1,"yes":"ON","no":"off","bikes":{"1390":"confirmed"}},{"name":"Daytime running light","b":4,"shift":0,"bits":1,"yes":"ON","no":"off","bikes":{"1390":"confirmed"}}]},{"id":"551","name":"Static with engine off (390 Adventure)","from":"key-on","bikes":{"Adv390":"candidate","1390":"candidate"},"note":"1390 Evo: also static, 00 00 00 00 00 04 C0 00 in every log.","fields":[{"b":0,"text":"static with the engine off; capture again riding"}]},{"id":"552","name":"Not decoded (1390 Evo)","ms":50,"from":"?","bikes":{"1390":"candidate"},"fields":[{"b":0,"text":"D0 and D7 vary (D7 looks like a counter); the other bytes stay put. Meaning not known."}]},{"id":"553","name":"Slow static: trip or service data?","ms":1000,"from":"key-on","bikes":{"Adv390":"candidate"},"fields":[{"b":0,"text":"1 Hz, static: odometer, trip or service-interval candidate"}]},{"id":"55A","name":"Odometer","ms":1000,"from":"key-on","try":"Compare with the dash odometer.","fields":[{"name":"Odometer","b":0,"n":3,"mul":0.1,"dp":1,"unit":"km","min":0,"max":999999,"bikes":{"Adv390":"confirmed","1390":"capture"},"note":"21430 = 2,143 km on the dash (390 Adventure). 1390 Evo: 00 17 16 = 591.0 km in sixteen logs and 00 17 83 = 601.9 km in a later one, so it counts up as the bike is ridden"}]},{"id":"5A0","name":"Slow status","from":"key-on","bikes":{"690":"candidate","Adv390":"candidate","1390":"candidate"},"fields":[{"b":1,"text":"690: D1 and D2 change once after the ECU wakes. 390 Adventure: static with the engine off. 1390 Evo: D3-D6 sit at 62 44 92 80 and D7 is a counter."}]},{"id":"5A2","name":"Static with engine off (390 Adventure)","from":"key-on","bikes":{"Adv390":"candidate","1390":"candidate"},"fields":[{"b":0,"text":"capture again riding"}]},{"id":"5B0","name":"Static with engine off (390 Adventure)","from":"key-on","bikes":{"Adv390":"candidate","1390":"candidate"},"fields":[{"b":0,"text":"capture again riding"}]},{"id":"5B1","name":"Counter only (1390 Evo)","ms":100,"from":"?","bikes":{"1390":"candidate"},"fields":[{"b":7,"text":"only D7 changes (a counter)"}]},{"id":"5B2","name":"Static (1390 Evo)","ms":100,"from":"?","bikes":{"1390":"candidate"},"fields":[{"b":0,"text":"always 08 00 00 00 00 00 00 00"}]},{"id":"5CF","name":"Not decoded (1390 Evo)","ms":100,"from":"?","bikes":{"1390":"candidate"},"fields":[{"b":0,"text":"D0-D5 change a little (six different frames in the whole set). Meaning not known."}]},{"id":"5D0","name":"Not decoded (1390 Evo)","ms":100,"from":"?","bikes":{"1390":"candidate"},"fields":[{"b":0,"text":"D0 and D2 change (four different frames in the whole set). Meaning not known."}]},{"id":"5D1","name":"Suspension mode (semi-active)","ms":20,"from":"?","try":"On a bike with electronic suspension, change the suspension profile and watch D0.","bikes":{"1390":"confirmed"},"note":"Only bikes with electronic (semi-active) suspension: not a Gen 1 Duke.","fields":[{"name":"Suspension profile","b":0,"map":{"0":"Automatic","34":"Sport","68":"Street","102":"Comfort","153":"Track","170":"Pro","187":"Rain"},"bikes":{"1390":"confirmed"},"note":"99 (Track), AA (Pro) and 00 (Automatic) were seen while switching profiles; the other values are from the repo README"},{"b":1,"text":"preload: 58 = auto standard in every log (the README also lists 60 auto low, 68 auto high, 50 = 100%, D8 = 95%, 48 = 90%, 08 = 10%, 90 = 5%, 00 = 0%)"}]},{"id":"5D2","name":"Not decoded (1390 Evo)","ms":100,"from":"?","bikes":{"1390":"candidate"},"fields":[{"b":0,"text":"D0-D5 change a little (six different frames in the whole set). Meaning not known."}]},{"id":"6AB","name":"Rolling security value? (1390 Evo)","ms":1000,"from":"?","bikes":{"1390":"candidate"},"fields":[{"b":0,"text":"random-looking bytes with only two different frames in the whole set; probably a checksum or security value"}]},{"id":"6B9","name":"Static with engine off (390 Adventure)","from":"key-on","bikes":{"Adv390":"candidate","1390":"candidate"},"fields":[{"b":0,"text":"capture again riding"}]},{"id":"6A6","to":"6A8","name":"Multi-frame transport (dash?)","ms":25,"from":"?","bikes":{"Adv390":"candidate"},"note":"D0 = 0F or 30 looks like multi-frame framing: probably the dash or a diagnostic transport, not live sensor data.","fields":[]},{"id":"6E2","name":"Static (1390 Evo)","ms":1000,"from":"?","bikes":{"1390":"candidate"},"fields":[{"b":0,"text":"always 05 44 00 00 00 00 00 00"}]},{"id":"7DF","name":"OBD-II request to all modules","ms":0,"from":"scan tool or this unit","bikes":{},"note":"Only appears while something is asking (a scan tool, or this unit's own requests).","fields":[]},{"id":"7E0","name":"Engine ECU: request","ms":0,"from":"scan tool or this unit","bikes":{},"note":"Confirmed on a 2015 390 Duke: the ECU listens on 7E0 and answers on 7E8.","fields":[]},{"id":"7E8","name":"Engine ECU: reply","ms":0,"from":"engine ECU","bikes":{},"note":"Only appears in answer to a request. Confirmed on a 2015 390 Duke.","fields":[]},{"id":"7E1","to":"7E7","name":"Other module: request","ms":0,"from":"scan tool or this unit","bikes":{},"note":"Physical request to another module (ABS, dash), seen while a scanner or Find modules is running.","fields":[]},{"id":"7E9","to":"7EF","name":"Other module: reply","ms":0,"from":"ABS, dash or similar","bikes":{},"note":"A module answering a request. Find modules lists who answered.","fields":[]}]};/*KIDS-END*/
const KIDX={};
KIDS.ids.forEach(e=>{const a=parseInt(e.id,16),b=e.to?parseInt(e.to,16):a;for(let i=a;i<=b;i++)KIDX[i]=e});
const kinfo=id=>id.length<=3?KIDX[parseInt(id,16)]:null;
// Value of one table field in a frame (d = array of byte values). null if the frame is too short.
function kval(f,d){
  const n=f.n||1;if(f.b+n>d.length)return null;
  let r=0;
  if(f.le)for(let i=n-1;i>=0;i--)r=r*256+d[f.b+i];else for(let i=0;i<n;i++)r=r*256+d[f.b+i];
  const bits=f.bits||8*n;
  r=Math.floor(r/Math.pow(2,f.shift||0))%Math.pow(2,bits);
  if(f.signed&&r>=Math.pow(2,bits-1))r-=Math.pow(2,bits);
  if(f.map){const m=f.map[r];return{t:m!==undefined?m:String(r),ok:m!==undefined||(f.min!==undefined&&r>=f.min&&r<=f.max)}}
  if(f.yes)return{t:r?f.yes:f.no,ok:true};
  const v=r*(f.mul||1)+(f.add||0);
  return{t:v.toFixed(f.dp||0)+(f.unit?' '+f.unit:''),ok:f.min===undefined||(v>=f.min&&v<=f.max)};
}
// A ready-to-paste Settings > Custom signals line, for fields that sit on whole bytes.
function ksig(e,f){
  const n=f.n||1;
  if(f.map||f.yes||f.shift||(f.bits&&f.bits!=8*n)||n>4)return'';
  const num=x=>String(+(+x).toPrecision(4));
  return e.id+':'+f.b+':'+n+':'+(f.le?(f.signed?'L':'l'):(f.signed?'B':'b'))+':'+num(f.mul||1)+':'+num(f.add||0)+':'+f.name.replace(/[:|]/g,' ').slice(0,28)+':'+(f.unit||'').replace(/[?:]/g,'').replace(/^raw$/,'');
}
let snOpen={};
function knToggle(id){snOpen[id]=!snOpen[id];sniffDraw()}
function kdetail(e,d){
  let h='';
  const eb=Object.entries(e.bikes||{});
  if(eb.length)h+='<div class="kxs">Seen on: '+eb.map(([b,s])=>esc(KIDS.bikes[b]||b)+' ('+s+')').join(' · ')+'</div>';
  if(e.try)h+='<div class="kxs">Test: '+esc(e.try)+'</div>';
  if(e.note)h+='<div class="kxs">'+esc(e.note)+'</div>';
  (e.fields||[]).forEach(f=>{
    if(f.text){h+=`<div class="kxf"><b>D${f.b}</b> ${esc(f.text)}</div>`;return}
    const v=kval(f,d),n=f.n||1,sg=ksig(e,f);
    h+=`<div class="kxf${v&&v.ok?'':' bad'}"><b>${esc(f.name)}</b> <span class="mut">D${f.b}${n>1?'-D'+(f.b+n-1):''}</span> <span class="kxv">${v?(v.ok?'':'✗ ')+esc(v.t):'-'}</span><div class="kxs">${Object.entries(f.bikes).map(([b,s])=>esc(b)+' '+s).join(' · ')}${f.note?' - '+esc(f.note):''}</div>${sg?'<div class="kxs mono">'+esc(sg)+'</div>':''}</div>`;
  });
  return'<div class="kxd">'+h+'</div>';
}

// ---------- bus sniffer ----------
let snOn=false,snT=null,snLast=null;
function sniffToggle(){snOn=!snOn;$('#snbtn').textContent=snOn?'Stop':'Start';cmd(snOn?'sniff start':'sniff stop').then(r=>{snLast=r;sniffDraw()});if(snOn)sniffPoll()}
async function sniffPoll(){
  clearTimeout(snT);
  if(!snOn)return;
  if(cur=='tools'&&!document.hidden){try{snLast=await cmd('sniff');sniffDraw()}catch(e){}}
  snT=setTimeout(sniffPoll,400);
}
function byteRow(hex,c,m){return hex.split(' ').map((h,i)=>`<i class="${c>>i&1?'c':(m>>i&1?'m':'')}">${h}</i>`).join('')}
const isDiag=id=>id.length==3&&id[0]=='7';
function sniffDraw(){
  const r=snLast,el=$('#sniff');if(!r||!r.ids){return}
  const hide=$('#snhide').checked,only=$('#snchg').checked,unk=$('#snunk').checked;
  let l=r.ids.filter(e=>!(hide&&isDiag(e.id))&&!(only&&!e.m)&&!(unk&&kinfo(e.id)));
  l.sort((a,b)=>parseInt(a.id,16)-parseInt(b.id,16));
  if(!l.length){el.innerHTML='<span class="mut">'+(r.on?'Listening… no frames yet.':'Not running.')+'</span>';return}
  const bus=r.ids.filter(e=>!isDiag(e.id)),known=bus.filter(e=>kinfo(e.id)),fresh=bus.filter(e=>!kinfo(e.id));
  const sum=bus.length?'<div class="mut" style="margin-bottom:6px">'+known.length+' of '+bus.length+' IDs on this bus are in the KTM list'+(fresh.length?'. Not in the list: '+fresh.map(e=>e.id).join(' '):'')+'.</div>':'';
  el.innerHTML=sum+(r.full?'<div class="mut">Table full (64 IDs) — tick the filters or Clear.</div>':'')+l.map(e=>{
    const lo=e.lo.split(' '),hi=e.hi.split(' ');let rg=[];
    for(let i=0;i<lo.length;i++)if(e.m>>i&1)rg.push('b'+i+' '+lo[i]+'–'+hi[i]);
    const k=kinfo(e.id),open=k&&snOpen[e.id];
    return `<div class="sr${e.age>2000?' stale':''}"><span class="id">${e.id}</span><span class="hz">${e.hz} Hz</span>${byteRow(e.d,e.c,e.m)}${rg.length?'<div class="rg">'+rg.join(' · ')+'</div>':''}${k?`<div class="kxl" onclick="knToggle('${e.id}')">${open?'▾':'▸'} ${esc(k.name)}</div>`:''}${open?kdetail(k,e.d.split(' ').map(h=>parseInt(h,16))):''}</div>`}).join('');
}
// Plain-text copy of the whole capture, for pasting into a message.
function sniffReport(){
  const r=snLast;if(!r||!r.ids||!r.ids.length)return'';
  const out=['OBDuke bus capture: '+Math.round((r.ms||0)/1000)+' s, '+r.ids.length+' IDs'+(r.full?' (table full)':'')];
  r.ids.slice().sort((a,b)=>parseInt(a.id,16)-parseInt(b.id,16)).forEach(e=>{
    const lo=e.lo.split(' '),hi=e.hi.split(' '),mv=[];
    for(let i=0;i<lo.length;i++)if(e.m>>i&1)mv.push('D'+i+' '+lo[i]+'-'+hi[i]);
    const k=kinfo(e.id);
    out.push(e.id.padEnd(4)+' '+String(e.hz).padStart(6)+' Hz  '+e.d+(mv.length?'  moved: '+mv.join(', '):'')+(k?'  ['+k.name+']':'  [not in KTM list]'));
  });
  return out.join('\n');
}
function sniffCopy(){
  const t=sniffReport();if(!t){toast('Nothing captured yet');return}
  const a=document.createElement('textarea');a.value=t;a.style.cssText='position:fixed;opacity:0';document.body.appendChild(a);a.select();
  let ok=false;try{ok=document.execCommand('copy')}catch(e){}
  document.body.removeChild(a);
  if(ok)toast('Capture copied');else{const b=$('#snrep');b.value=t;b.style.display='block';b.select();toast('Copy it from the box below')}
}

// ---------- watch a request ----------
let waOn=false,waT=null,waPrev=null,waLo=null,waHi=null,waM=0;
function watchToggle(){
  waOn=!waOn;$('#wabtn').textContent=waOn?'Stop':'Watch';
  if(waOn){waPrev=null;watchMark();watchPoll()}
}
function watchMark(){waLo=waHi=null;waM=0}
async function watchPoll(){
  clearTimeout(waT);if(!waOn)return;
  if(cur=='tools'&&!document.hidden){
    try{
      const r=await cmd('peek '+$('#watchin').value);
      if(!r.ok){$('#watch').innerHTML='<span class="mut">'+esc(r.msg)+'</span>'}
      else{
        const b=r.hex.split(' ').map(h=>parseInt(h,16));let c=0;
        if(!waLo){waLo=b.slice();waHi=b.slice()}
        b.forEach((v,i)=>{if(waPrev&&waPrev[i]!==v)c|=1<<Math.min(i,30);if(v<waLo[i])waLo[i]=v;if(v>waHi[i])waHi[i]=v;if(waLo[i]!==waHi[i])waM|=1<<Math.min(i,30)});
        waPrev=b;
        const hx=v=>v.toString(16).toUpperCase().padStart(2,'0');
        let rows='',rg=[];
        for(let i=0;i<b.length;i++){if(waLo[i]!==waHi[i])rg.push('byte '+i+': '+hx(waLo[i])+'–'+hx(waHi[i])+' ('+waLo[i]+'–'+waHi[i]+')')}
        rows=b.map((v,i)=>`<i class="${c>>i&1?'c':(waLo[i]!==waHi[i]?'m':'')}">${hx(v)}</i>`).join('');
        $('#watch').innerHTML=`<div class="sr"><div class="mut" style="margin-bottom:4px">Byte 0 = ${hx(b[0])} (reply code). For Custom signals / Torque, subtract 2 from the byte number.</div>${rows}${rg.length?'<div class="rg" style="margin-left:0">'+rg.join('<br>')+'</div>':''}${r.nrc?'<div class="rg" style="margin-left:0">ECU says: '+esc(r.nrc)+'</div>':''}</div>`;
      }
    }catch(e){}
  }
  waT=setTimeout(watchPoll,300);
}

// ---------- gear calibration ----------
function gearUI(ratios){
  $('#gearbtns').innerHTML=[1,2,3,4,5,6].map(g=>`<button onclick="gearLearn(${g})">${g}</button>`).join('')+'<button class="small" onclick="gearReset()">Defaults</button>';
  if(ratios)$('#gearinfo').textContent='rpm per km/h: '+ratios.map((r,i)=>(i+1)+'='+r).join('  ');
}
function gearLearn(g){act(async()=>{const r=await cmd('gear learn '+g);if(r.ok){toast('Gear '+g+' saved');gearUI(r.ratios)}else toast(r.msg)})}
function gearReset(){if(confirm('Put the gear ratios back to the 390 Duke defaults?'))cmd('gear reset').then(r=>gearUI(r.ratios))}

// ---------- log ----------
function logc(c){act(async()=>{
  const r=await cmd('log '+c);
  $('#logkv').innerHTML=`<div>Status</div><div style="font-weight:800;color:${r.on?'var(--ok)':'var(--mut)'}">${r.on?'LOGGING':'Stopped'}</div><div>Samples</div><div>${r.n} / ${r.max}</div><div>Interval</div><div>${r.ms} ms</div><div>Capacity</div><div>${Math.round(r.max*r.ms/60000)} min</div>`;
})}

// ---------- settings ----------
async function loadSettings(){
  const r=await cmd('settings');fields=r.fields;
  let g='',h='';
  fields.forEach(f=>{
    if(f.g!=g){h+=(g?'</div>':'')+'<h2>'+f.g+'</h2><div class="card">';g=f.g}
    const id='f_'+f.k;
    if(f.t=='bool')h+=`<div class="sw"><label for="${id}">${f.l}${f.r?' ↻':''}</label><input type="checkbox" id="${id}" ${f.v=='1'?'checked':''}></div>`;
    else if(f.t=='sel')h+=`<label for="${id}">${f.l}${f.r?' ↻':''}</label><select id="${id}">`+f.o.split('|').map(o=>{const[v,l]=o.split(':');return`<option value="${v}" ${v==f.v?'selected':''}>${l}</option>`}).join('')+'</select>';
    else if(f.t=='pass')h+=`<label for="${id}">${f.l}${f.r?' ↻':''}</label><div class="row" style="flex-wrap:nowrap"><input type="password" id="${id}" placeholder="${f.set?'•••••• (unchanged)':'not set'}" autocomplete="new-password">${f.set?`<button class="small" onclick="clearPass('${f.k}')">Remove</button>`:''}</div>`;
    else h+=`<label for="${id}">${f.l}${f.r?' ↻':''}</label><input id="${id}" value="${(f.v||'').replace(/"/g,'&quot;')}" ${f.t=='int'||f.t=='float'?'inputmode="decimal"':''}>`;
  });
  $('#setform').innerHTML=h+'</div><p class="mut" style="margin-top:8px">↻ = takes effect after reboot</p>';
  const s=await cmd('status');Object.assign(S,s);
  $('#about').innerHTML=`<div>Firmware</div><div>${s.fw}</div><div>ECU</div><div>${s.ecu?'Connected ('+s.ecu_id+', '+s.can+')':'Not connected'}</div><div>KTM RPM</div><div>${s.ktm_rpm?'Yes':'No'}</div><div>Bluetooth (Torque / tracker)</div><div>${s.bt_client?'Connected':'Not connected'}</div><div>Hotspot address</div><div>${s.ap_ip}</div><div>Bluetooth MAC</div><div>${s.bt_mac}</div><div>Wi-Fi clients</div><div>${s.wifi_clients}</div><div>Home Wi-Fi IP</div><div>${s.sta_ip||'—'}</div><div>Uptime</div><div>${Math.floor(s.uptime/60)} min</div><div>Free memory</div><div>${Math.round(s.heap/1024)} KB</div>`;
}
async function saveSettings(){
  let reboot=false,errs=[],n=0;
  for(const f of fields){
    const el=$('#f_'+f.k);if(!el)continue;
    let v=f.t=='bool'?(el.checked?'1':'0'):el.value.trim();
    if(f.t=='pass'){if(!v)continue}
    else if(v==f.v)continue;
    const r=await cmd('set '+f.k+' '+v);
    if(r.ok){n++;if(r.reboot)reboot=true}else errs.push(r.msg);
  }
  if(errs.length)alert(errs.join('\n'));
  else toast(n?'Saved '+n+' setting(s)':'Nothing changed');
  await loadSettings();buildTiles();gearUI(S.gears);
  if(reboot&&confirm('Some changes need a reboot. Reboot now?\n\nIf you changed Wi-Fi settings, rejoin the new hotspot afterwards.'))cmd('reboot');
}
async function clearPass(k){const r=await cmd('set '+k);toast(r.ok?'Removed':r.msg);loadSettings()}
function factory(){if(confirm('Reset ALL settings to defaults and reboot?'))cmd('factory yes').then(r=>toast(r.msg))}

// ---------- start ----------
(async()=>{try{Object.assign(S,await cmd('status'))}catch(e){}buildTiles();gearUI(S.gears);live()})();
</script>
</body></html>
)OBDUKE";
