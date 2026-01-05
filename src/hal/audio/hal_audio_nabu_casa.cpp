/**
 * Nabu Casa Audio HAL Implementation
 *
 * Implements audio support for Nabu Casa Voice PE with:
 * - Dual I2S buses (16kHz microphone input, 48kHz speaker output)
 * - XMOS XU316 audio processor (I2S master)
 * - TI AIC3204 codec
 *
 * Based on ESPHome configuration:
 * https://github.com/esphome/home-assistant-voice-pe
 */

#include "hal/audio/hal_audio_nabu_casa.h"
#include "devices/nabu_casa/device_config.h"
#include "devices/nabu_casa/device_pins.h"
#include <Arduino.h>
#include <Wire.h>
#include <driver/i2s.h>

// AIC3204 I2C address
#define AIC3204_I2C_ADDR    0x18

// AIC3204 Register Definitions
// Page 0 Registers
#define AIC3204_PAGE_CTRL       0x00    // Page Control Register
#define AIC3204_SW_RST          0x01    // Software Reset
#define AIC3204_CLK_PLL1        0x04    // Clock Setting Register 1
#define AIC3204_CLK_PLL2        0x05    // Clock Setting Register 2, P and R values
#define AIC3204_CLK_PLL3        0x06    // Clock Setting Register 3, J values
#define AIC3204_NDAC            0x0B    // NDAC Divider Value
#define AIC3204_MDAC            0x0C    // MDAC Divider Value
#define AIC3204_DOSR            0x0E    // DOSR Divider Value (OSR for DAC)
#define AIC3204_NADC            0x12    // NADC Divider Value
#define AIC3204_MADC            0x13    // MADC Divider Value
#define AIC3204_AOSR            0x14    // AOSR Divider Value (OSR for ADC)
#define AIC3204_CODEC_IF        0x1B    // CODEC Interface Control
#define AIC3204_AUDIO_IF_4      0x1F    // Audio Interface Setting Register 4
#define AIC3204_AUDIO_IF_5      0x20    // Audio Interface Setting Register 5
#define AIC3204_SCLK_MFP3       0x38    // SCLK/MFP3 Function Control
#define AIC3204_DAC_SIG_PROC    0x3C    // DAC Signal Processing Block Control
#define AIC3204_ADC_SIG_PROC    0x3D    // ADC Signal Processing Block Control
#define AIC3204_DAC_CH_SET1     0x3F    // DAC Channel Setup Register 1
#define AIC3204_DAC_CH_SET2     0x40    // DAC Channel Setup Register 2
#define AIC3204_DACL_VOL_D      0x41    // DAC Left Digital Volume Control
#define AIC3204_DACR_VOL_D      0x42    // DAC Right Digital Volume Control

// Page 1 Registers
#define AIC3204_LDO_CTRL        0x01    // LDO Control Register
#define AIC3204_PWR_CFG         0x02    // Power Configuration
#define AIC3204_PLAY_CFG1       0x03    // Playback Configuration Register 1
#define AIC3204_PLAY_CFG2       0x04    // Playback Configuration Register 2
#define AIC3204_CM_CTRL         0x0A    // Common Mode Control
#define AIC3204_HP_START        0x09    // Headphone Driver Startup Control
#define AIC3204_HPL_ROUTE       0x0C    // HPL Routing Selection
#define AIC3204_HPR_ROUTE       0x0D    // HPR Routing Selection
#define AIC3204_LOL_ROUTE       0x0E    // LOL Routing Selection
#define AIC3204_LOR_ROUTE       0x0F    // LOR Routing Selection
#define AIC3204_HPL_GAIN        0x10    // HPL Gain
#define AIC3204_HPR_GAIN        0x11    // HPR Gain
#define AIC3204_LOL_DRV_GAIN    0x12    // LOL Driver Gain
#define AIC3204_LOR_DRV_GAIN    0x13    // LOR Driver Gain
#define AIC3204_OP_PWR_CTRL     0x09    // Output Driver Power Control
#define AIC3204_REF_STARTUP     0x7B    // Reference Power Up Configuration

