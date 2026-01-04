/**
 * HERMES Audio Test Program
 *
 * Tests microphone input and speaker output with ES8311 audio codec
 *
 * Hardware: Waveshare ESP32-S3-Touch-AMOLED-1.8"
 * - Built-in microphone → ES8311 ADC → I2S (GPIO 1,2,42)
 * - I2S (GPIO 9,45,10) → ES8311 DAC → Built-in speaker
 * - ES8311 I2C control: SDA=GPIO15, SCL=GPIO14
 */

#include <Arduino.h>
#include <driver/i2s.h>
#include "src/audio/ES8311.h"

// Pin definitions based on Waveshare ESP32-S3-AMOLED-1.8 actual hardware
// Reference: ESPHome config for ESP32-S3-Touch-AMOLED-1.75 (same pinout)
#define I2S_MIC_NUM         I2S_NUM_0
#define I2S_MIC_SCK_PIN     9   // BCLK (shared with speaker)
#define I2S_MIC_WS_PIN      45  // LRCLK/WS (shared with speaker)
#define I2S_MIC_SD_PIN      10  // DIN (microphone data input)

#define I2S_SPK_NUM         I2S_NUM_1
#define I2S_SPK_SCK_PIN     9   // BCLK (shared with microphone)
#define I2S_SPK_WS_PIN      45  // LRCLK/WS (shared with microphone)
#define I2S_SPK_SD_PIN      8   // DOUT (speaker data output)

#define I2S_MCLK_PIN        42  // Master clock (optional, shared)

// I2C pins for ES8311 codec control
#define I2C_SDA_PIN         15
#define I2C_SCL_PIN         14

// Audio parameters
#define SAMPLE_RATE         16000
#define BITS_PER_SAMPLE     I2S_BITS_PER_SAMPLE_16BIT
#define CHANNELS            1
#define DMA_BUF_COUNT       8
#define DMA_BUF_LEN         512

// Test tone parameters
#define TONE_FREQUENCY      440  // Hz (A4 note)
#define TONE_DURATION_MS    1000

// Buffer for audio samples
int16_t audioBuffer[DMA_BUF_LEN];

// ES8311 codec instance
ES8311 codec;

void setupMicrophone() {
    Serial.println("[MIC] Initializing microphone...");
    Serial.println("[MIC] Testing PDM mode configuration");
    Serial.flush();

    // Try PDM mode configuration (for digital PDM microphones)
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = BITS_PER_SAMPLE,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = DMA_BUF_COUNT,
        .dma_buf_len = DMA_BUF_LEN,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0  // PDM doesn't use MCLK
    };

    i2s_pin_config_t pin_config = {
        .mck_io_num = I2S_PIN_NO_CHANGE,  // PDM doesn't use MCLK
        .bck_io_num = I2S_PIN_NO_CHANGE,  // PDM doesn't use separate CLK
        .ws_io_num = I2S_MIC_WS_PIN,      // PDM CLK on WS pin (GPIO 45)
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = I2S_MIC_SD_PIN     // PDM DATA on SD pin (GPIO 10)
    };

    esp_err_t err = i2s_driver_install(I2S_MIC_NUM, &i2s_config, 0, NULL);
    if (err != ESP_OK) {
        Serial.printf("[MIC] ERROR: Failed to install I2S driver: %d\n", err);
        return;
    }

    err = i2s_set_pin(I2S_MIC_NUM, &pin_config);
    if (err != ESP_OK) {
        Serial.printf("[MIC] ERROR: Failed to set I2S pins: %d\n", err);
        return;
    }

    Serial.println("[MIC] ✓ Microphone initialized in PDM mode");
}

void setupSpeaker() {
    Serial.println("[SPK] Initializing speaker (ES8311/MAX98357A)...");
    Serial.flush();

    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = BITS_PER_SAMPLE,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = DMA_BUF_COUNT,
        .dma_buf_len = DMA_BUF_LEN,
        .use_apll = false,
        .tx_desc_auto_clear = true,
        .fixed_mclk = SAMPLE_RATE * 256  // MCLK = 256 * sample_rate = 4.096 MHz
    };

    i2s_pin_config_t pin_config = {
        .mck_io_num = I2S_MCLK_PIN,  // Enable MCLK output on GPIO 42
        .bck_io_num = I2S_SPK_SCK_PIN,
        .ws_io_num = I2S_SPK_WS_PIN,
        .data_out_num = I2S_SPK_SD_PIN,
        .data_in_num = I2S_PIN_NO_CHANGE
    };

    esp_err_t err = i2s_driver_install(I2S_SPK_NUM, &i2s_config, 0, NULL);
    if (err != ESP_OK) {
        Serial.printf("[SPK] ERROR: Failed to install I2S driver: %d\n", err);
        return;
    }

    err = i2s_set_pin(I2S_SPK_NUM, &pin_config);
    if (err != ESP_OK) {
        Serial.printf("[SPK] ERROR: Failed to set I2S pins: %d\n", err);
        return;
    }

    Serial.println("[SPK] ✓ Speaker initialized");
}

