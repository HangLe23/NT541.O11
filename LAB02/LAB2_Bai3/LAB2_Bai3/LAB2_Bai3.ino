#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <LittleFS.h>

const char *ssid = "UiTiOt-E3.1";
const char *password = "UiTiOtAP";
const char *serverAddress = "http://172.31.11.115:8000";  // Địa chỉ của server HTTP
const char *configFilename = "/config.json";
const char *configFileAddress = "http://172.31.11.115:8000/config.json";

// Đối tượng quản lý kết nối WiFi
ESP8266WiFiMulti WiFiMulti;

// Dữ liệu cấu hình
struct Config {
  String name;
  int version;
};

Config savedConfig;

void setup_wifi() {
  // Kết nối đến mạng Wi-Fi
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
}

void createConfigFile() {
  // Tạo tệp cấu hình mẫu và lưu trữ trên hệ thống tệp nhúng ESP8266 (LittleFS)
  StaticJsonDocument<1024> config;

  config["name"] = "blink";
  config["version"] = 0;

  File configFile = LittleFS.open(configFilename, "w+");
  if (configFile) {
    serializeJson(config, configFile);
    configFile.close();
    Serial.println("config.json created.");
  } else {
    Serial.println("Failed to create config.json");
  }
}

void readFile(const char *filename) {
  // Đọc nội dung tệp và in ra Serial Monitor
  File file = LittleFS.open(filename, "r");
  if (!file) {
    Serial.println("Failed to open data file");
    return;
  }
  while (file.available()) {
    Serial.print((char)file.read());
  }
  Serial.println();
  file.close();
}

void saveConfiguration(const char *filename, const Config &config) {
  // Lưu thông tin cấu hình vào tệp JSON
  File file = LittleFS.open(filename, "w+");
  if (!file) {
    Serial.println("Failed to create file");
    return;
  }

  StaticJsonDocument<1024> doc;
  doc["name"] = config.name;
  doc["version"] = config.version;

  if (serializeJson(doc, file) == 0) {
    Serial.println("Failed to write to file");
  }
  file.close();
}

void loadConfiguration(const char *filename, Config &config) {
  // Đọc thông tin cấu hình từ tệp JSON
  File file = LittleFS.open(filename, "r");
  StaticJsonDocument<1024> doc;
  DeserializationError error = deserializeJson(doc, file);

  if (error) {
    Serial.println("Failed to read file, using default configuration");
  }

  config.name = doc["name"] | "0";
  config.version = doc["version"] | 0;
  file.close();
}

void getRemoteConfig() {
  // Lấy thông tin cấu hình từ máy chủ HTTP
  StaticJsonDocument<1024> config;
  WiFiClient clientToConfig;
  HTTPClient http;
  http.begin(clientToConfig, configFileAddress);
  int httpCode = http.GET();

  if (httpCode == 200) {
    String payload = http.getString();
    deserializeJson(config, payload);
    http.end();

    String path = config["path"];
    String name = config["name"];
    int version = config["version"];

    Serial.print("Path: ");
    Serial.println(path);
    Serial.print("Name: ");
    Serial.println(name);
    Serial.print("Version: ");
    Serial.println(version);

    if (name == savedConfig.name && version == savedConfig.version) {
      Serial.println("Configuration is the same");
    } else {
      Serial.println("Configuration is different");
      String url = String(serverAddress) + "/" + String(path);
     savedConfig.name = name;
     savedConfig.version = version;
     saveConfiguration(configFilename, savedConfig);
      updateFirmware(url, name, version);
    }
  } else {
    Serial.printf("HTTP request failed with error code: %d\n", httpCode);
  }
}

void updateFirmware(const String &path, String name, int version) {
  // Cập nhật firmware từ URL
  WiFiClient clientToBinFile;
  HTTPClient http;
  http.begin(clientToBinFile, path);
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    Serial.println("Start updating firmware...");
    ESPhttpUpdate.onStart(update_started);
    ESPhttpUpdate.onEnd(update_finished);
    ESPhttpUpdate.onProgress(update_progress);
    ESPhttpUpdate.onError(update_error);
    t_httpUpdate_return ret = ESPhttpUpdate.update(clientToBinFile, path);

    switch (ret) {
      case HTTP_UPDATE_FAILED:
        Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
        break;
      case HTTP_UPDATE_NO_UPDATES:
        Serial.println("HTTP_UPDATE_NO_UPDATES");
        break;
      case HTTP_UPDATE_OK:
        Serial.println("HTTP_UPDATE_OK");
        break;
    }
  }
  http.end();
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  pinMode(LED_BUILTIN, OUTPUT);
  LittleFS.begin();
  readFile(configFilename);
  loadConfiguration(configFilename, savedConfig);
  ESPhttpUpdate.setClientTimeout(8000);

  // Kiểm tra kết nối đến server
  WiFiClient client;
  HTTPClient http;
  http.begin(client, serverAddress);
  int httpCode = http.GET();

  if (httpCode > 0) {
    Serial.print("Server response: ");
    Serial.println(serverAddress);
  } else {
    Serial.printf("HTTP request failed with error code: %d\n", httpCode);
  }
  http.end();
}

void update_started() {
  Serial.println("CALLBACK: HTTP update process started");
}

void update_finished() {
  Serial.println("CALLBACK: HTTP update process finished");
}

void update_progress(int cur, int total) {
  Serial.printf("CALLBACK: HTTP update process at %d of %d bytes...\n", cur, total);
}

void update_error(int err) {
  Serial.printf("CALLBACK: HTTP update fatal error code %d\n", err);
}

void loop() {
  //Nếu là file trên server thì ghi code này  
  // Serial.println("This is the firmware update file on the server");
  // digitalWrite(LED_BUILTIN, HIGH);
  // delay(1000);
  // digitalWrite(LED_BUILTIN, LOW);
  // delay(1000);
  // getRemoteConfig();

//Nếu là file không trên server, build tại máy thì dùng code này
  Serial.println("This is not the firmware update file on the server");  
  getRemoteConfig();
}
