#ifndef JSON_HELPER_H
#define JSON_HELPER_H

#include <ArduinoJson.h>

#include "ringbuffer.h"

template <size_t BufferSize>
class JsonHelper {
 public:
  JsonHelper(size_t maxEntries) : maxEntries(maxEntries) {}

  // Converts an Array of Measurement-Objecs into a JSON-String.
  void toJson(const Measurement* measurementsBuffer, int count, String& jsonPayload) {
    doc.clear();  // Clear the JSON document

    JsonArray measurements = doc.createNestedArray("measurements");

    // Warning if less measurements are available than expected
    if (count < maxEntries) {
      Serial.println("Warnung: Weniger Messwerte vorhanden als erwartet!");
    }

    int limit = (count < maxEntries) ? count : maxEntries;
    for (int i = 0; i < limit; i++) {
      JsonObject obj = measurements.createNestedObject();
      obj["timestamp"] = measurementsBuffer[i].timestamp;
      obj["value"] = measurementsBuffer[i].value;
    }

    serializeJson(doc, jsonPayload);
  }

  void toJson(const Measurement& measurement, String& jsonPayload) {
    doc.clear();  // Clear the JSON document

    JsonObject obj = doc.createNestedObject();
    obj["timestamp"] = measurement.timestamp;
    obj["value"] = measurement.value;

    serializeJson(doc, jsonPayload);
  }

 private:
  StaticJsonDocument<BufferSize> doc;
  size_t maxEntries;
};

#endif  // JSON_HELPER_H
