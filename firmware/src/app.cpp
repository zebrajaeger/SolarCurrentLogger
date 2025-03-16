#include <Arduino.h>
#include <WiFi.h>
#include <esp_heap_caps.h>

#include "config.h"
#include "http_sender.h"
#include "json_helper.h"
#include "ntp.h"
#include "ota.h"
#include "ringbuffer.h"
#include "sensor.h"

// Compile-time checks for the configuration literals
static_assert(sizeof(WIFI_SSID) > 1, "WIFI_SSID must not be empty!");
static_assert(sizeof(WIFI_PASSWORD) > 1, "WIFI_PASSWORD must not be empty!");
static_assert(sizeof(SERVER_URL) > 1, "SERVER_URL must not be empty!");
static_assert(sizeof(BUFFER_SIZE) > 1, "BUFFER_SIZE must not be empty!");
static_assert(sizeof(CHUNK_SIZE) > 1, "CHUNK_SIZE must not be empty!");

INA219Sensor sensorINA219;
Sensor &sensor = sensorINA219;

unsigned long lastMeasureTime = 0;

// Send buffer
Measurement sendBuffer[CHUNK_SIZE];
int sendBufferSize = 0;

// Sending interval: a sending attempt is made every 5 seconds
unsigned long lastSendTime = 0;

// Interval for outputting free memory (here 60000 ms, configurable)
unsigned long lastMemoryPrintTime = 0;

// OTA
OTAHandler ota;

// NTP
NTPHandler ntp;

// RingBuffer
RingBuffer ringBuffer(BUFFER_SIZE);

HttpSender http;

void sendChunk() {
  if (http.isSending()) {
    Serial.println("Already sending. Skip.");
    return;
  }

  int sendCount = ringBuffer.getChunk(sendBuffer, CHUNK_SIZE);
  if (sendCount == 0) {
    Serial.println("No data available for sending.");
    return;
  }

  String jsonPayload;
  toJson(sendBuffer, sendCount, &jsonPayload);
  http.sendRequest(jsonPayload);

  sendBufferSize = sendCount;

  Serial.print("Asynchronous sending process started for ");
  Serial.print(sendCount);
  Serial.println(" records.");
}

int64_t getCurrentEpochUnixTimestamp() {
  timeval tv;
  gettimeofday(&tv, nullptr);
  return (int64_t)tv.tv_sec * 1000LL + (int64_t)tv.tv_usec / 1000LL;
}

void setup() {
  Serial.begin(115200);
  Serial.println("SolarCurrentLogger starting...");

  // Initialize sensor
  sensor.setup();

  // Establish WiFi connection
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");

  // OTA
  ota.setup("SolarCurrentLogger");

  // Synchronize NTP time
  ntp.setup();

  // HTTP
  http.setServerUrl(SERVER_URL);

#ifdef API_TOKEN
  http.setApiToken(API_TOKEN);
#endif

#ifdef BASIC_AUTH_USERNAME
  http.setBasicAuth(BASIC_AUTH_USERNAME, BASIC_AUTH_PASSWORD);
#endif

  http.setFailureCallback([](int httpCode, const String &response) {
    Serial.printf("HTTP request failed: %d %s\nSending failed, data remains in the ring buffer.\n", httpCode,
                  response.c_str());
  });

  http.setSuccessCallback([](int httpCode, const String &response) {
    Serial.printf("HTTP request successful: %d %s\n", httpCode, response.c_str());
    int removed = ringBuffer.removeChunk(sendBuffer, sendBufferSize);
    Serial.print("Successfully removed transmitted records: ");
    Serial.println(removed);
  });

  lastMeasureTime = millis();
  lastSendTime = millis();
  lastMemoryPrintTime = millis();
}

void loop() {
  // OTA
  ota.loop();

  // Measurement
  if (millis() - lastMeasureTime >= MEASURE_INTERVAL) {
    lastMeasureTime = millis();

    Measurement m = {.value = sensor.getCurrentInMa(), .timestamp = getCurrentEpochUnixTimestamp()};
    ringBuffer.addMeasurement(m);

    Serial.printf("Measurement: %.2f mA, Time: %lld Used slots: %d/%d\n", m.value, m.timestamp, ringBuffer.getCount(),
                  BUFFER_SIZE);
  }

  // Send data
  if (millis() - lastSendTime >= SEND_INTERVAL) {
    lastSendTime = millis();
    sendChunk();
  }

  // Output free heap memory
  if (millis() - lastMemoryPrintTime >= MEMORY_PRINT_INTERVAL) {
    lastMemoryPrintTime = millis();
    size_t freeHeap = esp_get_free_heap_size();
    Serial.print("Free heap: ");
    Serial.println(freeHeap);
  }
}