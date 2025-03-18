#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <Arduino.h>
#include <PsychicMqttClient.h>  // using bambo1543/MqttClientBinary

class MqttHandler {
 public:
  // Constructor: sets default topics for status and measurement.
  MqttHandler() : _statusTopic("status"), _measurementTopic("measurement") {}

  /**
   * Setup the MQTT client.
   * @param serverUrl MQTT server URL (e.g., "ws://mqtt.eclipseprojects.io:80/mqtt")
   * @param deviceTopicPrefix Device topic prefix (e.g., based on MAC address)
   */
  void setup(const String &serverUrl, const String &deviceTopicPrefix) {
    _serverUrl = serverUrl;
    _deviceTopicPrefix = deviceTopicPrefix;
    mqttClient.setServer(_serverUrl.c_str());

    // Register MQTT event callbacks
    mqttClient.onConnect(MqttHandler::onMqttConnect);
    mqttClient.onDisconnect(MqttHandler::onMqttDisconnect);
    mqttClient.onPublish(MqttHandler::onMqttPublish);

    // Initiate connection
    mqttClient.connect();
    while (!mqttClient.connected()) {
      delay(500);
    }
  }

  // Set the topic for status messages.
  void setStatusTopic(const String &topic) { _statusTopic = topic; }

  // Set the topic for measurement messages.
  void setMeasurementTopic(const String &topic) { _measurementTopic = topic; }

  /**
   * Publish a status string in a non-blocking way.
   * @param status The status message to be sent.
   */
  void publishStatus(const String &status) {
    if (!mqttClient.connected()) {
      Serial.println("MQTT not connected. Status message not sent.");
      return;
    }
    String topic = _deviceTopicPrefix + "/" + _statusTopic;
    // Publish with QoS 0 and no retain (non-blocking)
    mqttClient.publish(topic.c_str(), 0, false, status.c_str());
  }

  /**
   * Publish a measurement value in a non-blocking way.
   * @param measurement The measurement value to be sent.
   */
  void publishMeasurement(const String &measurement) {
    if (!mqttClient.connected()) {
      Serial.println("MQTT not connected. Measurement not sent.");
      return;
    }
    String topic = _deviceTopicPrefix + "/" + _measurementTopic;
    // Format the measurement to two decimal places.
    mqttClient.publish(topic.c_str(), 0, false, measurement.c_str());
  }

  /**
   * Returns true if the MQTT client is connected.
   */
  bool isConnected() { return mqttClient.connected(); }

 private:
  String _serverUrl;
  String _deviceTopicPrefix;
  String _statusTopic;
  String _measurementTopic;

  PsychicMqttClient mqttClient;

  // Callback for publish acknowledgment.
  static void onMqttPublish(uint16_t packetId) {
    Serial.println("Publish acknowledged.");
    Serial.printf("Packet ID: %d\r\n", packetId);
  }

  // Callback for successful MQTT connection.
  static void onMqttConnect(bool sessionPresent) {
    Serial.println("Connected to MQTT.");
    Serial.printf("Session present: %d\r\n", sessionPresent);
  }

  // Callback for MQTT disconnection.
  static void onMqttDisconnect(bool sessionPresent) { Serial.println("Disconnected from MQTT."); }
};

#endif  // MQTT_HANDLER_H
