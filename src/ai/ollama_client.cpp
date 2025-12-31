#include "ollama_client.h"
#include "config.h"

OllamaClient::OllamaClient()
    : serverPort(11434),
      modelName("llama3.2:3b"),
      systemPrompt("You are a helpful home assistant. Keep responses under 50 words."),
      timeout(10000),
      maxTokens(200),
      temperature(0.7) {
}

OllamaClient::~OllamaClient() {
    http.end();
}

bool OllamaClient::begin(const String& url, uint16_t port) {
    serverUrl = url;
    serverPort = port;

    Serial.printf("[OllamaClient] Initialized: %s:%d\n", serverUrl.c_str(), serverPort);
    Serial.printf("[OllamaClient] Model: %s\n", modelName.c_str());

    return testConnection();
}

OllamaClient::Response OllamaClient::query(const Query& q) {
    Response response;
    unsigned long startTime = millis();

    if (q.text.isEmpty()) {
        response.error = "Empty query";
        return response;
    }

    // Build request JSON
    String requestJson = buildChatRequest(q.text);

    Serial.printf("[OllamaClient] Sending query: %s\n", q.text.c_str());

    // Send HTTP POST
    String responseJson;
    if (!httpPost("/api/chat", requestJson, responseJson)) {
        response.error = "HTTP request failed";
        response.responseTime = millis() - startTime;
        return response;
    }

    // Parse response
    if (!parseResponse(responseJson, response)) {
        response.error = "Failed to parse response";
        response.responseTime = millis() - startTime;
        return response;
    }

    response.responseTime = millis() - startTime;
    response.success = true;

    // Add to conversation history if context enabled
    if (q.includeContext) {
        conversationHistory.push_back(Message("user", q.text));
        conversationHistory.push_back(Message("assistant", response.text));

        // Limit context size to prevent memory issues (keep last 10 messages)
        while (conversationHistory.size() > 10) {
            conversationHistory.erase(conversationHistory.begin());
        }
    }

    Serial.printf("[OllamaClient] Response (%lums): %s\n",
                  response.responseTime, response.text.substring(0, 100).c_str());

    return response;
}

void OllamaClient::setModel(const String& model) {
    modelName = model;
    Serial.printf("[OllamaClient] Model set to: %s\n", modelName.c_str());
}

void OllamaClient::setTimeout(uint32_t timeoutMs) {
    timeout = timeoutMs;
}

void OllamaClient::setMaxTokens(uint16_t tokens) {
    maxTokens = tokens;
}

void OllamaClient::setTemperature(float temp) {
    temperature = constrain(temp, 0.0f, 2.0f);
}

void OllamaClient::setSystemPrompt(const String& prompt) {
    systemPrompt = prompt;
    Serial.printf("[OllamaClient] System prompt: %s\n", systemPrompt.c_str());
}

void OllamaClient::clearContext() {
    conversationHistory.clear();
    Serial.println("[OllamaClient] Conversation context cleared");
}

size_t OllamaClient::getContextSize() const {
    return conversationHistory.size();
}

bool OllamaClient::testConnection() {
    Serial.println("[OllamaClient] Testing connection...");

    String responseJson;
    if (!httpPost("/api/tags", "{}", responseJson)) {
        Serial.println("[OllamaClient] Connection test failed");
        return false;
    }

    Serial.println("[OllamaClient] Connection test successful");
    return true;
}

String OllamaClient::buildChatRequest(const String& userQuery) {
    // Use ArduinoJson to build request
    DynamicJsonDocument doc(4096);

    doc["model"] = modelName;
    doc["stream"] = false;

    // Build messages array
    JsonArray messages = doc.createNestedArray("messages");

    // Add system prompt
    JsonObject systemMsg = messages.createNestedObject();
    systemMsg["role"] = "system";
    systemMsg["content"] = systemPrompt;

    // Add conversation history
    for (const auto& msg : conversationHistory) {
        JsonObject historyMsg = messages.createNestedObject();
        historyMsg["role"] = msg.role;
        historyMsg["content"] = msg.content;
    }

    // Add current user query
    JsonObject userMsg = messages.createNestedObject();
    userMsg["role"] = "user";
    userMsg["content"] = userQuery;

    // Add options
    JsonObject options = doc.createNestedObject("options");
    options["temperature"] = temperature;
    options["num_predict"] = maxTokens;

    // Serialize to string
    String output;
    serializeJson(doc, output);

    return output;
}

bool OllamaClient::parseResponse(const String& jsonResponse, Response& response) {
    // Parse JSON response
    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, jsonResponse);

    if (error) {
        Serial.printf("[OllamaClient] JSON parse error: %s\n", error.c_str());
        response.error = "JSON parse error";
        return false;
    }

    // Extract response text from message.content
    if (doc.containsKey("message") && doc["message"].containsKey("content")) {
        response.text = doc["message"]["content"].as<String>();
        return true;
    }

    // Fallback: check for direct "response" field (generate API)
    if (doc.containsKey("response")) {
        response.text = doc["response"].as<String>();
        return true;
    }

    response.error = "Missing response field in JSON";
    return false;
}

bool OllamaClient::httpPost(const String& endpoint, const String& payload, String& response) {
    String url = serverUrl + ":" + String(serverPort) + endpoint;

    http.begin(url);
    http.setTimeout(timeout);
    http.addHeader("Content-Type", "application/json");

    int httpCode = http.POST(payload);

    if (httpCode != HTTP_CODE_OK && httpCode != 200) {
        Serial.printf("[OllamaClient] HTTP error: %d\n", httpCode);
        http.end();
        return false;
    }

    response = http.getString();
    http.end();

    return true;
}
