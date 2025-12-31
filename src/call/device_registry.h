#ifndef DEVICE_REGISTRY_H
#define DEVICE_REGISTRY_H

#include <Arduino.h>
#include <vector>
#include <map>

/**
 * RemoteDevice
 *
 * Represents a remote intercom device on the network
 */
struct RemoteDevice {
    String deviceId;
    String roomName;
    String ipAddress;
    unsigned long lastSeen;  // millis() timestamp
    bool online;

    RemoteDevice()
        : lastSeen(0), online(false) {}

    RemoteDevice(const String& id, const String& room, const String& ip)
        : deviceId(id), roomName(room), ipAddress(ip),
          lastSeen(millis()), online(true) {}
};

/**
 * DeviceRegistry
 *
 * Maintains a list of discovered intercom devices on the network.
 * - Tracks device presence (online/offline)
 * - Handles device timeouts
 * - Provides device lookup by ID or room name
 */
class DeviceRegistry {
public:
    DeviceRegistry();
    ~DeviceRegistry();

    /**
     * Add or update a device in the registry
     * Called when device announces presence via MQTT
     */
    void updateDevice(const String& deviceId, const String& roomName, const String& ipAddress);

    /**
     * Mark device as offline
     */
    void removeDevice(const String& deviceId);

    /**
     * Check for device timeouts and mark as offline
     * Should be called periodically (e.g., every 10 seconds)
     */
    void checkTimeouts();

    /**
     * Get device by device ID
     */
    RemoteDevice* getDevice(const String& deviceId);

    /**
     * Get device by room name (case insensitive)
     */
    RemoteDevice* getDeviceByRoom(const String& roomName);

    /**
     * Get list of all online devices
     */
    std::vector<RemoteDevice*> getOnlineDevices();

    /**
     * Get count of online devices
     */
    size_t getOnlineCount() const;

    /**
     * Clear all devices
     */
    void clear();

    /**
     * Print registry to serial for debugging
     */
    void printRegistry() const;

private:
    std::map<String, RemoteDevice> devices;
    unsigned long timeoutMs;  // Timeout duration (from config)
};

#endif // DEVICE_REGISTRY_H
