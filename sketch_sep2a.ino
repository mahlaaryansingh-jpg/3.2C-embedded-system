
#include <WiFiNINA.h>
#include <ArduinoMqttClient.h>

// =====================================
// WIFI SETTINGS
// =====================================

char ssid[] = "Kakarot";
char password[] = "kakarot123";

// =====================================
// MQTT SETTINGS
// =====================================

const char broker[] = "broker.emqx.io";
const int mqttPort = 1883;

const char waveTopic[] = "ES/Wave";
const char patTopic[] = "ES/Pat";

const char studentName[] = "Aryan Mahla";

const char clientID[] = "Aryan_SIT210_MQTT_321";

// =====================================
// HARDWARE PINS
// =====================================

const int trigPin = 11;
const int echoPin = 10;

const int redLED = 5;
const int greenLED = 6;

// =====================================
// MQTT OBJECTS
// =====================================

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

// =====================================
// GESTURE SETTINGS
// =====================================

const float minDistance = 5.0;
const float maxDistance = 30.0;
const float resetDistance = 35.0;

const unsigned long waveMaxTime = 1000;
const unsigned long patMinTime = 1200;

unsigned long gestureStart = 0;
unsigned long lastSensorRead = 0;

bool handDetected = false;
bool patPublished = false;

// =====================================
// ULTRASONIC SENSOR
// =====================================

float readDistance() {

  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);

  digitalWrite(trigPin, LOW);

  unsigned long duration =
    pulseIn(echoPin, HIGH, 30000);

  if (duration == 0) {
    return 999;
  }

  float distance = duration * 0.0343 / 2.0;

  return distance;
}

// =====================================
// WIFI CONNECTION
// =====================================

void connectWiFi() {

  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {

    WiFi.begin(ssid, password);

    unsigned long start = millis();

    while (WiFi.status() != WL_CONNECTED &&
           millis() - start < 15000) {

      Serial.print(".");
      delay(500);
    }
  }

  Serial.println();
  Serial.println("WiFi Connected!");

  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

// =====================================
// MQTT MESSAGE RECEIVER
// =====================================

void onMqttMessage(int messageSize) {

  (void)messageSize;

  String topic = mqttClient.messageTopic();
  String message = "";

  while (mqttClient.available()) {
    message += (char)mqttClient.read();
  }

  Serial.println();
  Serial.println("MQTT MESSAGE RECEIVED");

  Serial.print("Topic: ");
  Serial.println(topic);

  Serial.print("Message: ");
  Serial.println(message);

  if (topic == waveTopic) {

    digitalWrite(redLED, HIGH);
    digitalWrite(greenLED, HIGH);

    Serial.println("BATHROOM LIGHT ON");
    Serial.println("HALLWAY LIGHT ON");
  }

  else if (topic == patTopic) {

    digitalWrite(redLED, LOW);
    digitalWrite(greenLED, LOW);

    Serial.println("BATHROOM LIGHT OFF");
    Serial.println("HALLWAY LIGHT OFF");
  }
}

// =====================================
// MQTT CONNECTION
// =====================================

void connectMQTT() {

  while (!mqttClient.connected()) {

    Serial.println("Connecting to EMQX...");

    if (mqttClient.connect(broker, mqttPort)) {

      Serial.println("MQTT Connected!");

      mqttClient.subscribe(waveTopic);
      mqttClient.subscribe(patTopic);

      Serial.println("Subscribed: ES/Wave");
      Serial.println("Subscribed: ES/Pat");
    }

    else {

      Serial.print("MQTT Error: ");
      Serial.println(mqttClient.connectError());

      delay(3000);
    }
  }
}

// =====================================
// PUBLISH MESSAGE
// =====================================

void publishGesture(const char* topic) {

  if (!mqttClient.connected()) {
    connectMQTT();
  }

  mqttClient.beginMessage(topic);
  mqttClient.print(studentName);
  mqttClient.endMessage();

  Serial.print("Published to: ");
  Serial.println(topic);
}

// =====================================
// SETUP
// =====================================

void setup() {

  Serial.begin(115200);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  pinMode(redLED, OUTPUT);
  pinMode(greenLED, OUTPUT);

  digitalWrite(trigPin, LOW);

  digitalWrite(redLED, LOW);
  digitalWrite(greenLED, LOW);

  Serial.println();
  Serial.println("================================");
  Serial.println("SIT210 TASK 3.2C MQTT");
  Serial.println("SMART LIGHTING SYSTEM");
  Serial.println("================================");

  mqttClient.setId(clientID);
  mqttClient.onMessage(onMqttMessage);

  connectWiFi();
  connectMQTT();

  Serial.println("SYSTEM READY!");
}

// =====================================
// MAIN LOOP
// =====================================

void loop() {

  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  if (!mqttClient.connected()) {
    connectMQTT();
  }

  mqttClient.poll();

  if (millis() - lastSensorRead >= 60) {

    lastSensorRead = millis();

    float distance = readDistance();

    Serial.print("Distance: ");

    if (distance == 999) {
      Serial.println("No echo");
    }
    else {
      Serial.print(distance);
      Serial.println(" cm");
    }

    // HAND ENTERS DETECTION ZONE

    if (!handDetected &&
        distance >= minDistance &&
        distance <= maxDistance) {

      handDetected = true;
      patPublished = false;

      gestureStart = millis();

      Serial.println("HAND DETECTED");
    }

    // HAND HELD IN DETECTION ZONE

    if (handDetected &&
        !patPublished &&
        millis() - gestureStart >= patMinTime) {

      Serial.println("PAT DETECTED");

      publishGesture(patTopic);

      patPublished = true;
    }

    // HAND LEAVES DETECTION ZONE

    if (handDetected &&
        distance > resetDistance) {

      unsigned long duration =
        millis() - gestureStart;

      if (!patPublished &&
          duration <= waveMaxTime) {

        Serial.println("WAVE DETECTED");

        publishGesture(waveTopic);
      }

      else if (!patPublished) {

        Serial.println("GESTURE NOT RECOGNISED");
      }

      else {

        Serial.println("PAT COMPLETED");
      }

      handDetected = false;
      patPublished = false;
    }
  }
}
