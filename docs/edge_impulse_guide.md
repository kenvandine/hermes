# Edge Impulse Wake Word Training Guide

This guide walks you through training a custom wake word model for the ESP32 intercom system using Edge Impulse.

## Prerequisites

- Edge Impulse account (free): https://edgeimpulse.com
- Microphone for recording samples
- ~30 minutes for training

## Step 1: Create Edge Impulse Project

1. Go to https://studio.edgeimpulse.com
2. Click "Create new project"
3. Name it: `hermes-wake-word`
4. Choose project type: "Audio"

## Step 2: Collect Training Data

You need to record samples of:
- **Your wake phrase**: "Hey Hermes" (or your custom phrase)
- **Background noise**: Ambient room noise, other voices, music
- **Similar phrases**: "Hey there", "OK Hermes", etc. (to reduce false positives)

### Recording Guidelines

**Wake Word Samples (Target: 50-100 samples)**
- Record in different voices (you, family members)
- Different volumes (normal, quiet, loud)
- Different distances from microphone (1ft, 3ft, 6ft)
- Different room environments
- Different speaking speeds

**Background Noise Samples (Target: 30-50 samples)**
- Silence in room
- TV/Radio playing
- Conversation (not saying wake word)
- Music
- Kitchen/household sounds

### Recording Methods

**Option A: Edge Impulse Data Forwarder (Recommended)**

1. Install Edge Impulse CLI:
   ```bash
   npm install -g edge-impulse-cli
   ```

2. Connect your device:
   ```bash
   edge-impulse-daemon
   ```

3. Record samples:
   - Go to "Data acquisition" in Edge Impulse Studio
   - Use "Record new data"
   - Label: `wake_word` for your phrase, `noise` for background

**Option B: Upload Pre-recorded Audio**

1. Record audio files (WAV format, 16kHz, mono)
2. Upload to Edge Impulse:
   - Data acquisition → Upload data
   - Label appropriately

**Option C: Use Your ESP32 Device**

1. Flash the data collection firmware
2. Connect to Edge Impulse
3. Record directly from your mic

## Step 3: Design Impulse

1. Go to **Impulse design** → **Create impulse**

2. Configure processing blocks:
   - **Window size**: 1000ms (1 second)
   - **Window increase**: 500ms (50% overlap)
   - **Frequency**: 16000 Hz

3. Add processing block:
   - Click "Add processing block"
   - Select **Audio (MFCC)**

4. Add learning block:
   - Click "Add learning block"
   - Select **Classification (Keras)**

5. Click **Save Impulse**

## Step 4: Configure MFCC Features

1. Go to **MFCC** tab

2. Configure parameters:
   - **Frame length**: 0.02 (20ms)
   - **Frame stride**: 0.01 (10ms)
   - **Filter number**: 32
   - **FFT length**: 512
   - **Low frequency**: 300 Hz
   - **High frequency**: 8000 Hz
   - **Noise floor**: -52 dB

3. Click **Save parameters**

4. Click **Generate features**
   - Wait for processing
   - Verify feature explorer shows good separation

## Step 5: Train Neural Network

1. Go to **NN Classifier** tab

2. Configure network:
   - **Number of training cycles**: 100
   - **Learning rate**: 0.005
   - **Validation set size**: 20%

   **Neural network architecture**:
   ```
   Input layer (auto)
   Dense layer: 20 neurons, ReLU
   Dropout: 0.25
   Dense layer: 10 neurons, ReLU
   Output layer: 2 neurons (wake_word, noise), Softmax
   ```

3. Click **Start training**

4. Review results:
   - **Target accuracy**: >95%
   - Check confusion matrix
   - Look for overfitting (training vs validation accuracy)

5. If accuracy is low:
   - Collect more diverse samples
   - Adjust network architecture
   - Increase training cycles

## Step 6: Test Model

1. Go to **Live classification** tab

2. Test with your microphone:
   - Say wake word → should classify as `wake_word`
   - Say other phrases → should classify as `noise`
   - Background sounds → should classify as `noise`

3. Note the confidence scores (0.0-1.0)
   - Adjust detection threshold based on results
   - Default: 0.8 (80% confidence)

