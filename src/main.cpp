#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <WiFiClientSecure.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <NewPing.h>
#include <UniversalTelegramBot.h>
#include "secrets.h"

#define BOT_TOKEN botToken
#define CHAT_ID chatID

X509List cert(TELEGRAM_CERTIFICATE_ROOT);
WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);

const char *ssid = wifiSSID;
const char *password = wifiPassword;
// const char *ssid = "Chebio";
// const char *password = "ruj12345kp";
const char *serverUrl = "https://www.gibroenterprise.com/aggregateESP_Readings";

// SRO4 DEPENDENCIES
const int TRIG_PIN = D5;
const int ECHO_PIN = D6;
const int NUM_READINGS = 20;
const int pingDelay = 500;
const int scriptDelay = 1200000;

NewPing sonar(TRIG_PIN, ECHO_PIN);
int readings[NUM_READINGS]; // Array to store the readings

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 10800);

void setup()
{
    Serial.begin(115200);
    delay(1000);

    // Connect to WiFi
    WiFi.begin(ssid, password);
    secured_client.setTrustAnchors(&cert);
    // secured_client.set (TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi");

    configTime(0, 0, "pool.ntp.org"); // get UTC time via NTP
    time_t now = time(nullptr);
    while (now < 24 * 3600)
    {
        Serial.print(".");
        delay(100);
        now = time(nullptr);
    }

    // // Initialize NTP client
    // timeClient.begin();

    // // Wait for NTP synchronization
    // while (!timeClient.update())
    // {
    //     timeClient.forceUpdate();
    // }

    bot.sendMessage(CHAT_ID, "Tank started", "");
}

void networkPost(unsigned int distance)
{
    timeClient.update();
    Serial.println(timeClient.getFormattedTime());
    delay(100);

    // Create JSON payload
    DynamicJsonDocument jsonDoc(256);
    jsonDoc["reading"] = distance;
    jsonDoc["esp_time"] = timeClient.getEpochTime();
    jsonDoc["espid"] = WiFi.macAddress();
    String requestBody;
    serializeJson(jsonDoc, requestBody);

    // Send POST requestl
    WiFiClientSecure client;
    HTTPClient http;
    client.setInsecure(); // Ignore SSL certificate verification (only for testing purposes)
    http.begin(client, serverUrl);
    http.addHeader("Content-Type", "application/json");
    Serial.print("payload:");
    Serial.println(requestBody);
    int httpResponseCode = http.POST(requestBody);

    if (httpResponseCode > 0)
    {
        String response = http.getString();
        Serial.println("HTTP Response code: " + String(httpResponseCode));
        Serial.println("Response: " + response);
    }
    else
    {
        Serial.println("Error occurred during HTTP request.");
    }

    http.end();
}

void loop()
{
    // bot.sendMessage(CHAT_ID,"test","");
    // Take 20 readings
    for (int i = 0; i < NUM_READINGS; i++)
    {
        delay(pingDelay); // 500ms delay between each reading

        unsigned int distance_cm = sonar.ping_cm();
        readings[i] = distance_cm;

        Serial.print("DISTANCE:");
        Serial.println(distance_cm);
    }

    // Sort the readings in ascending order to find the median
    for (int i = 0; i < NUM_READINGS - 1; i++)
    {
        for (int j = i + 1; j < NUM_READINGS; j++)
        {
            if (readings[j] < readings[i])
            {
                // Swap values
                int temp = readings[i];
                readings[i] = readings[j];
                readings[j] = temp;
            }
        }
    }

    // Calculate the median
    unsigned int median_distance = readings[NUM_READINGS / 2];
    networkPost(median_distance);

    if (median_distance > 124)
    {
        // Serial.println("less");
        bot.sendMessage(CHAT_ID, "Tank is almost empty", "");
    }
    else if (median_distance < 43)
    {
        // Serial.println("full");

        bot.sendMessage(CHAT_ID, "Tank is almost full", "");
    }

    // Print the median distance in centimeters
    Serial.print("Median Distance: ");
    Serial.print(median_distance);
    Serial.println(" cm");

    delay(scriptDelay);
}
