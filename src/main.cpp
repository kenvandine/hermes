/*
 * HERMES - Home ESP Room Message Exchange System
 * Main Entry Point
 *
 * "Swift communication, ancient wisdom"
 *
 * Phase 8: AI Assistant Integration - COMPLETE
 *
 * This application orchestrates all subsystems:
 * - Device management and configuration
 * - WiFi connectivity
 * - MQTT client for signaling and Home Assistant integration
 * - Device discovery and registry
 * - Audio pipeline (I2S, Opus, UDP)
 * - Call management (complete call lifecycle)
 * - Wake word detection (Edge Impulse)
 * - Voice command recognition (Edge Impulse keyword spotting)
 * - Touch screen UI (LVGL on 1.8" AMOLED)
 * - AI Assistant (Ollama LLM with Piper TTS)
 */

#include <Arduino.h>
#include <WiFi.h>
#include "config.h"

// Phase 2 Components
#include "core/device_manager.h"
#include "call/device_registry.h"
#include "network/mqtt_client.h"

// Phase 3 Components
#ifndef DISABLE_AUDIO_TEMP
#include "audio/audio_pipeline.h"
#endif

// Phase 4 Components
#include "core/state_machine.h"
#include "call/call_manager.h"

// Phase 6 Components
#ifndef DISABLE_AUDIO_TEMP
#include "commands/command_processor.h"
#include "commands/speech_recognizer.h"
#endif

// Phase 7 Components
#include <Arduino_GFX_Library.h>
#include <Wire.h>
#include "ui/Arduino_SH8601.h"  // Waveshare's working SH8601 driver
#include "ui/ui_manager.h"

// Phase 8 Components
#ifndef DISABLE_AUDIO_TEMP
#include "ai/ollama_client.h"
#include "ai/tts_engine.h"
#include "ai/ai_manager.h"
#endif

// ============================================================================
// GLOBAL OBJECTS
// ============================================================================

DeviceManager* deviceManager = nullptr;
DeviceRegistry* deviceRegistry = nullptr;
MqttClient* mqttClient = nullptr;
#ifndef DISABLE_AUDIO_TEMP
AudioPipeline* audioPipeline = nullptr;
#endif
StateMachine* stateMachine = nullptr;
CallManager* callManager = nullptr;

// Phase 6: Voice Commands
#ifndef DISABLE_AUDIO_TEMP
CommandProcessor* commandProcessor = nullptr;
SpeechRecognizer* speechRecognizer = nullptr;
#endif

// Phase 7: UI
Arduino_GFX* gfx = nullptr;
UIManager* uiManager = nullptr;

// Phase 8: AI Assistant
#ifndef DISABLE_AUDIO_TEMP
OllamaClient* ollamaClient = nullptr;
TTSEngine* ttsEngine = nullptr;
AIManager* aiManager = nullptr;
#endif

// Task handles for FreeRTOS tasks
TaskHandle_t uiTaskHandle = NULL;
#ifndef DISABLE_AUDIO_TEMP
TaskHandle_t audioTaskHandle = NULL;
#endif
TaskHandle_t mqttTaskHandle = NULL;

// Audio recording buffer for debugging
int16_t* debugAudioBuffer = nullptr;
const size_t DEBUG_BUFFER_SIZE = 16000 * 3; // 3 seconds at 16kHz (reduced to avoid download timeout)
size_t debugBufferIndex = 0;
bool isRecording = false;

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================

void setupSerial();
void setupDeviceManager();
void setupWiFi();
void setupDisplay();
#ifndef DISABLE_AUDIO_TEMP
void setupAudio();
#endif
void setupMQTT();
void setupCallManager();
#ifndef DISABLE_AUDIO_TEMP
void setupVoiceCommands();
void setupAI();
#endif
void printSystemInfo();

// FreeRTOS Tasks
void uiTask(void* parameter);
#ifndef DISABLE_AUDIO_TEMP
void audioTask(void* parameter);
#endif
void mqttTask(void* parameter);

// MQTT Callbacks
void onDeviceDiscovered(const String& deviceId, const String& roomName, const String& ip);
#ifndef DISABLE_AUDIO_TEMP
void onCallInitiated(const String& targetRoom);
void onCallRequest(const String& fromDeviceId, const String& fromRoom, const String& sessionId);
void onCallAccept(const String& sessionId, uint16_t udpPort);
void onCallReject(const String& sessionId, const String& reason);
void onCallHangup(const String& sessionId);
#endif

// State machine callback
void onStateChanged(AppState oldState, AppState newState);

// ============================================================================
// SETUP
// ============================================================================

