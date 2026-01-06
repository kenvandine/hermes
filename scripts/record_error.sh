#!/bin/bash

# Wake word training sample recorder
# Records 20 samples with user-controlled start/stop

set -e

if [ -z "$1" ]; then
    echo "Usage: $0 <speaker_id>"
    echo "Example: $0 ken"
    exit 1
fi

SPEAKER_ID="$1"
TOTAL_SAMPLES=20
SAMPLE_RATE=16000
CHANNELS=1
FORMAT="S16_LE"

echo "Wake Word Training Sample Recorder"
echo "==================================="
echo "Speaker ID: $SPEAKER_ID"
echo "Samples to record: $TOTAL_SAMPLES"
echo ""
echo "Instructions:"
echo "  - Press 'r' to START recording"
echo "  - Press SPACE to STOP recording"
echo ""

# Function to wait for a specific key
wait_for_key() {
    local target_key="$1"
    local key
    while true; do
        read -rsn1 key
        if [ "$key" = "$target_key" ]; then
            break
        fi
    done
}

# Function to wait for space (empty string from read)
wait_for_space() {
    local key
    while true; do
        read -rsn1 key
        if [ "$key" = " " ] || [ "$key" = "" ]; then
            break
        fi
    done
}

for COUNT in $(seq 1 $TOTAL_SAMPLES); do
    FILENAME="wrong_words.${SPEAKER_ID}.${COUNT}.wav"

    echo "----------------------------------------"
    echo "Sample $COUNT of $TOTAL_SAMPLES"
    echo "Output: $FILENAME"
    echo ""
    echo "Press 'r' to start recording..."

    wait_for_key "r"

    echo "RECORDING... Press SPACE to stop"

    # Start recording in background
    arecord -f $FORMAT -r $SAMPLE_RATE -c $CHANNELS -t wav "$FILENAME" &
    RECORD_PID=$!

    # Wait for space to stop
    wait_for_space

    # Stop recording
    kill $RECORD_PID 2>/dev/null || true
    wait $RECORD_PID 2>/dev/null || true

    echo "Saved: $FILENAME"
    echo ""
done

echo "=========================================="
echo "Recording complete!"
echo "Recorded $TOTAL_SAMPLES samples for speaker: $SPEAKER_ID"
echo ""
echo "Files created:"
ls -la wrong_words.${SPEAKER_ID}.*.wav 2>/dev/null || echo "No files found"
