#include <ESP8266WiFi.h>
#include <DHTesp.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

const char* ssid = "UiTiOt-E3.1";
const char* password = "UiTiOtAP";
const char* mqtt_server = "broker.mqtt-dashboard.com";

const int mqttPort = 1883;
char clientId[20];

unsigned long sendDataInterval = 10000;
const int dhtPin = 14;
unsigned long lastMillis = 0;

WiFiClient espClient;
PubSubClient client(espClient);
DHTesp dht;

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, mqttPort);
  client.setCallback(callback);

  dht.setup(dhtPin, DHTesp::DHT22);
  snprintf(clientId, 20, "ESP8266-%08X", ESP.getChipId());
  
  Serial.println(clientId);
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  String receivedMessage;
  for (int i = 0; i < length; i++) {
    receivedMessage += (char)payload[i];
  }
  Serial.println(receivedMessage);

  String espId = receivedMessage.substring(0, receivedMessage.indexOf("_"));
  Serial.println(espId);
  receivedMessage = receivedMessage.substring(receivedMessage.indexOf("_") + 1);
  
  Serial.println(receivedMessage);
  if (espId == String(clientId) || espId == "all") {
    processConfigRequest(receivedMessage);
  }
}

void processConfigRequest(String message) {
  unsigned long newInterval = atol(message.c_str());
  Serial.println(newInterval);
    
    if (newInterval > 0) {
      sendDataInterval = newInterval;
      Serial.println("Thời gian gửi dữ liệu tiếp theo đã được cấu hình thành " + String(sendDataInterval) + " milliseconds");
    } else {
      Serial.println("Thời gian cấu hình không hợp lệ");
    }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(clientId)) {
      Serial.println("connected");
      client.subscribe("nhom1/configTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long currentMillis = millis();

  if (currentMillis - lastMillis >= sendDataInterval) {
    sendData();
    lastMillis = currentMillis;
  }
}

void sendData() {
  delay(dht.getMinimumSamplingPeriod());

  float temperature = dht.getTemperature();
  float humidity = dht.getHumidity();

  if (!isnan(temperature) && !isnan(humidity)) {
    String temperatureTopic = "nhom1/" + String(clientId) + "/temperature";
    String humidityTopic = "nhom1/" + String(clientId) + "/humidity";
    String temperaturePayload = String(temperature);
    String humidityPayload = String(humidity);

    client.publish(temperatureTopic.c_str(), temperaturePayload.c_str());
    client.publish(humidityTopic.c_str(), humidityPayload.c_str());
    Serial.println("Temperature: " + temperaturePayload + " °C");
    Serial.println("Humidity: " + humidityPayload + " %");
  } else {
    Serial.println("Failed to read from DHT sensor. Retrying...");
  }
}
