#include "mqtt_client.h"
#include "../core/device_manager.h"
#include "../call/device_registry.h"
#include "../homeassistant/ha_discovery.h"
#include "mqtt_topics.h"
#include <ArduinoJson.h>
#include "config.h"

// Static instance pointer for callback
MqttClient* MqttClient::instance = nullptr;

MqttClient::MqttClient(DeviceManager* deviceMgr, DeviceRegistry* deviceReg)
    : deviceManager(deviceMgr),
      deviceRegistry(deviceReg),
      mqtt(nullptr),
      lastPresenceAnnounce(0),
      lastReconnectAttempt(0) {

    instance = this;

    // Create MQTT client
    mqtt = new PubSubClient(wifiClient);

    // Create HA discovery helper
    haDiscovery = new HomeAssistantDiscovery(deviceManager);
}

MqttClient::~MqttClient() {
    if (mqtt) {
        mqtt->disconnect();
        delete mqtt;
    }

    if (haDiscovery) {
        delete haDiscovery;
    }

    instance = nullptr;
}

bool MqttClient::begin() {
    String broker = deviceManager->getMqttBroker();
    uint16_t port = deviceManager->getMqttPort();

    if (broker.length() == 0) {
        Serial.println("[MQTT] No broker configured. Attempting mDNS discovery...");
        // TODO: Implement mDNS discovery for homeassistant.local
        Serial.println("[MQTT] For now, please configure MQTT broker manually");
        return false;
    }

    Serial.printf("[MQTT] Configuring broker: %s:%d\n", broker.c_str(), port);

    mqtt->setServer(broker.c_str(), port);
    mqtt->setCallback(MqttClient::messageCallback);
    mqtt->setKeepAlive(MQTT_KEEPALIVE);

    // Increase buffer size for larger payloads (Home Assistant discovery messages)
    mqtt->setBufferSize(2048);

    return true;
}

bool MqttClient::connect() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[MQTT] WiFi not connected!");
        return false;
    }

    if (mqtt->connected()) {
        return true;  // Already connected
    }

    String deviceId = deviceManager->getDeviceId();
    String roomName = deviceManager->getRoomName();

    Serial.printf("[MQTT] Connecting as %s (%s)...\n", deviceId.c_str(), roomName.c_str());

    // Last Will and Testament (LWT) - send "offline" when disconnected
    String statusTopic = MqttTopics::buildDeviceTopic(deviceId, "status");
    bool connected = false;

    String username = deviceManager->getMqttUsername();
    String password = deviceManager->getMqttPassword();

    if (username.length() > 0 && password.length() > 0) {
        connected = mqtt->connect(
            deviceId.c_str(),
            username.c_str(),
            password.c_str(),
            statusTopic.c_str(),
            MQTT_QOS,
            true,  // Retain
            MqttTopics::STATUS_OFFLINE
        );
    } else {
        connected = mqtt->connect(
            deviceId.c_str(),
            statusTopic.c_str(),
            MQTT_QOS,
            true,  // Retain
            MqttTopics::STATUS_OFFLINE
        );
    }

    if (connected) {
        Serial.println("[MQTT] Connected successfully!");

        // Publish online status
        publishStatus(MqttTopics::STATUS_ONLINE);

        // Subscribe to topics
        subscribeToTopics();

        // Publish Home Assistant discovery messages
        haDiscovery->publishAll(mqtt);

        // Announce presence
        publishPresence();

        return true;
    } else {
        Serial.printf("[MQTT] Connection failed! State: %d\n", mqtt->state());
        return false;
    }
}

void MqttClient::disconnect() {
    if (mqtt && mqtt->connected()) {
        // Publish offline status
        publishStatus(MqttTopics::STATUS_OFFLINE);

        mqtt->disconnect();
        Serial.println("[MQTT] Disconnected");
    }
}

bool MqttClient::isConnected() {
    return mqtt && mqtt->connected();
}