void setup() {
    // Initialize serial communication for debugging
    setupSerial();

    Serial.println("\n\n");
    Serial.println("========================================");
    Serial.println("       HERMES Intercom System");
    Serial.println("  Home ESP Room Message Exchange");
    Serial.println("========================================");
    Serial.println("  Version: " FIRMWARE_VERSION);
    Serial.println("  Build: " BUILD_DATE " " BUILD_TIME);
    Serial.println("  Phase: 8 - AI Assistant (Complete)");
    Serial.println("  Display: 1.8\" AMOLED (368x448)");
    Serial.println("  AI: Ollama + Piper TTS");
    Serial.println("========================================");
    Serial.println("  \"Swift communication, ancient wisdom\"");
    Serial.println("========================================\n");

    // Print system information
    printSystemInfo();

    // Initialize device manager (handles NVS storage)
    Serial.println("[SETUP] Initializing device manager...");
    setupDeviceManager();

    // Initialize WiFi
    Serial.println("[SETUP] Setting up WiFi...");
    setupWiFi();

    // Initialize display and UI
    Serial.println("[SETUP] Setting up display...");
    setupDisplay();

#ifndef DISABLE_AUDIO_TEMP
    // Initialize audio subsystem
    Serial.println("[SETUP] Setting up audio...");
    setupAudio();
#endif

    // Initialize MQTT client
    Serial.println("[SETUP] Setting up MQTT...");
    setupMQTT();

    // Initialize call manager
    Serial.println("[SETUP] Setting up call manager...");
    setupCallManager();

#ifndef DISABLE_AUDIO_TEMP
    // Initialize voice command system (Phase 6)
    Serial.println("[SETUP] Setting up voice commands...");
    setupVoiceCommands();

    // Initialize AI assistant system (Phase 8)
    Serial.println("[SETUP] Setting up AI assistant...");
    setupAI();
#endif

    Serial.println("\n[SETUP] ========================================");
    Serial.println("[SETUP] All subsystems initialized!");
    Serial.println("[SETUP] ========================================\n");

    // Print device info
    if (deviceManager) {
        deviceManager->printInfo();
    }

    Serial.println("[SETUP] Entering main loop...\n");
}

// ============================================================================
// MAIN LOOP
// ============================================================================

void loop() {
    // Main loop is kept minimal since most work happens in FreeRTOS tasks
    // This can be used for low-priority background tasks

#ifndef DISABLE_AUDIO_TEMP
    // Process call manager (check timeouts, state transitions)
    if (callManager) {
        callManager->process();
    }

    // Process AI manager (async operations)
    if (aiManager) {
        aiManager->update();
    }
#endif

    // Check device registry for timeouts
    if (deviceRegistry) {
        static unsigned long lastRegistryCheck = 0;
        if (millis() - lastRegistryCheck > 10000) {  // Every 10 seconds
            lastRegistryCheck = millis();
            deviceRegistry->checkTimeouts();
        }
    }

    // Handle debug recording commands
    if (Serial.available()) {
        char c = Serial.read();
        if (c == 'r') {
            if (!debugAudioBuffer) {
                debugAudioBuffer = (int16_t*)ps_malloc(DEBUG_BUFFER_SIZE * sizeof(int16_t));
                if (!debugAudioBuffer) {
                    debugAudioBuffer = (int16_t*)malloc(DEBUG_BUFFER_SIZE * sizeof(int16_t));
                }
            }

            if (debugAudioBuffer) {
                debugBufferIndex = 0;
                isRecording = true;
                Serial.println("Recording started (5s buffer)...");
            } else {
                Serial.println("Failed to allocate recording buffer!");
            }
        } else if (c == 'd') {
            isRecording = false;
            Serial.println("Recording stopped.");
            if (debugAudioBuffer && debugBufferIndex > 0) {
                Serial.printf("Downloading %d samples...\n", debugBufferIndex);

                // Print WAV header
                uint32_t sampleRate = 16000;
                uint32_t channels = 1;
                uint32_t bitsPerSample = 16;
                uint32_t byteRate = sampleRate * channels * bitsPerSample / 8;
                uint32_t blockAlign = channels * bitsPerSample / 8;
                uint32_t dataSize = debugBufferIndex * sizeof(int16_t);
                uint32_t riffSize = dataSize + 36;

                Serial.println("--- BEGIN WAV FILE ---");

                // RIFF header
                Serial.write("RIFF", 4);
                Serial.write((uint8_t*)&riffSize, 4);
                Serial.write("WAVE", 4);

                // fmt chunk
                Serial.write("fmt ", 4);
                uint32_t fmtSize = 16;
                Serial.write((uint8_t*)&fmtSize, 4);
                uint16_t audioFormat = 1; // PCM
                Serial.write((uint8_t*)&audioFormat, 2);
                Serial.write((uint8_t*)&channels, 2);
                Serial.write((uint8_t*)&sampleRate, 4);
                Serial.write((uint8_t*)&byteRate, 4);
                Serial.write((uint8_t*)&blockAlign, 2);
                Serial.write((uint8_t*)&bitsPerSample, 2);

                // data chunk
                Serial.write("data", 4);
                Serial.write((uint8_t*)&dataSize, 4);

                // Audio data - send in chunks to avoid watchdog timeout
                uint8_t* bytePtr = (uint8_t*)debugAudioBuffer;
                size_t remaining = dataSize;
                const size_t CHUNK_SIZE = 512;

                while (remaining > 0) {
                    size_t toWrite = (remaining > CHUNK_SIZE) ? CHUNK_SIZE : remaining;
                    Serial.write(bytePtr, toWrite);
                    bytePtr += toWrite;
                    remaining -= toWrite;

                    // Feed watchdog / allow background tasks
                    yield();
                }

                Serial.println(); // Newline at the end
                Serial.println("--- END WAV FILE ---");
                Serial.println("Download complete.");
            } else {
                Serial.println("Buffer empty or not allocated.");
            }
        }
    }

    delay(100);  // Small delay to prevent watchdog timeout
}

