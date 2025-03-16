# SolarCurrentLogger

## Overview
This project is an ESP32-based measurement logger that reads current values using an INA219 sensor and sends the data asynchronously to a remote server using HTTP or HTTPS. The data is stored in a ring buffer and transmitted in chunks to ensure reliable data logging.

## Features
- Reads current measurements from an INA219 sensor.
- Stores measurements in a ring buffer.
- Sends data asynchronously using HTTP or HTTPS.
- Supports API token and basic authentication.
- Uses FreeRTOS mutexes to protect shared resources.
- Synchronizes time using NTP.
- Monitors and logs free heap memory.

## Requirements
### Hardware
- ESP32 development board
- INA219 current sensor
- WiFi connection

### Software
- [PlatformIO](https://platformio.org/) installed
- ESP32 Arduino framework

## Installation
### 1. Clone the Repository
```sh
git clone <repository-url>
cd <repository-folder>
```

### 2. Install PlatformIO
Ensure you have PlatformIO installed. You can install it via:
```sh
pip install platformio
```

### 3. Configure the Project
#### Copy and Edit Configuration File
The `config.example.h` file contains example configurations. You must copy and modify it to match your settings.

```sh
cp src/config.example.h src/config.h
```

Edit `src/config.h` and update the following values:
```cpp
#define WIFI_SSID "your_wifi_ssid"
#define WIFI_PASSWORD "your_wifi_password"
#define SERVER_URL "http://your-server-url:port/api/v1/data"
```

### 4. Build and Upload
```sh
pio run --target upload
```

### 5. Monitor Serial Output
```sh
pio device monitor --baud 115200
```

## Project Structure
```
├── .gitignore
├── platformio.ini         # PlatformIO project configuration
├── src/
│   ├── config.example.h   # Example configuration file
│   ├── config.h           # Actual configuration file (ignored in git)
│   ├── main.cpp           # Main program logic
│   ├── INA219.h           # INA219 sensor library
└── README.md              # This file
```

## Dependencies
The required libraries are specified in `platformio.ini` and will be installed automatically:
```ini
lib_deps =
    bblanchon/ArduinoJson@6.18.5
    khoih-prog/AsyncTCP_SSL@1.3.1
    khoih-prog/AsyncHTTPRequest_Generic@1.13.0
    khoih-prog/AsyncHTTPSRequest_Generic@2.5.0
```

## Usage
- The ESP32 will continuously measure current and store the values in a ring buffer.
- Every 5 seconds, it will attempt to send the stored values to the server.
- If the transmission fails, the data remains in the buffer until successfully transmitted.
- Free heap memory is logged every 60 seconds.

## License
This project is licensed under the MIT License.