void MqttClient::loop() {
    if (!mqtt) {
        return;
    }

    // Maintain MQTT connection
    if (!mqtt->connected()) {
        unsigned long now = millis();

        if (now - lastReconnectAttempt > MQTT_RECONNECT_DELAY_MS) {
            lastReconnectAttempt = now;
            Serial.println("[MQTT] Reconnecting...");
            connect();
        }
    } else {
        mqtt->loop();

        // Periodic presence announcement
        unsigned long now = millis();
        if (now - lastPresenceAnnounce > DEVICE_ANNOUNCE_INTERVAL_MS) {
            lastPresenceAnnounce = now;
            publishPresence();

            // Also update sensor values periodically
            publishWifiRssi(WiFi.RSSI());
            publishUptime(millis() / 1000);
        }
    }
}

void MqttClient::subscribeToTopics() {
    String deviceId = deviceManager->getDeviceId();

    // Subscribe to discovery messages
    mqtt->subscribe(MqttTopics::DISCOVERY_ANNOUNCE);

    // Subscribe to our device-specific topics
    String callRequestTopic = MqttTopics::buildCallTopic(deviceId, "request");
    String callAcceptTopic = MqttTopics::buildCallTopic(deviceId, "accept");
    String callRejectTopic = MqttTopics::buildCallTopic(deviceId, "reject");
    String callHangupTopic = MqttTopics::buildCallTopic(deviceId, "hangup");
    String callInitiateTopic = MqttTopics::buildCallTopic(deviceId, "initiate");

    mqtt->subscribe(callRequestTopic.c_str());
    mqtt->subscribe(callAcceptTopic.c_str());
    mqtt->subscribe(callRejectTopic.c_str());
    mqtt->subscribe(callHangupTopic.c_str());
    mqtt->subscribe(callInitiateTopic.c_str());

    Serial.println("[MQTT] Subscribed to topics:");
    Serial.printf("  - %s\n", MqttTopics::DISCOVERY_ANNOUNCE);
    Serial.printf("  - %s\n", callRequestTopic.c_str());
    Serial.printf("  - %s\n", callAcceptTopic.c_str());
    Serial.printf("  - %s\n", callRejectTopic.c_str());
    Serial.printf("  - %s\n", callHangupTopic.c_str());
    Serial.printf("  - %s\n", callInitiateTopic.c_str());
}

void MqttClient::publishPresence() {
    String deviceId = deviceManager->getDeviceId();
    String roomName = deviceManager->getRoomName();
    String ipAddress = deviceManager->getIPAddress();

    DynamicJsonDocument doc(256);
    doc["device_id"] = deviceId;
    doc["room"] = roomName;
    doc["ip"] = ipAddress;

    String payload;
    serializeJson(doc, payload);

    mqtt->publish(MqttTopics::DISCOVERY_ANNOUNCE, payload.c_str());
}

void MqttClient::publishStatus(const char* status) {
    String deviceId = deviceManager->getDeviceId();
    String topic = MqttTopics::buildDeviceTopic(deviceId, "status");

    mqtt->publish(topic.c_str(), status, true);  // Retain
}

void MqttClient::publishCallStatus(const char* status) {
    String deviceId = deviceManager->getDeviceId();
    String topic = MqttTopics::buildCallTopic(deviceId, "status");

    mqtt->publish(topic.c_str(), status, true);  // Retain
}

void MqttClient::publishCallInfo(const String& callerRoom, const String& sessionId, unsigned long duration) {
    String deviceId = deviceManager->getDeviceId();
    String topic = MqttTopics::buildCallTopic(deviceId, "info");

    DynamicJsonDocument doc(256);
    doc["caller_room"] = callerRoom;
    doc["session_id"] = sessionId;
    doc["duration"] = duration;

    String payload;
    serializeJson(doc, payload);

    mqtt->publish(topic.c_str(), payload.c_str());
}

void MqttClient::publishWifiRssi(int rssi) {
    String deviceId = deviceManager->getDeviceId();
    String topic = MqttTopics::buildSensorTopic(deviceId, "wifi_rssi");

    mqtt->publish(topic.c_str(), String(rssi).c_str());
}