// ============================================================================
// INITIALIZATION FUNCTIONS
// ============================================================================

void setupSerial() {
    Serial.begin(SERIAL_BAUD_RATE);

    // Wait for serial to initialize (useful for debugging)
    #ifdef CORE_DEBUG_LEVEL
    delay(2000);
    while (!Serial && millis() < 5000) {
        delay(10);
    }
    #endif
}

void setupDeviceManager() {
    deviceManager = new DeviceManager();

    if (!deviceManager->begin()) {
        Serial.println("[ERROR] Failed to initialize device manager!");
        return;
    }

    if (deviceManager->isFirstBoot()) {
        Serial.println("\n========================================");
        Serial.println("  FIRST BOOT CONFIGURATION REQUIRED");
        Serial.println("========================================");
        Serial.println("Please configure:");
        Serial.println("  1. WiFi credentials");
        Serial.println("  2. Room name");
        Serial.println("  3. MQTT broker address");
        Serial.println("\nSee README.md for configuration instructions.");
        Serial.println("========================================\n");
    }
}

void setupWiFi() {
    String ssid = deviceManager->getWifiSsid();
    String password = deviceManager->getWifiPassword();

    if (ssid.length() == 0) {
        Serial.println("[WIFI] ========================================");
        Serial.println("[WIFI] NO WIFI CREDENTIALS CONFIGURED!");
        Serial.println("[WIFI] ========================================");
        Serial.println("[WIFI] To configure WiFi, use the serial monitor or preferences:");
        Serial.println("[WIFI]   preferences.putString(\"wifi_ssid\", \"YourSSID\");");
        Serial.println("[WIFI]   preferences.putString(\"wifi_pass\", \"YourPassword\");");
        Serial.println("[WIFI] ========================================");
        Serial.println("[WIFI] TODO: Implement WiFi Manager (captive portal) in future");
        Serial.println("[WIFI] ========================================\n");
        return;
    }

    Serial.printf("[WIFI] Connecting to: %s\n", ssid.c_str());

    // Set WiFi mode to station (client)
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(deviceManager->getDeviceId().c_str());
    WiFi.begin(ssid.c_str(), password.c_str());

    // Wait for connection with timeout
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED &&
           millis() - startTime < WIFI_CONNECT_TIMEOUT_MS) {
        delay(500);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n[WIFI] ✓ Connected successfully!");
        Serial.printf("[WIFI]   IP Address: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("[WIFI]   Hostname: %s\n", deviceManager->getDeviceId().c_str());
        Serial.printf("[WIFI]   Signal: %d dBm\n", WiFi.RSSI());
        Serial.printf("[WIFI]   MAC: %s\n", deviceManager->getMacAddress().c_str());
    } else {
        Serial.println("\n[WIFI] ✗ Connection failed!");
        Serial.println("[WIFI] Please check WiFi credentials and try again.");
    }
}

