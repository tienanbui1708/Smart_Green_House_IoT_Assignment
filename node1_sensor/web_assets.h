// node1_sensor/web_assets.h - SINH TU DONG boi tools/embed_web.py tu web/index.html. KHONG sua tay.
#pragma once
#include <pgmspace.h>

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="vi">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Smart Greenhouse</title>
<style>
:root{--bg:#f4f6f3;--card:#fff;--fg:#1d2a1f;--mut:#5f6b61;--ac:#2e7d32;--warn:#b26a00;--err:#c62828;--bd:#dde3dc}
@media(prefers-color-scheme:dark){:root{--bg:#121613;--card:#1c221d;--fg:#e6ece6;--mut:#9aa69c;--bd:#2c352d}}
*{box-sizing:border-box}
body{margin:0;font:15px/1.45 system-ui,-apple-system,"Segoe UI",Roboto,sans-serif;background:var(--bg);color:var(--fg)}
header{display:flex;align-items:center;justify-content:space-between;gap:8px;padding:12px 16px;background:var(--ac);color:#fff}
header h1{font-size:17px;margin:0}
#conn{font-size:13px;opacity:.9}
nav{display:flex;overflow-x:auto;background:var(--card);border-bottom:1px solid var(--bd)}
nav button{flex:1;min-width:90px;padding:11px 8px;border:0;background:none;color:var(--mut);font:inherit;cursor:pointer;border-bottom:3px solid transparent}
nav button.on{color:var(--ac);border-bottom-color:var(--ac);font-weight:600}
main{max-width:760px;margin:0 auto;padding:14px}
section{display:none}section.on{display:block}
.grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(150px,1fr));gap:10px}
.card{background:var(--card);border:1px solid var(--bd);border-radius:10px;padding:12px}
.card .k{font-size:12px;color:var(--mut);text-transform:uppercase;letter-spacing:.03em}
.card .v{font-size:24px;font-weight:600;margin-top:2px}
.row{display:flex;align-items:center;justify-content:space-between;gap:12px;padding:12px 0;border-bottom:1px solid var(--bd)}
.row:last-child{border-bottom:0}
table{width:100%;border-collapse:collapse}td{padding:7px 4px;border-bottom:1px solid var(--bd)}td:first-child{color:var(--mut);width:42%}
label.f{display:block;margin:10px 0 4px;color:var(--mut);font-size:13px}
input[type=text],input[type=password],input[type=number]{width:100%;padding:9px;border:1px solid var(--bd);border-radius:8px;background:var(--bg);color:var(--fg);font:inherit}
input[type=range]{width:100%}
.btn{padding:10px 16px;border:0;border-radius:8px;background:var(--ac);color:#fff;font:inherit;cursor:pointer;margin-top:14px}
.btn:disabled{opacity:.5}
.sw{position:relative;width:52px;height:28px;flex:none}
.sw input{opacity:0;width:0;height:0}
.sw span{position:absolute;inset:0;background:#9aa69c;border-radius:28px;transition:.2s;cursor:pointer}
.sw span:before{content:"";position:absolute;width:22px;height:22px;left:3px;top:3px;background:#fff;border-radius:50%;transition:.2s}
.sw input:checked+span{background:var(--ac)}.sw input:checked+span:before{transform:translateX(24px)}
.sw input:disabled+span{opacity:.45;cursor:not-allowed}
#msg{position:fixed;left:50%;bottom:18px;transform:translateX(-50%);padding:10px 16px;border-radius:8px;color:#fff;display:none;max-width:90%}
.badge{display:inline-block;padding:2px 8px;border-radius:12px;font-size:13px;background:var(--bd)}
.bad{color:var(--err)}.warn{color:var(--warn)}.ok{color:var(--ac)}
.hint{color:var(--mut);font-size:13px}
</style>
</head>
<body>
<header><h1 id="title">Smart Greenhouse</h1><span id="conn">đang kết nối…</span></header>
<nav>
<button data-t="home" class="on">Trang chủ</button>
<button data-t="ctrl">Điều khiển</button>
<button data-t="info">Thông tin</button>
<button data-t="set">Cài đặt</button>
</nav>
<main>
<section id="home" class="on">
<div class="grid" id="cards"></div>
<p class="hint" id="homeNote"></p>
</section>

<section id="ctrl">
<div class="card">
<div class="row"><div><b>Chế độ tự động</b><div class="hint">Bật Auto: thiết bị tự quyết, điều khiển tay bị khóa</div></div>
<label class="sw"><input type="checkbox" id="auto"><span></span></label></div>
<div id="ctl1" style="display:none">
<div class="row" style="display:block"><b>Sưởi mô phỏng</b> <span class="badge" id="htVal">0</span>
<input type="range" id="heater" min="0" max="255" step="1"></div>
</div>
<div id="ctl2" style="display:none">
<div class="row"><div><b>Quạt</b> <span class="hint">(LED D13)</span></div><label class="sw"><input type="checkbox" id="fan"><span></span></label></div>
<div class="row"><div><b>Bơm</b> <span class="hint">(mô phỏng trên LCD)</span></div><label class="sw"><input type="checkbox" id="pump"><span></span></label></div>
</div>
</div>
<p class="hint">Chấp hành chỉ là mô phỏng (NeoPixel/LED/LCD), kit không có bơm/quạt thật.</p>
</section>

<section id="info"><div class="card"><table id="infoT"></table></div></section>

<section id="set">
<div class="card">
<p class="hint">Để trống mật khẩu/token để giữ giá trị hiện tại. Lưu xong thiết bị sẽ khởi động lại.</p>
<label class="f">Wi-Fi SSID</label><input type="text" id="c_ssid" maxlength="32">
<label class="f">Mật khẩu Wi-Fi</label><input type="password" id="c_pass" maxlength="64" placeholder="(giữ nguyên)">
<label class="f">MQTT host</label><input type="text" id="c_host" maxlength="63">
<label class="f">MQTT port</label><input type="number" id="c_port" min="1" max="65535">
<label class="f">Access Token</label><input type="password" id="c_tok" maxlength="64" placeholder="(giữ nguyên)">
<button class="btn" id="save">Lưu và khởi động lại</button>
<p class="hint" id="cfgSrc"></p>
</div>
</section>
</main>
<div id="msg"></div>
<script>
const $=id=>document.getElementById(id);
const esc=v=>String(v).replace(/[&<>"']/g,c=>"&#"+c.charCodeAt(0)+";");
const AI=["Bình thường","Đất khô","Nóng/ẩm","Cảm biến lỗi"];
let st=null,busy=false,cfgFilled=false,sliding=false;
document.querySelectorAll("nav button").forEach(b=>b.onclick=()=>{
 document.querySelectorAll("nav button,section").forEach(e=>e.classList.remove("on"));
 b.classList.add("on");$(b.dataset.t).classList.add("on");});
function toast(t,ok){const m=$("msg");m.textContent=t;m.style.background=ok?"#2e7d32":"#c62828";m.style.display="block";
 clearTimeout(m._t);m._t=setTimeout(()=>m.style.display="none",3000);}
const REASON={auto_mode:"Đang ở chế độ Auto: chuyển sang Manual để điều khiển",bad_params:"Tham số không hợp lệ",
 confirm_required:"Thiếu xác nhận",save_failed:"Ghi cấu hình thất bại",not_found:"Không có API này"};
async function post(url,body){
 try{const r=await fetch(url,{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify(body)});
  const j=await r.json().catch(()=>({ok:false,reason:"HTTP "+r.status}));
  if(!j.ok)toast(REASON[j.reason]||("Lỗi: "+j.reason),false);
  return j.ok;}catch(e){toast("Mất kết nối tới thiết bị",false);return false;}
 finally{refresh();}}
const fmt=(v,u,d=1)=>v===undefined||v===null?"--":(typeof v=="number"?v.toFixed(d):v)+(u||"");
const card=(k,v,c)=>`<div class="card"><div class="k">${k}</div><div class="v ${c||""}">${esc(v)}</div></div>`;
function upTime(s){const h=Math.floor(s/3600),m=Math.floor(s%3600/60);return h+" giờ "+m+" phút";}
function renderHome(s){
 const n2=s.node==2;let h="";
 h+=card("Nhiệt độ",fmt(s.temperature," °C"))+card("Độ ẩm KK",fmt(s.humidity," %"))+card("Độ ẩm đất",fmt(s.soil," %",0));
 const cls=s.ai_class;h+=card("Nhãn AI",cls==255?"Khởi động…":(AI[cls]||cls),cls==3?"bad":(cls==1||cls==2?"warn":""));
 if(n2){h+=card("Quạt",s.fan?"BẬT":"TẮT",s.fan?"ok":"")+card("Bơm",s.pump?"BẬT":"TẮT",s.pump?"ok":"");
  h+=card("ESP-NOW",s.espnow_age_s>=999?"chưa có":s.espnow_age_s+" s trước",s.data_stale?"bad":"");
  if(s.alert_level)h+=card("Cảnh báo Cloud",s.alert_msg||("Mức "+s.alert_level),s.alert_level==2?"bad":"warn");}
 else{h+=card("Sưởi",s.heater+"/255")+card("ESP-NOW ack",s.espnow_ack_pct+" %",s.espnow_ack_pct<80?"warn":"");}
 h+=card("Chế độ",s.auto?"Auto":"Manual");
 $("cards").innerHTML=h;
 let note="";if(s.sensor_fault||s.data_fault)note="Cảm biến báo lỗi: giá trị đo bị ẩn, chấp hành ở chế độ an toàn.";
 else if(s.data_stale)note="Không nhận dữ liệu ESP-NOW từ Node 1 quá 10 s: chế độ an toàn (NO DATA).";
 $("homeNote").textContent=note;}
function renderCtrl(s){
 $("ctl1").style.display=s.node==1?"":"none";$("ctl2").style.display=s.node==2?"":"none";
 $("auto").checked=s.auto;const lock=s.auto;
 if(s.node==1){if(!sliding){$("heater").value=s.heater;$("htVal").textContent=s.heater;}$("heater").disabled=lock;}
 else{$("fan").checked=s.fan;$("pump").checked=s.pump;$("fan").disabled=lock;$("pump").disabled=lock;}}
function renderInfo(s){
 const r=[["Node",s.node],["Phiên bản firmware",s.fw_version],["Wi-Fi SSID",s.cfg.wifi_ssid],["IP",s.ip],["MAC",s.mac],
  ["RSSI",s.rssi+" dBm"],["Kênh",s.channel],["MQTT CoreIOT",s.mqtt?"đã kết nối":"chưa"],
  ["AP cứu hộ",s.ap?("bật, "+s.ap_ip+", "+s.ap_clients+" máy"):"tắt"],["Heap trống",s.heap+" B (min "+s.min_heap+")"],["Uptime",upTime(s.uptime)]];
 if(s.node==1)r.push(["ESP-NOW",(s.espnow_ready?"sẵn sàng":"lỗi")+", gửi "+s.espnow_sent+", ack "+s.espnow_acked+", RTT "+(s.espnow_rtt_us/1000).toFixed(1)+" ms"]);
 else r.push(["ESP-NOW",s.espnow_age_s>=999?"chưa nhận gói":"gói cuối "+s.espnow_age_s+" s trước (seq "+s.espnow_seq+")"]);
 $("infoT").innerHTML=r.map(x=>`<tr><td>${x[0]}</td><td>${esc(x[1])}</td></tr>`).join("");}
function renderSet(s){
 if(!cfgFilled){$("c_ssid").value=s.cfg.wifi_ssid;$("c_host").value=s.cfg.mqtt_host;$("c_port").value=s.cfg.mqtt_port;cfgFilled=true;}
 $("c_tok").placeholder=s.cfg.token_set?"(đã đặt, để trống = giữ nguyên)":"(chưa đặt)";
 $("cfgSrc").textContent="Nguồn cấu hình hiện tại: "+s.cfg.source;}
async function refresh(){
 if(busy)return;busy=true;
 try{const r=await fetch("/api/state",{cache:"no-store"});st=await r.json();
  $("title").textContent="Smart Greenhouse – Node "+st.node;
  $("conn").textContent=(st.mqtt?"CoreIOT ✓":"CoreIOT ✗")+" · "+(st.wifi?"Wi-Fi":"AP");
  renderHome(st);renderCtrl(st);renderInfo(st);renderSet(st);}
 catch(e){$("conn").textContent="mất kết nối";}
 finally{busy=false;}}
$("auto").onchange=e=>post("/api/mode",{auto:e.target.checked});
$("heater").oninput=e=>{sliding=true;$("htVal").textContent=e.target.value;};
$("heater").onchange=e=>{sliding=false;post("/api/heater",{value:+e.target.value});};
$("fan").onchange=e=>post("/api/fan",{on:e.target.checked});
$("pump").onchange=e=>post("/api/pump",{on:e.target.checked});
$("save").onclick=async()=>{
 const port=+$("c_port").value;
 if(!$("c_ssid").value||!$("c_host").value||!(port>=1&&port<=65535)){toast("Điền SSID, host và port 1..65535",false);return;}
 if(!confirm("Lưu cấu hình và khởi động lại thiết bị?"))return;
 const b={confirm:true,wifi_ssid:$("c_ssid").value,mqtt_host:$("c_host").value,mqtt_port:port};
 if($("c_pass").value)b.wifi_pass=$("c_pass").value;if($("c_tok").value)b.device_token=$("c_tok").value;
 $("save").disabled=true;
 if(await post("/api/config",b))toast("Đã lưu, thiết bị đang khởi động lại…",true);
 $("save").disabled=false;};
refresh();setInterval(refresh,2000);
</script>
</body>
</html>
)rawliteral";
