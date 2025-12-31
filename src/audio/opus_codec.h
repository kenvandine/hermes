#ifndef OPUS_CODEC_H
#define OPUS_CODEC_H

#include <Arduino.h>
#include <opus.h>
#include "config.h"

/**
 * OpusCodec
 *
 * Wrapper for Opus encoder and decoder.
 * Opus is a high-quality, low-latency audio codec ideal for VoIP.
 *
 * Features:
 * - Configurable bitrate and complexity
 * - Frame-based encoding/decoding
 * - Optimized for voice (OPUS_APPLICATION_VOIP)
 */
class OpusCodec {
public:
    OpusCodec();
    ~OpusCodec();

    /**
     * Initialize encoder
     * @param sampleRate Sample rate in Hz (8000, 12000, 16000, 24000, 48000)
     * @param channels Number of channels (1 = mono, 2 = stereo)
     * @param bitrate Target bitrate in bps (e.g., 24000)
     * @param complexity Computational complexity (0-10, higher = better quality)
     * @return true if successful
     */
    bool beginEncoder(uint32_t sampleRate = 16000,
                      int channels = 1,
                      int bitrate = OPUS_BITRATE,
                      int complexity = OPUS_COMPLEXITY);

    /**
     * Initialize decoder
     * @param sampleRate Sample rate in Hz (must match encoder)
     * @param channels Number of channels (must match encoder)
     * @return true if successful
     */
    bool beginDecoder(uint32_t sampleRate = 16000, int channels = 1);

    /**
     * Encode audio samples to Opus packet
     * @param pcm Input PCM samples (int16_t)
     * @param frameSize Number of samples per channel (must be 2.5, 5, 10, 20, 40, 60 ms worth)
     * @param output Output buffer for encoded packet
     * @param maxOutputBytes Maximum size of output buffer
     * @return Number of bytes in encoded packet, or negative on error
     */
    int encode(const int16_t* pcm, int frameSize, uint8_t* output, int maxOutputBytes);

    /**
     * Decode Opus packet to audio samples
     * @param input Input encoded packet
     * @param inputBytes Size of input packet
     * @param pcm Output PCM samples (int16_t)
     * @param frameSize Maximum number of samples per channel to decode
     * @param decodeFec Use FEC (Forward Error Correction) if available
     * @return Number of samples decoded per channel, or negative on error
     */
    int decode(const uint8_t* input, int inputBytes, int16_t* pcm, int frameSize, bool decodeFec = false);

    /**
     * Get encoder sample rate
     */
    uint32_t getEncoderSampleRate() const;

    /**
     * Get decoder sample rate
     */
    uint32_t getDecoderSampleRate() const;

    /**
     * Get frame size in samples for a given duration
     * @param durationMs Frame duration in milliseconds (2.5, 5, 10, 20, 40, 60)
     * @return Number of samples
     */
    int getFrameSize(float durationMs) const;

    /**
     * Check if encoder is ready
     */
    bool isEncoderReady() const;

    /**
     * Check if decoder is ready
     */
    bool isDecoderReady() const;

    /**
     * Set encoder bitrate
     */
    bool setBitrate(int bitrate);

    /**
     * Set encoder complexity
     */
    bool setComplexity(int complexity);

    /**
     * Get last error message
     */
    String getLastError() const;

private:
    OpusEncoder* encoder;
    OpusDecoder* decoder;

    uint32_t encoderSampleRate;
    uint32_t decoderSampleRate;
    int encoderChannels;
    int decoderChannels;

    bool encoderReady;
    bool decoderReady;

    String lastError;
};

#endif // OPUS_CODEC_H