void setupDisplay() {
    Serial.println("[DISPLAY] Initializing Waveshare ESP32-S3 1.8\" AMOLED display...");

    // Initialize I2C for touch controller and TCA9554 GPIO expander
    Wire.begin(TOUCH_SDA, TOUCH_SCL);
    Serial.println("[DISPLAY] I2C bus initialized for touch and GPIO expander");

    // Scan I2C bus to find devices
    Serial.println("[DISPLAY] Scanning I2C bus...");
    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        uint8_t error = Wire.endTransmission();
        if (error == 0) {
            Serial.printf("[DISPLAY]   Found I2C device at 0x%02X\n", addr);
        }
    }

    // Initialize TCA9554 GPIO expander - try multiple common I2C addresses
    // TCA9554 address depends on A0/A1/A2 pins: base 0x20-0x27
    Serial.println("[DISPLAY] Searching for TCA9554 GPIO expander...");
    uint8_t tca_addr = 0;
    uint8_t possible_addrs[] = {0x20, 0x24, 0x21, 0x22, 0x23, 0x25, 0x26, 0x27};

    for (int i = 0; i < 8; i++) {
        Wire.beginTransmission(possible_addrs[i]);
        if (Wire.endTransmission() == 0) {
            tca_addr = possible_addrs[i];
            Serial.printf("[DISPLAY]   Found TCA9554 at 0x%02X\n", tca_addr);
            break;
        }
    }

    if (tca_addr == 0) {
        Serial.println("[DISPLAY] ✗ TCA9554 not found on I2C bus!");
        Serial.println("[DISPLAY] Proceeding without GPIO expander...");
    } else {
        // Configure all TCA9554 pins as outputs (register 0x03, value 0x00)
        Wire.beginTransmission(tca_addr);
        Wire.write(0x03);  // Configuration register
        Wire.write(0x00);  // All pins as outputs (0 = output, 1 = input)
        if (Wire.endTransmission() != 0) {
            Serial.println("[DISPLAY] ✗ Failed to configure TCA9554!");
        } else {
            Serial.println("[DISPLAY] ✓ TCA9554 configured (all pins as outputs)");
        }

        // Enable display power by setting pins 0, 1, 2, 6 HIGH (all expander pins used on this board)
        Wire.beginTransmission(tca_addr);
        Wire.write(0x01);  // Output Port register
        Wire.write(0x47);  // Set pins 0, 1, 2, 6 HIGH (0b01000111)
        if (Wire.endTransmission() != 0) {
            Serial.println("[DISPLAY] ✗ Failed to enable display power!");
        } else {
            Serial.println("[DISPLAY] ✓ Display power enabled via TCA9554 pins 0,1,2,6");
        }
    }

    delay(200);  // Wait for display power to stabilize

    // Create QSPI bus using Arduino_GFX native QSPI support
    Serial.println("[DISPLAY] Initializing QSPI bus with Arduino_ESP32QSPI...");
    Arduino_DataBus *bus = new Arduino_ESP32QSPI(
        TFT_CS,    /* CS: 12 */
        TFT_SCL,   /* SCK: 11 */
        TFT_SDA0,  /* SDIO0: 4 */
        TFT_SDA1,  /* SDIO1: 5 */
        TFT_SDA2,  /* SDIO2: 6 */
        TFT_SDA3   /* SDIO3: 7 */
    );

    // Create SH8601 display driver using Arduino_GFX 1.4.9 built-in driver
    Serial.println("[DISPLAY] Creating SH8601 display driver (Arduino_GFX 1.4.9)...");
    gfx = new Arduino_SH8601(bus, GFX_NOT_DEFINED /* RST */, 0 /* rotation */, false /* IPS */, 368 /* width */, 448 /* height */);

    if (!gfx->begin()) {
        Serial.println("[DISPLAY] ✗ Failed to initialize display!");
        return;
    }

    Serial.println("[DISPLAY] ✓ Display initialized successfully");

    // Set brightness to maximum (AMOLED displays need this!)
    Serial.println("[DISPLAY] Setting brightness to maximum...");
    ((Arduino_SH8601*)gfx)->Display_Brightness(255);
    delay(100);

    // Test display with solid colors
    Serial.println("[DISPLAY] Testing display with colors...");
    gfx->fillScreen(RED);
    delay(1000);
    gfx->fillScreen(GREEN);
    delay(1000);
    gfx->fillScreen(BLUE);
    delay(1000);
    gfx->fillScreen(WHITE);
    delay(1000);
    gfx->fillScreen(BLACK);
    Serial.println("[DISPLAY] Color test complete");

    // Create UI Manager
    Serial.println("[DISPLAY] Creating UI Manager...");
    uiManager = new UIManager(stateMachine, deviceRegistry,
#ifndef DISABLE_AUDIO_TEMP
        callManager, audioPipeline
#else
        nullptr, nullptr  // No call manager or audio when disabled
#endif
    );

    if (!uiManager->begin(gfx, &Wire)) {
        Serial.println("[DISPLAY] ✗ Failed to initialize UI Manager!");
        delete uiManager;
        uiManager = nullptr;
        return;
    }

    Serial.println("[DISPLAY] ✓ UI Manager initialized successfully");

    // Create UI task
    xTaskCreatePinnedToCore(
        uiTask,
        "UI Task",
        TASK_STACK_UI,
        NULL,
        TASK_PRIORITY_UI,
        &uiTaskHandle,
        TASK_CORE_UI
    );

    Serial.println("[DISPLAY] UI task created");
}

#ifndef DISABLE_AUDIO_TEMP
void setupAudio() {
    Serial.println("[AUDIO] Initializing audio pipeline...");

    audioPipeline = new AudioPipeline();

    if (!audioPipeline->begin(AUDIO_SAMPLE_RATE)) {
        Serial.println("[AUDIO] ✗ Failed to initialize audio pipeline!");
        delete audioPipeline;
        audioPipeline = nullptr;
        return;
    }

    Serial.println("[AUDIO] ✓ Audio pipeline initialized successfully");

    // Register wake word callback
    audioPipeline->onWakeWordDetected([]() {
        Serial.println("[AUDIO] Wake word callback triggered!");
        if (stateMachine) {
            // Transition to LISTENING state
            stateMachine->setState(AppState::LISTENING);
        }
    });

    // Enable wake word detection
    audioPipeline->enableWakeWord(true);
    Serial.println("[AUDIO] Wake word detection enabled");

    // Optional: Test loopback mode
    // Uncomment to test mic → speaker loopback
    // audioPipeline->setMode(AudioMode::LOOPBACK);
    // Serial.println("[AUDIO] Loopback mode enabled for testing");

    // Create audio task
    xTaskCreatePinnedToCore(
        audioTask,
        "Audio Task",
        TASK_STACK_AUDIO_RX,
        NULL,
        TASK_PRIORITY_AUDIO_RX,
        &audioTaskHandle,
        TASK_CORE_AUDIO
    );

    Serial.println("[AUDIO] Audio task created");
}
#endif // DISABLE_AUDIO_TEMP

