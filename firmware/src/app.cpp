#include <Arduino.h>
#include <ArduinoJson.h>
#include <AsyncHTTPRequest_Generic.h>   // For HTTP
#include <AsyncHTTPSRequest_Generic.h>  // For HTTPS
#include <AsyncTCP_SSL.h>               // For asynchronous TCP/SSL functionality
#include <Base64.h>
#include <WiFi.h>
#include <esp_heap_caps.h>
#include <esp_sntp.h>
#include <time.h>

#include "INA219.h"
#include "config.h"
// #include "esp_timer.h"

// Compile-time checks for the configuration literals
static_assert(sizeof(WIFI_SSID) > 1, "WIFI_SSID must not be empty!");
static_assert(sizeof(WIFI_PASSWORD) > 1, "WIFI_PASSWORD must not be empty!");
static_assert(sizeof(SERVER_URL) > 1, "SERVER_URL must not be empty!");
static_assert(sizeof(BUFFER_SIZE) > 1, "BUFFER_SIZE must not be empty!");
static_assert(sizeof(CHUNK_SIZE) > 1, "CHUNK_SIZE must not be empty!");

// INA219 instance
INA219 INA(0x40);

// Definition of the measurement structure
struct Measurement {
  float value;
  int64_t timestamp;
};

unsigned long lastMeasureTime = 0;

// // Ring buffer configuration
Measurement ringBuffer[BUFFER_SIZE];
int headIndex = 0;          // Points to the oldest entry
int countMeasurements = 0;  // Number of measurements currently stored in the buffer

Measurement tempBuffer[CHUNK_SIZE];

// FreeRTOS mutex to protect the ring buffer
SemaphoreHandle_t ringBufferMutex;

// Variables for controlling asynchronous sending
bool sendInProgress = false;
int lastSentChunkSize = 0;

// Sending interval: a sending attempt is made every 5 seconds
unsigned long lastSendTime = 0;

// Interval for outputting free memory (here 60000 ms, configurable)
unsigned long lastMemoryPrintTime = 0;

// Global instances of the request objects
AsyncHTTPRequest httpRequest;    // For HTTP
AsyncHTTPSRequest httpsRequest;  // For HTTPS – standard constructor, no parameters

void requestCompleteHTTP(void *optParm, AsyncHTTPRequest *request, int readyState) {
  Serial.print("requestCompleteHTTP(redystate:");
  Serial.print(readyState);
  Serial.println(")");

if (readyState == readyStateDone) {
    int httpCode = request->responseHTTPcode();
    Serial.print("HTTP request completed. Code: ");
    Serial.println(httpCode);
    // Print the response as a readable string
    Serial.print("HTTP response: ");
    Serial.println(request->responseHTTPString());

    if (httpCode >= 200 && httpCode < 300) {
      // Remove the sent data from the ring buffer
      xSemaphoreTake(ringBufferMutex, portMAX_DELAY);
      int removed = 0;
      for (int i = 0; i < lastSentChunkSize && i < countMeasurements; i++) {
        int idx = (headIndex + i) % BUFFER_SIZE;
        if (ringBuffer[idx].timestamp == tempBuffer[i].timestamp &&
            fabs(ringBuffer[idx].value - tempBuffer[i].value) < 0.001) {
          removed++;
        } else {
          break;
        }
      }
      headIndex = (headIndex + removed) % BUFFER_SIZE;
      countMeasurements -= removed;
      xSemaphoreGive(ringBufferMutex);
      Serial.print("Successfully removed transmitted records: ");
      Serial.println(removed);
    } else {
      Serial.println("Sending failed, data remains in the ring buffer.");
    }
    sendInProgress = false;
  }
}

void requestCompleteHTTPS(void *optParm, AsyncHTTPSRequest *request, int readyState) {
  if (readyState == readyStateDone) {
    int httpCode = request->responseHTTPcode();
    Serial.print("HTTPS request completed. Code: ");
    Serial.println(httpCode);
    // Print the response as a readable string
    Serial.print("HTTPS response: ");
    Serial.println(request->responseHTTPString());

    if (httpCode >= 200 && httpCode < 300) {
      xSemaphoreTake(ringBufferMutex, portMAX_DELAY);
      int removed = 0;
      for (int i = 0; i < lastSentChunkSize && i < countMeasurements; i++) {
        int idx = (headIndex + i) % BUFFER_SIZE;
        if (ringBuffer[idx].timestamp == tempBuffer[i].timestamp &&
            fabs(ringBuffer[idx].value - tempBuffer[i].value) < 0.001) {
          removed++;
        } else {
          break;
        }
      }
      headIndex = (headIndex + removed) % BUFFER_SIZE;
      countMeasurements -= removed;
      xSemaphoreGive(ringBufferMutex);
      Serial.print("Successfully removed transmitted records: ");
      Serial.println(removed);
    } else {
      Serial.println("Sending failed, data remains in the ring buffer.");
    }
    sendInProgress = false;
  }
}

