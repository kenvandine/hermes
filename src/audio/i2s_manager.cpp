#include "i2s_manager.h"
#include "config.h"

I2SManager::I2SManager()
    : micPort(I2S_MIC_NUM),
      speakerPort(I2S_SPK_NUM),
      micSampleRate(0),
      speakerSampleRate(0),
      micReady(false),
      speakerReady(false),
      muted(false),
      volume(DEFAULT_SPEAKER_VOLUME) {
}

I2SManager::~I2SManager() {
    stopMicrophone();
    stopSpeaker();
}

bool I2SManager::beginMicrophone(uint32_t sampleRate, i2s_bits_per_sample_t bitsPerSample) {
    Serial.printf("[I2S] Initializing microphone on I2S%d @ %d Hz\n", micPort, sampleRate);

    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = sampleRate,
        .bits_per_sample = bitsPerSample,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = I2S_DMA_BUF_COUNT,
        .dma_buf_len = I2S_DMA_BUF_LEN,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_MIC_SCK_PIN,
        .ws_io_num = I2S_MIC_WS_PIN,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = I2S_MIC_SD_PIN
    };

    esp_err_t err = i2s_driver_install(micPort, &i2s_config, 0, NULL);
    if (err != ESP_OK) {
        Serial.printf("[I2S] ERROR: Failed to install microphone driver: %d\n", err);
        return false;
    }

    err = i2s_set_pin(micPort, &pin_config);
    if (err != ESP_OK) {
        Serial.printf("[I2S] ERROR: Failed to set microphone pins: %d\n", err);
        i2s_driver_uninstall(micPort);
        return false;
    }

    // Clear DMA buffers
    i2s_zero_dma_buffer(micPort);

    micSampleRate = sampleRate;
    micReady = true;

    Serial.println("[I2S] Microphone initialized successfully");
    return true;
}

bool I2SManager::beginSpeaker(uint32_t sampleRate, i2s_bits_per_sample_t bitsPerSample) {
    Serial.printf("[I2S] Initializing speaker on I2S%d @ %d Hz\n", speakerPort, sampleRate);

    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = sampleRate,
        .bits_per_sample = bitsPerSample,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = I2S_DMA_BUF_COUNT,
        .dma_buf_len = I2S_DMA_BUF_LEN,
        .use_apll = false,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_SPK_SCK_PIN,
        .ws_io_num = I2S_SPK_WS_PIN,
        .data_out_num = I2S_SPK_SD_PIN,
        .data_in_num = I2S_PIN_NO_CHANGE
    };

    esp_err_t err = i2s_driver_install(speakerPort, &i2s_config, 0, NULL);
    if (err != ESP_OK) {
        Serial.printf("[I2S] ERROR: Failed to install speaker driver: %d\n", err);
        return false;
    }

    err = i2s_set_pin(speakerPort, &pin_config);
    if (err != ESP_OK) {
        Serial.printf("[I2S] ERROR: Failed to set speaker pins: %d\n", err);
        i2s_driver_uninstall(speakerPort);
        return false;
    }

    // Clear DMA buffers
    i2s_zero_dma_buffer(speakerPort);

    speakerSampleRate = sampleRate;
    speakerReady = true;

    Serial.println("[I2S] Speaker initialized successfully");
    return true;
}

void I2SManager::stopMicrophone() {
    if (micReady) {
        i2s_driver_uninstall(micPort);
        micReady = false;
        Serial.println("[I2S] Microphone stopped");
    }
}

void I2SManager::stopSpeaker() {
    if (speakerReady) {
        i2s_driver_uninstall(speakerPort);
        speakerReady = false;
        Serial.println("[I2S] Speaker stopped");
    }
}

size_t I2SManager::readMicrophone(int16_t* buffer, size_t sampleCount) {
    if (!micReady || !buffer) {
        return 0;
    }

    size_t bytesRead = 0;
    size_t bytesToRead = sampleCount * sizeof(int16_t);

    esp_err_t err = i2s_read(micPort, buffer, bytesToRead, &bytesRead, portMAX_DELAY);

    if (err != ESP_OK) {
        Serial.printf("[I2S] ERROR: Microphone read failed: %d\n", err);
        return 0;
    }

    return bytesRead / sizeof(int16_t);
}

size_t I2SManager::writeSpeaker(const int16_t* buffer, size_t sampleCount) {
    if (!speakerReady || !buffer) {
        return 0;
    }

    // Apply volume and mute
    int16_t* processedBuffer = (int16_t*)buffer;
    if (muted) {
        // Write silence
        int16_t* silence = (int16_t*)calloc(sampleCount, sizeof(int16_t));
        if (silence) {
            size_t bytesWritten = 0;
            i2s_write(speakerPort, silence, sampleCount * sizeof(int16_t), &bytesWritten, portMAX_DELAY);
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

    esp_err_t err = i2s_write(speakerPort, processedBuffer, bytesToWrite, &bytesWritten, portMAX_DELAY);

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
}

uint8_t I2SManager::getVolume() const {
    return volume;
}

void I2SManager::mute(bool enabled) {
    muted = enabled;
}

bool I2SManager::isMicrophoneReady() const {
    return micReady;
}

bool I2SManager::isSpeakerReady() const {
    return speakerReady;
}

uint32_t I2SManager::getMicrophoneSampleRate() const {
    return micSampleRate;
}

uint32_t I2SManager::getSpeakerSampleRate() const {
    return speakerSampleRate;
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