void setupMQTT() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[MQTT] ✗ Skipping MQTT setup - WiFi not connected");
        return;
    }

    // Create device registry
    deviceRegistry = new DeviceRegistry();
    Serial.println("[MQTT] Device registry created");

    // Create MQTT client
    mqttClient = new MqttClient(deviceManager, deviceRegistry);
    Serial.println("[MQTT] MQTT client created");

    // Register callbacks
    mqttClient->onDeviceAnnounced(onDeviceDiscovered);
#ifndef DISABLE_AUDIO_TEMP
    mqttClient->onCallInitiate(onCallInitiated);
    mqttClient->onCallRequest(onCallRequest);
    mqttClient->onCallAccept(onCallAccept);
    mqttClient->onCallReject(onCallReject);
    mqttClient->onCallHangup(onCallHangup);
#endif

    // Initialize MQTT
    if (!mqttClient->begin()) {
        Serial.println("[MQTT] ✗ MQTT initialization failed!");
        Serial.println("[MQTT] Please configure MQTT broker:");
        Serial.println("[MQTT]   preferences.putString(\"mqtt_broker\", \"homeassistant.local\");");
        Serial.println("[MQTT]   preferences.putUShort(\"mqtt_port\", 1883);");
        return;
    }

    // Create MQTT task
    xTaskCreatePinnedToCore(
        mqttTask,
        "MQTT Task",
        TASK_STACK_MQTT,
        NULL,
        TASK_PRIORITY_MQTT,
        &mqttTaskHandle,
        TASK_CORE_MQTT
    );

    Serial.println("[MQTT] MQTT task created");
}

void setupCallManager() {
    // Create state machine
    stateMachine = new StateMachine();
    Serial.println("[CallManager] State machine created");

    // Register state change callback
    stateMachine->onStateChange(onStateChanged);

#ifndef DISABLE_AUDIO_TEMP
    // Create call manager
    if (!deviceRegistry || !mqttClient || !audioPipeline) {
        Serial.println("[CallManager] WARNING: Dependencies not ready, skipping");
        return;
    }

    callManager = new CallManager(stateMachine, deviceRegistry, mqttClient, audioPipeline);

    if (!callManager->begin()) {
        Serial.println("[CallManager] ✗ Failed to initialize call manager!");
        delete callManager;
        callManager = nullptr;
        return;
    }

    Serial.println("[CallManager] ✓ Call manager initialized successfully");
#else
    Serial.println("[CallManager] Call manager skipped (audio disabled)");
#endif
}

#ifndef DISABLE_AUDIO_TEMP
void setupVoiceCommands() {
    Serial.println("[VoiceCommands] Initializing voice command system...");

    // Create command processor
    commandProcessor = new CommandProcessor();

    // Load known rooms from device registry
    if (deviceRegistry) {
        auto devices = deviceRegistry->getOnlineDevices();
        for (auto device : devices) {
            commandProcessor->addRoom(device->roomName);
        }
        Serial.printf("[VoiceCommands] Loaded %d room names\n", devices.size());
    }

    // Create speech recognizer
    speechRecognizer = new SpeechRecognizer();
    if (speechRecognizer->begin(AUDIO_SAMPLE_RATE, COMMAND_THRESHOLD)) {
        Serial.println("[VoiceCommands] ✓ Speech recognizer initialized");

        // Register keyword callback
        speechRecognizer->onKeywordDetected([](const String& keyword, float confidence) {
            if (!commandProcessor) return;

            // Feed keyword to command processor
            CommandResult result = commandProcessor->processKeyword(keyword, confidence);

            if (result.isValid()) {
                // Complete command received, execute it
                Serial.printf("[VoiceCommands] Executing command: %s\n", result.toString().c_str());

                switch (result.command) {
                    case VoiceCommand::DROP_IN:
                    case VoiceCommand::CALL:
                        if (callManager) {
                            callManager->initiateCall(result.targetRoom);
                        }
                        break;

                    case VoiceCommand::HANG_UP:
                        if (callManager) {
                            callManager->hangupCall();
                        }
                        break;

                    case VoiceCommand::CANCEL:
                        // Return to idle
                        if (stateMachine) {
                            stateMachine->setState(AppState::IDLE);
                        }
                        break;

                    case VoiceCommand::ASK_AI:
                        // Process AI query
                        if (aiManager && !result.aiQuery.isEmpty()) {
                            aiManager->processQuery(result.aiQuery);
                        }
                        break;

                    default:
                        break;
                }
            }
        });
    } else {
        Serial.println("[VoiceCommands] Speech recognizer not available (model not loaded)");
    }

    Serial.println("[VoiceCommands] ✓ Voice command system initialized");
}

