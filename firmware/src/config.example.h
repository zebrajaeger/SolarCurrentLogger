#ifndef CONFIG_H
#define CONFIG_H

#define HOST_NAME "SolarCurrentLogger"

#define WIFI_SSID "myWifiSSID"
#define WIFI_PASSWORD "myWifiPassword"
#define SERVER_URL "http://192.168.178.100:7777/api/v1/data"

// For API Token
#define API_TOKEN "1234567890"

// For Basic Auth:
// #define BASIC_AUTH_USERNAME "MyUserName"
// #define BASIC_AUTH_PASSWORD "myPassword"

// Measure every second
#define MEASURE_INTERVAL 1000

// Up to 3600 measurements (approx. 1 hour at 1 measurement per second)
#define BUFFER_SIZE 3600

// Temporary buffer for transmission (fixed, global – maximum 256 entries)
#define CHUNK_SIZE 256

// Send interval: a transmission attempt is made every 5 seconds
#define SEND_INTERVAL 5000

// Interval for displaying free memory (here 60000 ms, configurable)
#define MEMORY_PRINT_INTERVAL 60000

#endif  // CONFIG_H
