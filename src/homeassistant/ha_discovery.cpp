#include "ha_discovery.h"
#include "../core/device_manager.h"
#include "../network/mqtt_topics.h"
#include <PubSubClient.h>
#include "config.h"

HomeAssistantDiscovery::HomeAssistantDiscovery(DeviceManager* deviceMgr)
    : deviceManager(deviceMgr) {
}

HomeAssistantDiscovery::~HomeAssistantDiscovery() {
}

bool HomeAssistantDiscovery::publishAll(PubSubClient* mqtt) {
    if (!mqtt || !mqtt->connected()) {
        Serial.println("[HA Discovery] ERROR: MQTT not connected!");
        return false;
    }

    Serial.println("[HA Discovery] Publishing discovery messages...");

    bool success = true;
    success &= publishCallBinarySensor(mqtt);
    success &= publishCallerSensor(mqtt);
    success &= publishWifiRssiSensor(mqtt);
    success &= publishUptimeSensor(mqtt);
    success &= publishHangupButton(mqtt);

    if (success) {
        Serial.println("[HA Discovery] All discovery messages published successfully!");
    } else {
        Serial.println("[HA Discovery] WARNING: Some discovery messages failed to publish");
    }

    return success;
}

bool HomeAssistantDiscovery::unpublishAll(PubSubClient* mqtt) {
    if (!mqtt || !mqtt->connected()) {
        return false;
    }

    Serial.println("[HA Discovery] Removing device from Home Assistant...");

    String deviceId = deviceManager->getDeviceId();

    // Publish empty payloads to remove entities
    publishDiscovery(mqtt, MqttTopics::buildHADiscoveryTopic("binary_sensor", deviceId + "_call"), "");
    publishDiscovery(mqtt, MqttTopics::buildHADiscoveryTopic("sensor", deviceId + "_caller"), "");
    publishDiscovery(mqtt, MqttTopics::buildHADiscoveryTopic("sensor", deviceId + "_rssi"), "");
    publishDiscovery(mqtt, MqttTopics::buildHADiscoveryTopic("sensor", deviceId + "_uptime"), "");
    publishDiscovery(mqtt, MqttTopics::buildHADiscoveryTopic("button", deviceId + "_hangup"), "");

    return true;
}

void HomeAssistantDiscovery::buildDeviceInfo(JsonObject& device) {
    String deviceId = deviceManager->getDeviceId();
    String roomName = deviceManager->getRoomName();

    JsonArray identifiers = device.createNestedArray("identifiers");
    identifiers.add(deviceId);

    device["name"] = roomName + " Intercom";
    device["model"] = "ESP32-S3 Intercom";
    device["manufacturer"] = "Custom";
    device["sw_version"] = FIRMWARE_VERSION;
}

bool HomeAssistantDiscovery::publishCallBinarySensor(PubSubClient* mqtt) {
    String deviceId = deviceManager->getDeviceId();
    String roomName = deviceManager->getRoomName();

    DynamicJsonDocument doc(1024);

    doc["name"] = roomName + " Call Active";
    doc["unique_id"] = deviceId + "_call";
    doc["device_class"] = "occupancy";
    doc["state_topic"] = MqttTopics::buildCallTopic(deviceId, "status");
    doc["payload_on"] = MqttTopics::CALL_STATUS_ACTIVE;
    doc["payload_off"] = MqttTopics::CALL_STATUS_IDLE;

    JsonObject device = doc.createNestedObject("device");
    buildDeviceInfo(device);

    String payload;
    serializeJson(doc, payload);

    String topic = MqttTopics::buildHADiscoveryTopic("binary_sensor", deviceId + "_call");
    return publishDiscovery(mqtt, topic, payload);
}