void testMicrophone() {
    Serial.println("\n========================================");
    Serial.println("  MICROPHONE TEST");
    Serial.println("========================================");
    Serial.println("Reading microphone for 5 seconds...");
    Serial.println("Speak into the microphone!\n");

    size_t bytesRead;
    unsigned long startTime = millis();
    int sampleCount = 0;

    while (millis() - startTime < 5000) {
        i2s_read(I2S_MIC_NUM, audioBuffer, sizeof(audioBuffer), &bytesRead, portMAX_DELAY);

        if (bytesRead > 0) {
            size_t samplesRead = bytesRead / sizeof(int16_t);

            // Calculate audio level (RMS)
            int32_t sum = 0;
            for (size_t i = 0; i < samplesRead; i++) {
                int32_t sample = audioBuffer[i];
                sum += (sample * sample);
            }

            float rms = sqrt((float)sum / samplesRead);
            int level = (int)(rms / 100);  // Scale for display

            // Display level bar every 100ms
            if (sampleCount % 10 == 0) {
                Serial.print("Level: [");
                for (int i = 0; i < 50; i++) {
                    if (i < level) Serial.print("█");
                    else Serial.print(" ");
                }
                Serial.printf("] %.0f\n", rms);
            }

            sampleCount++;
        }

        delay(10);
    }

    Serial.println("\n✓ Microphone test complete");
}

void testSpeaker() {
    Serial.println("\n========================================");
    Serial.println("  SPEAKER TEST");
    Serial.println("========================================");
    Serial.printf("Playing %d Hz tone for %d ms...\n\n", TONE_FREQUENCY, TONE_DURATION_MS);

    // Generate sine wave tone
    const int samplesPerCycle = SAMPLE_RATE / TONE_FREQUENCY;
    const int totalSamples = (SAMPLE_RATE * TONE_DURATION_MS) / 1000;
    const int amplitude = 8000;  // Moderate volume

    size_t bytesWritten;
    int sampleIndex = 0;

    while (sampleIndex < totalSamples) {
        // Fill buffer with sine wave
        for (int i = 0; i < DMA_BUF_LEN && sampleIndex < totalSamples; i++, sampleIndex++) {
            float angle = (2.0 * PI * sampleIndex) / samplesPerCycle;
            audioBuffer[i] = (int16_t)(amplitude * sin(angle));
        }

        // Write to speaker
        i2s_write(I2S_SPK_NUM, audioBuffer, DMA_BUF_LEN * sizeof(int16_t), &bytesWritten, portMAX_DELAY);
    }

    // Wait for playback to finish
    delay(100);

    Serial.println("✓ Speaker test complete");
}

void testLoopback() {
    Serial.println("\n========================================");
    Serial.println("  LOOPBACK TEST");
    Serial.println("========================================");
    Serial.println("Recording from mic and playing to speaker...");
    Serial.println("Speak into the microphone - you should hear yourself!");
    Serial.println("Running for 10 seconds...\n");

    size_t bytesRead, bytesWritten;
    unsigned long startTime = millis();

    while (millis() - startTime < 10000) {
        // Read from microphone
        i2s_read(I2S_MIC_NUM, audioBuffer, sizeof(audioBuffer), &bytesRead, portMAX_DELAY);

        if (bytesRead > 0) {
            // Apply some gain for better audibility
            for (int i = 0; i < DMA_BUF_LEN; i++) {
                audioBuffer[i] = audioBuffer[i] * 2;  // 2x gain
            }

            // Write to speaker
            i2s_write(I2S_SPK_NUM, audioBuffer, bytesRead, &bytesWritten, portMAX_DELAY);
        }
    }

    Serial.println("\n✓ Loopback test complete");
}

void printMenu() {
    Serial.println("\n========================================");
    Serial.println("  HERMES Audio Test Menu");
    Serial.println("========================================");
    Serial.println("1 - Test Microphone (show audio levels)");
    Serial.println("2 - Test Speaker (play 440 Hz tone)");
    Serial.println("3 - Test Loopback (mic → speaker)");
    Serial.println("4 - Run All Tests");
    Serial.println("5 - Show Menu");
    Serial.println("========================================\n");
}

void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial.println("\n\n========================================");
    Serial.println("  HERMES Audio Hardware Test");
    Serial.println("========================================");
    Serial.println("Starting initialization...\n");
    Serial.flush();

    // Initialize ES8311 codec first (controls audio routing)
    Serial.println("Step 1: Initializing ES8311 audio codec...");
    Serial.flush();
    if (!codec.begin(I2C_SDA_PIN, I2C_SCL_PIN, SAMPLE_RATE)) {
        Serial.println("[ERROR] Failed to initialize ES8311 codec!");
        Serial.println("Check I2C connections and codec address.");
        while(1) delay(100);  // Halt
    }
    Serial.println();

    // Initialize I2S ports
    Serial.println("Step 2: Setting up microphone I2S...");
    Serial.flush();
    setupMicrophone();

    Serial.println("Step 3: Setting up speaker I2S...");
    Serial.flush();
    setupSpeaker();

    Serial.println("\n✓ Audio hardware initialized");
    Serial.flush();

    printMenu();
}

void loop() {
    if (Serial.available()) {
        char cmd = Serial.read();

        switch (cmd) {
            case '1':
                testMicrophone();
                printMenu();
                break;

            case '2':
                testSpeaker();
                printMenu();
                break;

            case '3':
                testLoopback();
                printMenu();
                break;

            case '4':
                testMicrophone();
                delay(1000);
                testSpeaker();
                delay(1000);
                testLoopback();
                printMenu();
                break;

            case '5':
                printMenu();
                break;
        }
    }

    delay(100);
}
