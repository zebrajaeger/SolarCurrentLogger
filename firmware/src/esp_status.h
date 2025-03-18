#ifndef ESP_STATUS_H
#define ESP_STATUS_H

#include <Arduino.h>
#include <esp_heap_caps.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

class EspStatus {
 public:
  void getStatus(String &output) {
    output = "";
    output += "Free Heap: ";
    output += String(esp_get_free_heap_size());
    output += " Bytes\n";

    output += "Min. free Heap: ";
    output += String(esp_get_minimum_free_heap_size());
    output += " Bytes\n";

#ifdef CONFIG_SPIRAM_SUPPORT
    output += "Free PSRAM: ";
    output += String(esp_psram_get_free_size());
    output += " Bytes\n";
#endif

    output += "Uptime: ";
    output += String(millis() / 1000);
    output += " s\n";

    UBaseType_t stackHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
    output += "Task Stack High Water Mark: ";
    output += String(stackHighWaterMark * sizeof(StackType_t));
    output += " Bytes\n";
  }
};

#endif  // ESP_STATUS_H