#ifndef JSON_HELPER_H
#define JSON_HELPER_H

#include <ArduinoJson.h>
#include "ringbuffer.h"

// Generates a JSON payload from an array of Measurement objects.
// The generated JSON is stored in the provided jsonPayload string.
void toJson(const Measurement* buffer, int count, String* jsonPayload) {
    // Allocate a dynamic JSON document (adjust size as needed)
    DynamicJsonDocument doc(15000);
    // Create a JSON array for measurements.
    JsonArray measurementsArray = doc.createNestedArray("measurements");
    // Loop through each measurement and add it to the array.
    for (int i = 0; i < count; i++) {
        JsonObject obj = measurementsArray.createNestedObject();
        obj["timestamp"] = buffer[i].timestamp;
        obj["value"] = buffer[i].value;
    }
    // Serialize the JSON document into the jsonPayload string.
    serializeJson(doc, *jsonPayload);
}

#endif // JSON_HELPER_H
