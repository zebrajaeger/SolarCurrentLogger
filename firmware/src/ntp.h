#ifndef NTP_H
#define NTP_H

#include <Arduino.h>
#include <time.h>
#include <sys/time.h>
#include <esp_sntp.h>

class NTPHandler {
public:
    // Initializes NTP time synchronization.
    void setup() {
        // Set the time synchronization callback.
        sntp_set_time_sync_notification_cb(timeSyncCallback);
        
        // Configure the time using NTP servers.
        configTime(0, 0, "pool.ntp.org", "time.nist.gov");
        Serial.println("Synchronizing NTP time...");
        
        // Wait for time to be set.
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
    }

private:
    // Callback function for time synchronization.
    static void timeSyncCallback(struct timeval *tv) {
        Serial.println("\n----Time Sync----- Time should have been verified and updated if needed");
        Serial.println(tv->tv_sec);
        Serial.println(ctime(&tv->tv_sec));
    }
};

#endif // NTP_H