void setupAI() {
    Serial.println("[AI] Initializing AI assistant system...");

    // Check WiFi connectivity (required for Ollama server)
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[AI] ✗ WiFi not connected - AI assistant disabled");
        return;
    }

    // Create Ollama client
    ollamaClient = new OllamaClient();
    if (!ollamaClient->begin(OLLAMA_SERVER_URL, OLLAMA_SERVER_PORT)) {
        Serial.println("[AI] ✗ Failed to initialize Ollama client");
        delete ollamaClient;
        ollamaClient = nullptr;
        return;
    }
    ollamaClient->setModel(OLLAMA_MODEL_NAME);
    Serial.printf("[AI] ✓ Ollama client connected to %s:%d\n", OLLAMA_SERVER_URL, OLLAMA_SERVER_PORT);
    Serial.printf("[AI]   Model: %s\n", OLLAMA_MODEL_NAME);

    // Create TTS engine
    ttsEngine = new TTSEngine();
    if (!ttsEngine->begin(TTS_ENGINE, audioPipeline)) {
        Serial.println("[AI] ✗ Failed to initialize TTS engine");
        delete ttsEngine;
        ttsEngine = nullptr;
        return;
    }
    ttsEngine->setSpeed(TTS_SPEED);
    Serial.println("[AI] ✓ TTS engine initialized");

    // Create AI manager
    if (!ollamaClient || !ttsEngine || !uiManager || !mqttClient || !stateMachine) {
        Serial.println("[AI] ✗ Missing dependencies for AI manager");
        return;
    }

    aiManager = new AIManager(ollamaClient, ttsEngine, uiManager, mqttClient, stateMachine);
    if (!aiManager->begin()) {
        Serial.println("[AI] ✗ Failed to initialize AI manager");
        delete aiManager;
        aiManager = nullptr;
        return;
    }

    // Set response mode (TTS + Display + MQTT)
    aiManager->setResponseMode(AI_RESPONSE_MODE_DEFAULT);
    Serial.println("[AI] ✓ AI manager initialized");

    Serial.println("[AI] ✓ AI assistant system ready");
}
#endif // DISABLE_AUDIO_TEMP

void printSystemInfo() {
    Serial.println("[INFO] System Information:");
    Serial.printf("[INFO]   Chip Model: %s\n", ESP.getChipModel());
    Serial.printf("[INFO]   Chip Revision: %d\n", ESP.getChipRevision());
    Serial.printf("[INFO]   CPU Frequency: %d MHz\n", ESP.getCpuFreqMHz());
    Serial.printf("[INFO]   Flash Size: %d MB\n", ESP.getFlashChipSize() / (1024 * 1024));
    Serial.printf("[INFO]   Free Heap: %d bytes\n", ESP.getFreeHeap());

    // Check for PSRAM
    if (psramFound()) {
        Serial.printf("[INFO]   PSRAM Size: %d bytes\n", ESP.getPsramSize());
        Serial.printf("[INFO]   Free PSRAM: %d bytes\n", ESP.getFreePsram());
    } else {
        Serial.println("[WARNING] PSRAM not found! This may limit performance.");
    }

    Serial.println();
}

// ============================================================================
// FREERTOS TASKS
// ============================================================================

void uiTask(void* parameter) {
    Serial.println("[UI_TASK] UI task started");

    // Wait for UI manager to be initialized
    while (!uiManager) {
        delay(100);
    }

    Serial.println("[UI_TASK] UI manager ready, starting update loop");

    while (true) {
        // Update LVGL display and handle touch input
        uiManager->update();

        // Maintain target frame rate (30 FPS)
        delay(1000 / UI_UPDATE_RATE_HZ);
    }
}