void MqttClient::publishUptime(unsigned long seconds) {
    String deviceId = deviceManager->getDeviceId();
    String topic = MqttTopics::buildSensorTopic(deviceId, "uptime");

    mqtt->publish(topic.c_str(), String(seconds).c_str());
}

void MqttClient::sendCallRequest(const String& targetDeviceId, const String& sessionId) {
    String topic = MqttTopics::buildCallTopic(targetDeviceId, "request");

    DynamicJsonDocument doc(256);
    doc["from"] = deviceManager->getDeviceId();
    doc["from_room"] = deviceManager->getRoomName();
    doc["session_id"] = sessionId;

    String payload;
    serializeJson(doc, payload);

    mqtt->publish(topic.c_str(), payload.c_str());
    Serial.printf("[MQTT] Sent call request to %s (session: %s)\n",
                  targetDeviceId.c_str(), sessionId.c_str());
}

void MqttClient::sendCallAccept(const String& targetDeviceId, const String& sessionId, uint16_t udpPort) {
    String topic = MqttTopics::buildCallTopic(targetDeviceId, "accept");

    DynamicJsonDocument doc(256);
    doc["session_id"] = sessionId;
    doc["udp_port"] = udpPort;
    doc["ip"] = deviceManager->getIPAddress();

    String payload;
    serializeJson(doc, payload);

    mqtt->publish(topic.c_str(), payload.c_str());
    Serial.printf("[MQTT] Sent call accept to %s (session: %s, port: %d)\n",
                  targetDeviceId.c_str(), sessionId.c_str(), udpPort);
}

void MqttClient::sendCallReject(const String& targetDeviceId, const String& sessionId, const String& reason) {
    String topic = MqttTopics::buildCallTopic(targetDeviceId, "reject");

    DynamicJsonDocument doc(256);
    doc["session_id"] = sessionId;
    doc["reason"] = reason;

    String payload;
    serializeJson(doc, payload);

    mqtt->publish(topic.c_str(), payload.c_str());
    Serial.printf("[MQTT] Sent call reject to %s (session: %s, reason: %s)\n",
                  targetDeviceId.c_str(), sessionId.c_str(), reason.c_str());
}

void MqttClient::sendCallHangup(const String& targetDeviceId, const String& sessionId) {
    String topic = MqttTopics::buildCallTopic(targetDeviceId, "hangup");

    DynamicJsonDocument doc(128);
    doc["session_id"] = sessionId;

    String payload;
    serializeJson(doc, payload);

    mqtt->publish(topic.c_str(), payload.c_str());
    Serial.printf("[MQTT] Sent hangup to %s (session: %s)\n",
                  targetDeviceId.c_str(), sessionId.c_str());
}

// Callback registration
void MqttClient::onDeviceAnnounced(DeviceAnnouncedCallback callback) {
    deviceAnnouncedCallback = callback;
}

void MqttClient::onCallRequest(CallRequestCallback callback) {
    callRequestCallback = callback;
}

void MqttClient::onCallAccept(CallAcceptCallback callback) {
    callAcceptCallback = callback;
}

void MqttClient::onCallReject(CallRejectCallback callback) {
    callRejectCallback = callback;
}

void MqttClient::onCallHangup(CallHangupCallback callback) {
    callHangupCallback = callback;
}

void MqttClient::onCallInitiate(CallInitiateCallback callback) {
    callInitiateCallback = callback;
}

// Static message callback
void MqttClient::messageCallback(char* topic, uint8_t* payload, unsigned int length) {
    if (instance) {
        String topicStr = String(topic);
        String payloadStr = "";

        for (unsigned int i = 0; i < length; i++) {
            payloadStr += (char)payload[i];
        }

        instance->handleMessage(topicStr, payloadStr);
    }
}

