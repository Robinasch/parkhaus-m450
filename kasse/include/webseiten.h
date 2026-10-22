// HTML-Seiten der Kasse (Pico 1)
#pragma once

#define GEMEINSAMES_CSS R"css(
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<style>
body{font-family:system-ui,sans-serif;margin:0;background:#f2f4f7;color:#1d2733}
header{background:#1f4e79;color:#fff;padding:14px 20px}
header h1{margin:0;font-size:20px}
main{max-width:860px;margin:0 auto;padding:16px}
.karte{background:#fff;border-radius:8px;padding:16px;margin-bottom:16px;box-shadow:0 1px 3px #0002}
input,button{font-size:16px;padding:8px 10px;border-radius:6px;border:1px solid #9aa9b8}
button{background:#1f4e79;color:#fff;border:0;cursor:pointer}
button:disabled{background:#9aa9b8;cursor:default}
table{width:100%;border-collapse:collapse;font-size:14px}
th,td{text-align:left;padding:6px;border-bottom:1px solid #dde3ea}
.ok{color:#1a7f37;font-weight:bold}.nok{color:#c62828;font-weight:bold}
.meldung{margin-top:10px;font-weight:bold}
.betrag{font-size:28px;font-weight:bold}
small{color:#5b6b7b}
</style>
)css"

// Sendet die Uhrzeit des Browsers an die Kasse.
#define UHR_SYNC_JS R"js(
fetch('/api/zeit',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'t='+Math.floor(Date.now()/1000)});
const chf=r=>'CHF '+(r/100).toFixed(2);
const zeit=t=>new Date(t*1000).toLocaleString('de-CH',{day:'2-digit',month:'2-digit',year:'numeric',hour:'2-digit',minute:'2-digit'});
const dauer=m=>Math.floor(m/60)+' h '+(m%60)+' min';
const post=(url,daten)=>fetch(url,{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:new URLSearchParams(daten)});
)js"

const char SEITE_KASSE[] = R"html(<!doctype html><html lang="de"><head><title>Kasse – Parkhaus</title>)html" GEMEINSAMES_CSS R"html(
</head><body>
<header><h1>Parkhaus – Kasse / Automat</h1></header>
<main>
<div class="karte">
  <h2>Parkgebühr bezahlen</h2>
  <form id="suche"><input id="kz" placeholder="Kennzeichen, z.B. ZH 123456" required> <button>Suchen</button></form>
  <div id="resultat"></div>
</div>
<div class="karte">
  <h2>Parkierte Fahrzeuge <small id="frei"></small></h2>
  <table><thead><tr><th>Kennzeichen</th><th>Einfahrt</th><th>Dauer</th><th>Offen</th><th>Status</th></tr></thead>
  <tbody id="liste"></tbody></table>
  <p id="debuglink"></p>
</div>
</main>
<script>)html" UHR_SYNC_JS R"html(
let aktuell='';
async function zeigen(kz){
  aktuell=kz;
  const r=await fetch('/api/fahrzeug?kz='+encodeURIComponent(kz));
  const d=await r.json(); const el=document.getElementById('resultat');
  if(!r.ok){el.innerHTML='<p class="meldung nok">'+d.fehler+'</p>';return;}
  el.innerHTML='<p><b>'+d.kz+'</b><br>Einfahrt: '+zeit(d.einfahrt)+'<br>Parkdauer: '+dauer(d.dauerMin)+
    (d.bezahlt?'<br>Bezahlt: '+chf(d.bezahltTotal)+' um '+zeit(d.zahlung):'')+'</p>'+
    '<div class="betrag">Offen: '+chf(d.offen)+'</div>'+
    '<p class="'+(d.offen==0?'ok':'nok')+'">'+(d.offen==0?(d.bezahlt?'Status: bezahlt – Ausfahrt innert 15 Minuten':'Gratiszeit – keine Gebühr'):(d.bezahlt?'Ausfahrtszeit überschritten – Nachzahlung nötig':'Status: nicht bezahlt'))+'</p>'+
    '<button id="zahlen" '+(d.offen==0?'disabled':'')+'>Bezahlen</button><div id="zmeldung" class="meldung"></div>';
  document.getElementById('zahlen').onclick=async e=>{
    e.target.disabled=true;
    const z=await post('/api/bezahlen',{kz:d.kz}); const zd=await z.json();
    if(!z.ok){document.getElementById('zmeldung').innerHTML='<span class="nok">'+zd.fehler+'</span>';return;}
    await zeigen(d.kz); document.getElementById('zmeldung').innerHTML='<span class="ok">Zahlung erfolgreich</span>'; laden();
  };
}
document.getElementById('suche').onsubmit=e=>{e.preventDefault();zeigen(document.getElementById('kz').value);};
async function laden(){
  try{
    const d=await (await fetch('/api/fahrzeuge')).json();
    document.getElementById('frei').textContent='– '+d.frei+' freie Plätze von '+d.kapazitaet;
    document.getElementById('liste').innerHTML=d.fahrzeuge.map(f=>'<tr onclick="zeigen(\''+f.kz+'\')" style="cursor:pointer"><td>'+f.kz+'</td><td>'+zeit(f.einfahrt)+'</td><td>'+dauer(f.dauerMin)+'</td><td>'+chf(f.offen)+'</td><td class="'+(f.offen==0?'ok':'nok')+'">'+(f.bezahlt?(f.offen==0?'bezahlt':'Nachzahlung'):(f.offen==0?'gratis':'offen'))+'</td></tr>').join('')||'<tr><td colspan="5">Keine Fahrzeuge im Parkhaus</td></tr>';
    if(d.debug)document.getElementById('debuglink').innerHTML='<a href="/debug">Debug-Seite (nur Test)</a>';
  }catch(e){}
}
laden(); setInterval(laden,3000);
</script></body></html>)html";

const char SEITE_DEBUG[] = R"html(<!doctype html><html lang="de"><head><title>Debug – Parkhaus</title>)html" GEMEINSAMES_CSS R"html(
</head><body>
<header><h1>Debug-Seite (nur Test-Build)</h1></header>
<main>
<div class="karte">
  <p>Hier können Zeiten verstellt werden, damit Gebührentests ohne Warten möglich sind (siehe Testkonzept).
  Angabe jeweils in <b>Minuten vor jetzt</b>.</p>
  <table><thead><tr><th>Kennzeichen</th><th>Einfahrt vor … Min.</th><th>Zahlung vor … Min.</th></tr></thead><tbody id="liste"></tbody></table>
</div>
<div class="karte">
  <h2>Fahrzeug direkt erfassen</h2>
  <input id="neu" placeholder="z.B. BE 45678"> <button onclick="aktion('/api/einfahrt',{kz:neu.value})">Einfahrt erfassen</button>
</div>
<div class="karte">
  <h2>Kapazität</h2>
  <input id="kap" type="number" min="1" max="100" style="width:90px"> <button onclick="aktion('/api/debug/kapazitaet',{n:kap.value})">Setzen</button>
  <h2>Alles zurücksetzen</h2>
  <button onclick="if(confirm('Alle Fahrzeuge löschen?'))aktion('/api/debug/reset',{})">Reset</button>
  <div id="meldung" class="meldung"></div>
  <p><a href="/">Zurück zur Kasse</a></p>
</div>
</main>
<script>)html" UHR_SYNC_JS R"html(
async function aktion(url,daten){
  const r=await post(url,daten);
  document.getElementById('meldung').innerHTML='<span class="'+(r.ok?'ok':'nok')+'">'+await r.text()+'</span>'; laden();
}
async function laden(){
  const d=await (await fetch('/api/fahrzeuge')).json();
  document.getElementById('kap').placeholder=d.kapazitaet;
  document.getElementById('liste').innerHTML=d.fahrzeuge.map((f,i)=>'<tr><td>'+f.kz+'<br><small>'+dauer(f.dauerMin)+', offen '+chf(f.offen)+'</small></td>'+
    '<td><input id="e'+i+'" type="number" style="width:80px"> <button onclick="aktion(\'/api/debug/einfahrt\',{kz:\''+f.kz+'\',min:e'+i+'.value})">Setzen</button></td>'+
    '<td>'+(f.bezahlt?'<input id="z'+i+'" type="number" style="width:80px"> <button onclick="aktion(\'/api/debug/zahlung\',{kz:\''+f.kz+'\',min:z'+i+'.value})">Setzen</button>':'<small>nicht bezahlt</small>')+'</td></tr>').join('')||'<tr><td colspan="3">Keine Fahrzeuge</td></tr>';
}
laden();
</script></body></html>)html";
