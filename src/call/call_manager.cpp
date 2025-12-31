#include "call_manager.h"
#include "../network/mqtt_client.h"
#include "../network/mqtt_topics.h"
#include "../audio/audio_pipeline.h"
#include "config.h"

CallManager::CallManager(StateMachine* sm,
                         DeviceRegistry* dr,
                         MqttClient* mqtt,
                         AudioPipeline* audio)
    : stateMachine(sm),
      deviceRegistry(dr),
      mqttClient(mqtt),
      audioPipeline(audio),
      currentSession(nullptr),
      callStateTime(0) {
}

CallManager::~CallManager() {
    if (currentSession) {
        delete currentSession;
    }
}

bool CallManager::begin() {
    Serial.println("[CallManager] Initializing...");

    currentSession = new CallSession();

    Serial.println("[CallManager] Initialized successfully");
    return true;
}

void CallManager::process() {
    if (!currentSession || !stateMachine) {
        return;
    }

    // Check for timeouts
    checkTimeout();

    // Sync call session state with app state (if needed)
    // This ensures consistency between StateMachine and CallSession
}

bool CallManager::initiateCall(const String& roomName) {
    if (!deviceRegistry || !mqttClient || !audioPipeline) {
        Serial.println("[CallManager] ERROR: Not fully initialized");
        return false;
    }

    // Check if already in call
    if (currentSession->isInProgress()) {
        Serial.println("[CallManager] ERROR: Call already in progress");
        return false;
    }

    // Look up device by room name
    RemoteDevice* device = deviceRegistry->getDeviceByRoom(roomName);
    if (!device) {
        Serial.printf("[CallManager] ERROR: Device not found: %s\n", roomName.c_str());
        return false;
    }

    if (!device->online) {
        Serial.printf("[CallManager] ERROR: Device offline: %s\n", roomName.c_str());
        return false;
    }

    // Generate session ID
    uint32_t sessionId = generateSessionId();

    // Start outgoing call session
    currentSession->startOutgoing(device->deviceId, device->roomName, sessionId);

    // Update app state
    stateMachine->setState(AppState::CALLING);
    callStateTime = millis();

    // Send MQTT call request
    if (!sendCallRequest(device->deviceId, sessionId)) {
        Serial.println("[CallManager] ERROR: Failed to send call request");
        currentSession->fail("Failed to send request");
        stateMachine->setState(AppState::IDLE);
        return false;
    }

    // Update HA sensors
    updateHASensors();

    Serial.printf("[CallManager] Call initiated to %s (session %u)\n", roomName.c_str(), sessionId);
    return true;
}

void CallManager::handleIncomingCall(const String& fromDeviceId,
                                      const String& fromRoom,
                                      uint32_t sessionId) {
    Serial.printf("[CallManager] Incoming call from %s (%s) [Session: %u]\n",
                  fromRoom.c_str(), fromDeviceId.c_str(), sessionId);

    // Check if already in call
    if (currentSession->isInProgress()) {
        Serial.println("[CallManager] WARNING: Already in call, rejecting");
        sendCallReject(fromDeviceId, sessionId, "Busy");
        return;
    }

    // Start incoming call session
    currentSession->startIncoming(fromDeviceId, fromRoom, sessionId);

    // Update app state
    stateMachine->setState(AppState::RINGING);
    callStateTime = millis();

    // Update HA sensors
    updateHASensors();

    // TODO: Play ringtone
    // TODO: Update UI to show incoming call screen

    Serial.println("[CallManager] Ringing... waiting for user action");
}

void CallManager::handleCallAccepted(uint32_t sessionId,
                                      const IPAddress& remoteIp,
                                      uint16_t remotePort) {
    // Verify session ID matches
    if (sessionId != currentSession->getSessionId()) {
        Serial.printf("[CallManager] WARNING: Session ID mismatch: %u != %u\n",
                      sessionId, currentSession->getSessionId());
        return;
    }

    Serial.printf("[CallManager] Call accepted: %s:%d\n", remoteIp.toString().c_str(), remotePort);

    // Get local UDP port
    uint16_t localPort = getLocalUdpPort();

    // Activate call session
    currentSession->activate(remoteIp, remotePort, localPort);

    // Start audio pipeline
    if (!startAudio()) {
        Serial.println("[CallManager] ERROR: Failed to start audio");
        currentSession->fail("Audio startup failed");
        hangupCall();
        return;
    }

    // Update app state
    stateMachine->setState(AppState::ACTIVE_CALL);
    callStateTime = millis();

    // Update HA sensors
    updateHASensors();

    Serial.println("[CallManager] Call is now active!");
}