bool HomeAssistantDiscovery::publishCallerSensor(PubSubClient* mqtt) {
    String deviceId = deviceManager->getDeviceId();
    String roomName = deviceManager->getRoomName();

    DynamicJsonDocument doc(1024);

    doc["name"] = roomName + " Caller";
    doc["unique_id"] = deviceId + "_caller";
    doc["state_topic"] = MqttTopics::buildCallTopic(deviceId, "info");
    doc["value_template"] = "{{ value_json.caller_room | default('None') }}";
    doc["icon"] = "mdi:phone";

    JsonObject device = doc.createNestedObject("device");
    buildDeviceInfo(device);

    String payload;
    serializeJson(doc, payload);

    String topic = MqttTopics::buildHADiscoveryTopic("sensor", deviceId + "_caller");
    return publishDiscovery(mqtt, topic, payload);
}

bool HomeAssistantDiscovery::publishWifiRssiSensor(PubSubClient* mqtt) {
    String deviceId = deviceManager->getDeviceId();
    String roomName = deviceManager->getRoomName();

    DynamicJsonDocument doc(1024);

    doc["name"] = roomName + " WiFi Signal";
    doc["unique_id"] = deviceId + "_rssi";
    doc["state_topic"] = MqttTopics::buildSensorTopic(deviceId, "wifi_rssi");
    doc["unit_of_measurement"] = "dBm";
    doc["device_class"] = "signal_strength";
    doc["icon"] = "mdi:wifi";

    JsonObject device = doc.createNestedObject("device");
    buildDeviceInfo(device);

    String payload;
    serializeJson(doc, payload);

    String topic = MqttTopics::buildHADiscoveryTopic("sensor", deviceId + "_rssi");
    return publishDiscovery(mqtt, topic, payload);
}

bool HomeAssistantDiscovery::publishUptimeSensor(PubSubClient* mqtt) {
    String deviceId = deviceManager->getDeviceId();
    String roomName = deviceManager->getRoomName();

    DynamicJsonDocument doc(1024);

    doc["name"] = roomName + " Uptime";
    doc["unique_id"] = deviceId + "_uptime";
    doc["state_topic"] = MqttTopics::buildSensorTopic(deviceId, "uptime");
    doc["unit_of_measurement"] = "s";
    doc["device_class"] = "duration";
    doc["icon"] = "mdi:clock-outline";

    JsonObject device = doc.createNestedObject("device");
    buildDeviceInfo(device);

    String payload;
    serializeJson(doc, payload);

    String topic = MqttTopics::buildHADiscoveryTopic("sensor", deviceId + "_uptime");
    return publishDiscovery(mqtt, topic, payload);
}

bool HomeAssistantDiscovery::publishHangupButton(PubSubClient* mqtt) {
    String deviceId = deviceManager->getDeviceId();
    String roomName = deviceManager->getRoomName();

    DynamicJsonDocument doc(1024);

    doc["name"] = roomName + " Hang Up";
    doc["unique_id"] = deviceId + "_hangup";
    doc["command_topic"] = MqttTopics::buildCallTopic(deviceId, "hangup");
    doc["payload_press"] = "hangup";
    doc["icon"] = "mdi:phone-hangup";

    JsonObject device = doc.createNestedObject("device");
    buildDeviceInfo(device);

    String payload;
    serializeJson(doc, payload);

    String topic = MqttTopics::buildHADiscoveryTopic("button", deviceId + "_hangup");
    return publishDiscovery(mqtt, topic, payload);
}

bool HomeAssistantDiscovery::publishDiscovery(PubSubClient* mqtt, const String& topic, const String& payload) {
    Serial.printf("[HA Discovery] Publishing to: %s\n", topic.c_str());
    Serial.printf("[HA Discovery] Payload size: %d bytes\n", payload.length());

    bool success = mqtt->publish(topic.c_str(), payload.c_str(), true); // Retain = true

    if (success) {
        Serial.println("[HA Discovery] ✓ Published successfully");
    } else {
        Serial.println("[HA Discovery] ✗ Publish failed!");
    }

    return success;
}
