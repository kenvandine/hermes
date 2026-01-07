#include "state_machine.h"

StateMachine::StateMachine()
    : currentState(AppState::IDLE) {
}

StateMachine::~StateMachine() {
}

AppState StateMachine::getState() const {
    return currentState;
}

bool StateMachine::setState(AppState newState) {
    if (currentState == newState) {
        return true;  // Already in this state
    }

    // Validate transition
    if (!isValidTransition(currentState, newState)) {
        Serial.printf("[StateMachine] WARNING: Invalid transition %s -> %s\n",
                      stateToString(currentState).c_str(),
                      stateToString(newState).c_str());
        return false;
    }

    AppState oldState = currentState;
    currentState = newState;

    Serial.printf("[StateMachine] State: %s -> %s\n",
                  stateToString(oldState).c_str(),
                  stateToString(newState).c_str());

    // Notify callback
    notifyStateChange(oldState, newState);

    return true;
}

bool StateMachine::isIdle() const {
    return currentState == AppState::IDLE;
}

bool StateMachine::isListening() const {
    return currentState == AppState::LISTENING;
}

bool StateMachine::isCalling() const {
    return currentState == AppState::CALLING;
}

bool StateMachine::isRinging() const {
    return currentState == AppState::RINGING;
}

bool StateMachine::isInCall() const {
    return currentState == AppState::ACTIVE_CALL;
}

bool StateMachine::isHangingUp() const {
    return currentState == AppState::HANGING_UP;
}

bool StateMachine::isError() const {
    return currentState == AppState::ERROR;
}

void StateMachine::onStateChange(StateChangeCallback callback) {
    stateChangeCallback = callback;
}

String StateMachine::getStateString() const {
    return stateToString(currentState);
}

String StateMachine::stateToString(AppState state) {
    switch (state) {
        case AppState::IDLE:        return "IDLE";
        case AppState::LISTENING:   return "LISTENING";
        case AppState::CALLING:     return "CALLING";
        case AppState::RINGING:     return "RINGING";
        case AppState::ACTIVE_CALL: return "ACTIVE_CALL";
        case AppState::HANGING_UP:  return "HANGING_UP";
        case AppState::AI_QUERY:    return "AI_QUERY";
        case AppState::AI_RESPONSE: return "AI_RESPONSE";
        case AppState::ERROR:       return "ERROR";
        default:                    return "UNKNOWN";
    }
}

bool StateMachine::isValidTransition(AppState from, AppState to) const {
    // Define valid state transitions

    switch (from) {
        case AppState::IDLE:
            return (to == AppState::LISTENING ||
                    to == AppState::CALLING ||
                    to == AppState::RINGING ||
                    to == AppState::AI_QUERY ||
                    to == AppState::ERROR);

        case AppState::LISTENING:
            return (to == AppState::IDLE ||
                    to == AppState::CALLING ||
                    to == AppState::AI_QUERY ||
                    to == AppState::ERROR);

        case AppState::CALLING:
            return (to == AppState::ACTIVE_CALL ||
                    to == AppState::IDLE ||
                    to == AppState::ERROR);

        case AppState::RINGING:
            return (to == AppState::ACTIVE_CALL ||
                    to == AppState::IDLE ||
                    to == AppState::ERROR);

        case AppState::ACTIVE_CALL:
            return (to == AppState::HANGING_UP ||
                    to == AppState::ERROR);

        case AppState::HANGING_UP:
            return (to == AppState::IDLE ||
                    to == AppState::ERROR);

        case AppState::AI_QUERY:
            return (to == AppState::AI_RESPONSE ||
                    to == AppState::LISTENING ||  // Allow wake word to interrupt
                    to == AppState::IDLE ||
                    to == AppState::ERROR);

        case AppState::AI_RESPONSE:
            return (to == AppState::LISTENING ||  // Allow wake word to interrupt
                    to == AppState::IDLE ||
                    to == AppState::ERROR);

        case AppState::ERROR:
            return (to == AppState::IDLE);

        default:
            return false;
    }
}

void StateMachine::notifyStateChange(AppState oldState, AppState newState) {
    if (stateChangeCallback) {
        stateChangeCallback(oldState, newState);
    }
}