## Step 7: Optimize for ESP32

1. Go to **EON Tuner** (optional but recommended)

2. Configure target:
   - **Target device**: ESP32
   - **Max RAM**: 200KB
   - **Max ROM**: 500KB
   - **Target latency**: 200ms

3. Click **Start tuning**
   - EON will optimize network for ESP32
   - May reduce accuracy slightly for better performance

## Step 8: Export for Arduino

1. Go to **Deployment** tab

2. Select **Arduino library**

3. Configure:
   - **Optimization**: Enable EON Compiler (INT8 quantization)
   - This reduces size and improves performance

4. Click **Build**

5. Download the ZIP file

## Step 9: Integrate with Project

1. Extract the downloaded ZIP file

2. Copy the library to your project:
   ```bash
   cd hermes
   unzip ~/Downloads/ei-hermes-wake-word-arduino-*.zip -d lib/
   ```

3. Update `src/audio/wake_word.cpp`:

   Uncomment and update the include:
   ```cpp
   #include <ei-hermes-wake-word_inferencing.h>
   ```

4. Update `include/config.h`:
   ```cpp
   #define WAKE_WORD_ENABLED       true
   ```

5. Rebuild:
   ```bash
   pio run
   ```

## Step 10: Tune Detection Parameters

After deployment, you may need to adjust:

### In `include/config.h`:

```cpp
// Wake word detection threshold (0.0 - 1.0)
// Higher = fewer false positives, may miss some detections
// Lower = more detections, may have false positives
#define WAKE_WORD_THRESHOLD     0.8

// Debounce time (milliseconds)
// Prevents multiple triggers in quick succession
#define WAKE_WORD_DEBOUNCE_MS   1000

// Inference interval (milliseconds)
// How often to run wake word detection
#define INFERENCE_INTERVAL_MS   100
```

### Testing and Tuning:

1. Monitor serial output for confidence scores
2. Test in real-world conditions (TV on, conversations, etc.)
3. Adjust threshold based on results:
   - Too many false positives? Increase threshold (0.85-0.90)
   - Missing real wake words? Decrease threshold (0.70-0.75)

## Troubleshooting

### Low Accuracy

- **Collect more diverse training data**
  - Different speakers, environments, volumes

- **Increase network complexity**
  - Add more neurons to dense layers
  - Add more layers

- **Adjust MFCC parameters**
  - Try different frame lengths
  - Experiment with filter numbers

### False Positives

- **Collect more negative samples**
  - Similar-sounding phrases
  - Common household sounds

- **Increase detection threshold**
  - Try 0.85 or 0.90

- **Add more classes**
  - Create separate classes for common false triggers

### High Memory Usage

- **Reduce network size**
  - Fewer neurons per layer
  - Remove layers

- **Enable quantization**
  - Use INT8 optimization in deployment

- **Reduce window size**
  - Try 500ms or 750ms windows

## Advanced: Multi-Class Model

For Phase 6 (voice commands), you can extend this model:

**Classes**:
- `wake_word`: "Hey Hermes"
- `drop_in`: "Drop in on [room]"
- `hang_up`: "Hang up"
- `cancel`: "Cancel"
- `noise`: Background/other

This allows both wake word detection AND command recognition in one model.

## Performance Expectations

**ESP32-S3 @ 240MHz**:
- Inference time: 50-150ms
- RAM usage: 50-150KB
- CPU usage: ~10-15% continuous

**Accuracy targets**:
- Wake word detection: >95%
- False positive rate: <1 per hour
- Detection latency: <300ms

## Resources

- [Edge Impulse Documentation](https://docs.edgeimpulse.com/)
- [ESP32 + Edge Impulse Guide](https://docs.edgeimpulse.com/docs/development-platforms/officially-supported-mcu-targets/espressif-esp32)
- [Audio Classification Tutorial](https://docs.edgeimpulse.com/docs/tutorials/audio-classification)
- [Keyword Spotting](https://docs.edgeimpulse.com/docs/tutorials/respond-to-your-voice)

## Next Steps

After wake word is working:
- **Phase 6**: Add voice command recognition
- **Phase 7**: Integrate with touch screen UI
- **Phase 8**: Add more advanced commands and AI integration