#ifndef DISABLE_AUDIO_TEMP
void audioTask(void* parameter) {
    Serial.println("[AUDIO_TASK] Audio task started");

    // Wait for audio pipeline to be initialized
    while (!audioPipeline) {
        delay(100);
    }

    Serial.println("[AUDIO_TASK] Audio pipeline ready");

    unsigned long lastStatsTime = 0;
    int16_t* audioFrame = (int16_t*)malloc(320 * sizeof(int16_t));  // 20ms @ 16kHz

    while (true) {
        // Process audio pipeline (handles all audio I/O, encoding, decoding)
        // This includes wake word detection when in IDLE mode
        audioPipeline->process();

        // Debug recording
        if (isRecording && debugAudioBuffer) {
            int16_t* currentFrame = audioPipeline->getMicFrame();
            size_t frameSize = audioPipeline->getFrameSize();

            if (currentFrame && frameSize > 0) {
                // Copy frame to debug buffer
                size_t samplesToCopy = frameSize;
                if (debugBufferIndex + samplesToCopy > DEBUG_BUFFER_SIZE) {
                    samplesToCopy = DEBUG_BUFFER_SIZE - debugBufferIndex;
                    isRecording = false; // Buffer full
                    Serial.println("Recording stopped (buffer full).");
                }

                if (samplesToCopy > 0) {
                    memcpy(debugAudioBuffer + debugBufferIndex, currentFrame, samplesToCopy * sizeof(int16_t));
                    debugBufferIndex += samplesToCopy;
                }
            }
        }

        // Phase 6: Process speech recognition when in LISTENING state
        if (stateMachine && stateMachine->getState() == AppState::LISTENING) {
            if (speechRecognizer && speechRecognizer->isEnabled()) {
                // Read audio from microphone for speech recognition
                I2SManager* i2s = audioPipeline->getI2S();
                if (i2s && audioFrame) {
                    size_t samplesRead = i2s->readMicrophone(audioFrame, 320);
                    if (samplesRead > 0) {
                        speechRecognizer->process(audioFrame, samplesRead);
                    }
                }
            }

            // Check for command timeout
            if (commandProcessor && commandProcessor->hasTimedOut()) {
                // On timeout, check if we have a partial AI query
                CommandResult partial = commandProcessor->getPartialCommand();
                if (partial.command == VoiceCommand::ASK_AI && !partial.aiQuery.isEmpty()) {
                    Serial.printf("[AUDIO_TASK] Command timeout - processing partial AI query: '%s'\n",
                                  partial.aiQuery.c_str());
                    if (aiManager) {
                        aiManager->processQuery(partial.aiQuery);
                    }
                } else {
                    Serial.println("[AUDIO_TASK] Command timeout - returning to IDLE");
                    stateMachine->setState(AppState::IDLE);
                }
            }
        }

        // Print audio statistics periodically (every 10 seconds)
        if (millis() - lastStatsTime > 10000) {
            lastStatsTime = millis();

            if (audioPipeline->isInCall()) {
                uint32_t txPackets, rxPackets, lostPackets;
                audioPipeline->getStats(txPackets, rxPackets, lostPackets);

                uint8_t micLevel = audioPipeline->getMicrophoneLevel();
                uint8_t spkLevel = audioPipeline->getSpeakerLevel();

                Serial.println("[AUDIO_TASK] ========== Audio Stats ==========");
                Serial.printf("[AUDIO_TASK] TX Packets: %u\n", txPackets);
                Serial.printf("[AUDIO_TASK] RX Packets: %u\n", rxPackets);
                Serial.printf("[AUDIO_TASK] Lost Packets: %u (%.1f%%)\n",
                              lostPackets,
                              rxPackets > 0 ? (lostPackets * 100.0f / rxPackets) : 0.0f);
                Serial.printf("[AUDIO_TASK] Mic Level: %u%%\n", micLevel);
                Serial.printf("[AUDIO_TASK] Speaker Level: %u%%\n", spkLevel);
                Serial.println("[AUDIO_TASK] ==============================");
            }
        }

        // Small delay (audio processing is real-time)
        delay(1);
    }
}
#endif // DISABLE_AUDIO_TEMP

void mqttTask(void* parameter) {
    Serial.println("[MQTT_TASK] MQTT task started");

    // Wait for MQTT client to be initialized
    while (!mqttClient) {
        delay(100);
    }

    // Connect to MQTT broker
    Serial.println("[MQTT_TASK] Attempting to connect to broker...");
    mqttClient->connect();

    unsigned long lastRegistryPrint = 0;

    while (true) {
        // Maintain MQTT connection and process messages
        mqttClient->loop();

        // Print device registry periodically (for debugging)
        if (deviceRegistry && millis() - lastRegistryPrint > 30000) {
            lastRegistryPrint = millis();
            deviceRegistry->printRegistry();
        }

        delay(10);  // Small delay
    }
}

// ============================================================================
// MQTT CALLBACKS
// ============================================================================

void onDeviceDiscovered(const String& deviceId, const String& roomName, const String& ip) {
    Serial.println("[CALLBACK] ========================================");
    Serial.printf("[CALLBACK] Device discovered: %s\n", roomName.c_str());
    Serial.printf("[CALLBACK]   Device ID: %s\n", deviceId.c_str());
    Serial.printf("[CALLBACK]   IP Address: %s\n", ip.c_str());
    Serial.println("[CALLBACK] ========================================\n");

    // Phase 7: Update UI device list
    if (uiManager) {
        uiManager->updateDeviceList();
    }

#ifndef DISABLE_AUDIO_TEMP
    // Phase 6: Add room to voice command processor
    if (commandProcessor) {
        commandProcessor->addRoom(roomName);
    }
#endif
}

