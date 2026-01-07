#!/usr/bin/env python3
"""
Capture training samples from ESP32 for Edge Impulse wake word training.

This script records audio directly from the ESP32's microphone, ensuring
the training data matches the device's actual audio characteristics.

Usage:
    python capture_training_samples.py --type wake_word --count 20
    python capture_training_samples.py --type noise --count 15
"""
import serial
import time
import sys
import os
import argparse
import subprocess
from pathlib import Path

SERIAL_PORT = "/dev/ttyACM0"
BAUD_RATE = 115200
OUTPUT_DIR = "training_samples"


def capture_sample(
    ser: serial.Serial, sample_type: str, index: int, output_dir: Path
) -> bool:
    """Capture a single audio sample from ESP32."""

    # Clear any pending data
    ser.reset_input_buffer()

    # Send record command
    ser.write(b"r")
    ser.flush()

    # Wait for "SPEAK NOW" signal and show it to user
    speak_now_seen = False
    timeout = time.time() + 5
    while time.time() < timeout:
        if ser.in_waiting:
            line = ser.readline().decode("utf-8", errors="ignore").strip()
            if "SPEAK NOW" in line:
                print(f"  >>> SPEAK NOW! <<<")
                speak_now_seen = True
            elif "Recording complete" in line:
                print(f"  Recording complete.")
                break
            elif line and "[CMD]" in line:
                pass  # Skip other CMD messages
        else:
            time.sleep(0.01)

    if not speak_now_seen:
        print("  WARNING: Did not see SPEAK NOW signal")

    # Wait a bit more for save to complete
    time.sleep(0.5)

    # Read any remaining status messages
    while ser.in_waiting:
        line = ser.readline().decode("utf-8", errors="ignore").strip()
        if line:
            print(f"  {line}")

    # Send download command
    ser.write(b"d")
    ser.flush()
    time.sleep(0.3)

    # Collect WAV data
    all_data = b""
    start_found = False
    end_found = False
    start_time = time.time()
    max_wait = 10  # seconds

    while not end_found and (time.time() - start_time) < max_wait:
        if ser.in_waiting:
            chunk = ser.read(ser.in_waiting)
            all_data += chunk

            if b"--- BEGIN WAV FILE ---" in all_data:
                start_found = True
            if b"--- END WAV FILE ---" in all_data:
                end_found = True
                break
        else:
            time.sleep(0.01)

    if not start_found or not end_found:
        print(f"  ERROR: Failed to capture (start={start_found}, end={end_found})")
        return False

    # Extract WAV data
    start_marker = b"--- BEGIN WAV FILE ---"
    end_marker = b"--- END WAV FILE ---"

    start_idx = all_data.find(start_marker) + len(start_marker)
    end_idx = all_data.find(end_marker)

    # Skip newlines after start marker
    while start_idx < len(all_data) and all_data[start_idx : start_idx + 1] in (
        b"\n",
        b"\r",
    ):
        start_idx += 1

    wav_data = all_data[start_idx:end_idx]

    # Validate WAV header
    if not wav_data.startswith(b"RIFF"):
        print(f"  ERROR: Invalid WAV data (doesn't start with RIFF)")
        return False

    # Save to file
    filename = f"{sample_type}.{index:03d}.wav"
    filepath = output_dir / filename

    with open(filepath, "wb") as f:
        f.write(wav_data)

    print(f"  Saved: {filename} ({len(wav_data)} bytes)")

    # Play back the recording
    print(f"  Playing back...")
    try:
        subprocess.run(["aplay", "-q", str(filepath)], check=True)
    except subprocess.CalledProcessError:
        print(f"  WARNING: Playback failed")
    except FileNotFoundError:
        print(f"  WARNING: aplay not found, skipping playback")

    # Ask user to accept or re-record
    while True:
        response = input("  Press R to re-record, or ENTER to keep: ").strip().lower()
        if response == "r":
            # Delete the file and signal re-record
            filepath.unlink()
            print(f"  Deleted {filename}, re-recording...")
            return None  # Signal to re-record
        elif response == "":
            return True  # Accept the sample
        else:
            print("  Invalid input. Press R or ENTER.")


