#ifndef SENSOR_H
#define SENSOR_H

#include <Arduino.h>
#include <Wire.h>

#include "INA219.h"

// Abstract base class for sensors.
class Sensor {
 public:
  // Virtual destructor for proper cleanup of derived classes.
  virtual ~Sensor() {}

  // Initializes the sensor.
  virtual void setup() = 0;

  // Returns the current in mA measured by the sensor.
  virtual float getCurrentInMa() = 0;
};

// INA219 sensor implementation inheriting from Sensor.
class INA219Sensor : public Sensor {
 public:
  // Constructor with default I2C address 0x40.
  INA219Sensor(uint8_t addr = 0x40) : ina(addr) {}

  // Initializes the INA219 sensor.
  void setup() override {
    Wire.begin();
    if (!ina.begin()) {
      Serial.println("INA219 not found! Please check wiring.");
      // Block execution if sensor is not found.
      // while (1) {
      //     delay(1000);
      // }
      return;
    }
    ina.reset();
    ina.setGain(1);
    ina.setModeShuntContinuous();
    ina.setShuntSamples(7);  // 128 samples
  }

  // Returns the current in mA measured by the INA219 sensor.
  float getCurrentInMa() override {
    // The shunt value is divided by 10 to convert it to mA.
    return ((float)ina.getShuntValue()) / 10.0;
  }

 private:
  INA219 ina;
};

#endif  // SENSOR_H
