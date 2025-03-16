# SolarCurrentLogger

## Overview

**SolarCurrentLogger** is a two-component project designed to measure and log current flow through a solar cell. It consists of:

- **Firmware** (ESP32-based client): Measures current using an INA219 sensor and transmits data to a server.
- **Server** (Node.js & InfluxDB): Stores received data and provides visualization via Grafana.

## Features

- **ESP32 Client (Firmware)**
  - Reads current values from an INA219 sensor.
  - Stores measurements in a ring buffer.
  - Sends data asynchronously via HTTP/HTTPS.
  - Supports API token authentication.
  
- **Server**
  - Receives data via an Express.js API.
  - Stores measurements in an InfluxDB database.
  - Provides a Grafana dashboard for visualization.

## Installation

### 1. Clone the Repository

```sh
git clone <repository-url>
cd <repository-folder>
```

### 2. Set Up the Server

Navigate to the `server` directory and follow the installation steps in `server/README.md`.

```sh
cd server
```

Start the server with:
```sh
docker compose up -d
```

Set up **InfluxDB**:
- Open: `http://<your-server-ip>:8086`
- Log in (`admin` / `admin123` by default)
- Generate a **Write API Token** and update `docker-compose.yml`.
- Generate a **Read API Token** for Grafana.

Set up **Grafana**:
- Open: `http://<your-server-ip>:3000`
- Log in (`admin` / `admin` by default)
- Add InfluxDB as a data source using the Read API Token.

### 3. Set Up the ESP32 Client

Navigate to the `firmware` directory and follow the installation steps in `firmware/README.md`.

```sh
cd firmware
```

Modify `src/config.h`:
```cpp
#define WIFI_SSID "your_wifi_ssid"
#define WIFI_PASSWORD "your_wifi_password"
#define SERVER_URL "http://<your-server-ip>:7777/api/v1/data"
```

Compile and upload the firmware:
```sh
pio run --target upload
```

### 4. Verify Data Flow

- The ESP32 client should now send data to the server every few seconds.
- Data should appear in **InfluxDB**.
- **Grafana** should display the real-time measurements.

## License

This project is licensed under the MIT License.

