#include "command_processor.h"

CommandProcessor::CommandProcessor()
    : listening(false),
      startTime(0),
      timeoutMs(5000),
      currentCommand(VoiceCommand::NONE),
      targetRoom(""),
      commandConfidence(0.0f),
      roomConfidence(0.0f) {
}

CommandProcessor::~CommandProcessor() {
}

void CommandProcessor::startListening(unsigned long timeout) {
    listening = true;
    startTime = millis();
    timeoutMs = timeout;
    reset();

    Serial.printf("[CommandProcessor] Started listening (timeout: %lu ms)\n", timeoutMs);
}

void CommandProcessor::stopListening() {
    listening = false;
    Serial.println("[CommandProcessor] Stopped listening");
}

bool CommandProcessor::isListening() const {
    if (!listening) {
        return false;
    }

    // Check timeout
    if (hasTimedOut()) {
        return false;
    }

    return true;
}

bool CommandProcessor::hasTimedOut() const {
    if (!listening) {
        return false;
    }

    return (millis() - startTime) >= timeoutMs;
}

unsigned long CommandProcessor::getTimeRemaining() const {
    if (!listening) {
        return 0;
    }

    unsigned long elapsed = millis() - startTime;
    if (elapsed >= timeoutMs) {
        return 0;
    }

    return timeoutMs - elapsed;
}

CommandResult CommandProcessor::processKeyword(const String& keyword, float confidence) {
    if (!listening) {
        Serial.println("[CommandProcessor] WARNING: Not listening, ignoring keyword");
        return CommandResult();
    }

    Serial.printf("[CommandProcessor] Processing keyword: '%s' (confidence: %.2f)\n",
                  keyword.c_str(), confidence);

    // Check if this is a command keyword
    VoiceCommand cmd = parseCommandKeyword(keyword);
    if (cmd != VoiceCommand::NONE) {
        currentCommand = cmd;
        commandConfidence = confidence;

        Serial.printf("[CommandProcessor] Recognized command: %d\n", (int)cmd);

        // If it's a simple command (no room required), return immediately
        if (cmd == VoiceCommand::HANG_UP || cmd == VoiceCommand::CANCEL) {
            CommandResult result;
            result.command = cmd;
            result.confidence = confidence;
            result.rawText = keyword;

            Serial.printf("[CommandProcessor] Complete command: %s\n", result.toString().c_str());
            stopListening();
            return result;
        }

        // For ASK_AI, keep listening to accumulate query text
        if (cmd == VoiceCommand::ASK_AI) {
            Serial.println("[CommandProcessor] AI query started, listening for question...");
            return CommandResult();
        }

        // Otherwise, wait for room name (for CALL/DROP_IN)
        return CommandResult();
    }

    // Check if this is a room name
    String matchedRoom;
    if (matchRoom(keyword, matchedRoom)) {
        targetRoom = matchedRoom;
        roomConfidence = confidence;

        Serial.printf("[CommandProcessor] Recognized room: '%s'\n", matchedRoom.c_str());

        // If we already have a command, we're done
        if (currentCommand == VoiceCommand::DROP_IN || currentCommand == VoiceCommand::CALL) {
            CommandResult result;
            result.command = currentCommand;
            result.targetRoom = targetRoom;
            result.confidence = calculateCombinedConfidence();
            result.rawText = keyword;

            Serial.printf("[CommandProcessor] Complete command: %s\n", result.toString().c_str());
            stopListening();
            return result;
        }

        // Otherwise, assume DROP_IN if room is mentioned without explicit command
        currentCommand = VoiceCommand::DROP_IN;
        commandConfidence = 0.9f;  // High confidence for implicit command

        CommandResult result;
        result.command = currentCommand;
        result.targetRoom = targetRoom;
        result.confidence = calculateCombinedConfidence();
        result.rawText = keyword;

        Serial.printf("[CommandProcessor] Implicit DROP_IN command: %s\n", result.toString().c_str());
        stopListening();
        return result;
    }

    // If we're in AI query mode, accumulate keywords as query text
    if (currentCommand == VoiceCommand::ASK_AI) {
        if (!currentAIQuery.isEmpty()) {
            currentAIQuery += " ";
        }
        currentAIQuery += keyword;
        Serial.printf("[CommandProcessor] AI query building: '%s'\n", currentAIQuery.c_str());

        // Update confidence (use highest confidence seen)
        if (confidence > roomConfidence) {
            roomConfidence = confidence;
        }

        return CommandResult();  // Keep listening
    }

    // Unknown keyword
    Serial.printf("[CommandProcessor] Unknown keyword: '%s'\n", keyword.c_str());
    return CommandResult();
}

