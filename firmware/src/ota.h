#ifndef OTA_H
#define OTA_H

#include <ArduinoOTA.h>
#include <Arduino.h>

class OTAHandler {
public:
    // Initializes OTA functionality with the given hostname.
    void setup(const char* hostname) {
        // Set the OTA hostname.
        ArduinoOTA.setHostname(hostname);
        
        // Optionally, set an OTA password for authentication.
        // ArduinoOTA.setPassword("your_password");

        // Callback when OTA update starts.
        ArduinoOTA.onStart([]() {
            Serial.println("OTA Update starting.");
        });

        // Callback when OTA update ends.
        ArduinoOTA.onEnd([]() {
            Serial.println("OTA Update finished.");
        });

        // Callback to report OTA progress.
        ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
            Serial.printf("OTA Progress: %u%%\r", (progress * 100) / total);
        });

        // Callback for OTA errors.
        ArduinoOTA.onError([](ota_error_t error) {
            Serial.printf("OTA Error [%u]: ", error);
            if (error == OTA_AUTH_ERROR) Serial.println("Authentication failed");
            else if (error == OTA_BEGIN_ERROR) Serial.println("Begin failed");
            else if (error == OTA_CONNECT_ERROR) Serial.println("Connection failed");
            else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive failed");
            else if (error == OTA_END_ERROR) Serial.println("End failed");
        });

        // Start the OTA service.
        ArduinoOTA.begin();
        Serial.println("OTA is ready");
    }

    // Must be called in the main loop to handle OTA updates.
    void loop() {
        ArduinoOTA.handle();
    }
};

#endif // OTA_H
