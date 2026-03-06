#include <WiFi.h>
#include <WebServer.h>

// Konfigurasi WiFi & IP Statis
const char* ssid = "HOKIku";
const char* password = "Banjarmasin89";

IPAddress local_IP(192, 168, 176, 111);
IPAddress gateway(192, 168, 176, 1); // Asumsi gateway router pada umumnya
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
// TAMPILAN HALAMAN LOGIN (Black & Green Tech)
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
// TAMPILAN KONTROL PANEL (Black & Green Tech)
// ==========================================
const char dashboard_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Control Matrix</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { background-color: #050505; color: #00ff00; font-family: 'Courier New', Courier, monospace; text-align: center; margin: 0; padding: 20px; }
    h1 { text-transform: uppercase; letter-spacing: 2px; text-shadow: 0 0 10px #00ff00; margin-bottom: 30px; }
    .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(130px, 1fr)); gap: 15px; max-width: 900px; margin: 0 auto; }
    .card { border: 1px solid #00ff00; padding: 20px 10px; border-radius: 5px; background: #0a0a0a; box-shadow: 0 0 10px rgba(0, 255, 0, 0.1); }
    .pin-label { font-size: 18px; font-weight: bold; margin-bottom: 15px; }
    .btn { display: block; width: 90%; margin: 0 auto; padding: 12px; background: transparent; border: 1px solid #00ff00; color: #00ff00; cursor: pointer; font-weight: bold; transition: 0.3s; font-family: inherit; }
    .btn.on { background: #00ff00; color: #000; box-shadow: 0 0 15px #00ff00; }
    .btn:hover { background: rgba(0, 255, 0, 0.2); }
    .btn.on:hover { background: #00cc00; }
    .logout { display: inline-block; margin-top: 40px; text-decoration: none; color: #00ff00; border: 1px solid #00ff00; padding: 10px 30px; text-transform: uppercase; font-weight: bold; transition: 0.3s; }
    .logout:hover { background: #ff0000; color: #fff; border-color: #ff0000; box-shadow: 0 0 15px #ff0000; }
  </style>
</head>
<body>
  <h1>System Control Matrix</h1>
  <div class="grid" id="pins-container">
    </div>
  <a href="/logout" class="logout">LOGOUT</a>

  <script>
    const pins = [2, 4, 12, 13, 14, 15, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33];
    
    function initPins() {
      const container = document.getElementById('pins-container');
      pins.forEach(pin => {
        container.innerHTML += `
          <div class="card">
            <div class="pin-label">GPIO ${pin}</div>
            <button id="btn-${pin}" class="btn" onclick="togglePin(${pin})">OFF</button>
          </div>
        `;
      });
      updateStates();
      setInterval(updateStates, 3000); // Polling status setiap 3 detik
    }

    function togglePin(pin) {
      fetch(`/toggle?pin=${pin}`)
        .then(response => response.text())
        .then(state => { updateButton(pin, state); });
    }

    function updateStates() {
      fetch('/states')
        .then(response => response.json())
        .then(data => {
          for (let pin in data) { updateButton(pin, data[pin]); }
        });
    }

    function updateButton(pin, state) {
      const btn = document.getElementById(`btn-${pin}`);
      if(btn) {
        if(state == "1") {
          btn.classList.add("on");
          btn.innerHTML = "ON";
        } else {
          btn.classList.remove("on");
          btn.innerHTML = "OFF";
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
    digitalWrite(outputPins[i], LOW); // Set awal semua pin mati
  }

  // Set IP Statis
  if (!WiFi.config(local_IP, gateway, subnet)) {
    Serial.println("Gagal mengatur IP Statis");
  }

  // Mulai Koneksi WiFi
  WiFi.begin(ssid, password);
  Serial.print("Menghubungkan ke ");
  Serial.print(ssid);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nTerhubung!");
  Serial.print("Buka browser dan akses IP: ");
  Serial.println(WiFi.localIP());

  // Mendaftarkan header yang akan dibaca server (untuk cookie login)
  const char * headerkeys[] = {"Cookie"};
  size_t headerkeyssize = sizeof(headerkeys) / sizeof(char*);
  server.collectHeaders(headerkeys, headerkeyssize);

  // Routing Web Server
  server.on("/", HTTP_GET, []() {
    if (!is_authenticated()) {
      server.sendHeader("Location", "/login");
      server.send(301);
      return;
    }
    server.send(200, "text/html", dashboard_html);
  });

  server.on("/login", HTTP_GET, []() {
    server.send(200, "text/html", login_html);
  });

  server.on("/login", HTTP_POST, []() {
    if (server.hasArg("username") && server.hasArg("password")) {
      if (server.arg("username") == "admin" && server.arg("password") == "admin") {
        server.sendHeader("Location", "/");
        server.sendHeader("Cache-Control", "no-cache");
        server.sendHeader("Set-Cookie", "ESPSESSIONID=1; Path=/; Max-Age=86400"); // Login valid 1 hari
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

  // API Endpoint untuk Toggle Pin
  server.on("/toggle", HTTP_GET, []() {
    if (!is_authenticated()) { server.send(401, "text/plain", "Unauthorized"); return; }
    
    if (server.hasArg("pin")) {
      int pin = server.arg("pin").toInt();
      // Validasi agar hanya pin yang diizinkan yang bisa dikontrol
      bool validPin = false;
      for (int i = 0; i < numPins; i++) {
        if (outputPins[i] == pin) { validPin = true; break; }
      }
      
      if (validPin) {
        int currentState = digitalRead(pin);
        digitalWrite(pin, !currentState); // Balikkan status pin
        server.send(200, "text/plain", String(!currentState));
        return;
      }
    }
    server.send(400, "text/plain", "Invalid Pin");
  });

  // API Endpoint untuk sinkronisasi status pin
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

  // Mulai Server
  server.begin();
}

void loop() {
  server.handleClient();
}