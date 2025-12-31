/*
 * ESP32 Multi-Room Intercom System
 * Main Entry Point
 *
 * Phase 4: Call Management - COMPLETE INTERCOM SYSTEM
 *
 * This application orchestrates all subsystems:
 * - Device management and configuration
 * - WiFi connectivity
 * - MQTT client for signaling and Home Assistant integration
 * - Device discovery and registry
 * - Audio pipeline (I2S, Opus, UDP)
 * - Call management (complete call lifecycle)
 * - Touch screen UI (Phase 1+)
 */

#include <Arduino.h>
#include <WiFi.h>
#include "config.h"

// Phase 2 Components
#include "core/device_manager.h"
#include "call/device_registry.h"
#include "network/mqtt_client.h"

// Phase 3 Components
#include "audio/audio_pipeline.h"

// Phase 4 Components
#include "core/state_machine.h"
#include "call/call_manager.h"

// ============================================================================
// GLOBAL OBJECTS
// ============================================================================

DeviceManager* deviceManager = nullptr;
DeviceRegistry* deviceRegistry = nullptr;
MqttClient* mqttClient = nullptr;
AudioPipeline* audioPipeline = nullptr;
StateMachine* stateMachine = nullptr;
CallManager* callManager = nullptr;

// Task handles for FreeRTOS tasks
TaskHandle_t uiTaskHandle = NULL;
TaskHandle_t audioTaskHandle = NULL;
TaskHandle_t mqttTaskHandle = NULL;

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================

void setupSerial();
void setupDeviceManager();
void setupWiFi();
void setupDisplay();
void setupAudio();
void setupMQTT();
void setupCallManager();
void printSystemInfo();

// FreeRTOS Tasks
void uiTask(void* parameter);
void audioTask(void* parameter);
void mqttTask(void* parameter);

// MQTT Callbacks
void onDeviceDiscovered(const String& deviceId, const String& roomName, const String& ip);
void onCallInitiated(const String& targetRoom);
void onCallRequest(const String& fromDeviceId, const String& fromRoom, const String& sessionId);
void onCallAccept(const String& sessionId, uint16_t udpPort);
void onCallReject(const String& sessionId, const String& reason);
void onCallHangup(const String& sessionId);

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
    Serial.println("  ESP32 Multi-Room Intercom System");
    Serial.println("  Version: " FIRMWARE_VERSION);
    Serial.println("  Build: " BUILD_DATE " " BUILD_TIME);
    Serial.println("  Phase: 4 - Call Management");
    Serial.println("  Status: COMPLETE SYSTEM");
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

    // Initialize audio subsystem
    Serial.println("[SETUP] Setting up audio...");
    setupAudio();

    // Initialize MQTT client
    Serial.println("[SETUP] Setting up MQTT...");
    setupMQTT();

    // Initialize call manager
    Serial.println("[SETUP] Setting up call manager...");
    setupCallManager();

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

    // Process call manager (check timeouts, state transitions)
    if (callManager) {
        callManager->process();
    }

    // Check device registry for timeouts
    if (deviceRegistry) {
        static unsigned long lastRegistryCheck = 0;
        if (millis() - lastRegistryCheck > 10000) {  // Every 10 seconds
            lastRegistryCheck = millis();
            deviceRegistry->checkTimeouts();
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
    Serial.println("[DISPLAY] Phase 1+ - Display implementation pending");
    Serial.println("[DISPLAY] TODO: Initialize TFT_eSPI for Waveshare 4.3\"");
    Serial.println("[DISPLAY] TODO: Initialize LVGL graphics library");
    Serial.println("[DISPLAY] TODO: Setup touch screen driver");
    Serial.println("[DISPLAY] TODO: Create idle screen with device list");

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
    mqttClient->onCallInitiate(onCallInitiated);
    mqttClient->onCallRequest(onCallRequest);
    mqttClient->onCallAccept(onCallAccept);
    mqttClient->onCallReject(onCallReject);
    mqttClient->onCallHangup(onCallHangup);

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
}

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

    while (true) {
        // TODO Phase 1+: Update LVGL display
        // TODO Phase 1+: Handle touch input
        // TODO Phase 4: Update screen based on call state

        // LVGL needs to be called periodically
        // lv_timer_handler();

        delay(1000 / UI_UPDATE_RATE_HZ);
    }
}

void audioTask(void* parameter) {
    Serial.println("[AUDIO_TASK] Audio task started");

    // Wait for audio pipeline to be initialized
    while (!audioPipeline) {
        delay(100);
    }

    Serial.println("[AUDIO_TASK] Audio pipeline ready");

    unsigned long lastStatsTime = 0;

    while (true) {
        // Process audio pipeline (handles all audio I/O, encoding, decoding)
        audioPipeline->process();

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

    // TODO Phase 4: Update UI to show new device in list
}

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

void onStateChanged(AppState oldState, AppState newState) {
    Serial.println("[STATE] ========================================");
    Serial.printf("[STATE] State changed: %s -> %s\n",
                  StateMachine::stateToString(oldState).c_str(),
                  StateMachine::stateToString(newState).c_str());
    Serial.println("[STATE] ========================================\n");

    // TODO: Update UI based on state
    // - IDLE: Show dashboard with device list
    // - LISTENING: Show "Listening..." screen
    // - CALLING: Show "Calling [Room]..." screen
    // - RINGING: Show incoming call screen with Accept/Reject
    // - ACTIVE_CALL: Show active call screen with Hang Up button
    // - HANGING_UP: Show "Ending call..." message
    // - ERROR: Show error message
}