// Helper functions for AIC3204 I2C communication
static bool aic3204WriteReg(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(AIC3204_I2C_ADDR);
    Wire.write(reg);
    Wire.write(value);
    uint8_t error = Wire.endTransmission();
    if (error != 0) {
        Serial.printf("[AIC3204] I2C write error %d (reg=0x%02X, val=0x%02X)\n", error, reg, value);
        return false;
    }
    return true;
}

static bool aic3204SelectPage(uint8_t page) {
    return aic3204WriteReg(AIC3204_PAGE_CTRL, page);
}

HALAudioNabuCasa::HALAudioNabuCasa()
    : sampleRate_(DEVICE_SAMPLE_RATE)
    , volume_(DEVICE_DEFAULT_SPEAKER_VOL)
    , micGain_(DEVICE_DEFAULT_MIC_GAIN)
    , muted_(false)
    , micReady_(false)
    , speakerReady_(false) {
}

HALAudioNabuCasa::~HALAudioNabuCasa() {
    // Cleanup I2S drivers
    if (micReady_) {
        i2s_driver_uninstall(I2S_NUM_0);
    }
    if (speakerReady_) {
        i2s_driver_uninstall(I2S_NUM_1);
    }
}

bool HALAudioNabuCasa::begin(uint32_t sampleRate) {
    Serial.println("[HAL-Audio-NabuCasa] Initializing audio subsystem...");

    // Initialize I2C for AIC3204 control
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, 400000);  // 400kHz
    Serial.println("[HAL-Audio-NabuCasa] ✓ I2C initialized");

    // Reset XMOS voice kit (match ESPHome sequence exactly)
    pinMode(PIN_VOICE_KIT_RESET, OUTPUT);
    digitalWrite(PIN_VOICE_KIT_RESET, HIGH);  // HIGH first (ESPHome does this)
    delay(1);
    digitalWrite(PIN_VOICE_KIT_RESET, LOW);   // Then LOW
    Serial.println("[HAL-Audio-NabuCasa] Waiting for XMOS to boot (3 seconds)...");
    delay(3000);  // Wait for XMOS to fully boot (per ESPHome)
    Serial.println("[HAL-Audio-NabuCasa] ✓ XMOS reset complete");

    // Scan I2C bus to check if XMOS and AIC3204 are responding
    Serial.println("[HAL-Audio-NabuCasa] Scanning I2C bus...");
    int devicesFound = 0;
    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            Serial.printf("[HAL-Audio-NabuCasa]   Found device at 0x%02X\n", addr);
            devicesFound++;
        }
    }
    if (devicesFound == 0) {
        Serial.println("[HAL-Audio-NabuCasa] ⚠️  No I2C devices found!");
    } else {
        Serial.printf("[HAL-Audio-NabuCasa] ✓ Found %d I2C device(s)\n", devicesFound);
    }

    // Enable speaker amplifier
    pinMode(PIN_SPEAKER_AMP_ENABLE, OUTPUT);
    digitalWrite(PIN_SPEAKER_AMP_ENABLE, HIGH);
    Serial.println("[HAL-Audio-NabuCasa] ✓ Speaker amplifier enabled");

    // Enable LED power supply
    pinMode(PIN_LED_RING_POWER, OUTPUT);
    digitalWrite(PIN_LED_RING_POWER, HIGH);
    Serial.println("[HAL-Audio-NabuCasa] ✓ LED power enabled");

    // Initialize microphone (16kHz) and speaker (48kHz separately)
    // Note: We store the requested sample rate but use device-specific rates
    sampleRate_ = sampleRate;

    if (!beginMicrophone(16000)) {  // Fixed 16kHz for mic
        Serial.println("[HAL-Audio-NabuCasa] ✗ Failed to initialize microphone!");
        return false;
    }

    if (!beginSpeaker(48000)) {  // Fixed 48kHz for speaker
        Serial.println("[HAL-Audio-NabuCasa] ✗ Failed to initialize speaker!");
        return false;
    }

    Serial.println("[HAL-Audio-NabuCasa] ✓ Audio subsystem initialized successfully");
    return true;
}

