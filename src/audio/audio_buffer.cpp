#include "audio_buffer.h"
#include <cstring>
#include <cstdlib>

AudioBuffer::AudioBuffer(size_t capacity)
    : bufferSize(capacity),
      writePos(0),
      readPos(0),
      peakLevel(0),
      overruns(0),
      underruns(0) {

    // Allocate buffer (use PSRAM if available for large buffers)
    if (capacity > 8192 && psramFound()) {
        buffer = (int16_t*)ps_malloc(capacity * sizeof(int16_t));
        if (buffer) {
            Serial.printf("[AudioBuffer] Allocated %d samples in PSRAM\n", capacity);
        }
    } else {
        buffer = (int16_t*)malloc(capacity * sizeof(int16_t));
        if (buffer) {
            Serial.printf("[AudioBuffer] Allocated %d samples in heap\n", capacity);
        }
    }

    if (!buffer) {
        Serial.println("[AudioBuffer] ERROR: Failed to allocate buffer!");
        bufferSize = 0;
        return;
    }

    // Zero out buffer
    memset(buffer, 0, bufferSize * sizeof(int16_t));
}

AudioBuffer::~AudioBuffer() {
    if (buffer) {
        free(buffer);
        buffer = nullptr;
    }
}

size_t AudioBuffer::write(const int16_t* samples, size_t count) {
    if (!buffer || count == 0) {
        return 0;
    }

    size_t available = free();
    size_t toWrite = (count <= available) ? count : available;

    if (toWrite < count) {
        overruns++;
    }

    // Write samples
    for (size_t i = 0; i < toWrite; i++) {
        buffer[writePos] = samples[i];
        writePos = (writePos + 1) % bufferSize;
    }

    // Update peak level
    updatePeak(samples, toWrite);

    return toWrite;
}

size_t AudioBuffer::read(int16_t* samples, size_t count) {
    if (!buffer || count == 0) {
        return 0;
    }

    size_t availableCount = available();
    size_t toRead = (count <= availableCount) ? count : availableCount;

    if (toRead < count) {
        underruns++;
    }

    // Read samples
    for (size_t i = 0; i < toRead; i++) {
        samples[i] = buffer[readPos];
        readPos = (readPos + 1) % bufferSize;
    }

    // Fill remaining with silence if we didn't have enough samples
    if (toRead < count) {
        memset(samples + toRead, 0, (count - toRead) * sizeof(int16_t));
    }

    return toRead;
}

size_t AudioBuffer::peek(int16_t* samples, size_t count, size_t offset) {
    if (!buffer || count == 0) {
        return 0;
    }

    size_t availableCount = available();

    if (offset >= availableCount) {
        return 0;  // Offset beyond available data
    }

    size_t maxRead = availableCount - offset;
    size_t toRead = (count <= maxRead) ? count : maxRead;

    // Peek samples without advancing read position
    size_t peekPos = (readPos + offset) % bufferSize;
    for (size_t i = 0; i < toRead; i++) {
        samples[i] = buffer[peekPos];
        peekPos = (peekPos + 1) % bufferSize;
    }

    return toRead;
}

size_t AudioBuffer::available() const {
    if (writePos >= readPos) {
        return writePos - readPos;
    } else {
        return bufferSize - readPos + writePos;
    }
}

size_t AudioBuffer::free() const {
    // Keep one sample free to distinguish full from empty
    return bufferSize - available() - 1;
}

bool AudioBuffer::isEmpty() const {
    return writePos == readPos;
}

bool AudioBuffer::isFull() const {
    return free() == 0;
}

void AudioBuffer::clear() {
    readPos = 0;
    writePos = 0;
}

size_t AudioBuffer::capacity() const {
    return bufferSize;
}

int16_t AudioBuffer::getPeakLevel() {
    return peakLevel;
}

void AudioBuffer::resetPeak() {
    peakLevel = 0;
}

uint8_t AudioBuffer::getFillPercentage() const {
    if (bufferSize == 0) return 0;
    return (available() * 100) / bufferSize;
}

void AudioBuffer::resetStats() {
    overruns = 0;
    underruns = 0;
}

void AudioBuffer::updatePeak(const int16_t* samples, size_t count) {
    for (size_t i = 0; i < count; i++) {
        int16_t level = abs(samples[i]);
        if (level > peakLevel) {
            peakLevel = level;
        }
    }
}