CommandResult CommandProcessor::processText(const String& text) {
    if (!listening) {
        Serial.println("[CommandProcessor] WARNING: Not listening, ignoring text");
        return CommandResult();
    }

    Serial.printf("[CommandProcessor] Processing text: '%s'\n", text.c_str());

    String textLower = text;
    textLower.toLowerCase();
    textLower.trim();

    // Check for simple commands first (high priority)
    if (textLower.indexOf("hang up") >= 0 || textLower.indexOf("hangup") >= 0) {
        CommandResult result;
        result.command = VoiceCommand::HANG_UP;
        result.confidence = 0.9f;
        result.rawText = text;
        stopListening();
        return result;
    }

    if (textLower.indexOf("cancel") >= 0 || textLower.indexOf("never mind") >= 0) {
        CommandResult result;
        result.command = VoiceCommand::CANCEL;
        result.confidence = 0.9f;
        result.rawText = text;
        stopListening();
        return result;
    }

    // Classify the command (HA_CONTROL vs ASK_AI)
    VoiceCommand classified = classifyCommand(text);

    CommandResult result;
    result.command = classified;
    result.confidence = 0.9f;  // High confidence for text-based input
    result.rawText = text;

    if (classified == VoiceCommand::HA_CONTROL) {
        // Device control command - send to Home Assistant
        result.haCommand = text;
        Serial.printf("[CommandProcessor] HA device control: '%s'\n", text.c_str());
    }
    else if (classified == VoiceCommand::ASK_AI) {
        // AI query - send to Ollama
        // Remove "ask" prefix if present
        String query = text;
        if (textLower.startsWith("ask ")) {
            query = text.substring(4);
            query.trim();
        }
        result.aiQuery = query;
        Serial.printf("[CommandProcessor] AI query: '%s'\n", query.c_str());
    }

    stopListening();
    return result;
}

CommandResult CommandProcessor::getPartialCommand() const {
    CommandResult result;
    result.command = currentCommand;
    result.targetRoom = targetRoom;
    result.aiQuery = currentAIQuery;
    result.confidence = calculateCombinedConfidence();
    return result;
}

void CommandProcessor::reset() {
    currentCommand = VoiceCommand::NONE;
    targetRoom = "";
    currentAIQuery = "";
    commandConfidence = 0.0f;
    roomConfidence = 0.0f;
}

void CommandProcessor::setKnownRooms(const std::vector<String>& rooms) {
    knownRooms = rooms;
    Serial.printf("[CommandProcessor] Loaded %d known rooms\n", knownRooms.size());
}

void CommandProcessor::addRoom(const String& room) {
    // Convert to lowercase for matching
    String roomLower = room;
    roomLower.toLowerCase();

    // Check if already exists
    for (const auto& r : knownRooms) {
        if (r.equalsIgnoreCase(roomLower)) {
            return;  // Already exists
        }
    }

    knownRooms.push_back(roomLower);
    Serial.printf("[CommandProcessor] Added room: '%s'\n", roomLower.c_str());
}

std::vector<String> CommandProcessor::getKnownRooms() const {
    return knownRooms;
}

// ============================================================================
// Private Helper Functions
// ============================================================================

bool CommandProcessor::matchRoom(const String& keyword, String& matchedRoom) {
    String keywordLower = keyword;
    keywordLower.toLowerCase();

    for (const auto& room : knownRooms) {
        if (room.equalsIgnoreCase(keywordLower)) {
            matchedRoom = room;
            return true;
        }

        // Also try partial match (e.g., "living" matches "living_room")
        if (room.indexOf(keywordLower) >= 0 || keywordLower.indexOf(room) >= 0) {
            matchedRoom = room;
            return true;
        }
    }

    return false;
}