bool HALAudioNabuCasa::beginMicrophone(uint32_t sampleRate) {
    Serial.printf("[HAL-Audio-NabuCasa] Initializing microphone (I2S_NUM_0, %dHz)...\n", sampleRate);

    // I2S configuration for microphone input (ESP32 is SLAVE, XMOS is master)
    i2s_config_t i2s_mic_config = {
        .mode = (i2s_mode_t)(I2S_MODE_SLAVE | I2S_MODE_RX),  // SLAVE RX mode
        .sample_rate = sampleRate,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,  // 32-bit as per ESPHome config
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,  // Stereo
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 256,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0
    };

    // Pin configuration for microphone
    i2s_pin_config_t mic_pin_config = {
        .bck_io_num = PIN_I2S_MIC_BCLK,
        .ws_io_num = PIN_I2S_MIC_LRCLK,
        .data_out_num = I2S_PIN_NO_CHANGE,  // No output
        .data_in_num = PIN_I2S_MIC_DIN
    };

    // Install and configure I2S driver
    esp_err_t err = i2s_driver_install(I2S_NUM_0, &i2s_mic_config, 0, NULL);
    if (err != ESP_OK) {
        Serial.printf("[HAL-Audio-NabuCasa] ✗ Failed to install I2S microphone driver: %d\n", err);
        return false;
    }

    err = i2s_set_pin(I2S_NUM_0, &mic_pin_config);
    if (err != ESP_OK) {
        Serial.printf("[HAL-Audio-NabuCasa] ✗ Failed to set I2S microphone pins: %d\n", err);
        i2s_driver_uninstall(I2S_NUM_0);
        return false;
    }

    // Start I2S
    i2s_zero_dma_buffer(I2S_NUM_0);
    i2s_start(I2S_NUM_0);

    micReady_ = true;
    Serial.println("[HAL-Audio-NabuCasa] ✓ Microphone initialized");
    return true;
}

