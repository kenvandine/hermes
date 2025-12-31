#include "device_registry.h"
#include "config.h"

DeviceRegistry::DeviceRegistry()
    : timeoutMs(DEVICE_TIMEOUT_MS) {
}

DeviceRegistry::~DeviceRegistry() {
}

void DeviceRegistry::updateDevice(const String& deviceId, const String& roomName, const String& ipAddress) {
    auto it = devices.find(deviceId);

    if (it != devices.end()) {
        // Update existing device
        it->second.roomName = roomName;
        it->second.ipAddress = ipAddress;
        it->second.lastSeen = millis();

        if (!it->second.online) {
            Serial.printf("[DeviceRegistry] Device %s (%s) is back online\n",
                          deviceId.c_str(), roomName.c_str());
            it->second.online = true;
        }
    } else {
        // Add new device
        Serial.printf("[DeviceRegistry] New device discovered: %s (%s) at %s\n",
                      deviceId.c_str(), roomName.c_str(), ipAddress.c_str());

        devices[deviceId] = RemoteDevice(deviceId, roomName, ipAddress);
    }
}

void DeviceRegistry::removeDevice(const String& deviceId) {
    auto it = devices.find(deviceId);

    if (it != devices.end()) {
        Serial.printf("[DeviceRegistry] Device removed: %s (%s)\n",
                      deviceId.c_str(), it->second.roomName.c_str());
        devices.erase(it);
    }
}

void DeviceRegistry::checkTimeouts() {
    unsigned long now = millis();

    for (auto& pair : devices) {
        RemoteDevice& device = pair.second;

        if (device.online && (now - device.lastSeen > timeoutMs)) {
            Serial.printf("[DeviceRegistry] Device timeout: %s (%s)\n",
                          device.deviceId.c_str(), device.roomName.c_str());
            device.online = false;
        }
    }
}

RemoteDevice* DeviceRegistry::getDevice(const String& deviceId) {
    auto it = devices.find(deviceId);

    if (it != devices.end()) {
        return &(it->second);
    }

    return nullptr;
}

RemoteDevice* DeviceRegistry::getDeviceByRoom(const String& roomName) {
    // Case-insensitive search
    String searchName = roomName;
    searchName.toLowerCase();

    for (auto& pair : devices) {
        String deviceRoom = pair.second.roomName;
        deviceRoom.toLowerCase();

        if (deviceRoom == searchName && pair.second.online) {
            return &(pair.second);
        }
    }

    return nullptr;
}

std::vector<RemoteDevice*> DeviceRegistry::getOnlineDevices() {
    std::vector<RemoteDevice*> onlineDevices;

    for (auto& pair : devices) {
        if (pair.second.online) {
            onlineDevices.push_back(&(pair.second));
        }
    }

    return onlineDevices;
}

size_t DeviceRegistry::getOnlineCount() const {
    size_t count = 0;

    for (const auto& pair : devices) {
        if (pair.second.online) {
            count++;
        }
    }

    return count;
}

void DeviceRegistry::clear() {
    devices.clear();
    Serial.println("[DeviceRegistry] Registry cleared");
}

void DeviceRegistry::printRegistry() const {
    Serial.println("\n========== Device Registry ==========");
    Serial.printf("Total devices: %d\n", devices.size());
    Serial.printf("Online devices: %d\n", getOnlineCount());

    if (devices.empty()) {
        Serial.println("(No devices registered)");
    } else {
        Serial.println("\nDevice List:");
        for (const auto& pair : devices) {
            const RemoteDevice& device = pair.second;
            Serial.printf("  [%s] %s (%s) - %s @ %s\n",
                          device.online ? "ONLINE" : "OFFLINE",
                          device.deviceId.c_str(),
                          device.roomName.c_str(),
                          device.ipAddress.c_str(),
                          device.online ? String(millis() - device.lastSeen).c_str() : "N/A");
        }
    }

    Serial.println("=====================================\n");
}
