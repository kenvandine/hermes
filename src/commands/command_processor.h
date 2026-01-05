#ifndef COMMAND_PROCESSOR_H
#define COMMAND_PROCESSOR_H

#include <Arduino.h>
#include <functional>

/**
 * Voice Command Types
 */
enum class VoiceCommand {
    NONE,           // No command recognized
    DROP_IN,        // "Drop in on [ROOM]"
    CALL,           // "Call [ROOM]"
    HANG_UP,        // "Hang up"
    CANCEL,         // "Cancel"
    ASK_AI,         // "Ask [QUERY]" - AI assistant query
    HA_CONTROL,     // Home Assistant device control
    UNKNOWN         // Command detected but not understood
};

/**
 * CommandResult
 *
 * Encapsulates the result of command processing
 */
struct CommandResult {
    VoiceCommand command;
    String targetRoom;      // For DROP_IN/CALL commands
    String aiQuery;         // For ASK_AI command
    String haCommand;       // For HA_CONTROL command
    float confidence;       // 0.0-1.0
    String rawText;         // Raw recognized text (if available)

    CommandResult()
        : command(VoiceCommand::NONE),
          targetRoom(""),
          aiQuery(""),
          haCommand(""),
          confidence(0.0f),
          rawText("") {}

    bool isValid() const {
        return command != VoiceCommand::NONE && command != VoiceCommand::UNKNOWN;
    }

    // Helper methods for command type checking
    bool isDeviceControl() const { return command == VoiceCommand::HA_CONTROL; }
    bool isAIQuery() const { return command == VoiceCommand::ASK_AI; }

    String toString() const {
        switch (command) {
            case VoiceCommand::DROP_IN:
                return "DROP_IN: " + targetRoom;
            case VoiceCommand::CALL:
                return "CALL: " + targetRoom;
            case VoiceCommand::HANG_UP:
                return "HANG_UP";
            case VoiceCommand::CANCEL:
                return "CANCEL";
            case VoiceCommand::ASK_AI:
                return "ASK_AI: " + aiQuery;
            case VoiceCommand::HA_CONTROL:
                return "HA_CONTROL: " + haCommand;
            case VoiceCommand::UNKNOWN:
                return "UNKNOWN";
            default:
                return "NONE";
        }
    }
};

/**
 * CommandProcessor
 *
 * Processes voice commands after wake word detection.
 * Works with SpeechRecognizer to extract intent and parameters.
 *
 * Usage:
 * 1. After wake word detected, start listening for command
 * 2. Feed audio samples to SpeechRecognizer
 * 3. SpeechRecognizer calls back with recognized keywords
 * 4. CommandProcessor extracts intent and parameters
 * 5. Return CommandResult to caller
 *
 * Timeout:
 * - If no command received within timeout (default 5s), return NONE
 */
class CommandProcessor {
public:
    CommandProcessor();
    ~CommandProcessor();

    /**
     * Start listening for a command
     * @param timeoutMs Timeout in milliseconds (default 5000ms)
     */
    void startListening(unsigned long timeoutMs = 5000);

    /**
     * Stop listening for commands
     */
    void stopListening();

    /**
     * Check if currently listening
     */
    bool isListening() const;

    /**
     * Check if timeout occurred
     */
    bool hasTimedOut() const;

    /**
     * Get time remaining (milliseconds)
     */
    unsigned long getTimeRemaining() const;

    /**
     * Process recognized keyword/phrase
     * @param keyword Recognized keyword (e.g., "drop_in", "kitchen", "hang_up")
     * @param confidence Recognition confidence (0.0-1.0)
     * @return CommandResult if command is complete, otherwise NONE
     */
    CommandResult processKeyword(const String& keyword, float confidence);

    /**
     * Process raw text (for future text-based recognition)
     * @param text Recognized text
     * @return CommandResult
     */
    CommandResult processText(const String& text);

    /**
     * Get current partial command state
     */
    CommandResult getPartialCommand() const;

    /**
     * Reset command state
     */
    void reset();

    /**
     * Register known room names
     * @param rooms List of room names (lowercase)
     */
    void setKnownRooms(const std::vector<String>& rooms);

    /**
     * Add a room name to known rooms
     */
    void addRoom(const String& room);

    /**
     * Get known rooms
     */
    std::vector<String> getKnownRooms() const;

private:
    bool listening;
    unsigned long startTime;
    unsigned long timeoutMs;

    // Command building state
    VoiceCommand currentCommand;
    String targetRoom;
    String currentAIQuery;  // Accumulates AI query text
    float commandConfidence;
    float roomConfidence;

    // Known room names (for matching)
    std::vector<String> knownRooms;

    // Helper functions
    bool matchRoom(const String& keyword, String& matchedRoom);
    VoiceCommand parseCommandKeyword(const String& keyword);
    float calculateCombinedConfidence() const;

    /**
     * Classify command text as HA_CONTROL or ASK_AI
     * @param text Command text to classify
     * @return Classified VoiceCommand type
     */
    VoiceCommand classifyCommand(const String& text);
};

#endif // COMMAND_PROCESSOR_H
