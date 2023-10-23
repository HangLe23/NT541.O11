#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

const char *configFilename   = "/config.json";
unsigned long previousMillis = 0;


std::vector<String> WifiList;
AsyncWebServer server(80);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ESP Web Server</title>
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    h2 {font-size: 2.3rem;}
    p {font-size: 1.9rem;}
    pr {font-size: 1.9rem;}
    body {max-width: 400px; margin:0px auto; padding-bottom: 25px;}
    .slider { -webkit-appearance: none; margin: 14px; width: 360px; height: 25px; background: #FFD65C;
      outline: none; -webkit-transition: .2s; transition: opacity .2s;}
    .slider::-webkit-slider-thumb {-webkit-appearance: none; appearance: none; width: 35px; height: 35px; background: #003249; cursor: pointer;}
    .slider::-moz-range-thumb { width: 35px; height: 35px; background: #003249; cursor: pointer; } 
  </style>
</head>
<body>
  <h2>Sign in to network</h2>
  <p id="wifiList"></p>
  <p><input class="slider" type="text" id="ssid"></p>
  <p><input type="text" id="password" class="slider"></p>
  <input type="button" value="Save" onclick="saveWifiCredentials()">
  <input type="button" value="Scan" onclick="scanWifi()">
<script>
  function saveWifiCredentials() {
        var ssid = document.getElementById("ssid").value;
        var password = document.getElementById("password").value;
        var xhr = new XMLHttpRequest();
        xhr.open("POST", "/save-credentials?ssid="+ssid+"&password="+password, true);
        xhr.send();  
    }
  function scanWifi(){
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/scan-wifi", true);
  xhr.onload = function() {
    if (xhr.status == 200) {
      var wifiListHTML = xhr.responseText;
      var wifiListElement = document.getElementById("wifiList");
      wifiListElement.innerHTML = wifiListHTML;
    }
  };
  xhr.send();
}
</script>
</body>
</html>
)rawliteral";

int numNetworks; 

std::vector<String> ScanWifi() {
  std::vector<String> wifiList;
  int numNetworks = WiFi.scanNetworks();
  if (numNetworks > 0) {
    for (int i = 0; i < numNetworks; ++i) {
      wifiList.push_back(WiFi.SSID(i));
      delay(10);
    }
  }
  return wifiList;
}

const char* ssid2;
const char* password2;
String ssids = "";
String passwords = "";

void connectWifi(String ssid, String password) {
  int cnt = 0;
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    ++cnt;
    delay(500);
    Serial.print(".");
    if(cnt > 10) break;
  }
  
  if (cnt > 10) Serial.println("Không thể kết nối");
  else {
    Serial.println("Connecting...");
    Serial.print("WiFi Connected. IP Address:");
    Serial.println(WiFi.localIP());
  }
}

void setup(){
  Serial.begin(115200);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    WiFi.mode(WIFI_AP);
    WiFi.softAP("gbAP", "12345678");
    IPAddress myIP = WiFi.softAPIP();
    Serial.print("Đã thiết lập chế độ AP với tên mạng: ");
    Serial.println("gbAP");
    Serial.println(myIP);

    WifiList = ScanWifi();
    
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", index_html);
    });

    server.on("/save-credentials", HTTP_POST, [](AsyncWebServerRequest *request){
    if(request->hasParam("ssid") && request->hasParam("password")){
      ssids = request->getParam("ssid")->value();
      passwords = request->getParam("password")->value();
      ssid2 = ssids.c_str();
      password2 = passwords.c_str();
    } else 
      Serial.println("error");
    });

    server.on("/scan-wifi", HTTP_GET, [](AsyncWebServerRequest *request){
    String wifiListHTML = "<html><body><b>WiFi Networks:</b><br>";
    for (const String& ssid : WifiList) {
      wifiListHTML += ssid + "<br>";
      delay(10);
    }
    wifiListHTML += "</body></html>";
    request->send(200, "text/html", wifiListHTML);
    });
  
    if (!ssids.isEmpty()){
      WiFi.mode(WIFI_AP);
      connectWifi(ssids, passwords);
      WiFi.begin(ssid2, password2);
      delay(10000);
      if (WiFi.status() == WL_CONNECTED) {
        Serial.println(WiFi.localIP());
        break;
      } 
    }

    server.begin();
    Serial.println("HTTP server started");
  };
}
  
void loop() {
}