VoiceCommand CommandProcessor::parseCommandKeyword(const String& keyword) {
    String keywordLower = keyword;
    keywordLower.toLowerCase();

    // Remove underscores and spaces for matching
    keywordLower.replace("_", "");
    keywordLower.replace(" ", "");

    // Match command keywords
    if (keywordLower == "dropin" || keywordLower == "drop") {
        return VoiceCommand::DROP_IN;
    }

    if (keywordLower == "call") {
        return VoiceCommand::CALL;
    }

    if (keywordLower == "hangup" || keywordLower == "hang") {
        return VoiceCommand::HANG_UP;
    }

    if (keywordLower == "cancel" || keywordLower == "stop" || keywordLower == "nevermind") {
        return VoiceCommand::CANCEL;
    }

    if (keywordLower == "ask" || keywordLower == "question") {
        return VoiceCommand::ASK_AI;
    }

    return VoiceCommand::NONE;
}

float CommandProcessor::calculateCombinedConfidence() const {
    // If only command confidence is set
    if (commandConfidence > 0.0f && roomConfidence == 0.0f) {
        return commandConfidence;
    }

    // If only room confidence is set
    if (roomConfidence > 0.0f && commandConfidence == 0.0f) {
        return roomConfidence;
    }

    // If both are set, use average
    if (commandConfidence > 0.0f && roomConfidence > 0.0f) {
        return (commandConfidence + roomConfidence) / 2.0f;
    }

    return 0.0f;
}

VoiceCommand CommandProcessor::classifyCommand(const String& text) {
    String lower = text;
    lower.toLowerCase();
    lower.trim();

    // Device control action verbs
    const char* actionVerbs[] = {
        "turn on", "turn off", "switch on", "switch off",
        "set", "dim", "brighten", "increase", "decrease",
        "open", "close", "lock", "unlock", "start", "stop"
    };

    // Device entity types
    const char* deviceTypes[] = {
        "light", "lights", "lamp", "lamps",
        "switch", "switches",
        "thermostat", "temperature", "heating", "cooling",
        "fan", "fans", "climate",
        "door", "doors", "garage",
        "blind", "blinds", "shade", "shades", "curtain", "curtains"
    };

    // Check for device control indicators
    for (const char* verb : actionVerbs) {
        if (lower.indexOf(verb) >= 0) {
            // Has action verb, check for device type
            for (const char* device : deviceTypes) {
                if (lower.indexOf(device) >= 0) {
                    Serial.printf("[CommandProcessor] Classified as HA_CONTROL (verb: %s, device: %s)\n",
                                  verb, device);
                    return VoiceCommand::HA_CONTROL;
                }
            }
            // Has action but no clear device - still likely control
            Serial.printf("[CommandProcessor] Classified as HA_CONTROL (action verb: %s)\n", verb);
            return VoiceCommand::HA_CONTROL;
        }
    }

    // Check for question indicators (these suggest AI query)
    const char* questionWords[] = {
        "what", "when", "where", "who", "why", "how",
        "tell me", "explain", "describe", "define"
    };

    for (const char* question : questionWords) {
        String questionStr = question;
        // Check if starts with question word or contains "tell me" / "explain" pattern
        if (lower.startsWith(questionStr) ||
            (lower.indexOf(" " + questionStr) == 0) ||
            lower.indexOf(questionStr + " ") == 0) {
            Serial.printf("[CommandProcessor] Classified as ASK_AI (question word: %s)\n", question);
            return VoiceCommand::ASK_AI;
        }
    }

    // If contains "ask" keyword, it's AI
    if (lower.indexOf("ask") >= 0 || lower.indexOf("question") >= 0) {
        Serial.println("[CommandProcessor] Classified as ASK_AI (contains 'ask' or 'question')");
        return VoiceCommand::ASK_AI;
    }

    // When uncertain, default to HA (safer for device control)
    Serial.println("[CommandProcessor] Defaulting to HA_CONTROL (uncertain classification)");
    return VoiceCommand::HA_CONTROL;
}