void sendAsyncRequest(const String &jsonPayload) {
  bool useHttps = (strncmp(SERVER_URL, "https", 5) == 0);

  if (useHttps) {
    httpsRequest.onReadyStateChange(requestCompleteHTTPS);  // Register callback
    if (httpsRequest.readyState() == readyStateUnsent || httpsRequest.readyState() == readyStateDone) {
      bool openResult = httpsRequest.open("POST", SERVER_URL);
      if (openResult) {
        httpsRequest.setReqHeader("Content-Type", "application/json");
#ifdef API_TOKEN
        httpsRequest.setReqHeader("X-API-Token", API_TOKEN);
#endif
#if defined(BASIC_AUTH_USERNAME) && defined(BASIC_AUTH_PASSWORD)
        String credentials = String(BASIC_AUTH_USERNAME) + ":" + String(BASIC_AUTH_PASSWORD);
        String base64Credentials = base64::encode(credentials);
        httpsRequest.setReqHeader("Authorization", ("Basic " + base64Credentials).c_str());
#endif
        httpsRequest.send(jsonPayload);
      } else {
        Serial.println(F("Can't open HTTPS request"));
      }
    } else {
      Serial.println(F("HTTPS request not ready"));
    }
  } else {
    httpRequest.onReadyStateChange(requestCompleteHTTP);  // Register callback
    if (httpRequest.readyState() == readyStateUnsent || httpRequest.readyState() == readyStateDone) {
      bool openResult = httpRequest.open("POST", SERVER_URL);
      if (openResult) {
        httpRequest.setReqHeader("Content-Type", "application/json");
#ifdef API_TOKEN
        httpRequest.setReqHeader("X-API-Token", API_TOKEN);
#endif
#if defined(BASIC_AUTH_USERNAME) && defined(BASIC_AUTH_PASSWORD)
        String credentials = String(BASIC_AUTH_USERNAME) + ":" + String(BASIC_AUTH_PASSWORD);
        String base64Credentials = base64::encode(credentials);
        httpRequest.setReqHeader("Authorization", ("Basic " + base64Credentials).c_str());
#endif
        httpRequest.send(jsonPayload);
      } else {
        Serial.println(F("Can't open HTTP request"));
      }
    } else {
      Serial.println(F("HTTP request not ready"));
    }
  }
}

void sendBuffer() {
  if (sendInProgress) {
    Serial.println("Already sending. Skip.");
    return;
  }

  // Copy the oldest measurements into the temporary buffer
  xSemaphoreTake(ringBufferMutex, portMAX_DELAY);
  int sendCount = (countMeasurements < CHUNK_SIZE) ? countMeasurements : CHUNK_SIZE;
  if (sendCount == 0) {
    xSemaphoreGive(ringBufferMutex);
    Serial.println("No data available for sending.");
    return;
  }
  int localHead = headIndex;
  for (int i = 0; i < sendCount; i++) {
    int idx = (localHead + i) % BUFFER_SIZE;
    tempBuffer[i] = ringBuffer[idx];
  }
  xSemaphoreGive(ringBufferMutex);

  // Create JSON document for the current chunk
  DynamicJsonDocument doc(15000);
  JsonArray measurementsArray = doc.createNestedArray("measurements");
  for (int i = 0; i < sendCount; i++) {
    JsonObject obj = measurementsArray.createNestedObject();
    obj["timestamp"] = tempBuffer[i].timestamp;
    obj["value"] = tempBuffer[i].value;
  }
  String jsonPayload;
  serializeJson(doc, jsonPayload);

  sendAsyncRequest(jsonPayload);

  sendInProgress = true;
  lastSentChunkSize = sendCount;

  Serial.print("Asynchronous sending process started for ");
  Serial.print(sendCount);
  Serial.println(" records.");
}

// void dumpTime() {
//   Serial.println("######### TIME DUMP ##########");
//   Serial.print("# sync  : ");
//   Serial.println(sntp_get_sync_interval());