def main():
    parser = argparse.ArgumentParser(
        description="Capture training samples from ESP32 for Edge Impulse"
    )
    parser.add_argument(
        "--type",
        "-t",
        choices=["wake_word", "hermes", "noise"],
        default="wake_word",
        help="Sample type (wake_word/hermes or noise)",
    )
    parser.add_argument(
        "--count", "-c", type=int, default=10, help="Number of samples to capture"
    )
    parser.add_argument(
        "--port",
        "-p",
        default=SERIAL_PORT,
        help=f"Serial port (default: {SERIAL_PORT})",
    )
    parser.add_argument(
        "--output",
        "-o",
        default=OUTPUT_DIR,
        help=f"Output directory (default: {OUTPUT_DIR})",
    )
    parser.add_argument(
        "--start-index",
        "-s",
        type=int,
        default=1,
        help="Starting index for filenames (default: 1)",
    )

    args = parser.parse_args()

    # Normalize sample type
    sample_type = "hermes" if args.type in ["wake_word", "hermes"] else "noise"

    # Create output directory
    output_dir = Path(args.output)
    output_dir.mkdir(exist_ok=True)

    print(f"=" * 60)
    print(f"Edge Impulse Training Sample Capture")
    print(f"=" * 60)
    print(f"Sample type: {sample_type}")
    print(f"Count: {args.count}")
    print(f"Output: {output_dir.absolute()}")
    print(f"Port: {args.port}")
    print()

    # Instructions based on sample type
    if sample_type == "hermes":
        print("Instructions:")
        print("  - Say 'Hermes' clearly when prompted")
        print("  - Vary your volume, speed, and distance from mic")
        print("  - Get different people to record if possible")
    else:
        print("Instructions:")
        print("  - Make various background noises (TV, music, talking)")
        print("  - Include silence, household sounds, etc.")
        print("  - Say words that are NOT the wake word")
    print()

    try:
        print(f"Opening {args.port}...")
        ser = serial.Serial(args.port, BAUD_RATE, timeout=1)
        time.sleep(2)  # Wait for ESP32 to settle

        # Clear any startup messages
        while ser.in_waiting:
            ser.readline()

        print("Connected! Starting capture...\n")

        successful = 0
        for i in range(args.count):
            index = args.start_index + i

            # Loop until sample is accepted (not re-recorded)
            while True:
                print(f"[{i+1}/{args.count}] Recording sample {index}...")

                if sample_type == "hermes":
                    input("  Press ENTER, then say 'Hermes' clearly...")
                else:
                    input("  Press ENTER to capture noise/background...")

                result = capture_sample(ser, sample_type, index, output_dir)

                if result is True:
                    successful += 1
                    break  # Sample accepted, move to next
                elif result is None:
                    continue  # Re-record same sample
                else:
                    # Capture failed, try again
                    print("  Retrying...")
                    continue

            # Small delay between captures
            time.sleep(0.3)

        ser.close()

        print()
        print(f"=" * 60)
        print(f"Capture complete!")
        print(f"  Successful: {successful}/{args.count}")
        print(f"  Output directory: {output_dir.absolute()}")
        print()
        print("Next steps:")
        print("  1. Go to Edge Impulse Studio -> Data acquisition")
        print("  2. Click 'Upload data'")
        print(f"  3. Select all files from {output_dir.absolute()}")
        print(f"  4. Set label to '{sample_type}'")
        print("  5. Upload and retrain your model")
        print()

    except serial.SerialException as e:
        print(f"ERROR: Could not open serial port: {e}")
        print(f"Make sure the ESP32 is connected and {args.port} is correct")
        sys.exit(1)
    except KeyboardInterrupt:
        print("\nCapture cancelled by user")
        sys.exit(0)


if __name__ == "__main__":
    main()