bool HALAudioNabuCasa::beginSpeaker(uint32_t sampleRate) {
    Serial.printf("[HAL-Audio-NabuCasa] Initializing speaker (I2S_NUM_1, %dHz)...\n", sampleRate);

    // I2S configuration for speaker output (ESP32 is SLAVE, XMOS/AIC3204 is master)
    i2s_config_t i2s_spk_config = {
        .mode = (i2s_mode_t)(I2S_MODE_SLAVE | I2S_MODE_TX),  // SLAVE TX mode
        .sample_rate = sampleRate,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,  // 32-bit as per ESPHome config
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,  // Stereo
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 256,
        .use_apll = false,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0
    };

    // Pin configuration for speaker
    i2s_pin_config_t spk_pin_config = {
        .bck_io_num = PIN_I2S_SPK_BCLK,
        .ws_io_num = PIN_I2S_SPK_LRCLK,
        .data_out_num = PIN_I2S_SPK_DOUT,
        .data_in_num = I2S_PIN_NO_CHANGE  // No input
    };

    // Install and configure I2S driver
    esp_err_t err = i2s_driver_install(I2S_NUM_1, &i2s_spk_config, 0, NULL);
    if (err != ESP_OK) {
        Serial.printf("[HAL-Audio-NabuCasa] ✗ Failed to install I2S speaker driver: %d\n", err);
        return false;
    }

    err = i2s_set_pin(I2S_NUM_1, &spk_pin_config);
    if (err != ESP_OK) {
        Serial.printf("[HAL-Audio-NabuCasa] ✗ Failed to set I2S speaker pins: %d\n", err);
        i2s_driver_uninstall(I2S_NUM_1);
        return false;
    }

    // Start I2S
    i2s_zero_dma_buffer(I2S_NUM_1);
    i2s_start(I2S_NUM_1);

    // Initialize AIC3204 codec via I2C
    Serial.println("[HAL-Audio-NabuCasa] Initializing AIC3204 codec...");

    // Page 0: Clock and digital settings
    if (!aic3204SelectPage(0)) {
        Serial.println("[HAL-Audio-NabuCasa] ✗ Failed to select AIC3204 page 0");
        return false;
    }

    aic3204WriteReg(AIC3204_SW_RST, 0x01);       // Software reset
    delay(10);

    aic3204WriteReg(AIC3204_NDAC, 0x82);         // Power up NDAC, divider = 2
    aic3204WriteReg(AIC3204_MDAC, 0x82);         // Power up MDAC, divider = 2
    aic3204WriteReg(AIC3204_DOSR, 0x80);         // DOSR = 128
    aic3204WriteReg(AIC3204_CODEC_IF, 0x30);     // I2S mode, 32-bit, DOUT always driving
    aic3204WriteReg(AIC3204_SCLK_MFP3, 0x02);    // SCLK/MFP3 as audio data in
    aic3204WriteReg(AIC3204_AUDIO_IF_4, 0x01);   // Audio interface setting 4
    aic3204WriteReg(AIC3204_AUDIO_IF_5, 0x01);   // Audio interface setting 5
    aic3204WriteReg(AIC3204_DAC_SIG_PROC, 0x01); // DAC processing block PRB_P1

    Serial.println("[HAL-Audio-NabuCasa]   ✓ Page 0 configured (clock/digital)");

    // Page 1: Analog configuration
    if (!aic3204SelectPage(1)) {
        Serial.println("[HAL-Audio-NabuCasa] ✗ Failed to select AIC3204 page 1");
        return false;
    }

    aic3204WriteReg(AIC3204_LDO_CTRL, 0x09);     // Enable internal AVDD LDO
    aic3204WriteReg(AIC3204_PWR_CFG, 0x08);      // Disable weak AVDD
    aic3204WriteReg(AIC3204_LDO_CTRL, 0x01);     // Enable master analog power
    aic3204WriteReg(AIC3204_CM_CTRL, 0x40);      // Common mode = 0.9V
    aic3204WriteReg(AIC3204_PLAY_CFG1, 0x00);    // Playback config 1
    aic3204WriteReg(AIC3204_PLAY_CFG2, 0x00);    // PowerTune PTM_P3/P4

    Serial.println("[HAL-Audio-NabuCasa]   ✓ Page 1 configured (analog power)");

    // Reference and output configuration
    aic3204WriteReg(AIC3204_REF_STARTUP, 0x01);  // REF charging time = 40ms
    delay(50);  // Wait for reference to charge

    aic3204WriteReg(AIC3204_HP_START, 0x25);     // Headphone soft stepping

    // Route DAC to outputs
    aic3204WriteReg(AIC3204_HPL_ROUTE, 0x08);    // DAC_L routed to HPL
    aic3204WriteReg(AIC3204_HPR_ROUTE, 0x08);    // DAC_R routed to HPR
    aic3204WriteReg(AIC3204_LOL_ROUTE, 0x08);    // DAC_L routed to LOL
    aic3204WriteReg(AIC3204_LOR_ROUTE, 0x08);    // DAC_R routed to LOR

    // Set output gains - bit 7 is mute (0=unmuted), bits 6-0 are gain
    aic3204WriteReg(AIC3204_HPL_GAIN, 0x00);     // HPL: Unmute, 0dB gain
    aic3204WriteReg(AIC3204_HPR_GAIN, 0x00);     // HPR: Unmute, 0dB gain
    aic3204WriteReg(AIC3204_LOL_DRV_GAIN, 0x00); // LOL: Unmute, 0dB gain
    aic3204WriteReg(AIC3204_LOR_DRV_GAIN, 0x00); // LOR: Unmute, 0dB gain

    // Power up output drivers
    aic3204WriteReg(AIC3204_OP_PWR_CTRL, 0x3C);  // Power up HPL, HPR, LOL, LOR

    Serial.println("[HAL-Audio-NabuCasa]   ✓ Page 1 configured (outputs/routing)");

    // Wait for analog settling
    delay(2500);

    // Page 0: Power up DAC
    if (!aic3204SelectPage(0)) {
        Serial.println("[HAL-Audio-NabuCasa] ✗ Failed to select AIC3204 page 0");
        return false;
    }

    aic3204WriteReg(AIC3204_DAC_CH_SET1, 0xD4);  // Power up left and right DAC channels
    aic3204WriteReg(AIC3204_DAC_CH_SET2, 0x00);  // Unmute DAC channels (bit 3=L mute, bit 2=R mute)
    aic3204WriteReg(AIC3204_DACL_VOL_D, 0x00);   // Left DAC digital volume = 0dB
    aic3204WriteReg(AIC3204_DACR_VOL_D, 0x00);   // Right DAC digital volume = 0dB

    Serial.println("[HAL-Audio-NabuCasa] ✓ AIC3204 codec initialized");

    speakerReady_ = true;
    Serial.println("[HAL-Audio-NabuCasa] ✓ Speaker initialized");
    return true;
}

