#ifndef CONFIG_H
#define CONFIG_H

#define HOST_NAME "SolarCurrentLogger"

#define WIFI_SSID "myWifiSSID"
#define WIFI_PASSWORD "myWifiPassword"
#define WIFI_RECONNECT_INTERVAL 30000

#define USE_HTTP_SENDER false
#define HTTP_SERVER_URL "http://192.168.178.2:7777/api/v1/data"

#define USE_MQTT_SENDER true
#define MQTT_SERVER_URL "mqtt://192.168.178.2:1883"

// For API Token
#define API_TOKEN "1234567890"

// For Basic Auth:
// #define BASIC_AUTH_USERNAME "MyUserName"
// #define BASIC_AUTH_PASSWORD "myPassword"

// Measure every second
#define MEASURE_INTERVAL 1000

// Up to 3600 measurements (approx. 1 hour at 1 measurement per second)
#define BUFFER_SIZE 3600

// Temporary buffer for transmission
#define CHUNK_SIZE 64
#define JSON_BUFFER_SIZE 8192
// 0: 56 B 1.1 KB peak
// 1: 104 B 1.1 KB peak
// 2: 152 B 1.1 KB peak
// 4: 248 B 1.2 KB peak
// 8: 440 B 1.4 KB peak
// 16: 824 B 1.7 KB peak
// * 64: 3.1 KB / 6.6 KB peak
// 128: 6.1 KB / 12.1 KB peak

// Send interval: a transmission attempt is made every 5 seconds
#define SEND_INTERVAL 5000

// Interval for displaying free memory (here 60000 ms, configurable)
#define MEMORY_PRINT_INTERVAL 60000

#endif  // CONFIG_H