void CallManager::handleCallRejected(uint32_t sessionId, const String& reason) {
    // Verify session ID
    if (sessionId != currentSession->getSessionId()) {
        return;
    }

    Serial.printf("[CallManager] Call rejected: %s\n", reason.c_str());

    currentSession->reject(reason);
    stateMachine->setState(AppState::IDLE);

    // Update HA sensors
    updateHASensors();
}

void CallManager::handleCallHangup(uint32_t sessionId) {
    // Verify session ID
    if (sessionId != currentSession->getSessionId()) {
        return;
    }

    Serial.println("[CallManager] Remote hung up");

    hangupCall();
}

bool CallManager::acceptCall() {
    if (currentSession->getState() != CallState::RINGING) {
        Serial.println("[CallManager] ERROR: No incoming call to accept");
        return false;
    }

    Serial.println("[CallManager] Accepting call...");

    // Get local UDP port
    uint16_t localPort = getLocalUdpPort();

    // Get remote device info from registry
    RemoteDevice* device = deviceRegistry->getDevice(currentSession->getRemoteDeviceId());
    if (!device) {
        Serial.println("[CallManager] ERROR: Remote device not found");
        rejectCall("Device not found");
        return false;
    }

    IPAddress remoteIp;
    remoteIp.fromString(device->ipAddress);

    // Determine remote UDP port (use base port + offset based on device)
    uint16_t remotePort = getLocalUdpPort();  // Remote will use similar logic

    // Accept in session
    currentSession->accept(remoteIp, remotePort);

    // Send MQTT accept message
    if (!sendCallAccept(currentSession->getRemoteDeviceId(), currentSession->getSessionId())) {
        Serial.println("[CallManager] ERROR: Failed to send accept message");
        return false;
    }

    // Activate call
    currentSession->activate(remoteIp, remotePort, localPort);

    // Start audio
    if (!startAudio()) {
        Serial.println("[CallManager] ERROR: Failed to start audio");
        rejectCall("Audio failed");
        return false;
    }

    // Update app state
    stateMachine->setState(AppState::ACTIVE_CALL);
    callStateTime = millis();

    // Update HA sensors
    updateHASensors();

    Serial.println("[CallManager] Call accepted and active!");
    return true;
}

bool CallManager::rejectCall(const String& reason) {
    if (currentSession->getState() != CallState::RINGING) {
        Serial.println("[CallManager] ERROR: No incoming call to reject");
        return false;
    }

    Serial.printf("[CallManager] Rejecting call: %s\n", reason.c_str());

    // Send MQTT reject message
    sendCallReject(currentSession->getRemoteDeviceId(),
                   currentSession->getSessionId(),
                   reason);

    currentSession->reject(reason);
    stateMachine->setState(AppState::IDLE);

    // Update HA sensors
    updateHASensors();

    return true;
}

bool CallManager::hangupCall() {
    if (!currentSession->isInProgress()) {
        Serial.println("[CallManager] ERROR: No call to hang up");
        return false;
    }

    Serial.println("[CallManager] Hanging up call...");

    // Update state
    stateMachine->setState(AppState::HANGING_UP);
    currentSession->hangup();

    // Send MQTT hangup if we have a remote device
    if (currentSession->getRemoteDeviceId().length() > 0) {
        sendCallHangup(currentSession->getRemoteDeviceId(), currentSession->getSessionId());
    }

    // Stop audio
    stopAudio();

    // Reset session
    currentSession->reset();

    // Return to idle
    stateMachine->setState(AppState::IDLE);
    callStateTime = millis();

    // Update HA sensors
    updateHASensors();

    Serial.println("[CallManager] Call ended");
    return true;
}