size_t HALAudioNabuCasa::readMicrophone(int16_t* buffer, size_t sampleCount) {
    // Debug: Log function entry (first 5 calls)
    static int callCount = 0;
    if (callCount++ < 5) {
        Serial.printf("[HAL-Audio-NabuCasa] readMicrophone() call #%d: micReady=%d, buffer=%p, sampleCount=%d\n",
                      callCount, micReady_, buffer, sampleCount);
    }

    if (!micReady_ || !buffer || sampleCount == 0) {
        if (callCount <= 5) {
            Serial.printf("[HAL-Audio-NabuCasa] Early return: micReady=%d, buffer=%p, sampleCount=%d\n",
                          micReady_, buffer, sampleCount);
        }
        return 0;
    }

    // Read 32-bit samples from I2S, convert to 16-bit
    // The microphone is stereo, so we need to read stereo samples
    size_t bytesToRead = sampleCount * sizeof(int32_t) * 2;  // Stereo 32-bit
    int32_t* tempBuffer = (int32_t*)malloc(bytesToRead);
    if (!tempBuffer) {
        if (callCount <= 5) {
            Serial.println("[HAL-Audio-NabuCasa] malloc failed for tempBuffer!");
        }
        return 0;
    }

    if (callCount <= 5) {
        Serial.printf("[HAL-Audio-NabuCasa] About to call i2s_read, bytesToRead=%d\n", bytesToRead);
    }

    size_t bytesRead = 0;
    // Use 100ms timeout instead of portMAX_DELAY to avoid blocking forever
    esp_err_t err = i2s_read(I2S_NUM_0, tempBuffer, bytesToRead, &bytesRead, pdMS_TO_TICKS(100));

    if (callCount <= 5) {
        Serial.printf("[HAL-Audio-NabuCasa] i2s_read returned: err=%d, bytesRead=%d\n", err, bytesRead);
    }

    // Debug: Log I2S read results
    static int readCount = 0;
    if (readCount++ < 5) {  // Only log first 5 reads
        Serial.printf("[HAL-Audio-NabuCasa] I2S read #%d: err=%d, bytesRead=%d/%d\n",
                      readCount, err, bytesRead, bytesToRead);
    }

    if (err != ESP_OK || bytesRead == 0) {
        if (readCount <= 5) {
            Serial.printf("[HAL-Audio-NabuCasa] I2S read failed or empty (err=%d)\n", err);
        }
        free(tempBuffer);
        return 0;
    }

    // Convert 32-bit stereo to 16-bit mono (average channels, downsample)
    size_t samplesRead = bytesRead / (sizeof(int32_t) * 2);

    // Debug: Check first few samples
    static bool debugPrinted = false;
    if (!debugPrinted && samplesRead > 0) {
        Serial.printf("[HAL-Audio-NabuCasa] First I2S samples (32-bit): L=%d, R=%d\n",
                      tempBuffer[0], tempBuffer[1]);
        debugPrinted = true;
    }

    for (size_t i = 0; i < samplesRead && i < sampleCount; i++) {
        // Average left and right channels, then downshift from 32-bit to 16-bit
        int64_t left = tempBuffer[i * 2];
        int64_t right = tempBuffer[i * 2 + 1];
        int64_t avg = (left + right) / 2;
        buffer[i] = (int16_t)(avg >> 16);  // Convert 32-bit to 16-bit
    }

    free(tempBuffer);
    return samplesRead;
}

