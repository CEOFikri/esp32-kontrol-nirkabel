#include <WiFi.h>
#include <WebServer.h>

// Konfigurasi WiFi & IP Statis
const char* ssid = "HOKIku";
const char* password = "Banjarmasin89";

IPAddress local_IP(192, 168, 176, 111);
IPAddress gateway(192, 168, 176, 1);
IPAddress subnet(255, 255, 255, 0);

WebServer server(80);

// Daftar pin ESP32 yang AMAN digunakan sebagai OUTPUT
const int outputPins[] = {2, 4, 12, 13, 14, 15, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33};
const int numPins = sizeof(outputPins) / sizeof(outputPins[0]);

// Fungsi cek status login (cookie)
bool is_authenticated() {
  if (server.hasHeader("Cookie")) {
    String cookie = server.header("Cookie");
    if (cookie.indexOf("ESPSESSIONID=1") != -1) {
      return true;
    }
  }
  return false;
}

// ==========================================
// TAMPILAN HALAMAN LOGIN
// ==========================================
const char login_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>System Login</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { background-color: #050505; color: #00ff00; font-family: 'Courier New', Courier, monospace; display: flex; justify-content: center; align-items: center; height: 100vh; margin: 0; }
    .login-box { border: 1px solid #00ff00; padding: 40px; border-radius: 5px; box-shadow: 0 0 20px rgba(0, 255, 0, 0.2); text-align: center; background: #0a0a0a; width: 300px; }
    h2 { text-transform: uppercase; letter-spacing: 3px; border-bottom: 1px dashed #00ff00; padding-bottom: 10px; margin-bottom: 30px; }
    input[type="text"], input[type="password"] { background: transparent; border: 1px solid #00ff00; color: #00ff00; padding: 12px; margin: 10px 0; width: 90%; text-align: center; font-family: inherit; font-size: 16px; outline: none; }
    input[type="text"]:focus, input[type="password"]:focus { box-shadow: 0 0 10px #00ff00; }
    input[type="submit"] { background: transparent; color: #00ff00; border: 1px solid #00ff00; padding: 12px 20px; cursor: pointer; font-weight: bold; text-transform: uppercase; margin-top: 20px; width: 100%; transition: 0.3s; font-family: inherit; font-size: 16px; }
    input[type="submit"]:hover { background: #00ff00; color: #000; box-shadow: 0 0 15px #00ff00; }
  </style>
</head>
<body>
  <div class="login-box">
    <h2>Admin Access</h2>
    <form action="/login" method="POST">
      <input type="text" name="username" placeholder="USERNAME" required><br>
      <input type="password" name="password" placeholder="PASSWORD" required><br>
      <input type="submit" value="LOGIN">
    </form>
  </div>
</body>
</html>
)rawliteral";

// ==========================================
// TAMPILAN APLIKASI UTAMA (TABS: DASHBOARD & CONTROLLING)
// ==========================================
const char app_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>System Matrix</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { background-color: #050505; color: #00ff00; font-family: 'Courier New', Courier, monospace; margin: 0; padding: 0; }
    .header { text-align: center; padding: 20px; border-bottom: 1px solid #00ff00; background: #0a0a0a; }
    h1 { margin: 0; text-transform: uppercase; letter-spacing: 2px; text-shadow: 0 0 10px #00ff00; }
    
    /* Tabs Navigation */
    .tab-nav { display: flex; justify-content: center; background: #0a0a0a; border-bottom: 1px solid #00ff00; }
    .tab-btn { background: transparent; color: #00ff00; border: none; padding: 15px 30px; cursor: pointer; font-size: 16px; font-weight: bold; text-transform: uppercase; transition: 0.3s; font-family: inherit; }
    .tab-btn:hover { background: rgba(0, 255, 0, 0.1); }
    .tab-btn.active { border-bottom: 3px solid #00ff00; background: rgba(0, 255, 0, 0.2); }
    
    .content-area { padding: 20px; max-width: 1000px; margin: 0 auto; }
    .tab-content { display: none; }
    .tab-content.active { display: block; animation: fadeIn 0.5s; }
    @keyframes fadeIn { from { opacity: 0; } to { opacity: 1; } }

    /* Dashboard Widgets */
    .widget-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 20px; margin-bottom: 20px; }
    .widget { border: 1px solid #00ff00; padding: 20px; border-radius: 5px; background: #0a0a0a; box-shadow: 0 0 10px rgba(0, 255, 0, 0.1); text-align: center; }
    .widget h3 { margin-top: 0; color: #aaa; font-size: 14px; text-transform: uppercase; }
    .widget .value { font-size: 24px; font-weight: bold; }
    
    /* Terminal Log */
    .log-box { border: 1px solid #00ff00; background: #000; padding: 15px; height: 200px; overflow-y: auto; font-size: 14px; }
    .log-entry { margin-bottom: 5px; border-bottom: 1px dashed #333; padding-bottom: 5px; }
    .log-time { color: #888; margin-right: 10px; }

    /* Controlling Grid */
    .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(130px, 1fr)); gap: 15px; }
    .card { border: 1px solid #00ff00; padding: 20px 10px; border-radius: 5px; background: #0a0a0a; text-align: center; }
    .pin-label { font-size: 18px; font-weight: bold; margin-bottom: 15px; }
    .btn { display: block; width: 90%; margin: 0 auto; padding: 12px; background: transparent; border: 1px solid #00ff00; color: #00ff00; cursor: pointer; font-weight: bold; transition: 0.3s; font-family: inherit; }
    .btn.on { background: #00ff00; color: #000; box-shadow: 0 0 15px #00ff00; }
    .btn:hover { background: rgba(0, 255, 0, 0.2); }
    
    .logout { display: block; width: 150px; margin: 30px auto; text-align: center; text-decoration: none; color: #00ff00; border: 1px solid #00ff00; padding: 10px; text-transform: uppercase; font-weight: bold; transition: 0.3s; }
    .logout:hover { background: #ff0000; color: #fff; border-color: #ff0000; box-shadow: 0 0 15px #ff0000; }
  </style>
</head>
<body>
  <div class="header">
    <h1>System Matrix ESP32</h1>
  </div>

  <div class="tab-nav">
    <button class="tab-btn active" onclick="switchTab('dashboard')">Dashboard</button>
    <button class="tab-btn" onclick="switchTab('controlling')">Controlling</button>
  </div>

  <div class="content-area">
    <div id="dashboard" class="tab-content active">
      <div class="widget-grid">
        <div class="widget"><h3>Uptime System</h3><div id="sys-uptime" class="value">00:00:00</div></div>
        <div class="widget"><h3>Free RAM (Heap)</h3><div id="sys-ram" class="value">0 KB</div></div>
        <div class="widget"><h3>CPU Freq</h3><div id="sys-cpu" class="value">0 MHz</div></div>
        <div class="widget"><h3>WiFi Signal (RSSI)</h3><div id="sys-rssi" class="value">0 dBm</div></div>
        <div class="widget"><h3>Network IP</h3><div id="sys-ip" class="value">0.0.0.0</div></div>
        <div class="widget"><h3>MAC Address</h3><div id="sys-mac" class="value">00:00:00:00</div></div>
      </div>
      <h3>System Logs</h3>
      <div class="log-box" id="terminal-log">
        </div>
    </div>

    <div id="controlling" class="tab-content">
      <div class="grid" id="pins-container"></div>
    </div>
    
    <a href="/logout" class="logout">LOGOUT</a>
  </div>

  <script>
    const pins = [2, 4, 12, 13, 14, 15, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33];
    let pinStates = {}; // Menyimpan status pin sebelumnya untuk deteksi perubahan log

    // Navigasi Tab
    function switchTab(tabId) {
      document.querySelectorAll('.tab-content').forEach(el => el.classList.remove('active'));
      document.querySelectorAll('.tab-btn').forEach(el => el.classList.remove('active'));
      document.getElementById(tabId).classList.add('active');
      event.target.classList.add('active');
    }

    // Format Uptime (Detik -> HH:MM:SS)
    function formatUptime(seconds) {
      const h = Math.floor(seconds / 3600).toString().padStart(2, '0');
      const m = Math.floor((seconds % 3600) / 60).toString().padStart(2, '0');
      const s = (seconds % 60).toString().padStart(2, '0');
      return `${h}:${m}:${s}`;
    }

    // Menambah Log ke Terminal
    function addLog(message) {
      const logBox = document.getElementById('terminal-log');
      const now = new Date();
      const timeStr = now.toLocaleTimeString();
      const logEntry = `<div class="log-entry"><span class="log-time">[${timeStr}]</span> ${message}</div>`;
      logBox.innerHTML = logEntry + logBox.innerHTML; // Tambah di paling atas
    }

    // Inisialisasi Tampilan Pin
    function initPins() {
      const container = document.getElementById('pins-container');
      pins.forEach(pin => {
        pinStates[pin] = "-1"; // Status awal tidak diketahui
        container.innerHTML += `
          <div class="card">
            <div class="pin-label">GPIO ${pin}</div>
            <button id="btn-${pin}" class="btn" onclick="togglePin(${pin})">OFF</button>
          </div>
        `;
      });
      fetchData(); // Fetch awal
      setInterval(fetchData, 3000); // Polling data setiap 3 detik
      addLog("System Frontend Initialized. Connected to ESP32.");
    }

    function togglePin(pin) {
      fetch(`/toggle?pin=${pin}`)
        .then(response => response.text())
        .then(state => { 
          updateButton(pin, state); 
          addLog(`Action: User toggled GPIO ${pin} to ${state == "1" ? "ON" : "OFF"}`);
        });
    }

    // Fetch Status Pin & Data Sistem sekaligus
    function fetchData() {
      // Ambil data PIN
      fetch('/states')
        .then(response => response.json())
        .then(data => {
          for (let pin in data) { 
            if(pinStates[pin] !== "-1" && pinStates[pin] !== String(data[pin])) {
              // Jika status berubah dari polling, catat di log
              addLog(`System Update: GPIO ${pin} status changed to ${data[pin] == 1 ? "ON" : "OFF"}`);
            }
            pinStates[pin] = String(data[pin]);
            updateButton(pin, data[pin]); 
          }
        });

      // Ambil data SYSTEM
      fetch('/sysinfo')
        .then(response => response.json())
        .then(data => {
          document.getElementById('sys-uptime').innerText = formatUptime(data.uptime);
          document.getElementById('sys-ram').innerText = (data.free_heap / 1024).toFixed(2) + " KB";
          document.getElementById('sys-cpu').innerText = data.cpu_freq + " MHz";
          document.getElementById('sys-rssi').innerText = data.rssi + " dBm";
          document.getElementById('sys-ip').innerText = data.ip;
          document.getElementById('sys-mac').innerText = data.mac;
        });
    }

    function updateButton(pin, state) {
      const btn = document.getElementById(`btn-${pin}`);
      if(btn) {
        if(state == "1" || state == 1) {
          btn.classList.add("on"); btn.innerHTML = "ON";
        } else {
          btn.classList.remove("on"); btn.innerHTML = "OFF";
        }
      }
    }

    window.onload = initPins;
  </script>
</body>
</html>
)rawliteral";

void setup() {
  Serial.begin(115200);

  // Inisialisasi Mode Pin
  for (int i = 0; i < numPins; i++) {
    pinMode(outputPins[i], OUTPUT);
    digitalWrite(outputPins[i], LOW);
  }

  // Set IP Statis
  if (!WiFi.config(local_IP, gateway, subnet)) {
    Serial.println("Gagal mengatur IP Statis");
  }

  WiFi.begin(ssid, password);
  Serial.print("Menghubungkan ke "); Serial.print(ssid);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  
  Serial.println("\nTerhubung!");
  Serial.print("Buka browser dan akses IP: "); Serial.println(WiFi.localIP());

  const char * headerkeys[] = {"Cookie"};
  size_t headerkeyssize = sizeof(headerkeys) / sizeof(char*);
  server.collectHeaders(headerkeys, headerkeyssize);

  // Routing Web Server
  server.on("/", HTTP_GET, []() {
    if (!is_authenticated()) {
      server.sendHeader("Location", "/login"); server.send(301); return;
    }
    server.send(200, "text/html", app_html); // Memanggil halaman yang sudah ada Tab
  });

  server.on("/login", HTTP_GET, []() { server.send(200, "text/html", login_html); });

  server.on("/login", HTTP_POST, []() {
    if (server.hasArg("username") && server.hasArg("password")) {
      if (server.arg("username") == "admin" && server.arg("password") == "admin") {
        server.sendHeader("Location", "/");
        server.sendHeader("Cache-Control", "no-cache");
        server.sendHeader("Set-Cookie", "ESPSESSIONID=1; Path=/; Max-Age=86400");
        server.send(301);
        return;
      }
    }
    server.send(401, "text/plain", "Akses Ditolak. Username atau Password salah.");
  });

  server.on("/logout", HTTP_GET, []() {
    server.sendHeader("Location", "/login");
    server.sendHeader("Cache-Control", "no-cache");
    server.sendHeader("Set-Cookie", "ESPSESSIONID=0; Path=/; Max-Age=0");
    server.send(301);
  });

  // API Endpoint: Toggle Pin
  server.on("/toggle", HTTP_GET, []() {
    if (!is_authenticated()) { server.send(401, "text/plain", "Unauthorized"); return; }
    if (server.hasArg("pin")) {
      int pin = server.arg("pin").toInt();
      bool validPin = false;
      for (int i = 0; i < numPins; i++) {
        if (outputPins[i] == pin) { validPin = true; break; }
      }
      if (validPin) {
        int currentState = digitalRead(pin);
        digitalWrite(pin, !currentState);
        server.send(200, "text/plain", String(!currentState));
        return;
      }
    }
    server.send(400, "text/plain", "Invalid Pin");
  });

  // API Endpoint: Pin States
  server.on("/states", HTTP_GET, []() {
    if (!is_authenticated()) { server.send(401, "text/plain", "Unauthorized"); return; }
    String json = "{";
    for (int i = 0; i < numPins; i++) {
      json += "\"" + String(outputPins[i]) + "\":" + String(digitalRead(outputPins[i]));
      if (i < numPins - 1) json += ",";
    }
    json += "}";
    server.send(200, "application/json", json);
  });

  // API Endpoint BARU: System Info (Untuk Dashboard)
  server.on("/sysinfo", HTTP_GET, []() {
    if (!is_authenticated()) { server.send(401, "text/plain", "Unauthorized"); return; }
    
    // Format IP address menjadi String
    String ipStr = WiFi.localIP().toString();
    
    // Menyusun response JSON berisi data sistem
    String json = "{";
    json += "\"uptime\":" + String(millis() / 1000) + ",";
    json += "\"free_heap\":" + String(ESP.getFreeHeap()) + ",";
    json += "\"cpu_freq\":" + String(ESP.getCpuFreqMHz()) + ",";
    json += "\"rssi\":" + String(WiFi.RSSI()) + ",";
    json += "\"ip\":\"" + ipStr + "\",";
    json += "\"mac\":\"" + WiFi.macAddress() + "\"";
    json += "}";
    
    server.send(200, "application/json", json);
  });

  server.begin();
}

void loop() {
  server.handleClient();
}