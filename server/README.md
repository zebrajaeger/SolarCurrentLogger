# SolarCurrentLogger Server

## Overview

This repository contains the server-side components for the **SolarCurrentLogger** project. The server receives current measurement data from an ESP32 device, stores it in an InfluxDB database, and provides visualization through Grafana.

## Features

- Express.js API to receive data from the ESP32 logger.
- Stores measurement data in InfluxDB.
- Provides visualization with Grafana.
- Docker-based deployment.
- Uses API token authentication for secure data submission.

## Requirements

### Software

- Docker & Docker Compose installed.
- InfluxDB 2.7 for data storage.
- Grafana for visualization.

## Installation

### 1. Clone the Repository

```sh
git clone <repository-url>
cd <repository-folder>
```

### 2. Start the Server

Run the following command to start all services:

```sh
docker compose up -d
```

### 3. Configure InfluxDB

Once the server is running, open InfluxDB in your browser:

```
http://<your-server-ip>:8086
```

Log in with:

- **Username**: `admin`
- **Password**: `admin123`

#### Create API Tokens

1. Navigate to **Load Data > API Tokens**.
2. Generate a **Write API Token** with access to the `solar` bucket.
3. Copy the token and update `docker-compose.yml`:

```yaml
- INFLUXDB_TOKEN=<your-write-token>
```

4. Generate a **Read API Token** for Grafana to access the `solar` bucket.

### 4. Restart the Server

After updating the `INFLUXDB_TOKEN`, restart the server:

```sh
docker compose down && docker compose up -d
```

## API Endpoints

### 1. Data Submission Endpoint

**URL:** `POST /api/v1/data`

**Headers:**

```json
{
  "Content-Type": "application/json",
  "X-API-Token": "1234567890"
}
```

**Body:**

```json
{d
```

### 2. Sending Test Data

You can manually test the API using `curl`:

```sh
curl -X POST "http://localhost:7777/api/v1/data" \
     -H "Content-Type: application/json" \
     -H "X-API-Token: 1234567890" \
     -d '{"measurements":[{"timestamp": 1710590900000, "value": 12.5}]}'
```

## Grafana Setup

Grafana runs on port `3000`. Open your browser and navigate to:

```
http://<your-server-ip>:3000
```

Log in with:

- **Username**: `admin`
- **Password**: `admin`

### Configure InfluxDB as a Data Source

1. Go to **Settings > Data Sources**.
2. Select **InfluxDB**.
3. Set the following values:
   - **URL**: `http://<your-server-ip>:8086`
   - **Organization**: `zj`
   - **Default Bucket**: `solar`
   - **Query Language**: `Flux`
4. Under **Custom HTTP Headers**, add:
   ```
   "Authorization": "Token <Grafana API TOKEN>"
   ```
5. Click **Save & Test**.

### Example Flux Query

To visualize the current measurements, use the following Flux query:

```flux
from(bucket: "solar")
  |> range(start: v.timeRangeStart, stop: v.timeRangeStop)
  |> filter(fn: (r) => r["_measurement"] == "current")
  |> aggregateWindow(every: v.windowPeriod, fn: median, createEmpty: false)
  |> yield(name: "median")
```

## Project Structure

```
├── .gitignore
├── docker-compose.yml      # Docker setup for server components
├── Dockerfile              # Express.js API container
├── index.js                # Express.js server logic
├── package.json            # Node.js dependencies
└── README.md               # This file
```

## Dependencies

The server dependencies are listed in `package.json`:

```json
"dependencies": {
    "axios": "^1.8.3",
    "express": "^4.21.2"
}
```

## License

This project is licensed under the MIT License.

