#ifndef AUDIO_BUFFER_H
#define AUDIO_BUFFER_H

#include <Arduino.h>

/**
 * AudioBuffer
 *
 * Thread-safe circular buffer for audio samples.
 * Used for buffering audio between I2S, codec, and network layers.
 *
 * Features:
 * - Lock-free single producer, single consumer
 * - Overrun/underrun detection
 * - Peak level monitoring
 */
class AudioBuffer {
public:
    /**
     * Create audio buffer
     * @param capacity Number of samples (int16_t) to store
     */
    AudioBuffer(size_t capacity);
    ~AudioBuffer();

    /**
     * Write samples to buffer
     * @param samples Pointer to sample data
     * @param count Number of samples to write
     * @return Number of samples actually written (may be less if buffer full)
     */
    size_t write(const int16_t* samples, size_t count);

    /**
     * Read samples from buffer
     * @param samples Pointer to destination buffer
     * @param count Number of samples to read
     * @return Number of samples actually read (may be less if buffer empty)
     */
    size_t read(int16_t* samples, size_t count);

    /**
     * Peek at samples without removing them
     * @param samples Pointer to destination buffer
     * @param count Number of samples to peek
     * @param offset Offset from read position
     * @return Number of samples peeked
     */
    size_t peek(int16_t* samples, size_t count, size_t offset = 0);

    /**
     * Get number of samples available to read
     */
    size_t available() const;

    /**
     * Get free space available for writing
     */
    size_t free() const;

    /**
     * Check if buffer is empty
     */
    bool isEmpty() const;

    /**
     * Check if buffer is full
     */
    bool isFull() const;

    /**
     * Clear buffer (reset read/write pointers)
     */
    void clear();

    /**
     * Get buffer capacity
     */
    size_t capacity() const;

    /**
     * Get peak level since last reset (0-32767)
     */
    int16_t getPeakLevel();

    /**
     * Reset peak level
     */
    void resetPeak();

    /**
     * Get buffer fill percentage (0-100)
     */
    uint8_t getFillPercentage() const;

    /**
     * Get statistics
     */
    uint32_t getOverruns() const { return overruns; }
    uint32_t getUnderruns() const { return underruns; }
    void resetStats();

private:
    int16_t* buffer;
    size_t bufferSize;
    volatile size_t writePos;
    volatile size_t readPos;

    int16_t peakLevel;
    uint32_t overruns;
    uint32_t underruns;

    // Helper to update peak level
    void updatePeak(const int16_t* samples, size_t count);
};

#endif // AUDIO_BUFFER_H
