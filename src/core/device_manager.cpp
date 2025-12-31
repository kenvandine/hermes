#include "device_manager.h"
#include <WiFi.h>
#include "config.h"

DeviceManager::DeviceManager()
    : micVolume(DEFAULT_MIC_VOLUME),
      speakerVolume(DEFAULT_SPEAKER_VOLUME),
      brightness(BACKLIGHT_BRIGHTNESS),
      wakeWordThreshold(WAKE_WORD_THRESHOLD),
      mqttPort(MQTT_PORT),
      firstBoot(false) {
}

DeviceManager::~DeviceManager() {
    preferences.end();
}

bool DeviceManager::begin() {
    // Initialize NVS
    if (!preferences.begin(NVS_NAMESPACE, false)) {
        Serial.println("[DeviceManager] ERROR: Failed to initialize NVS!");
        return false;
    }

    // Generate MAC-based device ID
    generateDeviceId();

    // Load configuration from NVS
    loadConfig();

    return true;
}

void DeviceManager::generateDeviceId() {
    // Get MAC address
    uint8_t mac[6];
    WiFi.macAddress(mac);

    // Format as string
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    macAddress = String(macStr);

    // Create device ID (remove colons)
    char deviceIdStr[18];
    snprintf(deviceIdStr, sizeof(deviceIdStr), "%02X%02X%02X%02X%02X%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    deviceId = "esp32_" + String(deviceIdStr);
}

void DeviceManager::loadConfig() {
    // Check if device ID exists in NVS (first boot check)
    String storedDeviceId = preferences.getString(NVS_KEY_DEVICE_ID, "");

    if (storedDeviceId.length() == 0) {
        // First boot - initialize with defaults
        firstBoot = true;
        Serial.println("[DeviceManager] First boot detected!");

        roomName = "Unconfigured";

        // Save initial config
        saveConfig();
    } else {
        // Load existing config
        firstBoot = false;

        roomName = preferences.getString(NVS_KEY_ROOM_NAME, "Unconfigured");
        micVolume = preferences.getUChar(NVS_KEY_VOLUME_MIC, DEFAULT_MIC_VOLUME);
        speakerVolume = preferences.getUChar(NVS_KEY_VOLUME_SPK, DEFAULT_SPEAKER_VOLUME);
        brightness = preferences.getUChar(NVS_KEY_BRIGHTNESS, BACKLIGHT_BRIGHTNESS);

        // Load MQTT settings
        mqttBroker = preferences.getString(NVS_KEY_MQTT_BROKER, "");
        mqttPort = preferences.getUShort(NVS_KEY_MQTT_PORT, MQTT_PORT);
        mqttUsername = preferences.getString(NVS_KEY_MQTT_USER, "");
        mqttPassword = preferences.getString(NVS_KEY_MQTT_PASS, "");

        // Load WiFi settings
        wifiSsid = preferences.getString(NVS_KEY_WIFI_SSID, "");
        wifiPassword = preferences.getString(NVS_KEY_WIFI_PASS, "");
    }
}

void DeviceManager::saveConfig() {
    preferences.putString(NVS_KEY_DEVICE_ID, deviceId);
    preferences.putString(NVS_KEY_ROOM_NAME, roomName);
    preferences.putUChar(NVS_KEY_VOLUME_MIC, micVolume);
    preferences.putUChar(NVS_KEY_VOLUME_SPK, speakerVolume);
    preferences.putUChar(NVS_KEY_BRIGHTNESS, brightness);

    if (mqttBroker.length() > 0) {
        preferences.putString(NVS_KEY_MQTT_BROKER, mqttBroker);
    }
    preferences.putUShort(NVS_KEY_MQTT_PORT, mqttPort);

    if (mqttUsername.length() > 0) {
        preferences.putString(NVS_KEY_MQTT_USER, mqttUsername);
    }
    if (mqttPassword.length() > 0) {
        preferences.putString(NVS_KEY_MQTT_PASS, mqttPassword);
    }

    if (wifiSsid.length() > 0) {
        preferences.putString(NVS_KEY_WIFI_SSID, wifiSsid);
    }
    if (wifiPassword.length() > 0) {
        preferences.putString(NVS_KEY_WIFI_PASS, wifiPassword);
    }
}

// Getters
String DeviceManager::getDeviceId() const {
    return deviceId;
}

String DeviceManager::getRoomName() const {
    return roomName;
}

String DeviceManager::getMacAddress() const {
    return macAddress;
}

String DeviceManager::getIPAddress() const {
    if (WiFi.status() == WL_CONNECTED) {
        return WiFi.localIP().toString();
    }
    return "0.0.0.0";
}

uint8_t DeviceManager::getMicVolume() const {
    return micVolume;
}

uint8_t DeviceManager::getSpeakerVolume() const {
    return speakerVolume;
}

uint8_t DeviceManager::getBrightness() const {
    return brightness;
}

float DeviceManager::getWakeWordThreshold() const {
    return wakeWordThreshold;
}

String DeviceManager::getMqttBroker() const {
    return mqttBroker;
}

uint16_t DeviceManager::getMqttPort() const {
    return mqttPort;
}

String DeviceManager::getMqttUsername() const {
    return mqttUsername;
}

String DeviceManager::getMqttPassword() const {
    return mqttPassword;
}

String DeviceManager::getWifiSsid() const {
    return wifiSsid;
}

String DeviceManager::getWifiPassword() const {
    return wifiPassword;
}

bool DeviceManager::isFirstBoot() const {
    return firstBoot;
}

// Setters
void DeviceManager::setRoomName(const String& name) {
    roomName = name;
    preferences.putString(NVS_KEY_ROOM_NAME, roomName);
}

void DeviceManager::setMicVolume(uint8_t volume) {
    micVolume = volume;
    preferences.putUChar(NVS_KEY_VOLUME_MIC, micVolume);
}

void DeviceManager::setSpeakerVolume(uint8_t volume) {
    speakerVolume = volume;
    preferences.putUChar(NVS_KEY_VOLUME_SPK, speakerVolume);
}

void DeviceManager::setBrightness(uint8_t brightnessValue) {
    brightness = brightnessValue;
    preferences.putUChar(NVS_KEY_BRIGHTNESS, brightness);
}

void DeviceManager::setWakeWordThreshold(float threshold) {
    wakeWordThreshold = threshold;
    preferences.putFloat(NVS_KEY_WAKE_THRESHOLD, wakeWordThreshold);
}

void DeviceManager::setMqttBroker(const String& broker) {
    mqttBroker = broker;
    preferences.putString(NVS_KEY_MQTT_BROKER, mqttBroker);
}

void DeviceManager::setMqttPort(uint16_t port) {
    mqttPort = port;
    preferences.putUShort(NVS_KEY_MQTT_PORT, mqttPort);
}

void DeviceManager::setMqttUsername(const String& username) {
    mqttUsername = username;
    preferences.putString(NVS_KEY_MQTT_USER, mqttUsername);
}

void DeviceManager::setMqttPassword(const String& password) {
    mqttPassword = password;
    preferences.putString(NVS_KEY_MQTT_PASS, mqttPassword);
}

void DeviceManager::setWifiSsid(const String& ssid) {
    wifiSsid = ssid;
    preferences.putString(NVS_KEY_WIFI_SSID, wifiSsid);
}

void DeviceManager::setWifiPassword(const String& password) {
    wifiPassword = password;
    preferences.putString(NVS_KEY_WIFI_PASS, wifiPassword);
}

void DeviceManager::factoryReset() {
    Serial.println("[DeviceManager] Performing factory reset...");
    preferences.clear();
    Serial.println("[DeviceManager] All settings cleared. Reboot required.");
}

void DeviceManager::printInfo() const {
    Serial.println("\n========== Device Information ==========");
    Serial.printf("Device ID:     %s\n", deviceId.c_str());
    Serial.printf("Room Name:     %s\n", roomName.c_str());
    Serial.printf("MAC Address:   %s\n", macAddress.c_str());
    Serial.printf("IP Address:    %s\n", getIPAddress().c_str());
    Serial.println("\n===== Audio Settings =====");
    Serial.printf("Mic Volume:    %d\n", micVolume);
    Serial.printf("Speaker Vol:   %d\n", speakerVolume);
    Serial.println("\n===== Display Settings =====");
    Serial.printf("Brightness:    %d\n", brightness);
    Serial.println("\n===== Network Settings =====");
    Serial.printf("WiFi SSID:     %s\n", wifiSsid.length() > 0 ? wifiSsid.c_str() : "(not configured)");
    Serial.printf("MQTT Broker:   %s:%d\n",
                  mqttBroker.length() > 0 ? mqttBroker.c_str() : "(not configured)",
                  mqttPort);
    Serial.println("========================================\n");
}
