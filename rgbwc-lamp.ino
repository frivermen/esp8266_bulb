#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <ArduinoOTA.h>

// Пины светодиодов
#define RED_PIN 4
#define GREEN_PIN 12  
#define BLUE_PIN 14
#define COLD_PIN 5
#define WARM_PIN 13

// Настройки PWM
#define PWM_MAX 1023

// Структура для хранения настроек
struct Settings {
  char ssid[32] = "";
  char password[32] = "";
  uint16_t red = 0;
  uint16_t green = 0;
  uint16_t blue = 0;
  uint16_t cold = 0;
  uint16_t warm = 200;  // По умолчанию warm на 200
};

Settings settings;
ESP8266WebServer server(80);

// HTML страница (минималистичная)
// HTML страница (минималистичная)
const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Wi-Fi Lamp</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: Arial; margin: 20px; background: #f0f0f0; }
    .container { max-width: 400px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; }
    .slider-container { margin: 15px 0; }
    .slider { width: 100%; height: 25px; }
    .button-row { display: flex; gap: 10px; margin: 20px 0; }
    .btn { padding: 12px; border: none; border-radius: 5px; cursor: pointer; flex: 1; font-size: 16px; }
    .apply-btn { background: #4CAF50; color: white; }
    .setup-btn { background: #9E9E9E; color: white; }
    .setup-section { display: none; margin: 20px 0; padding: 15px; background: #e9e9e9; border-radius: 5px; }
    input[type="text"], input[type="password"] { width: 100%; padding: 8px; margin: 5px 0; box-sizing: border-box; }
    .ota-section { margin-top: 15px; }
  </style>
</head>
<body>
  <div class="container">
    <h2>Wi-Fi Lamp Control</h2>
    
    <div class="slider-container">
      <label>Red: <span id="redValue">0</span></label>
      <input type="range" min="0" max="1023" value="0" class="slider" id="redSlider">
    </div>
    
    <div class="slider-container">
      <label>Green: <span id="greenValue">0</span></label>
      <input type="range" min="0" max="1023" value="0" class="slider" id="greenSlider">
    </div>
    
    <div class="slider-container">
      <label>Blue: <span id="blueValue">0</span></label>
      <input type="range" min="0" max="1023" value="0" class="slider" id="blueSlider">
    </div>
    
    <div class="slider-container">
      <label>Cold White: <span id="coldValue">0</span></label>
      <input type="range" min="0" max="1023" value="0" class="slider" id="coldSlider">
    </div>
    
    <div class="slider-container">
      <label>Warm White: <span id="warmValue">200</span></label>
      <input type="range" min="0" max="1023" value="200" class="slider" id="warmSlider">
    </div>
    
    <div class="button-row">
      <button class="btn apply-btn" onclick="applySettings()">Apply</button>
      <button class="btn setup-btn" onclick="toggleSetup()">Setup</button>
    </div>
    
    <div class="setup-section" id="setupSection">
      <h3>Wi-Fi Settings</h3>
      <input type="text" id="ssid" placeholder="Wi-Fi Name (SSID)">
      <input type="password" id="password" placeholder="Wi-Fi Password">
      <button class="btn apply-btn" onclick="saveWiFi()" style="margin-top: 10px;">Save WiFi Settings</button>
      
      <div class="ota-section">
        <h3>OTA Update</h3>
        <form method="POST" action="/update" enctype="multipart/form-data">
          <input type="file" name="update" style="margin: 10px 0;">
          <button type="submit" class="btn apply-btn">Update Firmware</button>
        </form>
      </div>
    </div>
  </div>

  <script>
    const sliders = ['red','green','blue','cold','warm'];
    let setupVisible = false;
    
    // Загружаем текущие значения при старте
    window.onload = function() {
      fetch('/get').then(r => r.json()).then(data => {
        document.getElementById('redSlider').value = data.r;
        document.getElementById('greenSlider').value = data.g;
        document.getElementById('blueSlider').value = data.b;
        document.getElementById('coldSlider').value = data.c;
        document.getElementById('warmSlider').value = data.w;
        
        document.getElementById('redValue').textContent = data.r;
        document.getElementById('greenValue').textContent = data.g;
        document.getElementById('blueValue').textContent = data.b;
        document.getElementById('coldValue').textContent = data.c;
        document.getElementById('warmValue').textContent = data.w;
      });
    };
    
    // Обновляем значения при движении слайдеров (только отображение)
    sliders.forEach(color => {
      document.getElementById(color + 'Slider').addEventListener('input', function() {
        document.getElementById(color + 'Value').textContent = this.value;
      });
    });
    
    function applySettings() {
      const values = sliders.map(color => 
        document.getElementById(color + 'Slider').value
      );
      
      // Сначала устанавливаем значения
      fetch('/set?r=' + values[0] + '&g=' + values[1] + '&b=' + values[2] + 
            '&c=' + values[3] + '&w=' + values[4])
        .then(() => {
          // Затем сохраняем в EEPROM
          return fetch('/save');
        })
        .then(response => {
          if(response.ok) {
            // Визуальное подтверждение
            const btn = document.querySelector('.apply-btn');
            const originalText = btn.textContent;
            btn.textContent = 'Applied!';
            btn.style.background = '#2E7D32';
            setTimeout(() => {
              btn.textContent = originalText;
              btn.style.background = '#4CAF50';
            }, 1000);
          }
        });
    }
    
    
    function toggleSetup() {
      setupVisible = !setupVisible;
      const setupSection = document.getElementById('setupSection');
      const setupBtn = document.querySelector('.setup-btn');
      
      if(setupVisible) {
        setupSection.style.display = 'block';
        setupBtn.style.background = '#757575';
        setupBtn.textContent = 'Hide Setup';
      } else {
        setupSection.style.display = 'none';
        setupBtn.style.background = '#9E9E9E';
        setupBtn.textContent = 'Setup';
      }
    }
    
    function saveWiFi() {
      const ssid = document.getElementById('ssid').value;
      const password = document.getElementById('password').value;
      
      if(ssid) {
        fetch('/wifi?ssid=' + encodeURIComponent(ssid) + '&password=' + encodeURIComponent(password))
          .then(() => {
            alert('WiFi settings saved. Rebooting...');
          });
      } else {
        alert('Please enter WiFi name');
      }
    }
  </script>
</body>
</html>
)rawliteral";

void saveSettings() {
  EEPROM.begin(512);
  EEPROM.put(0, settings);
  EEPROM.commit();
  EEPROM.end();
}

void loadSettings() {
  EEPROM.begin(512);
  EEPROM.get(0, settings);
  EEPROM.end();
}

void setupPins() {
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  pinMode(COLD_PIN, OUTPUT);
  pinMode(WARM_PIN, OUTPUT);
  
  analogWriteRange(PWM_MAX);
}

void setLeds(uint16_t r, uint16_t g, uint16_t b, uint16_t c, uint16_t w) {
  analogWrite(RED_PIN, r);
  analogWrite(GREEN_PIN, g);
  analogWrite(BLUE_PIN, b);
  analogWrite(COLD_PIN, c);
  analogWrite(WARM_PIN, w);
  
  // Сохраняем в настройки
  settings.red = r;
  settings.green = g;
  settings.blue = b;
  settings.cold = c;
  settings.warm = w;
}

void handleRoot() {
  server.send(200, "text/html", htmlPage);
}

void handleSet() {
  uint16_t r = server.arg("r").toInt();
  uint16_t g = server.arg("g").toInt();
  uint16_t b = server.arg("b").toInt();
  uint16_t c = server.arg("c").toInt();
  uint16_t w = server.arg("w").toInt();
  
  setLeds(r, g, b, c, w);
  server.send(200, "text/plain", "OK");
}

void handleGet() {
  String json = "{";
  json += "\"r\":" + String(settings.red) + ",";
  json += "\"g\":" + String(settings.green) + ",";
  json += "\"b\":" + String(settings.blue) + ",";
  json += "\"c\":" + String(settings.cold) + ",";
  json += "\"w\":" + String(settings.warm);
  json += "}";
  server.send(200, "application/json", json);
}

void handleSave() {
  // Сохраняем текущие настройки в EEPROM
  saveSettings();
  server.send(200, "text/plain", "Settings saved");
}

void handleWiFi() {
  String newSSID = server.arg("ssid");
  String newPass = server.arg("password");
  
  if(newSSID.length() > 0) {
    newSSID.toCharArray(settings.ssid, sizeof(settings.ssid));
    newPass.toCharArray(settings.password, sizeof(settings.password));
    saveSettings();
    
    server.send(200, "text/plain", "WiFi settings saved. Rebooting...");
    delay(1000);
    ESP.restart();
  } else {
    server.send(400, "text/plain", "SSID cannot be empty");
  }
}

void setupOTA() {
  ArduinoOTA.setHostname("wifi-lamp");
  ArduinoOTA.onStart([]() {
    setLeds(50, 0, 0, 0, 0);
  });
  ArduinoOTA.onEnd([]() {
    setLeds(0, 200, 0, 0, 0);
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    static bool ledState = false;
    ledState = !ledState;
    if(ledState) {
      setLeds(50, 0, 0, 0, 0);
    } else {
      setLeds(0, 0, 0, 0, 0);
    }
  });
  ArduinoOTA.begin();
}

void setup() {  
  setupPins();
  loadSettings();
  if (settings.red + settings.green + settings.blue + settings.cold + settings.warm) {
    setLeds(settings.red, settings.green, settings.blue, settings.cold, settings.warm);
  } 
  else setLeds(0, 0, 0, 0, 100);
  
  // Проверяем настройки WiFi
  bool apMode = true;
  if(strlen(settings.ssid) > 0) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(settings.ssid, settings.password);
    // Ждем подключения 10 секунд
    for(int i = 0; i < 20 && WiFi.status() != WL_CONNECTED; i++) {
      delay(500);
    }
    if(WiFi.status() == WL_CONNECTED) {
      apMode = false;
    }
  }
  
  // Режим точки доступа
  if(apMode) {
    WiFi.mode(WIFI_AP);
    WiFi.softAP("WiFi-Lamp", "");
    // Синий индикатор в AP режиме
    setLeds(0, 0, 100, 0, 0);
  }
  
  // Настраиваем сервер
  server.on("/", handleRoot);
  server.on("/set", handleSet);
  server.on("/get", handleGet);
  server.on("/wifi", handleWiFi);
  server.on("/save", handleSave);
  server.onNotFound(handleRoot);
  
  // OTA update
  server.on("/update", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart();
  }, []() {
    HTTPUpload& upload = server.upload();
    if(upload.status == UPLOAD_FILE_START) {
      setLeds(50, 0, 0, 0, 0);
    } else if(upload.status == UPLOAD_FILE_END) {
      setLeds(0, 200, 0, 0, 0);
    }
  });
  
  setupOTA();
  server.begin();
}

void loop() {
  server.handleClient();
  ArduinoOTA.handle();
  delay(10);
}