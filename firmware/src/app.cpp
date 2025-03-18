#include <Arduino.h>
#include <WiFi.h>
#include <esp_heap_caps.h>

#include "config.h"
#include "esp_status.h"
#include "http_sender.h"
#include "json_helper.h"
#include "mqtt_handler.h"
#include "ntp.h"
#include "ota.h"
#include "ringbuffer.h"
#include "sensor.h"

SET_LOOP_TASK_STACK_SIZE(16 * 1024);

// Compile-time checks for the configuration literals
static_assert(sizeof(WIFI_RECONNECT_INTERVAL) > 0, "WIFI_RECONNECT_INTERVAL must not be empty!");
static_assert(sizeof(HOST_NAME) > 0, "HOST_NAME must not be empty!");
static_assert(sizeof(WIFI_SSID) > 0, "WIFI_SSID must not be empty!");
static_assert(sizeof(WIFI_PASSWORD) > 0, "WIFI_PASSWORD must not be empty!");

static_assert(sizeof(BUFFER_SIZE) > 0, "BUFFER_SIZE must not be empty!");
static_assert(sizeof(CHUNK_SIZE) > 0, "CHUNK_SIZE must not be empty!");
static_assert(sizeof(JSON_BUFFER_SIZE) > 0, "JSON_BUFFER_SIZE must not be empty!");

static_assert(sizeof(USE_HTTP_SENDER) > 0, "USE_HTTP_SENDER must not be empty!");
static_assert(sizeof(HTTP_SERVER_URL) > 0, "HTTP_SERVER_URL must not be empty!");

static_assert(sizeof(USE_MQTT_SENDER) > 0, "USE_MQTT_SENDER must not be empty!");
static_assert(sizeof(MQTT_SERVER_URL) > 0, "MQTT_SERVER_URL must not be empty!");

INA219Sensor sensorINA219;
Sensor &sensor = sensorINA219;

unsigned long lastMeasureTime = 0;
unsigned long lastSendTime = 0;

// Status
EspStatus espStatus;
unsigned long lastStatusTime = 0;

// OTA
OTAHandler ota;

// NTP
NTPHandler ntp;

// Sending stuff
Measurement sendBuffer[CHUNK_SIZE];
int sendBufferSize = 0;
RingBuffer ringBuffer(BUFFER_SIZE);

HttpSender http;
JsonHelper<JSON_BUFFER_SIZE> jsonHelper(CHUNK_SIZE);
MqttHandler mqttHandler;

unsigned long lastWifiReconnectAttempt = 0;

void wifiReconnect() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  unsigned long currentMillis = millis();
  if (currentMillis - lastWifiReconnectAttempt >= WIFI_RECONNECT_INTERVAL) {
    lastWifiReconnectAttempt = currentMillis;
    Serial.println("WiFi disconnected. Attempting reconnect...");

    WiFi.disconnect();

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  }
}

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
  jsonHelper.toJson(sendBuffer, sendCount, jsonPayload);
  http.sendRequest(jsonPayload);

  sendBufferSize = sendCount;

  Serial.print("Asynchronous sending process started for ");
  Serial.print(sendCount);
  Serial.println(" records.");
}

void sendStatus() {
  String status;
  espStatus.getStatus(status);
  Serial.println(status);
  if (USE_MQTT_SENDER) {
    mqttHandler.publishStatus(status);
  }
}

int64_t getCurrentEpochUnixTimestamp() {
  timeval tv;
  gettimeofday(&tv, nullptr);
  return (int64_t)tv.tv_sec * 1000LL + (int64_t)tv.tv_usec / 1000LL;
}

void setup() {
  Serial.begin(115200);
  Serial.println("SolarCurrentLogger starting...");

  Serial.println("Stacksize is: " + String(getArduinoLoopTaskStackSize()));
  Serial.println("CrystalFreq: " + String(ESP.getCpuFreqMHz()) + " MHz");
  Serial.println("CPUFreq: " + String(ESP.getCpuFreqMHz()) + " MHz");
  Serial.println("FreeHeap: " + String(esp_get_free_heap_size()) + " Bytes");
  Serial.println("MinFreeHeap: " + String(esp_get_minimum_free_heap_size()) + " Bytes");

  // Initialize sensor
  sensor.setup();

  // Establish WiFi connection
  WiFi.setHostname(HOST_NAME);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // OTA
  ota.setup(HOST_NAME);

  // Synchronize NTP time
  ntp.setup();

  // HTTP
  http.setServerUrl(HTTP_SERVER_URL);

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

  // MQTT
  mqttHandler.setup(MQTT_SERVER_URL, HOST_NAME);

  lastMeasureTime = millis();
  lastSendTime = millis();

  sendStatus();
  lastStatusTime = millis();
}

void loop() {
  wifiReconnect();

  // OTA
  ota.loop();

  // Measurement
  if (millis() - lastMeasureTime >= MEASURE_INTERVAL) {
    lastMeasureTime = lastMeasureTime + MEASURE_INTERVAL;

    Measurement m = {.value = sensor.getCurrentInMa(), .timestamp = getCurrentEpochUnixTimestamp()};
    if (USE_HTTP_SENDER) {
      ringBuffer.addMeasurement(m);
    }

    if (USE_MQTT_SENDER) {
      String jsonPayload;
      jsonHelper.toJson(m, jsonPayload);
      mqttHandler.publishMeasurement(jsonPayload);
    }
    Serial.printf("Measurement: %.2f mA, Time: %lld Used slots: %d/%d\n", m.value, m.timestamp, ringBuffer.getCount(),
                  BUFFER_SIZE);
  }

  if (USE_HTTP_SENDER) {
    // Send data
    if (millis() - lastSendTime >= SEND_INTERVAL) {
      lastSendTime = lastSendTime + SEND_INTERVAL;
      sendChunk();
    }
  }

  // Output status
  if (millis() - lastStatusTime >= STATUS_PRINT_INTERVAL) {
    lastStatusTime = lastStatusTime + STATUS_PRINT_INTERVAL;
    sendStatus();
  }
}