CallSession* CallManager::getCurrentSession() {
    return currentSession;
}

bool CallManager::isInCall() const {
    return currentSession && currentSession->isActive();
}

bool CallManager::isCallInProgress() const {
    return currentSession && currentSession->isInProgress();
}

uint32_t CallManager::generateSessionId() {
    // Simple session ID: timestamp + random
    return millis() + random(1000, 9999);
}

bool CallManager::startAudio() {
    if (!audioPipeline) {
        return false;
    }

    IPAddress remoteIp = currentSession->getRemoteIp();
    uint16_t remotePort = currentSession->getRemotePort();
    uint16_t localPort = currentSession->getLocalPort();
    uint32_t sessionId = currentSession->getSessionId();

    Serial.printf("[CallManager] Starting audio: %s:%d (local %d)\n",
                  remoteIp.toString().c_str(), remotePort, localPort);

    return audioPipeline->startCall(remoteIp, remotePort, localPort, sessionId);
}

void CallManager::stopAudio() {
    if (audioPipeline) {
        audioPipeline->endCall();
    }
}

void CallManager::checkTimeout() {
    if (!currentSession->isInProgress()) {
        return;
    }

    unsigned long elapsed = millis() - callStateTime;
    CallState state = currentSession->getState();

    // Check for ring timeout (30 seconds)
    if ((state == CallState::INITIATING || state == CallState::RINGING) &&
        elapsed > CALL_RING_TIMEOUT_MS) {
        Serial.println("[CallManager] Call timeout");

        if (state == CallState::INITIATING) {
            sendCallHangup(currentSession->getRemoteDeviceId(), currentSession->getSessionId());
        }

        currentSession->fail("Timeout");
        stateMachine->setState(AppState::IDLE);
        updateHASensors();
    }
}

bool CallManager::sendCallRequest(const String& targetDeviceId, uint32_t sessionId) {
    if (mqttClient) {
        mqttClient->sendCallRequest(targetDeviceId, String(sessionId));
        return true;
    }
    return false;
}

bool CallManager::sendCallAccept(const String& targetDeviceId, uint32_t sessionId) {
    if (mqttClient) {
        uint16_t localPort = getLocalUdpPort();
        mqttClient->sendCallAccept(targetDeviceId, String(sessionId), localPort);
        return true;
    }
    return false;
}

bool CallManager::sendCallReject(const String& targetDeviceId, uint32_t sessionId, const String& reason) {
    if (mqttClient) {
        mqttClient->sendCallReject(targetDeviceId, String(sessionId), reason);
        return true;
    }
    return false;
}

bool CallManager::sendCallHangup(const String& targetDeviceId, uint32_t sessionId) {
    if (mqttClient) {
        mqttClient->sendCallHangup(targetDeviceId, String(sessionId));
        return true;
    }
    return false;
}

void CallManager::updateHASensors() {
    if (!mqttClient) {
        return;
    }

    // Update call status sensor
    if (currentSession->isActive()) {
        mqttClient->publishCallStatus(MqttTopics::CALL_STATUS_ACTIVE);

        // Update call info with caller room and duration
        mqttClient->publishCallInfo(
            currentSession->getRemoteRoom(),
            String(currentSession->getSessionId()),
            currentSession->getDuration()
        );
    } else if (currentSession->getState() == CallState::RINGING) {
        mqttClient->publishCallStatus(MqttTopics::CALL_STATUS_RINGING);

        // Update call info with caller room
        mqttClient->publishCallInfo(
            currentSession->getRemoteRoom(),
            String(currentSession->getSessionId()),
            0
        );
    } else {
        mqttClient->publishCallStatus(MqttTopics::CALL_STATUS_IDLE);

        // Clear call info
        mqttClient->publishCallInfo("None", "", 0);
    }
}

uint16_t CallManager::getLocalUdpPort() {
    // Use base port from config
    // In a more complex system, could allocate dynamic ports
    return UDP_AUDIO_PORT_BASE;
}
