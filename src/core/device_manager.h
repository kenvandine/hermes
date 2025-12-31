#ifndef DEVICE_MANAGER_H
#define DEVICE_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>

/**
 * DeviceManager
 *
 * Manages device identity and configuration:
 * - Device ID (unique identifier based on MAC address)
 * - Room name (user configurable)
 * - Configuration persistence (NVS)
 * - Device metadata for Home Assistant
 */
class DeviceManager {
public:
    DeviceManager();
    ~DeviceManager();

    // Initialize the device manager
    bool begin();

    // Device identity
    String getDeviceId() const;
    String getRoomName() const;
    String getMacAddress() const;
    String getIPAddress() const;

    // Configuration
    void setRoomName(const String& roomName);

    // Audio settings
    uint8_t getMicVolume() const;
    void setMicVolume(uint8_t volume);
    uint8_t getSpeakerVolume() const;
    void setSpeakerVolume(uint8_t volume);

    // Display settings
    uint8_t getBrightness() const;
    void setBrightness(uint8_t brightness);

    // Wake word settings
    float getWakeWordThreshold() const;
    void setWakeWordThreshold(float threshold);

    // MQTT broker settings
    String getMqttBroker() const;
    void setMqttBroker(const String& broker);
    uint16_t getMqttPort() const;
    void setMqttPort(uint16_t port);
    String getMqttUsername() const;
    void setMqttUsername(const String& username);
    String getMqttPassword() const;
    void setMqttPassword(const String& password);

    // WiFi settings
    String getWifiSsid() const;
    void setWifiSsid(const String& ssid);
    String getWifiPassword() const;
    void setWifiPassword(const String& password);

    // First boot check
    bool isFirstBoot() const;

    // Factory reset
    void factoryReset();

    // Print device info
    void printInfo() const;

private:
    Preferences preferences;

    String deviceId;
    String macAddress;
    String roomName;

    uint8_t micVolume;
    uint8_t speakerVolume;
    uint8_t brightness;
    float wakeWordThreshold;

    String mqttBroker;
    uint16_t mqttPort;
    String mqttUsername;
    String mqttPassword;

    String wifiSsid;
    String wifiPassword;

    bool firstBoot;

    // Generate device ID from MAC address
    void generateDeviceId();

    // Load configuration from NVS
    void loadConfig();

    // Save configuration to NVS
    void saveConfig();
};

#endif // DEVICE_MANAGER_H
