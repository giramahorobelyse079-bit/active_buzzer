#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// ==========================
// Wi-Fi credentials
// ==========================
const char* ssid = "bc";
const char* password = "clusters123";

// ==========================
// MQTT broker settings
// ==========================
const char* mqttServer = "broker.hivemq.com";   // or your broker IP/domain
const int mqttPort = 1883;
const char* mqttUser = "";                      // leave empty if not used
const char* mqttPassword = "";                  // leave empty if not used

// ==========================
// MQTT topics
// ==========================
const char* buzzerTopic = "nit/buzzer/007";

// ==========================
// Globals
// ==========================
WiFiClient espClient;
PubSubClient client(espClient);

const int buzzerPin = 5;
bool buzzerState = false;

// ==========================
// Connect to Wi-Fi
// ==========================
void connectWiFi() {
  Serial.print("Connecting to Wi-Fi");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("Wi-Fi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}


void playTone() {
  ledcWriteTone(buzzerPin, 1000);
  delay(300);
  ledcWriteTone(buzzerPin, 0);
  delay(50);
}


// ==========================
// MQTT message callback
// ==========================
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.println();
  Serial.print("Message received on topic: ");
  Serial.println(topic);

  // Convert payload bytes to a null-terminated char array
  char message[length + 1];
  for (unsigned int i = 0; i < length; i++) {
    message[i] = (char)payload[i];
  }
  message[length] = '\0';

  Serial.print("Raw payload: ");
  Serial.println(message);

  // Parse JSON
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, message);

  if (error) {
    Serial.print("JSON parse failed: ");
    Serial.println(error.c_str());
    return;
  }

  // Safely read fields
  const char* state = doc["state"] | "";

  Serial.print("received state: ");
  Serial.println(state);

  // Example actions
  if (strcmp(state, "on") == 0) {
    buzzerState = true;
    Serial.println("Buzzer turned ON");
  } 
  else if (strcmp(state, "off") == 0) {
    buzzerState = false;
    Serial.println("Buzzer turned OFF");
  }
}

// ==========================
// Connect/reconnect MQTT
// ==========================
void connectMQTT() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");

    String clientId = "ESP32Client-";
    clientId += String((uint32_t)ESP.getEfuseMac(), HEX);

    bool connected;
    if (strlen(mqttUser) > 0) {
      connected = client.connect(clientId.c_str(), mqttUser, mqttPassword);
    } else {
      connected = client.connect(clientId.c_str());
    }

    if (connected) {
      Serial.println("connected");
      client.subscribe(buzzerTopic);
      Serial.print("Subscribed to: ");
      Serial.println(buzzerTopic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" retrying in 3 seconds");
      delay(3000);
    }
  }
}


// ==========================
// Setup
// ==========================
void setup() {
  Serial.begin(115200);
  ledcAttach(buzzerPin, 2000, 8);

  connectWiFi();

  client.setServer(mqttServer, mqttPort);
  client.setCallback(mqttCallback);
}

// ==========================
// Main loop
// ==========================
void loop() {
  // Reconnect Wi-Fi if needed
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi lost. Reconnecting...");
    connectWiFi();
  }

  // Reconnect MQTT if needed
  if (!client.connected()) {
    connectMQTT();
  }

  // Must be called regularly for MQTT
  client.loop();


  if(buzzerState){
    playTone();
  }
}