//   Serial.print("# millis: ");
//   Serial.println(millis() / 1000.0);

//   // Serial.print("# ESP:    ");
//   // Serial.println(esp_timer_get_time()/1000000.0);

//   // Serial.print("# RTC:    ");
//   // time_t now = time(nullptr);
//   // Serial.println(now);
//   timeval tv;
//   gettimeofday(&tv, nullptr);
//   Serial.print("# rtc_s:  ");
//   Serial.println(tv.tv_sec);
//   Serial.print("# rtc_us: ");
//   Serial.println(tv.tv_usec / 1000000.0);

//   int64_t time_ms = (int64_t)tv.tv_sec * 1000LL + (int64_t)tv.tv_usec / 1000LL;
//   Serial.print("# rtc_ms: ");
//   Serial.println(time_ms);

//   Serial.println("######### TIME DUMP ##########");
// }

void timeSyncCallback(struct timeval *tv) {
  Serial.println("\n----Time Sync----- Time should have been verified and updated if needed");
  Serial.println(tv->tv_sec);
  Serial.println(ctime(&tv->tv_sec));
}

int64_t getCurrentEpochUnixTimestamp() {
  timeval tv;
  gettimeofday(&tv, nullptr);
  return (int64_t)tv.tv_sec * 1000LL + (int64_t)tv.tv_usec / 1000LL;
}

void setup() {
  Serial.begin(115200);
  Serial.println("Current measurement started with asynchronous sending, mutex, and memory output.");

  // Initialize INA219
  Wire.begin();
  if (!INA.begin()) {
    Serial.println("INA219 not found! Please check wiring.");
    while (1);
  }

  INA.reset();
  INA.setGain(1);
  INA.setModeShuntContinuous();
  INA.setShuntSamples(7);  // 128 samples

  delay(1000);

  // Establish WiFi connection
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");

  // Synchronize NTP time
  sntp_set_time_sync_notification_cb(timeSyncCallback);
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  Serial.println("Synchronizing NTP time...");
  struct tm timeinfo;
  int retry = 0;
  while (!getLocalTime(&timeinfo) && retry < 10) {
    delay(1000);
    Serial.print(".");
    retry++;
  }
  if (retry == 10) {
    Serial.println("\nError: No NTP time received.");
  } else {
    Serial.println("\nNTP time synchronized.");
  }

  lastMeasureTime = millis();
  lastSendTime = millis();
  lastMemoryPrintTime = millis();

  // Create the mutex
  ringBufferMutex = xSemaphoreCreateMutex();
  if (ringBufferMutex == NULL) {
    Serial.println("Error creating the mutex.");
    while (1);
  }
}

void loop() {
  if (millis() - lastMeasureTime >= MEASURE_INTERVAL) {
    lastMeasureTime = millis();
    // dumpTime();

    float current_mA = ((float)INA.getShuntValue()) / (float)10;

    int64_t now = getCurrentEpochUnixTimestamp();
    Serial.print("Measurement: ");
    Serial.print(current_mA);
    Serial.print(" mA, Time: ");
    Serial.print(now);

    Serial.print(" Used slots: ");
    Serial.print(countMeasurements);
    Serial.print("/");
    Serial.println(BUFFER_SIZE);

    // Write the new measurement into the ring buffer (mutex-protected)
    xSemaphoreTake(ringBufferMutex, portMAX_DELAY);
    if (countMeasurements < BUFFER_SIZE) {
      int tail = (headIndex + countMeasurements) % BUFFER_SIZE;
      ringBuffer[tail].value = current_mA;
      ringBuffer[tail].timestamp = now;
      countMeasurements++;
    } else {
      // Buffer full: Overwrite the oldest entry
      ringBuffer[headIndex].value = current_mA;
      ringBuffer[headIndex].timestamp = now;
      headIndex = (headIndex + 1) % BUFFER_SIZE;
    }
    xSemaphoreGive(ringBufferMutex);
  }

  // Attempt to send data
  if (millis() - lastSendTime >= SEND_INTERVAL) {
    lastSendTime = millis();
    sendBuffer();
  }

  // Output free heap memory (configurable interval)
  if (millis() - lastMemoryPrintTime >= MEMORY_PRINT_INTERVAL) {
    lastMemoryPrintTime = millis();
    size_t freeHeap = esp_get_free_heap_size();
    Serial.print("Free heap: ");
    Serial.println(freeHeap);
  }
}