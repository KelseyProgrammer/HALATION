# HALATION

**Feedback pitch ecosystem** — VST3 / AU / CLAP / Standalone
*Ament Audio*

[![Build](https://github.com/KelseyProgrammer/HALATION/actions/workflows/build_and_test.yml/badge.svg)](https://github.com/KelseyProgrammer/HALATION/actions)

---

HALATION splits your audio into up to 8 parallel delay paths, each pitch-shifted by a configurable interval and fed back into itself at a controlled rate. At zero feedback it's a lush harmonic doubler. Turn the bloom knob up and it becomes a self-generating ambient texture — the plugin becomes an instrument.

Named after the photographic bloom effect. Designed for shoegaze, ambient, drone, and experimental music.

---

## Installation

### macOS
1. Download `HALATION-x.x.x-macOS.pkg` from the [Releases](../../releases) page
2. Run the installer — copies VST3, AU, and CLAP to the system plugin folders
3. Rescan plugins in your DAW

### Windows
1. Download `HALATION-x.x.x-Windows.exe` from the [Releases](../../releases) page
2. Run the installer — choose VST3, CLAP, and/or Standalone
3. VST3 installs to `C:\Program Files\Common Files\VST3\`
4. Rescan plugins in your DAW

### Linux
1. Download `HALATION-x.x.x-Linux.zip` from the [Releases](../../releases) page
2. Extract and copy `HALATION.vst3` to `~/.vst3/` or `/usr/lib/vst3/`

---

## How to Use

### The Core Idea

HALATION works on a single feedback loop: your dry signal enters each pitch-shifted path, gets delayed slightly, and a portion of each path's output feeds back into itself — accumulating harmonics over time.

- **No feedback** → staggered harmonic chord, fades after the delay time
- **Low feedback** → harmonics sustain and breathe, wall-of-sound texture
- **High feedback** → self-sustaining; the plugin generates sound even after your input stops

### Signal Flow

```
Input ──→ [Path 1: delay + pitch shift] ─┐
       ──→ [Path 2: delay + pitch shift]  ├──→ mix ──→ Output
       ──→ ...                            │
       ──→ [Path N: delay + pitch shift] ─┘
                ↑ each path feeds back into itself
```

### Parameters

#### Global Controls

| Knob | What It Does |
|---|---|
| **Bloom** | Feedback amount. 0 = no feedback (pure doubler). Above ~0.7 the plugin becomes self-sustaining. Stay below 0.99 for controlled results. |
| **Stagger** | Spreads path delay times from 20ms (tight) to 20–180ms (evenly spread). High stagger = arpeggiated bloom. Low stagger = tight chorus/unison. |
| **Tilt** | Spectral tilt applied each feedback cycle. Positive = brighter per cycle (shimmer). Negative = darker per cycle (tape warmth). |
| **Damping** | High-frequency rolloff in the feedback loop. Combine with negative Tilt for a dense, wooly texture. |
| **Chaos** | Adds a slow, independent pitch drift to each path (±15 cents). Low = organic life. High = paths wander apart. |
| **Mix** | Dry/wet blend. Full wet works well on an aux send/return. |

#### Path Matrix (left panel)

Each row is one active path. Use the **+/−** button to add or remove paths (2–8).

| Control | What It Does |
|---|---|
| **Interval** (e.g. P5, Oct↑) | The pitch shift for this path in musical intervals |
| **Level fader** | Scales this path's wet output — does not affect feedback amount |
| **Pan dot** | Stereo position for this path |

#### Interval Presets (dropdown)

| Preset | Character |
|---|---|
| **Unison** | Pure doubler / thickener, no pitch shift |
| **Octaves** | Octaves above and below — dramatic |
| **Fifths** | Stacked fifths — classic shoegaze shimmer |
| **Harmonic Series** | Overtone stack, increasingly dense |
| **Chromatic** | Cluster / micro-detune |
| **Custom** | Set manually — preset snaps here when you adjust any interval |

### Starting Points

**Shimmer reverb send** (put HALATION on an aux, blend to taste):
- Preset: Fifths, 4 paths · Bloom: 0.5 · Stagger: 0.6 · Tilt: +0.1 · Chaos: 0.1 · Mix: 1.0

**Self-generating drone** (hold a note, then mute input — it lives on):
- Preset: Fifths, 6–8 paths · Bloom: 0.85 · Stagger: 0.4 · Tilt: −0.15 · Damping: 0.5 · Chaos: 0.2

**Harmonic doubler** (subtle thickening, zero feedback):
- Preset: Octaves or Fifths, 4 paths · Bloom: 0 · Stagger: 0.3 · Mix: 0.4–0.6

**Cluster frost** (chromatic shimmer, unpredictable):
- Preset: Chromatic, 6 paths · Bloom: 0.3 · Stagger: 0.7 · Chaos: 0.5 · Mix: 0.5

### Tips

- **Latency**: HALATION introduces ~2048 samples of latency (the pitch shifter's FFT window). Your DAW compensates automatically via Plugin Delay Compensation (PDC). Make sure PDC is enabled.
- **Self-oscillation control**: If Bloom is high and things run away, reduce Bloom or increase Damping. The DC blocker prevents true instability, but high Chaos + high Bloom gets unpredictable — that's part of the design.
- **Per-path level as a mixer**: With 8 paths, use level faders to sculpt the harmonic balance. Attenuating octave-down paths keeps the mix from getting muddy.
- **Stagger as rhythm**: At high Stagger, transients (drums, plucked notes) produce an arpeggiated bloom — each path arrives ~20ms later than the last, turning a single hit into a cascade.

---

## System Requirements

- macOS 11+ (Apple Silicon and Intel universal binary)
- Windows 10/11 (64-bit)
- Ubuntu 20.04+ (64-bit)
- DAW supporting VST3, AU, or CLAP
- ~15% CPU on M1 Mac at 44.1kHz / 512 samples / 8 paths / full feedback

---

## Building from Source

Requires CMake 3.22+, JUCE 8 (included as submodule), and a C++20 compiler.

```bash
git clone --recurse-submodules https://github.com/KelseyProgrammer/HALATION
cd HALATION
cmake -B Builds -DCMAKE_BUILD_TYPE=Release
cmake --build Builds --config Release
ctest --test-dir Builds --verbose
```

---

*Ament Audio*
