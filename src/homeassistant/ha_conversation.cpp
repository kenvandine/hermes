#include "homeassistant/ha_conversation.h"

HAConversation::HAConversation() : initialized_(false) {
}

HAConversation::~HAConversation() {
    http_.end();
}

bool HAConversation::begin(const String& host, const String& token) {
    if (host.isEmpty() || token.isEmpty()) {
        Serial.println("[HA] Error: Host or token is empty");
        return false;
    }

    // Remove trailing slash from host if present
    haHost_ = host;
    if (haHost_.endsWith("/")) {
        haHost_.remove(haHost_.length() - 1);
    }

    authToken_ = token;
    initialized_ = true;

    Serial.printf("[HA] Initialized with host: %s\n", haHost_.c_str());
    return true;
}

HAConversation::Response HAConversation::process(const String& text, const String& conversationId) {
    Response result;

    if (!initialized_) {
        result.error = "Not initialized";
        Serial.println("[HA] Error: Client not initialized");
        return result;
    }

    if (text.isEmpty()) {
        result.error = "Empty command text";
        Serial.println("[HA] Error: Empty command text");
        return result;
    }

    // Build JSON request
    DynamicJsonDocument doc(512);
    doc["text"] = text;
    doc["language"] = "en";

    if (!conversationId.isEmpty()) {
        doc["conversation_id"] = conversationId;
    }

    String payload;
    serializeJson(doc, payload);

    Serial.printf("[HA] Sending command: %s\n", text.c_str());

    // POST to /api/conversation/process
    String endpoint = haHost_ + "/api/conversation/process";
    String responseText;

    if (!httpPost(endpoint, payload, responseText)) {
        result.error = "HTTP request failed";
        Serial.println("[HA] HTTP request failed");
        return result;
    }

    // Parse response
    parseResponse(responseText, result);

    if (result.success) {
        Serial.printf("[HA] Success: %s\n", result.speech.c_str());
    } else {
        Serial.printf("[HA] Failed: %s\n", result.error.c_str());
    }

    return result;
}

bool HAConversation::testConnection() {
    if (!initialized_) {
        Serial.println("[HA] Error: Client not initialized");
        return false;
    }

    // Simple test: GET /api/ to verify HA is reachable
    String endpoint = haHost_ + "/api/";

    http_.begin(endpoint);
    http_.addHeader("Authorization", "Bearer " + authToken_);
    http_.setTimeout(5000);  // 5 second timeout

    int httpCode = http_.GET();
    http_.end();

    if (httpCode == 200) {
        Serial.println("[HA] Connection test: OK");
        return true;
    } else {
        Serial.printf("[HA] Connection test failed: HTTP %d\n", httpCode);
        return false;
    }
}

bool HAConversation::httpPost(const String& endpoint, const String& payload, String& response) {
    http_.begin(endpoint);
    http_.addHeader("Content-Type", "application/json");
    http_.addHeader("Authorization", "Bearer " + authToken_);
    http_.setTimeout(10000);  // 10 second timeout for commands

    int httpCode = http_.POST(payload);

    if (httpCode == 200) {
        response = http_.getString();
        http_.end();
        return true;
    } else {
        Serial.printf("[HA] HTTP POST failed: %d\n", httpCode);
        if (httpCode > 0) {
            // Got a response, log it
            String errorBody = http_.getString();
            Serial.printf("[HA] Error response: %s\n", errorBody.c_str());
        }
        http_.end();
        return false;
    }
}

void HAConversation::parseResponse(const String& json, Response& result) {
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, json);

    if (error) {
        result.success = false;
        result.error = "JSON parse error: " + String(error.c_str());
        Serial.printf("[HA] JSON parse error: %s\n", error.c_str());
        return;
    }

    // Extract response fields
    // Response structure: response.speech.plain.speech
    if (doc.containsKey("response")) {
        JsonObject response = doc["response"];

        if (response.containsKey("speech") && response["speech"].containsKey("plain")) {
            JsonObject plain = response["speech"]["plain"];
            if (plain.containsKey("speech")) {
                result.speech = plain["speech"].as<String>();
            }
        }

        if (response.containsKey("response_type")) {
            result.responseType = response["response_type"].as<String>();
        }
    }

    // Extract conversation ID
    if (doc.containsKey("conversation_id")) {
        result.conversationId = doc["conversation_id"].as<String>();
    }

    // Check if we got a valid response
    if (!result.speech.isEmpty()) {
        result.success = true;
    } else {
        result.success = false;
        result.error = "No speech response from HA";

        // Log the response for debugging
        Serial.println("[HA] Response JSON:");
        serializeJsonPretty(doc, Serial);
        Serial.println();
    }
}