#ifndef DISABLE_AUDIO_TEMP
void onCallInitiated(const String& targetRoom) {
    Serial.println("[CALLBACK] ========================================");
    Serial.printf("[CALLBACK] Call initiation requested from Home Assistant\n");
    Serial.printf("[CALLBACK]   Target room: %s\n", targetRoom.c_str());
    Serial.println("[CALLBACK] ========================================\n");

    if (!callManager) {
        Serial.println("[CALLBACK] ERROR: Call manager not initialized");
        return;
    }

    // Initiate call through call manager
    if (callManager->initiateCall(targetRoom)) {
        Serial.println("[CALLBACK] Call initiated successfully");
    } else {
        Serial.println("[CALLBACK] Failed to initiate call");
    }
}

void onCallRequest(const String& fromDeviceId, const String& fromRoom, const String& sessionId) {
    Serial.println("[CALLBACK] ========================================");
    Serial.printf("[CALLBACK] Incoming call from %s (%s)\n", fromRoom.c_str(), fromDeviceId.c_str());
    Serial.printf("[CALLBACK]   Session ID: %s\n", sessionId.c_str());
    Serial.println("[CALLBACK] ========================================\n");

    if (!callManager) {
        Serial.println("[CALLBACK] ERROR: Call manager not initialized");
        return;
    }

    // Handle incoming call
    callManager->handleIncomingCall(fromDeviceId, fromRoom, sessionId.toInt());

    // TODO: Update UI to show incoming call screen with accept/reject buttons
    // For now, auto-accept after 2 seconds (for testing)
    // In production, user should accept via touch screen or voice command
}

void onCallAccept(const String& sessionId, uint16_t udpPort) {
    Serial.println("[CALLBACK] ========================================");
    Serial.printf("[CALLBACK] Call accepted\n");
    Serial.printf("[CALLBACK]   Session ID: %s\n", sessionId.c_str());
    Serial.printf("[CALLBACK]   UDP Port: %d\n", udpPort);
    Serial.println("[CALLBACK] ========================================\n");

    if (!callManager) {
        Serial.println("[CALLBACK] ERROR: Call manager not initialized");
        return;
    }

    // Get remote device IP from registry
    CallSession* session = callManager->getCurrentSession();
    if (!session) {
        Serial.println("[CALLBACK] ERROR: No active session");
        return;
    }

    RemoteDevice* device = deviceRegistry->getDevice(session->getRemoteDeviceId());
    if (!device) {
        Serial.println("[CALLBACK] ERROR: Remote device not found");
        return;
    }

    IPAddress remoteIp;
    remoteIp.fromString(device->ipAddress);

    // Handle call accepted
    callManager->handleCallAccepted(sessionId.toInt(), remoteIp, udpPort);
}

void onCallReject(const String& sessionId, const String& reason) {
    Serial.println("[CALLBACK] ========================================");
    Serial.printf("[CALLBACK] Call rejected\n");
    Serial.printf("[CALLBACK]   Session ID: %s\n", sessionId.c_str());
    Serial.printf("[CALLBACK]   Reason: %s\n", reason.c_str());
    Serial.println("[CALLBACK] ========================================\n");

    if (!callManager) {
        Serial.println("[CALLBACK] ERROR: Call manager not initialized");
        return;
    }

    // Handle call rejected
    callManager->handleCallRejected(sessionId.toInt(), reason);
}

void onCallHangup(const String& sessionId) {
    Serial.println("[CALLBACK] ========================================");
    Serial.printf("[CALLBACK] Call hangup\n");
    Serial.printf("[CALLBACK]   Session ID: %s\n", sessionId.c_str());
    Serial.println("[CALLBACK] ========================================\n");

    if (!callManager) {
        Serial.println("[CALLBACK] ERROR: Call manager not initialized");
        return;
    }

    // Handle call hangup
    callManager->handleCallHangup(sessionId.toInt());
}
#endif // DISABLE_AUDIO_TEMP

void onStateChanged(AppState oldState, AppState newState) {
    Serial.println("[STATE] ========================================");
    Serial.printf("[STATE] State changed: %s -> %s\n",
                  StateMachine::stateToString(oldState).c_str(),
                  StateMachine::stateToString(newState).c_str());
    Serial.println("[STATE] ========================================\n");

#ifndef DISABLE_AUDIO_TEMP
    // Handle LISTENING state (Phase 6: Voice Commands)
    if (newState == AppState::LISTENING) {
        if (commandProcessor && speechRecognizer) {
            // Start listening for voice command
            commandProcessor->startListening(COMMAND_TIMEOUT_MS);
            speechRecognizer->enable(true);
            Serial.println("[STATE] Started listening for voice command");
        }
    } else {
        // Stop listening when leaving LISTENING state
        if (commandProcessor && commandProcessor->isListening()) {
            commandProcessor->stopListening();
        }
        if (speechRecognizer) {
            speechRecognizer->enable(false);
        }
    }
#endif

    // Phase 7: Update UI based on state
    if (uiManager) {
        uiManager->onStateChanged(oldState, newState);
    }
}
