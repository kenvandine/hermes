#include "i2s_manager.h"
#include "config.h"

I2SManager::I2SManager()
    : i2sPort(I2S_MIC_NUM), // Using I2S_NUM_0 (MIC_NUM) as the main port
      currentSampleRate(0),
      codecReady(false),
      i2sReady(false),
      micEnabled(false),
      speakerEnabled(false),
      muted(false),
      volume(DEFAULT_SPEAKER_VOLUME) {
}

I2SManager::~I2SManager() {
    stopMicrophone();
    stopSpeaker();
    if (i2sReady) {
        i2s_driver_uninstall(i2sPort);
    }
    if (codecReady) {
        codec.powerDown();
    }
}

bool I2SManager::begin(uint32_t sampleRate) {
    Serial.println("[I2S] Initializing ES8311 audio codec and I2S...");

    // Initialize ES8311 codec on I2C bus
    // I2C pins are defined in config.h (TOUCH_SDA=15, TOUCH_SCL=14)
    if (!codec.begin(15, 14, sampleRate)) {
        Serial.println("[I2S] ERROR: Failed to initialize ES8311 codec");
        return false;
    }
    codecReady = true;
    Serial.println("[I2S] ES8311 codec initialized successfully");

    // Initialize I2S in Full Duplex Mode (Master)
    // We use the pin definitions from config.h
    // The hardware uses ES8311 for both Mic and Speaker via I2S

    Serial.printf("[I2S] Initializing I2S port %d @ %d Hz\n", i2sPort, sampleRate);

    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX),
        .sample_rate = sampleRate,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT, // ES8311 is mono or stereo, but we usually handle mono for voice
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = I2S_DMA_BUF_COUNT,
        .dma_buf_len = I2S_DMA_BUF_LEN,
        .use_apll = true, // Use APLL for better audio quality
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0
    };

    i2s_pin_config_t pin_config = {
        .mck_io_num = I2S_MIC_MCLK_PIN,   // MCLK on GPIO 42
        .bck_io_num = I2S_MIC_SCK_PIN,    // BCLK on GPIO 9
        .ws_io_num = I2S_MIC_WS_PIN,      // WS/LRCLK on GPIO 45
        .data_out_num = I2S_SPK_SD_PIN,   // DOUT on GPIO 8 (Speaker)
        .data_in_num = I2S_MIC_SD_PIN     // DIN on GPIO 10 (Microphone)
    };

    esp_err_t err = i2s_driver_install(i2sPort, &i2s_config, 0, NULL);
    if (err != ESP_OK) {
        Serial.printf("[I2S] ERROR: Failed to install I2S driver: %d\n", err);
        return false;
    }

    err = i2s_set_pin(i2sPort, &pin_config);
    if (err != ESP_OK) {
        Serial.printf("[I2S] ERROR: Failed to set I2S pins: %d\n", err);
        i2s_driver_uninstall(i2sPort);
        return false;
    }

    // Ensure MCLK is outputting
    // Some ESP-IDF versions need explicit MCLK enabling, but i2s_set_pin with mck_io_num should handle it.

    currentSampleRate = sampleRate;
    i2sReady = true;

    // Clear DMA buffers
    i2s_zero_dma_buffer(i2sPort);

    Serial.println("[I2S] I2S driver initialized successfully (Full Duplex)");
    return true;
}

bool I2SManager::beginMicrophone(uint32_t sampleRate, i2s_bits_per_sample_t bitsPerSample) {
    if (!codecReady || !i2sReady) {
        Serial.println("[I2S] ERROR: Call begin() first to initialize I2S/Codec");
        return false;
    }

    if (sampleRate != currentSampleRate) {
        Serial.println("[I2S] WARNING: Requested sample rate differs from initialized rate. Ignoring.");
    }

    Serial.println("[I2S] Microphone enabled");
    micEnabled = true;
    return true;
}

