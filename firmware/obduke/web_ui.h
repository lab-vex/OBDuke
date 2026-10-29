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
  <p class="mut" style="margin:8px 0">Everything the bike broadcasts. Orange box = changed just now, orange text = changed since Mark (with its range below). Tap Mark, spin the front wheel by hand, and watch which bytes light up.</p>
  <div class="sn mono" id="sniff"><span class="mut">Not running.</span></div>
  <p class="mut" style="margin-top:8px">Found one? Add it in Settings &gt; Custom signals, e.g. <span class="mono">120:2:2:b:0.01:0:Front wheel:km/h</span></p>

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
function sniffDraw(){
  const r=snLast,el=$('#sniff');if(!r||!r.ids){return}
  const hide=$('#snhide').checked,only=$('#snchg').checked;
  let l=r.ids.filter(e=>!(hide&&e.id.length==3&&e.id[0]=='7')&&!(only&&!e.m));
  l.sort((a,b)=>parseInt(a.id,16)-parseInt(b.id,16));
  if(!l.length){el.innerHTML='<span class="mut">'+(r.on?'Listening… no frames yet.':'Not running.')+'</span>';return}
  el.innerHTML=(r.full?'<div class="mut">Table full (64 IDs) — tick the filters or Clear.</div>':'')+l.map(e=>{
    const lo=e.lo.split(' '),hi=e.hi.split(' ');let rg=[];
    for(let i=0;i<lo.length;i++)if(e.m>>i&1)rg.push('b'+i+' '+lo[i]+'–'+hi[i]);
    return `<div class="sr${e.age>2000?' stale':''}"><span class="id">${e.id}</span><span class="hz">${e.hz} Hz</span>${byteRow(e.d,e.c,e.m)}${rg.length?'<div class="rg">'+rg.join(' · ')+'</div>':''}</div>`}).join('');
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
