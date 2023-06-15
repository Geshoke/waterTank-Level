#include <Arduino.h>

// put function declarations here:

#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid = "chebio";
const char* password = "123";
const char* apiUrl = "https://gibroenterprises.com/level_reading";

const int trigPin = 12;
const int echoPin = 11;
const int maxDistance = 200;
const int numReadings = 20;



void connectToWiFi();
int getDistance();
int findMode(int, int);
void uploadReading(int);


void setup() {
  Serial.begin(115200);
  delay(1000);

  connectToWiFi();
}

void loop() {
  int readings[numReadings];

  for (int i = 0; i < numReadings; i++) {
    delay(50);
    int distance = getDistance();
    readings[i] = distance;
  }

  int mode = findMode(readings, numReadings);

  uploadReading(mode);

  delay(10000); // Delay before taking the next set of readings
}


void connectToWiFi() {
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("Connected to WiFi");
}

int getDistance() {
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  unsigned long duration = pulseIn(echoPin, HIGH);
  int distance = duration * 0.034 / 2;

  return distance;
}

int findMode(int values[], int size) {
  int mode = 0;
  int maxCount = 0;

  for (int i = 0; i < size; i++) {
    int value = values[i];
    int count = 0;

    for (int j = 0; j < size; j++) {
      if (values[j] == value) {
        count++;
      }
    }

    if (count > maxCount) {
      maxCount = count;
      mode = value;
    }
  }

  return mode;
}

void uploadReading(int reading) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    String url = String(apiUrl) + "?reading=" + String(reading);

    http.begin(url);
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("HTTP Response: " + response);
    } else {
      Serial.println("Error occurred during HTTP request");
    }

    http.end();
  } else {
    Serial.println("Not connected to WiFi");
  }
}
