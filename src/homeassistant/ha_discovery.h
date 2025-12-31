#ifndef HA_DISCOVERY_H
#define HA_DISCOVERY_H

#include <Arduino.h>
#include <ArduinoJson.h>

// Forward declarations
class DeviceManager;
class PubSubClient;

/**
 * HomeAssistantDiscovery
 *
 * Manages MQTT Discovery for Home Assistant integration.
 * Publishes discovery messages for:
 * - Device registration
 * - Binary sensors (call active)
 * - Sensors (caller info, wifi RSSI, uptime, AI query, AI response)
 * - Buttons (hangup)
 *
 * See: https://www.home-assistant.io/integrations/mqtt/#mqtt-discovery
 */
class HomeAssistantDiscovery {
public:
    HomeAssistantDiscovery(DeviceManager* deviceMgr);
    ~HomeAssistantDiscovery();

    /**
     * Publish all discovery messages to Home Assistant
     * Should be called after MQTT connection is established
     */
    bool publishAll(PubSubClient* mqtt);

    /**
     * Remove device from Home Assistant (publish empty config)
     * Call before shutdown or factory reset
     */
    bool unpublishAll(PubSubClient* mqtt);

private:
    DeviceManager* deviceManager;

    // Build device JSON block (common for all entities)
    void buildDeviceInfo(JsonObject& device);

    // Publish individual discovery messages
    bool publishDeviceConfig(PubSubClient* mqtt);
    bool publishCallBinarySensor(PubSubClient* mqtt);
    bool publishCallerSensor(PubSubClient* mqtt);
    bool publishWifiRssiSensor(PubSubClient* mqtt);
    bool publishUptimeSensor(PubSubClient* mqtt);
    bool publishHangupButton(PubSubClient* mqtt);
    bool publishAIQuerySensor(PubSubClient* mqtt);
    bool publishAIResponseSensor(PubSubClient* mqtt);

    // Helper to publish discovery message
    bool publishDiscovery(PubSubClient* mqtt, const String& topic, const String& payload);
};

#endif // HA_DISCOVERY_H