bool I2SManager::beginSpeaker(uint32_t sampleRate, i2s_bits_per_sample_t bitsPerSample) {
    if (!codecReady || !i2sReady) {
        Serial.println("[I2S] ERROR: Call begin() first to initialize I2S/Codec");
        return false;
    }

    if (sampleRate != currentSampleRate) {
        Serial.println("[I2S] WARNING: Requested sample rate differs from initialized rate. Ignoring.");
    }

    Serial.println("[I2S] Speaker enabled");
    speakerEnabled = true;
    return true;
}

void I2SManager::stopMicrophone() {
    if (micEnabled) {
        micEnabled = false;
        Serial.println("[I2S] Microphone disabled");
    }
}

void I2SManager::stopSpeaker() {
    if (speakerEnabled) {
        speakerEnabled = false;
        Serial.println("[I2S] Speaker disabled");
    }
}

size_t I2SManager::readMicrophone(int16_t* buffer, size_t sampleCount) {
    if (!micEnabled || !i2sReady || !buffer) {
        return 0;
    }

    size_t bytesRead = 0;
    size_t bytesToRead = sampleCount * sizeof(int16_t);

    esp_err_t err = i2s_read(i2sPort, buffer, bytesToRead, &bytesRead, portMAX_DELAY);

    if (err != ESP_OK) {
        Serial.printf("[I2S] ERROR: Microphone read failed: %d\n", err);
        return 0;
    }

    return bytesRead / sizeof(int16_t);
}

size_t I2SManager::writeSpeaker(const int16_t* buffer, size_t sampleCount) {
    if (!speakerEnabled || !i2sReady || !buffer) {
        return 0;
    }

    // Apply volume and mute
    int16_t* processedBuffer = (int16_t*)buffer;
    if (muted) {
        // Write silence
        int16_t* silence = (int16_t*)calloc(sampleCount, sizeof(int16_t));
        if (silence) {
            size_t bytesWritten = 0;
            i2s_write(i2sPort, silence, sampleCount * sizeof(int16_t), &bytesWritten, portMAX_DELAY);
            free(silence);
            return bytesWritten / sizeof(int16_t);
        }
        return 0;
    } else if (volume != 100) {
        // Create temporary buffer for volume adjustment
        processedBuffer = (int16_t*)malloc(sampleCount * sizeof(int16_t));
        if (processedBuffer) {
            memcpy(processedBuffer, buffer, sampleCount * sizeof(int16_t));
            applyVolume(processedBuffer, sampleCount);
        } else {
            processedBuffer = (int16_t*)buffer;  // Fallback to original buffer
        }
    }

    size_t bytesWritten = 0;
    size_t bytesToWrite = sampleCount * sizeof(int16_t);

    esp_err_t err = i2s_write(i2sPort, processedBuffer, bytesToWrite, &bytesWritten, portMAX_DELAY);

    // Free temporary buffer if allocated
    if (processedBuffer != buffer) {
        free(processedBuffer);
    }

    if (err != ESP_OK) {
        Serial.printf("[I2S] ERROR: Speaker write failed: %d\n", err);
        return 0;
    }

    return bytesWritten / sizeof(int16_t);
}

void I2SManager::setVolume(uint8_t vol) {
    volume = (vol > 100) ? 100 : vol;

    // Also set codec hardware volume
    if (codecReady) {
        codec.setVolume(volume);
    }
}

uint8_t I2SManager::getVolume() const {
    return volume;
}

void I2SManager::mute(bool enabled) {
    muted = enabled;
}

bool I2SManager::isMicrophoneReady() const {
    return micEnabled && i2sReady;
}

bool I2SManager::isSpeakerReady() const {
    return speakerEnabled && i2sReady;
}

uint32_t I2SManager::getMicrophoneSampleRate() const {
    return currentSampleRate;
}

uint32_t I2SManager::getSpeakerSampleRate() const {
    return currentSampleRate;
}

void I2SManager::applyVolume(int16_t* buffer, size_t sampleCount) {
    if (volume == 100) {
        return;  // No adjustment needed
    }

    // Apply volume scaling (0-100 -> 0.0-1.0)
    float volumeScale = volume / 100.0f;

    for (size_t i = 0; i < sampleCount; i++) {
        buffer[i] = (int16_t)(buffer[i] * volumeScale);
    }
}
