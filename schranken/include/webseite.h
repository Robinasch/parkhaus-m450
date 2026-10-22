// HTML-Seite der Kamera-Simulation (Pico 2)
#pragma once

const char SEITE_SCHRANKEN[] = R"html(<!doctype html><html lang="de"><head><title>Schranken – Parkhaus</title>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<style>
body{font-family:system-ui,sans-serif;margin:0;background:#f2f4f7;color:#1d2733}
header{background:#1f4e79;color:#fff;padding:14px 20px}
header h1{margin:0;font-size:20px}
main{max-width:860px;margin:0 auto;padding:16px;display:grid;grid-template-columns:repeat(auto-fit,minmax(280px,1fr));gap:16px}
.karte{background:#fff;border-radius:8px;padding:16px;box-shadow:0 1px 3px #0002}
input,button{font-size:16px;padding:8px 10px;border-radius:6px;border:1px solid #9aa9b8}
input{width:100%;box-sizing:border-box;margin-bottom:8px}
button{background:#1f4e79;color:#fff;border:0;cursor:pointer;width:100%}
button:disabled{background:#9aa9b8}
.schranke{font-size:22px;font-weight:bold;margin:10px 0}
.offen{color:#1a7f37}.zu{color:#c62828}
.meldung{min-height:24px;font-weight:bold}
.ok{color:#1a7f37}.nok{color:#c62828}
#status{grid-column:1/-1}
</style></head><body>
<header><h1>Parkhaus – Kamera-Simulation und Schranken</h1></header>
<main>
<div class="karte">
  <h2>Einfahrt</h2>
  <form data-art="einfahrt"><input placeholder="Kennzeichen, z.B. ZH 123456" required><button>Einfahren</button></form>
  <div class="schranke" id="s-einfahrt">Schranke: –</div>
  <div class="meldung" id="m-einfahrt"></div>
</div>
<div class="karte">
  <h2>Ausfahrt</h2>
  <form data-art="ausfahrt"><input placeholder="Kennzeichen, z.B. ZH 123456" required><button>Ausfahren</button></form>
  <div class="schranke" id="s-ausfahrt">Schranke: –</div>
  <div class="meldung" id="m-ausfahrt"></div>
</div>
<div class="karte" id="status">Status wird geladen …</div>
</main>
<script>
// Uhrzeit des Browsers an die Kasse senden (für korrekte Einfahrtszeiten)
fetch('http://192.168.42.1/api/zeit',{method:'POST',mode:'no-cors',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'t='+Math.floor(Date.now()/1000)}).catch(()=>{});
document.querySelectorAll('form').forEach(f=>f.onsubmit=async e=>{
  e.preventDefault();
  const art=f.dataset.art, btn=f.querySelector('button'), m=document.getElementById('m-'+art);
  btn.disabled=true; m.textContent='…';
  try{
    const r=await fetch('/api/'+art,{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:new URLSearchParams({kz:f.querySelector('input').value})});
    const d=await r.json();
    m.className='meldung '+(d.ok?'ok':'nok'); m.textContent=d.meldung;
    if(d.ok)f.querySelector('input').value='';
  }catch(err){m.className='meldung nok';m.textContent='Schranken-Pico nicht erreichbar';}
  btn.disabled=false; status();
});
async function status(){
  try{
    const d=await (await fetch('/api/status')).json();
    for(const s of ['einfahrt','ausfahrt']){
      const el=document.getElementById('s-'+s);
      el.textContent='Schranke: '+(d[s]?'OFFEN':'ZU'); el.className='schranke '+(d[s]?'offen':'zu');
    }
    document.getElementById('status').innerHTML='WLAN: <b class="'+(d.wlan?'ok':'nok')+'">'+(d.wlan?'verbunden':'getrennt')+'</b> &nbsp; Kasse: <b class="'+(d.kasse?'ok':'nok')+'">'+(d.kasse?'erreichbar':'nicht erreichbar / noch nicht kontaktiert')+'</b> &nbsp; <a href="http://192.168.42.1/" target="_blank">Zur Kasse</a>';
  }catch(e){}
}
status(); setInterval(status,1000);
</script></body></html>)html";
