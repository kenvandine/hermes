# Edge Impulse Voice Command Training Guide

This guide walks you through training a keyword spotting model for voice commands on the ESP32 intercom system.

## Overview

Unlike wake word detection (Phase 5), which detects a single phrase, this model recognizes multiple keywords:
- **Command keywords**: "drop in", "call", "hang up", "cancel"
- **Room names**: Your actual room names (e.g., "kitchen", "living room", "bedroom")

The model uses multi-class keyword spotting to detect any of these keywords in continuous speech.

## Prerequisites

- Completed Phase 5 wake word training (recommended but not required)
- Edge Impulse account: https://edgeimpulse.com
- Microphone for recording samples
- ~1-2 hours for training (more classes = more time)
- List of room names in your home

## Step 1: Plan Your Classes

Before starting, decide which classes you need:

### Required Command Classes:
1. `drop_in` - Primary command to initiate calls
2. `call` - Alternative command to initiate calls
3. `hang_up` - End active call
4. `cancel` - Cancel current operation

### Room Name Classes:
Create one class per room in your home. Examples:
- `kitchen`
- `living_room` (or `living` if you'll say "living" not "living room")
- `bedroom`
- `office`
- `garage`
- `nursery`

### Special Class:
- `noise` - Background audio, silence, other speech

**Total Classes**: Usually 5-12 classes depending on number of rooms

**Example Setup** (3 rooms):
1. drop_in
2. call
3. hang_up
4. cancel
5. kitchen
6. living_room
7. bedroom
8. noise

## Step 2: Create Edge Impulse Project

1. Go to https://studio.edgeimpulse.com
2. Click "Create new project"
3. Name it: `hermes-commands`
4. Choose project type: "Audio"

## Step 3: Collect Training Data

You need balanced data across all classes:

### Command Keywords (Target: 40-60 samples per command)
- Record each command word in different contexts:
  - "Drop in on kitchen"
  - "Drop in living room"
  - Just "drop in" (partial command)
- Vary: volume, speed, emphasis
- Multiple speakers if possible

### Room Names (Target: 40-60 samples per room)
- Record each room name:
  - As part of command: "Call kitchen"
  - Standalone: "kitchen"
  - With filler: "uhh kitchen"
- Vary: pronunciation, volume, accent

### Noise Class (Target: 50-70 samples)
- Silence in room
- TV/Radio playing
- Music
- Conversation (NOT saying keywords)
- Household sounds
- Similar-sounding words (e.g., "all in" not "call", "kids" not "kitchen")

### Recording Guidelines

**Important**: Record in 1-second clips for each keyword/phrase

**Good Recording Practices**:
- Speak naturally, not robot-like
- Include accidental mispronunciations
- Vary distance from mic (1ft, 3ft, 6ft)
- Different background noise levels
- Different times of day (ambient noise varies)

**Bad Practices**:
- Repeating the same recording
- Only your voice (get family members!)
- Perfect studio conditions (won't match real use)

### Recording Methods

**Option A: Edge Impulse Data Forwarder (Recommended)**

```bash
npm install -g edge-impulse-cli
edge-impulse-daemon
```

Then use "Record new data" in Edge Impulse Studio.

**Option B: Use Your ESP32 Device**

Flash data collection firmware, connect to Edge Impulse, record directly from your hardware.

**Option C: Batch Upload**

Record WAV files (16kHz, mono) and upload via Data acquisition → Upload data.

## Step 4: Label Your Data

Critical step! Each sample must be labeled correctly:

**For command keywords**:
- Clip contains "drop in on kitchen" → Label: `drop_in`
- Clip contains "call the bedroom" → Label: `call`
- Clip contains "hang up" → Label: `hang_up`

**For room names**:
- Clip contains "kitchen" → Label: `kitchen`
- Clip contains "living room" → Label: `living_room`

**For noise**:
- Any audio without keywords → Label: `noise`

**Data Split**:
- 80% Training set
- 20% Test set
- Use "Auto-balance" to split evenly across classes

## Step 5: Design Impulse

1. Go to **Impulse design** → **Create impulse**

2. Configure processing:
   - **Window size**: 1000ms (1 second)
   - **Window increase**: 500ms (50% overlap)
   - **Frequency**: 16000 Hz

3. Add processing block:
   - Select **Audio (MFCC)**

4. Add learning block:
   - Select **Classification (Keras)**

5. Click **Save Impulse**

## Step 6: Configure MFCC Features

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
   - Verify feature explorer shows good separation between classes
   - If classes overlap heavily, collect more diverse data

## Step 7: Train Neural Network

1. Go to **NN Classifier** tab

2. Configure network:
   - **Number of training cycles**: 100-150
   - **Learning rate**: 0.005
   - **Validation set size**: 20%

   **Neural network architecture**:
   ```
   Input layer (auto from MFCC)
   Dense layer: 32 neurons, ReLU
   Dropout: 0.25
   Dense layer: 16 neurons, ReLU
   Dropout: 0.25
   Output layer: N neurons (# of classes), Softmax
   ```

3. Click **Start training**

4. Review results:
   - **Target accuracy**: >90% (harder than binary wake word)
   - Check confusion matrix:
     - Low confusion between room names is critical
     - Low false positives on noise class
   - Look for overfitting (training vs validation gap)

5. If accuracy is low:
   - Collect more data for confused classes
   - Increase network size (more neurons)
   - Train for more cycles
   - Check for labeling errors

## Step 8: Test Model

1. Go to **Live classification** tab

2. Test with your microphone:
   - Say "drop in on kitchen" → should detect `drop_in` and `kitchen`
   - Say "call living room" → should detect `call` and `living_room`
   - Say "hang up" → should detect `hang_up`
   - Say random words → should classify as `noise`
   - Silence → should classify as `noise`

3. Note the confidence scores:
   - Adjust threshold in config.h based on real-world testing
   - Default: 0.8 (80% confidence)
   - Lower if missing real commands
   - Higher if too many false positives

## Step 9: Optimize for ESP32

1. Go to **EON Tuner** (optional but recommended)

2. Configure target:
   - **Target device**: ESP32
   - **Max RAM**: 200KB
   - **Max ROM**: 500KB
   - **Target latency**: 200ms

3. Click **Start tuning**
   - EON will optimize network architecture
   - May slightly reduce accuracy for better performance

## Step 10: Export for Arduino

1. Go to **Deployment** tab

2. Select **Arduino library**

3. Configure:
   - **Optimization**: Enable EON Compiler (INT8 quantization)
   - This reduces model size significantly

4. Click **Build**

5. Download the ZIP file

## Step 11: Integrate with Project

1. Extract the downloaded ZIP file

2. Copy the library to your project:
   ```bash
   cd hermes
   unzip ~/Downloads/ei-hermes-commands-arduino-*.zip -d lib/
   ```

3. Update `src/commands/speech_recognizer.cpp`:

   Uncomment and update the include:
   ```cpp
   #include <ei-hermes-commands_inferencing.h>
   ```

4. Update `include/config.h`:
   ```cpp
   #define VOICE_COMMANDS_ENABLED  true
   ```

5. Rebuild:
   ```bash
   pio run
   ```

## Step 12: Runtime Configuration

After deployment, configure in your code:

### In `include/config.h`:

```cpp
// Voice command recognition threshold (0.0 - 1.0)
// Higher = fewer false positives, may miss some commands
// Lower = more detections, may have false positives
#define COMMAND_THRESHOLD       0.8

// Command listening timeout (milliseconds)
// How long to wait for command after wake word
#define COMMAND_TIMEOUT_MS      5000
```

### Register Room Names in `main.cpp`:

The system automatically loads room names from discovered devices, but you can also manually add them:

```cpp
// In setupVoiceCommands()
if (commandProcessor) {
    commandProcessor->addRoom("kitchen");
    commandProcessor->addRoom("living_room");
    commandProcessor->addRoom("bedroom");
}
```

## Step 13: Testing & Tuning

### Test Flow:
1. Say "Hey Hermes" (wake word)
2. Device enters LISTENING state
3. Say "Drop in on kitchen"
4. Device should detect `drop_in` and `kitchen` keywords
5. CommandProcessor builds CALL command with targetRoom="kitchen"
6. Call initiated to kitchen

### Monitor Serial Output:
```
[WakeWord] *** WAKE WORD DETECTED *** (confidence: 0.92)
[STATE] State changed: IDLE -> LISTENING
[SpeechRecognizer] *** KEYWORD DETECTED: drop_in (0.87) ***
[SpeechRecognizer] *** KEYWORD DETECTED: kitchen (0.91) ***
[CommandProcessor] Complete command: DROP_IN: kitchen
[CallManager] Initiating call to: kitchen
```

### Common Issues:

**Missing Keywords:**
- Threshold too high → Lower COMMAND_THRESHOLD to 0.70-0.75
- Insufficient training data → Add more samples for that keyword
- Background noise too loud → Retrain with more noise examples

**False Positives:**
- Threshold too low → Increase COMMAND_THRESHOLD to 0.85-0.90
- Noise class insufficient → Add more noise samples, especially similar-sounding words
- Room names too similar → Use more distinct names or add more training data

**Confusing Room Names:**
- "Living room" detected as "bedroom" → Add more distinct training examples
- Check confusion matrix in Edge Impulse
- Consider renaming rooms to more phonetically distinct names

**Timeout Issues:**
- User not speaking fast enough → Increase COMMAND_TIMEOUT_MS to 7000-10000
- User speaking too slowly → Train with slower speech samples

## Advanced: Multi-Word Room Names

If your room names are multi-word (e.g., "living room"):

**Option 1: Train as single keyword**
- Label: `living_room`
- User says: "Drop in living room"
- Model detects: `drop_in`, `living_room`

**Option 2: Train individual words**
- Labels: `living`, `room`
- CommandProcessor needs update to combine words
- More complex but handles variations better

**Recommendation**: Use Option 1 for simplicity. Train with both "living room" and "living" as `living_room` class.

## Advanced: Adding New Rooms

When you add a new device to a different room:

1. **Retrain model** with new room name class:
   - Record 40-60 samples of new room name
   - Add to existing project
   - Retrain (incremental training)
   - Re-deploy to all devices

2. **Update all devices** with new model

**OR**

3. **Use phonetic matching** (Phase 8 enhancement):
   - Integration with speech-to-text API
   - Match any room name without retraining
   - Higher latency, requires internet

## Performance Expectations

**ESP32-S3 @ 240MHz**:
- Inference time: 100-200ms
- RAM usage: 80-200KB
- CPU usage: ~15-20% continuous (when listening)

**Accuracy targets**:
- Per-keyword accuracy: >90%
- Overall command completion: >85%
- False positive rate: <1 per minute (when listening)
- Detection latency: <400ms from speech end

## Troubleshooting

### Model Won't Build
- Too many classes → Reduce neurons per layer
- Model too large → Enable INT8 quantization
- Out of memory → Use EON Tuner to optimize

### Poor Accuracy on Device
- Works in Edge Impulse Live Classification but not on device:
  - Check microphone gain settings
  - Verify sample rate matches (16kHz)
  - Check for clipping in audio input
  - Test loopback mode to verify audio pipeline

### High Latency
- Inference >300ms:
  - Reduce network complexity
  - Use EON Compiler optimization
  - Check CPU isn't busy with other tasks

## Next Steps

After voice commands are working:
- **Phase 7**: Add touch screen UI for visual feedback
- **Phase 8**: Enhance with:
  - Multi-party conference calls
  - AI assistant integration (Ollama)
  - Cloud speech-to-text for flexible commands

## Resources

- [Edge Impulse Documentation](https://docs.edgeimpulse.com/)
- [Keyword Spotting Tutorial](https://docs.edgeimpulse.com/docs/tutorials/respond-to-your-voice)
- [Multi-Class Audio Classification](https://docs.edgeimpulse.com/docs/tutorials/audio-classification)
- [ESP32 Optimization Guide](https://docs.edgeimpulse.com/docs/edge-ai-hardware/cpu/espressif-esp32)
