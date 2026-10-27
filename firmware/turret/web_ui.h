#pragma once
// Control page served by the turret at /. Plain HTML/CSS/JS, no external URLs.
const char WEB_UI[] = R"HTML(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Gesture Sentry</title>
<style>
:root{color-scheme:dark;font:16px system-ui,sans-serif;background:#0b1019;color:#f3f6ff}
*{box-sizing:border-box}
body{margin:0;padding:24px 16px 32px}
main{max-width:660px;margin:auto}
h1{font-size:clamp(28px,6vw,38px);margin:0 0 10px;letter-spacing:-1px}
h2{font-size:16px;margin:0 0 12px}
p{line-height:1.5}
.status{color:#c6d2e6;min-height:48px;margin:0 0 16px;overflow-wrap:anywhere}
.card{background:#151e2d;border:1px solid #34445f;border-radius:18px;padding:16px;margin-bottom:16px;min-width:0}
.row{display:flex;gap:10px}
.row>*{flex:1;min-width:0}
button{font:inherit;font-weight:700;color:#eef4ff;background:#26364f;border:2px solid #536985;border-radius:12px;min-height:52px;padding:12px;cursor:pointer;touch-action:manipulation}
button[aria-pressed="true"]{background:#a7e4ff;color:#071826;border-color:#a7e4ff}
button:focus-visible,input:focus-visible{outline:3px solid #ffd67a;outline-offset:4px}
button:disabled{cursor:default;opacity:.45}
.hint{color:#b7c6dd;font-size:14px;margin:12px 0 0}
.error{color:#ff8f95;font-weight:800}
#connection{margin:0 0 12px}
#driveError,#sourceError{margin:12px 0 0}
[hidden]{display:none!important}
#controls.disabled{opacity:.45}
#controls.disabled #pad{pointer-events:none}
#pad{width:min(80vw,300px);max-width:100%;aspect-ratio:1;margin:18px auto;background:radial-gradient(circle,#243a53 0 10%,#1b2a3e 11% 67%,#31445f 68%);border:2px solid #647b99;border-radius:50%;position:relative;touch-action:none;user-select:none;overflow:hidden}
#pad:before,#pad:after{content:"";position:absolute;background:#6c83a0;opacity:.45;pointer-events:none}
#pad:before{width:1px;height:100%;left:50%}
#pad:after{height:1px;width:100%;top:50%}
#knob{width:64px;height:64px;border-radius:50%;background:#b2e9ff;border:4px solid #e7f8ff;position:absolute;left:50%;top:50%;margin:-32px;box-shadow:0 4px 18px #0008;pointer-events:none;z-index:1}
.stop{display:block;width:100%;min-height:64px;background:#b92336;border-color:#ff7d8d;color:white;font-size:20px;letter-spacing:1px}
#held{min-height:48px;padding:12px;background:#0c1421;border-radius:10px;overflow-wrap:anywhere}
kbd{font:inherit;font-weight:700;color:#fff}
.readout{display:flex;justify-content:space-between;gap:8px;font-variant-numeric:tabular-nums;color:#d1e3fb;margin:16px 0}
label{display:flex;justify-content:space-between;gap:12px;font-weight:700}
input[type="range"]{display:block;width:100%;height:48px;margin:8px 0 0;accent-color:#a7e4ff;cursor:pointer}
footer{color:#aabbd4;font-size:13px;line-height:1.5}
</style>
</head>
<body>
<main>
<header>
<h1>Gesture Sentry</h1>
<p id="status" class="status" role="status">Connecting to Sentry…</p>
<p id="connection" class="error" role="alert" hidden>NO CONNECTION</p>
</header>
<section class="card" aria-labelledby="sourceTitle">
<h2 id="sourceTitle">Control source</h2>
<div class="row">
<button id="glove" type="button" aria-label="Use glove control" aria-pressed="false" disabled>Glove</button>
<button id="web" type="button" aria-label="Use web control" aria-pressed="false" disabled>Web</button>
</div>
<p id="sourceError" class="error" role="alert" hidden>Could not change control source.</p>
</section>
<section id="methodCard" class="card" aria-labelledby="methodTitle" hidden>
<h2 id="methodTitle">Input method</h2>
<div class="row">
<button id="touch" type="button" aria-label="Use touch joystick" aria-pressed="false">Touch</button>
<button id="keyboard" type="button" aria-label="Use keyboard controls" aria-pressed="false">Keyboard</button>
</div>
</section>
<section id="controls" class="card disabled" aria-labelledby="driveTitle" aria-disabled="true">
<h2 id="driveTitle">Drive controls</h2>
<p id="locked" class="hint">Select Web to enable driving.</p>
<div id="touchPanel">
<p class="hint">Drag up to move forward and right to steer right. Release to stop.</p>
<div id="pad" role="group" aria-label="Touch joystick: drag up for forward, down for reverse, left or right to steer">
<div id="knob"></div>
</div>
</div>
<div id="keyboardPanel" hidden>
<p class="hint"><kbd>W / ↑</kbd> forward · <kbd>S / ↓</kbd> reverse<br><kbd>A / ←</kbd> left · <kbd>D / →</kbd> right<br>Combine keys to move and steer. <kbd>Space</kbd> stops.</p>
<p id="held" aria-live="polite">Held keys: none</p>
</div>
<button id="stop" class="stop" type="button" aria-label="Stop all drive motion" disabled>STOP</button>
<div class="readout" aria-live="off">
<span>Throttle: <output id="throttle">0</output></span>
<span>Steer: <output id="steer">0</output></span>
</div>
<label for="speed">Speed limit <output id="speedValue" for="speed">60%</output></label>
<input id="speed" type="range" min="20" max="100" step="1" value="60" aria-label="Drive speed limit" disabled>
<p id="driveError" class="error" role="alert" hidden>DRIVE LINK ERROR</p>
</section>
<footer>Motion stops when you release the joystick or movement keys. Leaving this page sends a stop command.</footer>
</main>
<script>
'use strict';
const $=id=>document.getElementById(id);
let source=null,method=matchMedia('(pointer: coarse)').matches?'touch':'keyboard';
let throttle=0,steer=0,speed=60,pointer=null;
let switching=false,sourceVersion=0,lastOK=Date.now(),polling=false;
let inFlight=false,revision=0;
const keys=new Set();
const movement=new Set(['KeyW','KeyS','KeyA','KeyD','ArrowUp','ArrowDown','ArrowLeft','ArrowRight']);
const names={KeyW:'W',KeyS:'S',KeyA:'A',KeyD:'D',ArrowUp:'↑',ArrowDown:'↓',ArrowLeft:'←',ArrowRight:'→'};
const active=()=>source==='web'&&!switching;
const available=()=>active()&&!document.hidden;

function draw(){
  const enabled=active();
  $('glove').setAttribute('aria-pressed',String(source==='glove'));
  $('web').setAttribute('aria-pressed',String(source==='web'));
  $('glove').disabled=switching;
  $('web').disabled=switching;
  $('methodCard').hidden=source!=='web';
  $('touch').setAttribute('aria-pressed',String(method==='touch'));
  $('keyboard').setAttribute('aria-pressed',String(method==='keyboard'));
  $('touchPanel').hidden=method!=='touch';
  $('keyboardPanel').hidden=method!=='keyboard';
  $('controls').classList.toggle('disabled',!enabled);
  $('controls').setAttribute('aria-disabled',String(!enabled));
  $('locked').hidden=enabled;
  $('stop').disabled=!enabled;
  $('speed').disabled=!enabled;
}
function readout(){
  $('throttle').value=String(Math.round(throttle*speed/100));
  $('steer').value=String(Math.round(steer*speed/100));
  $('held').textContent='Held keys: '+([...keys].map(k=>names[k]).join(' + ')||'none');
}
function setValues(t,s){
  t=Math.max(-100,Math.min(100,Math.round(t)));
  s=Math.max(-100,Math.min(100,Math.round(s)));
  if(t===throttle&&s===steer){readout();return;}
  throttle=t;steer=s;revision++;
  readout();
  sendDrive();
}
function releasePad(){
  const id=pointer;
  pointer=null;
  if(id!==null&&$('pad').hasPointerCapture(id))$('pad').releasePointerCapture(id);
  $('knob').style.transform='translate(0px,0px)';
}
function stopNow(leaving=false){
  keys.clear();
  releasePad();
  throttle=0;steer=0;revision++;
  readout();
  sendDrive(leaving);
}
async function sendDrive(leaving=false){
  if(!active()||inFlight)return;
  if(!available()&&(throttle!==0||steer!==0)){
    throttle=0;steer=0;revision++;
    readout();
  }
  const sentRevision=revision;
  const t=Math.round(throttle*speed/100),s=Math.round(steer*speed/100);
  const generation=sourceVersion;
  inFlight=true;
  const controller=new AbortController();
  const timeout=setTimeout(()=>controller.abort(),1500);
  try{
    const response=await fetch('/api/drive?t='+t+'&s='+s,{
      cache:'no-store',signal:controller.signal,keepalive:leaving
    });
    if(!response.ok)throw new Error('drive');
    await response.text();
    if(generation===sourceVersion)$('driveError').hidden=true;
  }catch(e){
    if(generation===sourceVersion)$('driveError').hidden=false;
  }finally{
    clearTimeout(timeout);
    inFlight=false;
    if(active()&&revision!==sentRevision)sendDrive(document.hidden);
  }
}
function applyStatus(data){
  if(!data||!['glove','web'].includes(data.source)||
     ![0,1].includes(data.link)||!Number.isFinite(data.cm))throw new Error('status');
  if(source!==data.source){
    source=data.source;
    sourceVersion++;
    stopNow();
  }
  $('status').textContent='Source: '+(data.source==='web'?'Web':'Glove')+
    ' · Glove: '+(data.link?'connected':'not connected')+
    ' · Range: '+(data.cm<0?'no echo':data.cm+' cm');
  lastOK=Date.now();
  $('connection').hidden=true;
  draw();
}
async function getStatus(){
  if(polling||switching)return;
  polling=true;
  const generation=sourceVersion;
  const controller=new AbortController();
  const timeout=setTimeout(()=>controller.abort(),1800);
  try{
    const response=await fetch('/api/status',{cache:'no-store',signal:controller.signal});
    if(!response.ok)throw new Error('status');
    const data=await response.json();
    if(!switching&&generation===sourceVersion)applyStatus(data);
  }catch(e){
    // The connection indicator uses elapsed time since the last valid status.
  }finally{
    clearTimeout(timeout);
    polling=false;
  }
}
async function selectSource(value){
  if(switching||source===value)return;
  stopNow();
  switching=true;
  sourceVersion++;
  draw();
  $('sourceError').hidden=true;
  const controller=new AbortController();
  const timeout=setTimeout(()=>controller.abort(),2000);
  try{
    const response=await fetch('/api/source?v='+value,{cache:'no-store',signal:controller.signal});
    if(!response.ok)throw new Error('source');
    const data=await response.json();
    applyStatus(data);
  }catch(e){
    source=null;
    $('sourceError').hidden=false;
    $('status').textContent='Control source unknown. Reconnecting…';
  }finally{
    clearTimeout(timeout);
    switching=false;
    draw();
    stopNow();
    getStatus();
  }
}
$('glove').addEventListener('click',()=>selectSource('glove'));
$('web').addEventListener('click',()=>selectSource('web'));
for(const choice of ['touch','keyboard']){
  $(choice).addEventListener('click',()=>{
    stopNow();
    method=choice;
    draw();
  });
}
$('stop').addEventListener('click',()=>stopNow());
$('speed').addEventListener('input',()=>{
  speed=Number($('speed').value);
  $('speedValue').value=speed+'%';
  revision++;
  readout();
  sendDrive();
});

function movePad(event){
  const rect=$('pad').getBoundingClientRect();
  const radius=rect.width/2;
  const travel=Math.max(1,radius-34);
  let x=(event.clientX-rect.left-radius)/travel;
  let y=(event.clientY-rect.top-radius)/travel;
  const distance=Math.hypot(x,y);
  if(distance>1){x/=distance;y/=distance;}
  $('knob').style.transform='translate('+(x*travel)+'px,'+(y*travel)+'px)';
  const deadZone=value=>Math.abs(value)<=.08?0:Math.sign(value)*(Math.abs(value)-.08)/.92*100;
  setValues(deadZone(-y),deadZone(x));
}
$('pad').addEventListener('pointerdown',event=>{
  if(!available()||method!=='touch'||pointer!==null||event.button!==0)return;
  event.preventDefault();
  pointer=event.pointerId;
  $('pad').setPointerCapture(pointer);
  movePad(event);
});
$('pad').addEventListener('pointermove',event=>{
  if(event.pointerId!==pointer)return;
  event.preventDefault();
  if(!available()){stopNow();return;}
  movePad(event);
});
function endPad(event){
  if(event.pointerId!==pointer)return;
  stopNow();
}
$('pad').addEventListener('pointerup',endPad);
$('pad').addEventListener('pointercancel',endPad);
$('pad').addEventListener('lostpointercapture',endPad);

function keyboardValues(){
  const held=(a,b)=>keys.has(a)||keys.has(b);
  setValues(100*(Number(held('KeyW','ArrowUp'))-Number(held('KeyS','ArrowDown'))),
    100*(Number(held('KeyD','ArrowRight'))-Number(held('KeyA','ArrowLeft'))));
}
window.addEventListener('keydown',event=>{
  if(!available()||method!=='keyboard')return;
  const code=event.code;
  if(code.startsWith('Arrow')||code==='Space')event.preventDefault();
  if(event.repeat)return;
  if(code==='Space'){stopNow();return;}
  if(!movement.has(code)||event.ctrlKey||event.metaKey||event.altKey)return;
  keys.add(code);
  keyboardValues();
});
window.addEventListener('keyup',event=>{
  if(active()&&method==='keyboard'&&(event.code.startsWith('Arrow')||event.code==='Space'))event.preventDefault();
  if(keys.delete(event.code))keyboardValues();
});
window.addEventListener('blur',()=>stopNow());
document.addEventListener('visibilitychange',()=>{
  if(document.hidden)stopNow(true);
});
window.addEventListener('pagehide',()=>stopNow(true));
window.addEventListener('pageshow',()=>{stopNow();getStatus();});
setInterval(()=>sendDrive(document.hidden),100);
setInterval(()=>{
  $('connection').hidden=Date.now()-lastOK<=2000;
  getStatus();
},500);
draw();
readout();
getStatus();
</script>
</body>
</html>)HTML";
