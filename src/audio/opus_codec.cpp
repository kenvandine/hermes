#include "opus_codec.h"
#include "config.h"

OpusCodec::OpusCodec()
    : encoder(nullptr),
      decoder(nullptr),
      encoderSampleRate(0),
      decoderSampleRate(0),
      encoderChannels(0),
      decoderChannels(0),
      encoderReady(false),
      decoderReady(false) {
}

OpusCodec::~OpusCodec() {
    if (encoder) {
        opus_encoder_destroy(encoder);
        encoder = nullptr;
    }

    if (decoder) {
        opus_decoder_destroy(decoder);
        decoder = nullptr;
    }
}

bool OpusCodec::beginEncoder(uint32_t sampleRate, int channels, int bitrate, int complexity) {
    Serial.printf("[OpusCodec] Initializing encoder: %d Hz, %d ch, %d bps, complexity %d\n",
                  sampleRate, channels, bitrate, complexity);

    int error = 0;
    encoder = opus_encoder_create(sampleRate, channels, OPUS_APPLICATION_VOIP, &error);

    if (error != OPUS_OK || !encoder) {
        lastError = "Failed to create encoder: " + String(opus_strerror(error));
        Serial.printf("[OpusCodec] ERROR: %s\n", lastError.c_str());
        return false;
    }

    // Set bitrate
    error = opus_encoder_ctl(encoder, OPUS_SET_BITRATE(bitrate));
    if (error != OPUS_OK) {
        lastError = "Failed to set bitrate: " + String(opus_strerror(error));
        Serial.printf("[OpusCodec] WARNING: %s\n", lastError.c_str());
    }

    // Set complexity
    error = opus_encoder_ctl(encoder, OPUS_SET_COMPLEXITY(complexity));
    if (error != OPUS_OK) {
        lastError = "Failed to set complexity: " + String(opus_strerror(error));
        Serial.printf("[OpusCodec] WARNING: %s\n", lastError.c_str());
    }

    // Enable FEC (Forward Error Correction) for packet loss resilience
    error = opus_encoder_ctl(encoder, OPUS_SET_INBAND_FEC(1));
    if (error != OPUS_OK) {
        Serial.printf("[OpusCodec] WARNING: Failed to enable FEC: %s\n", opus_strerror(error));
    }

    // Set expected packet loss percentage (helps encoder optimize)
    error = opus_encoder_ctl(encoder, OPUS_SET_PACKET_LOSS_PERC(5));  // Expect 5% loss
    if (error != OPUS_OK) {
        Serial.printf("[OpusCodec] WARNING: Failed to set packet loss: %s\n", opus_strerror(error));
    }

    encoderSampleRate = sampleRate;
    encoderChannels = channels;
    encoderReady = true;

    Serial.println("[OpusCodec] Encoder initialized successfully");
    return true;
}

bool OpusCodec::beginDecoder(uint32_t sampleRate, int channels) {
    Serial.printf("[OpusCodec] Initializing decoder: %d Hz, %d ch\n", sampleRate, channels);

    int error = 0;
    decoder = opus_decoder_create(sampleRate, channels, &error);

    if (error != OPUS_OK || !decoder) {
        lastError = "Failed to create decoder: " + String(opus_strerror(error));
        Serial.printf("[OpusCodec] ERROR: %s\n", lastError.c_str());
        return false;
    }

    decoderSampleRate = sampleRate;
    decoderChannels = channels;
    decoderReady = true;

    Serial.println("[OpusCodec] Decoder initialized successfully");
    return true;
}

int OpusCodec::encode(const int16_t* pcm, int frameSize, uint8_t* output, int maxOutputBytes) {
    if (!encoderReady || !encoder) {
        lastError = "Encoder not initialized";
        return -1;
    }

    if (!pcm || !output) {
        lastError = "Null pointer passed to encode";
        return -1;
    }

    int encoded = opus_encode(encoder, pcm, frameSize, output, maxOutputBytes);

    if (encoded < 0) {
        lastError = "Encode error: " + String(opus_strerror(encoded));
        Serial.printf("[OpusCodec] ERROR: %s\n", lastError.c_str());
        return encoded;
    }

    return encoded;
}

int OpusCodec::decode(const uint8_t* input, int inputBytes, int16_t* pcm, int frameSize, bool decodeFec) {
    if (!decoderReady || !decoder) {
        lastError = "Decoder not initialized";
        return -1;
    }

    if (!pcm) {
        lastError = "Null output pointer passed to decode";
        return -1;
    }

    int decoded;

    if (decodeFec && inputBytes == 0) {
        // Decode using FEC (packet lost, use FEC from previous packet)
        decoded = opus_decode(decoder, NULL, 0, pcm, frameSize, 1);
    } else {
        // Normal decode
        decoded = opus_decode(decoder, input, inputBytes, pcm, frameSize, 0);
    }

    if (decoded < 0) {
        lastError = "Decode error: " + String(opus_strerror(decoded));
        Serial.printf("[OpusCodec] ERROR: %s\n", lastError.c_str());
        return decoded;
    }

    return decoded;
}

uint32_t OpusCodec::getEncoderSampleRate() const {
    return encoderSampleRate;
}

uint32_t OpusCodec::getDecoderSampleRate() const {
    return decoderSampleRate;
}

int OpusCodec::getFrameSize(float durationMs) const {
    if (encoderSampleRate == 0) {
        return 0;
    }

    return (int)(encoderSampleRate * durationMs / 1000.0f);
}

bool OpusCodec::isEncoderReady() const {
    return encoderReady;
}

bool OpusCodec::isDecoderReady() const {
    return decoderReady;
}

bool OpusCodec::setBitrate(int bitrate) {
    if (!encoderReady || !encoder) {
        return false;
    }

    int error = opus_encoder_ctl(encoder, OPUS_SET_BITRATE(bitrate));
    if (error != OPUS_OK) {
        lastError = "Failed to set bitrate: " + String(opus_strerror(error));
        return false;
    }

    Serial.printf("[OpusCodec] Bitrate set to %d bps\n", bitrate);
    return true;
}

bool OpusCodec::setComplexity(int complexity) {
    if (!encoderReady || !encoder) {
        return false;
    }

    int error = opus_encoder_ctl(encoder, OPUS_SET_COMPLEXITY(complexity));
    if (error != OPUS_OK) {
        lastError = "Failed to set complexity: " + String(opus_strerror(error));
        return false;
    }

    Serial.printf("[OpusCodec] Complexity set to %d\n", complexity);
    return true;
}

String OpusCodec::getLastError() const {
    return lastError;
}