size_t HALAudioNabuCasa::writeSpeaker(const int16_t* buffer, size_t sampleCount) {
    // Debug: Log function entry (first 5 calls)
    static int callCount = 0;
    if (callCount++ < 5) {
        Serial.printf("[HAL-Audio-NabuCasa] writeSpeaker() call #%d: speakerReady=%d, buffer=%p, sampleCount=%d\n",
                      callCount, speakerReady_, buffer, sampleCount);
    }

    if (!speakerReady_ || !buffer || sampleCount == 0) {
        return 0;
    }

    // Convert 16-bit mono to 32-bit stereo for output
    size_t stereoSamples = sampleCount * 2;  // Stereo
    int32_t* tempBuffer = (int32_t*)malloc(stereoSamples * sizeof(int32_t));
    if (!tempBuffer) {
        if (callCount <= 5) {
            Serial.println("[HAL-Audio-NabuCasa] malloc failed for speaker tempBuffer!");
        }
        return 0;
    }

    // Convert 16-bit mono to 32-bit stereo (duplicate mono to both channels)
    for (size_t i = 0; i < sampleCount; i++) {
        int32_t sample = ((int32_t)buffer[i]) << 16;  // Convert 16-bit to 32-bit
        tempBuffer[i * 2] = sample;      // Left channel
        tempBuffer[i * 2 + 1] = sample;  // Right channel (same as left)
    }

    if (callCount <= 5) {
        Serial.printf("[HAL-Audio-NabuCasa] About to call i2s_write, bytesToWrite=%d\n",
                      stereoSamples * sizeof(int32_t));
    }

    size_t bytesWritten = 0;
    // Use 100ms timeout instead of portMAX_DELAY to avoid blocking forever
    esp_err_t err = i2s_write(I2S_NUM_1, tempBuffer, stereoSamples * sizeof(int32_t),
                              &bytesWritten, pdMS_TO_TICKS(100));

    if (callCount <= 5) {
        Serial.printf("[HAL-Audio-NabuCasa] i2s_write returned: err=%d, bytesWritten=%d\n",
                      err, bytesWritten);
    }

    free(tempBuffer);

    if (err != ESP_OK) {
        return 0;
    }

    return bytesWritten / (sizeof(int32_t) * 2);  // Return number of mono samples written
}

void HALAudioNabuCasa::setVolume(uint8_t volume) {
    volume_ = volume;

    // NOTE: Cannot do I2C writes here because this is called from ISR context
    // (rotary encoder interrupt). Hardware volume control would need to be
    // applied from a non-ISR context using a flag/queue system.
    // For now, just track the volume in software.

    Serial.printf("[HAL-Audio-NabuCasa] Volume set to %d%%\n", volume);
}

void HALAudioNabuCasa::setMicGain(uint8_t gainStep) {
    micGain_ = gainStep;

    // TODO: Send I2C commands to AIC3204 to set ADC gain
    // For now, gain is controlled by XMOS processing
    Serial.printf("[HAL-Audio-NabuCasa] Mic gain set to %ddB\n", gainStep);
}

void HALAudioNabuCasa::mute(bool enabled) {
    muted_ = enabled;

    if (enabled) {
        digitalWrite(PIN_SPEAKER_AMP_ENABLE, LOW);
        Serial.println("[HAL-Audio-NabuCasa] 🔇 Muted");
    } else {
        digitalWrite(PIN_SPEAKER_AMP_ENABLE, HIGH);
        Serial.println("[HAL-Audio-NabuCasa] 🔊 Unmuted");
    }
}

uint32_t HALAudioNabuCasa::getSampleRate() const {
    return sampleRate_;
}

uint8_t HALAudioNabuCasa::getChannelCount() const {
    return 1;  // Return mono (we convert internally)
}

bool HALAudioNabuCasa::isMicrophoneReady() const {
    return micReady_;
}

bool HALAudioNabuCasa::isSpeakerReady() const {
    return speakerReady_;
}