void MqttClient::handleMessage(const String& topic, const String& payload) {
    Serial.printf("[MQTT] Received message on: %s\n", topic.c_str());
    Serial.printf("[MQTT] Payload: %s\n", payload.c_str());

    // Handle device announcements
    if (topic == MqttTopics::DISCOVERY_ANNOUNCE) {
        handleDeviceAnnounce(payload);
        return;
    }

    // Extract device ID from topic
    String deviceId = extractDeviceId(topic);

    // Handle call-related messages
    if (topic.indexOf("/call/request") > 0) {
        handleCallRequest(deviceId, payload);
    } else if (topic.indexOf("/call/accept") > 0) {
        handleCallAccept(deviceId, payload);
    } else if (topic.indexOf("/call/reject") > 0) {
        handleCallReject(deviceId, payload);
    } else if (topic.indexOf("/call/hangup") > 0) {
        handleCallHangup(deviceId, payload);
    } else if (topic.indexOf("/call/initiate") > 0) {
        handleCallInitiate(payload);
    }
}

void MqttClient::handleDeviceAnnounce(const String& payload) {
    DynamicJsonDocument doc(256);
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
        Serial.printf("[MQTT] JSON parse error: %s\n", error.c_str());
        return;
    }

    String deviceId = doc["device_id"] | "";
    String room = doc["room"] | "";
    String ip = doc["ip"] | "";

    // Don't add ourselves to the registry
    if (deviceId == deviceManager->getDeviceId()) {
        return;
    }

    // Update device registry
    deviceRegistry->updateDevice(deviceId, room, ip);

    // Call callback if registered
    if (deviceAnnouncedCallback) {
        deviceAnnouncedCallback(deviceId, room, ip);
    }
}

void MqttClient::handleCallRequest(const String& deviceId, const String& payload) {
    DynamicJsonDocument doc(256);
    DeserializationError error = deserializeJson(doc, payload);

    if (error) return;

    String fromDeviceId = doc["from"] | "";
    String fromRoom = doc["from_room"] | "";
    String sessionId = doc["session_id"] | "";

    if (callRequestCallback) {
        callRequestCallback(fromDeviceId, fromRoom, sessionId);
    }
}

void MqttClient::handleCallAccept(const String& deviceId, const String& payload) {
    DynamicJsonDocument doc(256);
    DeserializationError error = deserializeJson(doc, payload);

    if (error) return;

    String sessionId = doc["session_id"] | "";
    uint16_t udpPort = doc["udp_port"] | 0;

    if (callAcceptCallback) {
        callAcceptCallback(sessionId, udpPort);
    }
}

void MqttClient::handleCallReject(const String& deviceId, const String& payload) {
    DynamicJsonDocument doc(256);
    DeserializationError error = deserializeJson(doc, payload);

    if (error) return;

    String sessionId = doc["session_id"] | "";
    String reason = doc["reason"] | "unknown";

    if (callRejectCallback) {
        callRejectCallback(sessionId, reason);
    }
}

void MqttClient::handleCallHangup(const String& deviceId, const String& payload) {
    DynamicJsonDocument doc(128);
    DeserializationError error = deserializeJson(doc, payload);

    if (error) return;

    String sessionId = doc["session_id"] | "";

    if (callHangupCallback) {
        callHangupCallback(sessionId);
    }
}

void MqttClient::handleCallInitiate(const String& payload) {
    DynamicJsonDocument doc(256);
    DeserializationError error = deserializeJson(doc, payload);

    if (error) return;

    String targetRoom = doc["target_room"] | "";

    if (callInitiateCallback && targetRoom.length() > 0) {
        callInitiateCallback(targetRoom);
    }
}

String MqttClient::extractDeviceId(const String& topic) {
    // Extract device ID from topic like "intercom/devices/{device_id}/..."
    int start = topic.indexOf("/devices/");
    if (start < 0) return "";

    start += 9;  // Length of "/devices/"
    int end = topic.indexOf("/", start);

    if (end < 0) return "";

    return topic.substring(start, end);
}
