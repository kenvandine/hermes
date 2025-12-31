#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <functional>

// Forward declarations
class DeviceManager;
class DeviceRegistry;
class HomeAssistantDiscovery;

/**
 * MqttClient
 *
 * Manages MQTT connection and messaging for the intercom system:
 * - Connects to Home Assistant's MQTT broker
 * - Publishes device presence and status
 * - Subscribes to device-specific topics
 * - Handles Home Assistant discovery
 * - Routes incoming messages to callbacks
 */
class MqttClient {
public:
    // Message callback types
    typedef std::function<void(const String& deviceId, const String& roomName, const String& ip)> DeviceAnnouncedCallback;
    typedef std::function<void(const String& fromDeviceId, const String& fromRoom, const String& sessionId)> CallRequestCallback;
    typedef std::function<void(const String& sessionId, uint16_t udpPort)> CallAcceptCallback;
    typedef std::function<void(const String& sessionId, const String& reason)> CallRejectCallback;
    typedef std::function<void(const String& sessionId)> CallHangupCallback;
    typedef std::function<void(const String& targetRoom)> CallInitiateCallback;

    MqttClient(DeviceManager* deviceMgr, DeviceRegistry* deviceReg);
    ~MqttClient();

    /**
     * Initialize MQTT client
     */
    bool begin();

    /**
     * Connect to MQTT broker
     * Returns true if connected, false otherwise
     */
    bool connect();

    /**
     * Disconnect from MQTT broker
     */
    void disconnect();

    /**
     * Check if connected to broker
     */
    bool isConnected();

    /**
     * Process MQTT messages (call regularly in loop)
     */
    void loop();

    /**
     * Publish device presence announcement
     */
    void publishPresence();

    /**
     * Publish device status (online/offline)
     */
    void publishStatus(const char* status);

    /**
     * Publish call status
     */
    void publishCallStatus(const char* status);

    /**
     * Publish call info (caller, duration, session ID)
     */
    void publishCallInfo(const String& callerRoom, const String& sessionId, unsigned long duration = 0);

    /**
     * Publish sensor values
     */
    void publishWifiRssi(int rssi);
    void publishUptime(unsigned long seconds);

    /**
     * Call signaling methods
     */
    void sendCallRequest(const String& targetDeviceId, const String& sessionId);
    void sendCallAccept(const String& targetDeviceId, const String& sessionId, uint16_t udpPort);
    void sendCallReject(const String& targetDeviceId, const String& sessionId, const String& reason);
    void sendCallHangup(const String& targetDeviceId, const String& sessionId);

    /**
     * Register callbacks for incoming messages
     */
    void onDeviceAnnounced(DeviceAnnouncedCallback callback);
    void onCallRequest(CallRequestCallback callback);
    void onCallAccept(CallAcceptCallback callback);
    void onCallReject(CallRejectCallback callback);
    void onCallHangup(CallHangupCallback callback);
    void onCallInitiate(CallInitiateCallback callback);

private:
    DeviceManager* deviceManager;
    DeviceRegistry* deviceRegistry;
    HomeAssistantDiscovery* haDiscovery;

    WiFiClient wifiClient;
    PubSubClient* mqtt;

    unsigned long lastPresenceAnnounce;
    unsigned long lastReconnectAttempt;

    // Callbacks
    DeviceAnnouncedCallback deviceAnnouncedCallback;
    CallRequestCallback callRequestCallback;
    CallAcceptCallback callAcceptCallback;
    CallRejectCallback callRejectCallback;
    CallHangupCallback callHangupCallback;
    CallInitiateCallback callInitiateCallback;

    // Subscribe to topics
    void subscribeToTopics();

    // Message handler (static for PubSubClient)
    static void messageCallback(char* topic, uint8_t* payload, unsigned int length);

    // Instance pointer for static callback
    static MqttClient* instance;

    // Handle incoming message
    void handleMessage(const String& topic, const String& payload);

    // Parse JSON messages
    void handleDeviceAnnounce(const String& payload);
    void handleCallRequest(const String& deviceId, const String& payload);
    void handleCallAccept(const String& deviceId, const String& payload);
    void handleCallReject(const String& deviceId, const String& payload);
    void handleCallHangup(const String& deviceId, const String& payload);
    void handleCallInitiate(const String& payload);

    // Helper to extract device ID from topic
    String extractDeviceId(const String& topic);
};

#endif // MQTT_CLIENT_H
