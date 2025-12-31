#include "ai_manager.h"
#include "config.h"

AIManager::AIManager(OllamaClient* ollama, TTSEngine* tts,
                     UIManager* ui, MqttClient* mqtt, StateMachine* sm)
    : ollamaClient(ollama),
      ttsEngine(tts),
      uiManager(ui),
      mqttClient(mqtt),
      stateMachine(sm),
      state(IDLE),
      responseMode(ALL),
      responseTime(0),
      queryInProgress(false),
      ttsInProgress(false) {
}

AIManager::~AIManager() {
    cancel();
}

bool AIManager::begin() {
    if (!ollamaClient || !ttsEngine) {
        Serial.println("[AI] ERROR: Missing required components");
        return false;
    }

    Serial.println("[AI] AI Manager initialized");
    Serial.printf("[AI] Response mode: 0x%02X\n", responseMode);

    return true;
}

void AIManager::processQuery(const String& query, bool includeContext) {
    if (query.isEmpty()) {
        Serial.println("[AI] ERROR: Empty query");
        return;
    }

    if (state != IDLE) {
        Serial.println("[AI] Busy, canceling current operation");
        cancel();
    }

    startQuery(query, includeContext);
}

void AIManager::update() {
    switch (state) {
        case IDLE:
            // Nothing to do
            break;

        case QUERYING:
            // Query is blocking, handled in startQuery
            break;

        case DELIVERING:
            // Check if TTS is still playing
            if (ttsInProgress && !ttsEngine->isSpeaking()) {
                Serial.println("[AI] TTS playback complete");
                ttsInProgress = false;
                setState(IDLE);
                updateStateMachine();
            }
            break;

        case ERROR_STATE:
            // Stay in error state until reset or new query
            break;
    }
}

void AIManager::cancel() {
    Serial.println("[AI] Canceling operation");

    if (ttsEngine && ttsEngine->isSpeaking()) {
        ttsEngine->stop();
    }

    queryInProgress = false;
    ttsInProgress = false;

    setState(IDLE);
    updateStateMachine();
}

void AIManager::setResponseMode(uint8_t mode) {
    responseMode = mode;
    Serial.printf("[AI] Response mode set to: 0x%02X\n", responseMode);
}

uint8_t AIManager::getResponseMode() const {
    return responseMode;
}

AIManager::State AIManager::getState() const {
    return state;
}

String AIManager::getLastQuery() const {
    return currentQuery;
}

String AIManager::getLastResponse() const {
    return currentResponse;
}

String AIManager::getError() const {
    return errorMessage;
}

uint32_t AIManager::getResponseTime() const {
    return responseTime;
}

void AIManager::reset() {
    cancel();
    currentQuery = "";
    currentResponse = "";
    errorMessage = "";
    responseTime = 0;
}

void AIManager::startQuery(const String& query, bool includeContext) {
    Serial.printf("[AI] Starting query: %s\n", query.c_str());

    currentQuery = query;
    queryStartTime = millis();
    setState(QUERYING);
    updateStateMachine();

    // Send query to Ollama (blocking call)
    OllamaClient::Query q(query, includeContext);
    OllamaClient::Response response = ollamaClient->query(q);

    responseTime = millis() - queryStartTime;

    if (response.success) {
        handleQueryComplete(response);
    } else {
        handleError(response.error);
    }
}

void AIManager::handleQueryComplete(const OllamaClient::Response& response) {
    Serial.printf("[AI] Query complete (%lums)\n", responseTime);

    currentResponse = response.text;

    // Deliver response via configured modes
    deliverResponse(response.text);

    // Publish to Home Assistant
    if (responseMode & MQTT_ONLY) {
        publishToHA(currentQuery, response.text);
    }

    setState(DELIVERING);

    // If TTS is enabled, we'll stay in DELIVERING until TTS completes
    if (!(responseMode & TTS_ONLY) || !ttsEngine->isSpeaking()) {
        // No TTS or TTS not speaking, go back to idle
        setState(IDLE);
        updateStateMachine();
    }
}

void AIManager::deliverResponse(const String& response) {
    // Display on screen
    if (responseMode & DISPLAY_ONLY) {
        Serial.println("[AI] Displaying response on screen");
        // UI will be updated via state machine callback
    }

    // Speak via TTS
    if (responseMode & TTS_ONLY) {
        Serial.println("[AI] Speaking response via TTS");

        if (ttsEngine && ttsEngine->speak(response)) {
            ttsInProgress = true;
        } else {
            Serial.println("[AI] WARNING: TTS failed");
        }
    }
}

void AIManager::publishToHA(const String& query, const String& response) {
    if (!mqttClient) {
        return;
    }

    Serial.println("[AI] Publishing to Home Assistant");

    // Publish query
    mqttClient->publishAIQuery(query);

    // Publish response
    mqttClient->publishAIResponse(query, response, responseTime);
}

void AIManager::handleError(const String& error) {
    Serial.printf("[AI] ERROR: %s\n", error.c_str());

    errorMessage = error;
    setState(ERROR_STATE);
    updateStateMachine();

    // Display error on screen
    if (uiManager) {
        // Error message will be shown via state machine callback
    }

    // Speak error if TTS enabled
    if ((responseMode & TTS_ONLY) && ttsEngine) {
        String errorMsg = "Sorry, I encountered an error: " + error;
        ttsEngine->speak(errorMsg);
    }

    // Auto-reset after 3 seconds
    delay(3000);
    setState(IDLE);
    updateStateMachine();
}

void AIManager::setState(State newState) {
    if (state != newState) {
        Serial.printf("[AI] State: %d -> %d\n", state, newState);
        state = newState;
    }
}

void AIManager::updateStateMachine() {
    if (!stateMachine) {
        return;
    }

    // Update global state machine based on AI state
    switch (state) {
        case QUERYING:
            stateMachine->setState(AppState::AI_QUERY);
            break;

        case DELIVERING:
            stateMachine->setState(AppState::AI_RESPONSE);
            break;

        case ERROR_STATE:
        case IDLE:
            if (stateMachine->getState() == AppState::AI_QUERY ||
                stateMachine->getState() == AppState::AI_RESPONSE) {
                stateMachine->setState(AppState::IDLE);
            }
            break;
    }
}
