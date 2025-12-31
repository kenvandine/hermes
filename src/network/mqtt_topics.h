#ifndef MQTT_TOPICS_H
#define MQTT_TOPICS_H

#include <Arduino.h>

/**
 * MQTT Topic Structure for ESP32 Intercom System
 *
 * Topic hierarchy:
 *
 * intercom/
 * ├── discovery/
 * │   ├── announce                     # Device presence announcements
 * │   └── devices                      # Device list updates
 * │
 * ├── devices/{device_id}/
 * │   ├── status                       # online/offline (Last Will & Testament)
 * │   ├── state                        # idle/listening/calling/ringing/active
 * │   ├── call/
 * │   │   ├── status                   # idle/ringing/active
 * │   │   ├── info                     # Call information JSON
 * │   │   ├── initiate                # Trigger call (from HA)
 * │   │   ├── request                 # Incoming call request
 * │   │   ├── accept                  # Accept call
 * │   │   ├── reject                  # Reject call
 * │   │   └── hangup                  # Hang up call
 * │   └── sensors/
 * │       ├── wifi_rssi               # WiFi signal strength
 * │       └── uptime                  # Device uptime
 * │
 * └── homeassistant/                   # HA MQTT Discovery
 *     ├── device/{device_id}/config
 *     ├── binary_sensor/{device_id}_call/config
 *     ├── sensor/{device_id}_caller/config
 *     └── button/{device_id}_hangup/config
 */

namespace MqttTopics {

// Base topic prefix
const char* const BASE = "intercom";
const char* const HA_DISCOVERY_PREFIX = "homeassistant";

// Discovery topics
const char* const DISCOVERY_ANNOUNCE = "intercom/discovery/announce";
const char* const DISCOVERY_DEVICES = "intercom/discovery/devices";

/**
 * Build device-specific topic
 * Example: buildDeviceTopic("esp32_ABC123", "status") -> "intercom/devices/esp32_ABC123/status"
 */
inline String buildDeviceTopic(const String& deviceId, const char* subtopic) {
    return String(BASE) + "/devices/" + deviceId + "/" + subtopic;
}

/**
 * Build call-related topic
 * Example: buildCallTopic("esp32_ABC123", "request") -> "intercom/devices/esp32_ABC123/call/request"
 */
inline String buildCallTopic(const String& deviceId, const char* action) {
    return String(BASE) + "/devices/" + deviceId + "/call/" + action;
}

/**
 * Build sensor topic
 * Example: buildSensorTopic("esp32_ABC123", "wifi_rssi") -> "intercom/devices/esp32_ABC123/sensors/wifi_rssi"
 */
inline String buildSensorTopic(const String& deviceId, const char* sensor) {
    return String(BASE) + "/devices/" + deviceId + "/sensors/" + sensor;
}

/**
 * Build Home Assistant discovery topic
 * Example: buildHADiscoveryTopic("binary_sensor", "esp32_ABC123_call", "config")
 *          -> "homeassistant/binary_sensor/esp32_ABC123_call/config"
 */
inline String buildHADiscoveryTopic(const char* component, const String& objectId, const char* suffix = "config") {
    return String(HA_DISCOVERY_PREFIX) + "/" + component + "/" + objectId + "/" + suffix;
}

// Device status payloads
const char* const STATUS_ONLINE = "online";
const char* const STATUS_OFFLINE = "offline";

// Device state payloads
const char* const STATE_IDLE = "idle";
const char* const STATE_LISTENING = "listening";
const char* const STATE_CALLING = "calling";
const char* const STATE_RINGING = "ringing";
const char* const STATE_ACTIVE = "active";

// Call status payloads
const char* const CALL_STATUS_IDLE = "idle";
const char* const CALL_STATUS_RINGING = "ringing";
const char* const CALL_STATUS_ACTIVE = "active";

} // namespace MqttTopics

#endif // MQTT_TOPICS_H
