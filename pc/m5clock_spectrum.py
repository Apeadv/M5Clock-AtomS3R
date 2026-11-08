#!/usr/bin/env python3
"""
M5Clock PC spectrum sender.

Captures the computer's loopback audio (what the speakers are playing) and
sends 12 logarithmic band levels to the AtomS3R over USB serial.

Install once:
    py -m pip install -r pc/requirements.txt

Run:
    py pc/m5clock_spectrum.py --port COM4

Close Arduino Serial Monitor / M5Burner serial windows before running.
"""

import argparse
import ctypes
import math
import os
import sys
import time

import numpy as np
import serial
from serial.tools import list_ports

try:
    import soundcard as sc
except ImportError:
    print("Missing dependency: soundcard")
    print("Install with: py -m pip install -r pc/requirements.txt")
    raise SystemExit(1)

BANDS = 12
SAMPLE_RATE = 48000
BLOCK = 2048
MIN_HZ = 60.0
MAX_HZ = 7000.0
HEADER_1 = 0xA5
HEADER_2 = 0x5A
DEFAULT_FPS = 30
MUTEX = None


def already_running():
    global MUTEX
    if os.name != "nt":
        return False
    MUTEX = ctypes.windll.kernel32.CreateMutexW(None, False, "M5ClockSpectrum")
    return ctypes.windll.kernel32.GetLastError() == 183


def checksum(levels):
    value = HEADER_2
    for level in levels:
        value ^= int(level)
    return value


def list_devices():
    print("Serial ports:")
    ports = list(list_ports.comports())
    if not ports:
        print("  (none)")
    for port in ports:
        print(f"  {port.device}  {port.description}")
    print()
    print("Audio speakers:")
    for speaker in sc.all_speakers():
        print(f"  {speaker.name}")
    print()
    print("Tip: use --port COMx; use --device \"speaker name\" if the default")
    print("speaker is not the one playing music.")


def find_atom_port():
    ports = list(list_ports.comports())
    atom = [p for p in ports if (p.vid == 0x303A) or ("AtomS3R" in (p.description or ""))]
    if len(atom) == 1:
        return atom[0].device
    if not atom and len(ports) == 1:
        return ports[0].device
    return None


def get_loopback(device_name):
    speaker = None
    if device_name:
        for item in sc.all_speakers():
            if device_name.lower() in item.name.lower():
                speaker = item
                break
        if speaker is None:
            raise RuntimeError(f"Speaker not found: {device_name}")
    else:
        speaker = sc.default_speaker()

    try:
        mic = sc.get_microphone(id=str(speaker.name), include_loopback=True)
        return mic, speaker
    except Exception as exc:
        raise RuntimeError(
            f"Cannot open loopback for {speaker.name!r}: {exc}\n"
            "On Windows, enable Stereo Mix or select the correct speaker with --device."
        ) from exc


def band_edges():
    ratio = (MAX_HZ / MIN_HZ) ** (1.0 / BANDS)
    return [MIN_HZ * (ratio ** i) for i in range(BANDS + 1)]


def audio_to_levels(samples, edges, gain_db):
    # samples: float32 [-1, 1], shape (frames, channels)
    mono = np.asarray(samples, dtype=np.float32)
    if mono.ndim > 1:
        mono = mono.mean(axis=1)
    if mono.size < BLOCK:
        mono = np.pad(mono, (0, BLOCK - mono.size))
    else:
        mono = mono[:BLOCK]

    mono = mono - float(mono.mean())
    window = np.hanning(BLOCK)
    spectrum = np.abs(np.fft.rfft(mono * window)) / (BLOCK / 2.0)
    freqs = np.fft.rfftfreq(BLOCK, 1.0 / SAMPLE_RATE)

    levels = []
    for i in range(BANDS):
        mask = (freqs >= edges[i]) & (freqs < edges[i + 1])
        peak = float(spectrum[mask].max()) if np.any(mask) else 0.0
        db = 20.0 * math.log10(peak + 1e-8) + gain_db
        normalized = (db + 62.0) / 62.0
        normalized = max(0.0, min(1.0, normalized))
        levels.append(int(round(normalized * 8.0)))
    return levels


def main():
    parser = argparse.ArgumentParser(description="Send PC audio spectrum to M5Clock")
    parser.add_argument("--port", help="Serial port; omitted = auto-detect AtomS3R")
    parser.add_argument("--device", help="Speaker name for loopback capture")
    parser.add_argument("--gain", type=float, default=12.0, help="Extra gain in dB (default 12)")
    parser.add_argument("--fps", type=int, default=DEFAULT_FPS, help="Frames per second (default 30)")
    parser.add_argument("--list", action="store_true", help="List serial ports and speakers")
    parser.add_argument("--autostart", action="store_true", help="Keep retrying until AtomS3R is connected")
    args = parser.parse_args()

    if args.list:
        list_devices()
        return

    if already_running():
        print("M5Clock spectrum is already running.")
        return 0

    edges = band_edges()
    delay = 1.0 / max(1, min(args.fps, 60))
    last_wait_notice = 0.0

    while True:
        port = args.port or find_atom_port()
        if not port:
            now = time.time()
            if now - last_wait_notice > 5:
                print("Waiting for AtomS3R USB serial...")
                last_wait_notice = now
            time.sleep(2)
            continue

        try:
            loopback, speaker = get_loopback(args.device)
            with loopback.recorder(samplerate=SAMPLE_RATE) as recorder:
                link = serial.Serial(port, 115200, timeout=1, write_timeout=1)
                print(f"Loopback : {loopback.name}")
                print(f"Serial   : {port}")
                print(f"Bands    : {MIN_HZ:.0f} Hz - {MAX_HZ / 1000:.1f} kHz, {BANDS} bands")
                print("Press Ctrl+C to stop.")

                smoothed = [0] * BANDS
                last_device_check = 0.0
                try:
                    while True:
                        if not args.device and time.time() - last_device_check > 1.5:
                            last_device_check = time.time()
                            try:
                                current = sc.default_speaker()
                            except Exception:
                                current = None
                            if current is not None and current.id != speaker.id:
                                print(f"Audio output changed to {current.name}; reopening loopback.")
                                break

                        started = time.time()
                        data = recorder.record(numframes=BLOCK)
                        levels = audio_to_levels(data, edges, args.gain)
                        for i, level in enumerate(levels):
                            if level > smoothed[i]:
                                smoothed[i] = level
                            else:
                                smoothed[i] = max(level, smoothed[i] - 1)

                        packet = bytes([HEADER_1, HEADER_2]) + bytes(smoothed) + bytes([checksum(smoothed)])
                        link.write(packet)
                        link.flush()

                        rest = delay - (time.time() - started)
                        if rest > 0:
                            time.sleep(rest)
                finally:
                    link.close()
        except KeyboardInterrupt:
            print("\nStopped.")
            return 0
        except Exception as exc:
            print(f"Connection lost: {exc}")
            print("Waiting for AtomS3R / audio device...")
            time.sleep(2)


if __name__ == "__main__":
    sys.exit(main() or 0)
