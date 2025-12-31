#include "call_session.h"

CallSession::CallSession()
    : state(CallState::IDLE),
      direction(CallDirection::NONE),
      sessionId(0),
      remotePort(0),
      localPort(0),
      startTime(0),
      connectTime(0),
      endTime(0) {
}

CallSession::~CallSession() {
}

void CallSession::startOutgoing(const String& deviceId, const String& room, uint32_t sessId) {
    reset();

    state = CallState::INITIATING;
    direction = CallDirection::OUTGOING;
    sessionId = sessId;
    remoteDeviceId = deviceId;
    remoteRoom = room;
    startTime = millis();

    Serial.printf("[CallSession] Outgoing call started: %s (%s) [Session: %u]\n",
                  room.c_str(), deviceId.c_str(), sessionId);
}

void CallSession::startIncoming(const String& deviceId, const String& room, uint32_t sessId) {
    reset();

    state = CallState::RINGING;
    direction = CallDirection::INCOMING;
    sessionId = sessId;
    remoteDeviceId = deviceId;
    remoteRoom = room;
    startTime = millis();

    Serial.printf("[CallSession] Incoming call: %s (%s) [Session: %u]\n",
                  room.c_str(), deviceId.c_str(), sessionId);
}

void CallSession::accept(const IPAddress& ip, uint16_t port) {
    if (state != CallState::RINGING) {
        Serial.println("[CallSession] WARNING: Cannot accept - not ringing");
        return;
    }

    state = CallState::CONNECTING;
    remoteIp = ip;
    remotePort = port;

    Serial.printf("[CallSession] Call accepted: %s:%d\n", ip.toString().c_str(), port);
}

void CallSession::reject(const String& reason) {
    Serial.printf("[CallSession] Call rejected: %s\n", reason.c_str());

    failureReason = reason;
    state = CallState::FAILED;
    endTime = millis();
}

void CallSession::activate(const IPAddress& ip, uint16_t remote, uint16_t local) {
    if (state != CallState::INITIATING && state != CallState::CONNECTING) {
        Serial.println("[CallSession] WARNING: Cannot activate from current state");
    }

    state = CallState::ACTIVE;
    remoteIp = ip;
    remotePort = remote;
    localPort = local;
    connectTime = millis();

    Serial.printf("[CallSession] Call active: %s:%d (local port %d)\n",
                  ip.toString().c_str(), remote, local);
}

void CallSession::hangup() {
    if (state == CallState::IDLE) {
        return;
    }

    Serial.println("[CallSession] Call hanging up");

    state = CallState::HANGING_UP;
    endTime = millis();
}

void CallSession::fail(const String& reason) {
    Serial.printf("[CallSession] Call failed: %s\n", reason.c_str());

    failureReason = reason;
    state = CallState::FAILED;
    endTime = millis();
}

void CallSession::reset() {
    state = CallState::IDLE;
    direction = CallDirection::NONE;
    sessionId = 0;
    remoteDeviceId = "";
    remoteRoom = "";
    remoteIp = IPAddress(0, 0, 0, 0);
    remotePort = 0;
    localPort = 0;
    startTime = 0;
    connectTime = 0;
    endTime = 0;
    failureReason = "";
}

CallState CallSession::getState() const {
    return state;
}

CallDirection CallSession::getDirection() const {
    return direction;
}

uint32_t CallSession::getSessionId() const {
    return sessionId;
}

String CallSession::getRemoteDeviceId() const {
    return remoteDeviceId;
}

String CallSession::getRemoteRoom() const {
    return remoteRoom;
}

IPAddress CallSession::getRemoteIp() const {
    return remoteIp;
}

uint16_t CallSession::getRemotePort() const {
    return remotePort;
}

uint16_t CallSession::getLocalPort() const {
    return localPort;
}

unsigned long CallSession::getDuration() const {
    if (state == CallState::IDLE || connectTime == 0) {
        return 0;
    }

    unsigned long endTimeToUse = (endTime > 0) ? endTime : millis();
    return endTimeToUse - connectTime;
}

bool CallSession::isActive() const {
    return state == CallState::ACTIVE;
}

bool CallSession::isInProgress() const {
    return state != CallState::IDLE && state != CallState::FAILED;
}

String CallSession::getStateString() const {
    switch (state) {
        case CallState::IDLE:        return "IDLE";
        case CallState::INITIATING:  return "INITIATING";
        case CallState::RINGING:     return "RINGING";
        case CallState::CONNECTING:  return "CONNECTING";
        case CallState::ACTIVE:      return "ACTIVE";
        case CallState::HANGING_UP:  return "HANGING_UP";
        case CallState::FAILED:      return "FAILED";
        default:                     return "UNKNOWN";
    }
}

void CallSession::printInfo() const {
    Serial.println("\n========== Call Session Info ==========");
    Serial.printf("State: %s\n", getStateString().c_str());
    Serial.printf("Direction: %s\n", direction == CallDirection::OUTGOING ? "OUTGOING" :
                                      direction == CallDirection::INCOMING ? "INCOMING" : "NONE");
    Serial.printf("Session ID: %u\n", sessionId);
    Serial.printf("Remote: %s (%s)\n", remoteRoom.c_str(), remoteDeviceId.c_str());

    if (remoteIp != IPAddress(0, 0, 0, 0)) {
        Serial.printf("Remote IP: %s:%d\n", remoteIp.toString().c_str(), remotePort);
    }

    if (localPort > 0) {
        Serial.printf("Local Port: %d\n", localPort);
    }

    if (state == CallState::ACTIVE && connectTime > 0) {
        Serial.printf("Duration: %lu seconds\n", getDuration() / 1000);
    }

    if (failureReason.length() > 0) {
        Serial.printf("Failure Reason: %s\n", failureReason.c_str());
    }

    Serial.println("======================================\n");
}
