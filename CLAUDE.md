# HALATION — Claude Code Brief
**Ament Audio | JUCE 8 | VST3 + AU | macOS-first**

---

## 1. What This Plugin Is

HALATION is a **feedback pitch ecosystem** — a stereo audio effect that splits the input into 2–8 parallel delay paths, each pitch-shifted by a configurable interval, feeding back into themselves at a controlled rate. The result ranges from a lush harmonic doubler (zero feedback) to a self-generating ambient texture (high feedback).

Named after the photographic bloom effect. Shoegaze aesthetic. The sound should feel like light bleeding outward.

Three distinct use modes exist on a single feedback knob:
- **Zero feedback** → harmonically-rich doubler / staggered chord bloom
- **Low feedback** → living harmonic sustain, breathing wall-of-sound
- **High feedback** → self-generating texture, the plugin becomes an instrument

---

## 2. Repository & Build System

This project uses JUCE 8, CMake, Catch2, and GitHub Actions. JUCE is a git submodule at `JUCE/`. Do not use Projucer. Everything is CMake.

**Key CMake facts:**
- Project name: `HALATION`
- Company: `Ament Audio`
- `PLUGIN_MANUFACTURER_CODE`: `AmnA`
- `PLUGIN_CODE`: `Hala`
- Formats: `VST3 AU Standalone`
- All source files must be **manually listed** in `CMakeLists.txt` — no globbing
- JUCE modules needed: `juce_audio_utils`, `juce_audio_processors`, `juce_dsp`, `juce_graphics`, `juce_gui_basics`, `juce_gui_extra`
- C++ standard: **C++20**

When adding a new `.cpp` file, always add it to the `target_sources` block in `CMakeLists.txt`.

**Build commands:**
```bash
# Configure (run once, or after CMakeLists.txt changes)
cmake -B Builds -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build Builds --config Release

# Run tests
ctest --test-dir Builds --verbose --output-on-failure

# Or run tests directly
./Builds/Tests

# Run benchmarks
./Builds/Benchmarks
```

For faster builds, add Ninja: `cmake -B Builds -G Ninja -DCMAKE_BUILD_TYPE=Release`

On macOS for universal binary: `-DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"`

---

## 3. Prescribed Architecture

These class boundaries are fixed. Internal implementation is flexible.

```
PluginProcessor        — APVTS owner, processBlock, prepareToPlay
PluginEditor           — JUCE Component, owns all UI widgets
HalationEngine         — orchestrates all DSP, one instance owned by Processor
  ├── PathProcessor[8] — one per active path: PitchShifter + delay buffer + feedback tap
  ├── SpectralTilt      — applied in the feedback loop per path
  └── IntervalPresets   — static utility: named interval sets → semitone arrays
```

**`HalationEngine`** is the only DSP class the Processor talks to. It exposes:
```cpp
void prepare(double sampleRate, int maxBlockSize);
void reset();
void process(juce::AudioBuffer<float>& buffer);
void setNumPaths(int n);           // 2–8, safe to call on message thread w/ lock
void setPathInterval(int path, float semitones);
void setGlobalParameters(float bloomRate, float stagger, float spectralTilt,
                         float damping, float chaos, float mix);
```

**`PathProcessor`** handles one voice. It owns:
- A circular delay buffer (max ~2 seconds)
- A `PitchShifter` instance (phase vocoder, see §6)
- Its own feedback tap with per-path gain

**`SpectralTilt`** applies a high+low shelf EQ in the feedback path. Positive tilt = brighter per cycle. Negative = darker. Separate damping parameter adds a 1-pole LP for classic tape-style rolloff.

**`IntervalPresets`** is a pure static class:
```cpp
static std::array<float, 8> getPreset(PresetID id);
enum class PresetID { Unison, Octaves, Fifths, HarmonicSeries, Chromatic, Custom };
```
Presets populate the per-path semitone values. Custom means the user has manually adjusted at least one path — preset selector snaps back to Custom when any interval knob moves.

---

## 4. Parameter System (APVTS)

All parameters live in `PluginProcessor::createParameterLayout()`. Use the prefix convention below — this is the canonical ID format.

### Global Parameters

| Parameter ID | Type | Range | Default | Notes |
|---|---|---|---|---|
| `global_num_paths` | Int | 2–8 | 4 | Triggers path reallocation |
| `global_bloom_rate` | Float | 0.0–1.0 | 0.3 | Feedback amount (all paths) |
| `global_stagger` | Float | 0.0–1.0 | 0.5 | Delay offset between paths |
| `global_spectral_tilt` | Float | -1.0–1.0 | -0.2 | Per-cycle brightness shift |
| `global_damping` | Float | 0.0–1.0 | 0.4 | HF rolloff in feedback |
| `global_chaos` | Float | 0.0–1.0 | 0.15 | Pitch drift per voice |
| `global_mix` | Float | 0.0–1.0 | 0.7 | Dry/wet |
| `global_interval_preset` | Int | 0–5 | 2 | Maps to `PresetID` enum |

### Per-Path Parameters (paths 0–7)

| Parameter ID | Type | Range | Default |
|---|---|---|---|
| `path_N_semitones` | Float | -24.0–24.0 | see presets |
| `path_N_level` | Float | 0.0–1.0 | 1.0 |
| `path_N_pan` | Float | -1.0–1.0 | spread by index |

`N` is zero-indexed: `path_0_semitones`, `path_1_semitones`, etc.

### Parameter Smoothing
All float parameters must go through `juce::SmoothedValue<float>` with a 20ms ramp in `HalationEngine`. Never read APVTS values directly in the audio thread — copy to smoothed values in `processBlock` before passing to the engine.

---

## 5. Interval Preset Definitions

```
Unison         :  [0, 0, 0, 0, 0, 0, 0, 0]        (pure doubler)
Octaves        :  [0, 12, -12, 24, -24, 12, 0, -12]
Fifths         :  [0, 7, 12, 19, -5, 7, -12, 5]    (stacked fifths)
HarmonicSeries :  [0, 12, 19, 24, 28, 31, 34, 36]  (overtone stack)
Chromatic      :  [0, 1, 2, 3, -1, -2, -3, -4]     (cluster / detune)
Custom         :  (user-defined, no auto-fill)
```

When `global_num_paths` < 8, use the first N values of the active preset.

---

## 6. DSP Implementation Notes

### Pitch Shifting
Use a **phase vocoder** approach (FFT-based). Suggested parameters:
- FFT size: 2048 samples
- Hop size: FFT/4 (75% overlap)
- Window: Hann, normalised for overlap-add
- Use `juce::dsp::FFT` for the transform

The phase vocoder introduces latency of approximately `FFT size` samples. Report this to the host via `getLatencySamples()` in the Processor.

For the **chaos** parameter: add a slow per-path LFO (0.1–0.5 Hz, sine) that offsets the pitch ratio by ±`chaos * 15` cents. Each path's LFO should be phase-offset so voices drift independently.

### Delay / Stagger
Each path gets a delay time calculated as:
```
path_delay[i] = BASE_DELAY_MS + (i * stagger * MAX_STAGGER_MS)
BASE_DELAY_MS = 20ms
MAX_STAGGER_MS = 80ms
```
So at full stagger, paths arrive spread across 20–180ms. At zero stagger, all paths arrive together at 20ms (the minimum needed to accommodate the pitch shifter's lookahead).

### Feedback Topology
Each `PathProcessor` feeds back into **itself only** (single-path feedback). A cross-feedback mode can be a v1.1 feature — do not implement it in v1.0. This keeps CPU predictable and avoids instability edge cases.

Feedback gain = `bloom_rate` (global). Per-path `level` scales the wet output only, not the feedback tap.

### DC Blocking
Insert a DC-blocking filter at the end of each feedback path. Use a 1-pole highpass at 10 Hz. Without this, DC can accumulate in long feedback tails.

### CPU Budget
Target ≤ 15% CPU on an M1 Mac at 44.1kHz, 512 samples, 8 paths, full feedback. If the phase vocoder is too expensive at 8 paths, document this and add a quality/performance toggle (`high_quality` bool parameter, off = halve FFT size to 1024).

### Realtime Safety
For anything in the audio thread / hot DSP path (e.g. `processBlock`):
- Allocate in constructors or `prepareToPlay`, not while rendering audio
- Avoid dynamic allocations and container growth (`std::vector::push_back`, map insertion, string building)
- Prefer fixed-size storage (`std::array`, preallocated buffers, fixed-capacity queues)
- Keep operations deterministic and lock-free where possible

---

## 7. UI / Editor Guidelines

### Aesthetic Direction
Tie-dye / photographic bloom. Radial, organic, warm. Inspired directly by the reference image: deep navy-indigo backgrounds with radial amber/magenta/violet blooms spreading outward from centers. This is NOT a minimal dark plugin — it uses color freely, with multiple accent colors tied to individual paths.

**Confirmed color palette (do not deviate):**
- Background: `#12111a` (warm near-black, blue-black not pure black)
- Primary accent / bloom core: `#e8a84a` (burnt amber)
- Path colors (one per path, in order): `#e8a84a`, `#c4707a`, `#8870c0`, `#4aa4b8`, `#d46a30`, `#7a9e3a`, `#b05090`, `#5a7ac4`
- Section borders: `rgba(200,160,80,0.12–0.18)`
- Muted label text: `#5a5070` / `#6a5f7a`
- Typography: Space Mono (monospace) for labels, values, logo — DM Sans for body/secondary text

**Key visual element — Bloom Visualizer (center panel):**
- A radial SVG/Canvas element showing each active path as a bloom arm radiating outward
- Arm angle encodes path index, arm length encodes interval (larger interval = longer radius)
- Arm fill opacity responds to path level; glow intensity responds to bloom rate
- Each arm uses that path's color from the palette above
- A bright amber core dot sits at center; outer ring subtly tracks bloom rate
- Interval names (e.g. "P5", "Oct↑") float at arm tips in path color
- This is the signature UI element — it should be immediately recognizable

**Typography:**
- Plugin name "HALATION": Space Mono, 20px, 700, letter-spacing 0.12em, amber `#e8a84a`
- Section labels: Space Mono, 9px, letter-spacing 0.18em, uppercase, muted purple
- Knob labels: Space Mono, 8px, uppercase, `#5a5070`
- Knob values: Space Mono, 9px, `#9a8868`

**No skeuomorphism.** Flat arc knobs, thin stroke indicators, glow on active state only (CSS box-shadow or canvas glow, not fake 3D).

### Layout (confirmed from mockup)
```
┌──────────────────────────────────────────────────────────────┐
│  HALATION [logo]                          [AMENT AUDIO]      │
│  feedback pitch ecosystem [subtitle]                         │
├──────────────┬───────────────────────────┬───────────────────┤
│  PATH MATRIX │   BLOOM VISUALIZER        │  GLOBAL KNOBS     │
│  ─────────── │   (canvas, center)        │  ─────────────    │
│  [paths+]    │   radial bloom arms       │  Bloom            │
│  [preset ↓]  │   one per active path     │  Stagger          │
│              │                           │  Tilt             │
│  P1 semi lvl │                           │  Damping          │
│  P2 semi lvl │                           │  Chaos            │
│  P3 semi lvl │                           │                   │
│  P4 semi lvl │                           │                   │
├──────────────┴───────────────────────────┴───────────────────┤
│  [Mix slider]    BLOOM: 0.30   STAGGER: 0.50   CHAOS: 0.15  │
└──────────────────────────────────────────────────────────────┘
```

Layout is three-column: path matrix left (240px), bloom visualizer center (flex), global knobs right (160px). Bottom strip holds mix + live readouts.

- Inactive path rows are hidden entirely — matrix shrinks
- Semitone displays show musical interval names (P5, Oct↑, m3, etc.) from a lookup table
- Preset dropdown snaps to "Custom" when any interval is touched manually
- Plugin window size: **680 × 480px**, non-resizable for v1.0

### JUCE Component Approach
- Use custom `LookAndFeel` subclass (`HalationLookAndFeel`) for all knob/slider rendering — do not mix stock JUCE look
- Knobs are `juce::Slider` (rotary style) with custom `drawRotarySlider` override using the arc style from the mockup — thin track arc, filled arc in path/accent color, small indicator dot
- Path matrix rows are a `juce::Component` subclass (`PathRowComponent`) — each holds a semitone display label, a level mini-fader, and a pan dot. Show/hide by setting `setVisible(false)` on inactive rows and calling `resized()` on the parent.
- Bloom visualizer is a `juce::AnimatedAppComponent` or `juce::Component` with a `juce::Timer` at 30fps — redraws when any relevant parameter changes
- The `PathRowComponent` semitone display is a `juce::Label` styled in path color, showing the interval name string. Clicking it could open an interval editor popover (v1.1 feature — skip for v1.0, just show the value).

### Interval Name Lookup
```cpp
// In a utility header
static juce::String semitoneToIntervalName(int semitones) {
    static const std::map<int, juce::String> names = {
        {0,"P1"}, {1,"m2"}, {2,"M2"}, {3,"m3"}, {4,"M3"}, {5,"P4"},
        {6,"A4"}, {7,"P5"}, {8,"m6"}, {9,"M6"}, {10,"m7"}, {11,"M7"},
        {12,"Oct↑"}, {-12,"Oct↓"}, {19,"12↑"}, {24,"2Oct↑"}, {-24,"2Oct↓"},
        {-5,"P4↓"}, {-7,"P5↓"}
    };
    auto it = names.find(semitones);
    if (it != names.end()) return it->second;
    return (semitones > 0 ? "+" : "") + juce::String(semitones);
}
```

---

## 8. Naming & Code Style

- **Namespace**: `halation::` for all DSP classes. Editor and Processor live in global namespace (JUCE convention).
- **Class names**: PascalCase — `HalationEngine`, `PathProcessor`, `SpectralTilt`
- **Member variables**: `m_camelCase` prefix — `m_sampleRate`, `m_numPaths`
- **Constants**: `kPascalCase` — `kFFTSize`, `kMaxPaths`
- **Parameter IDs**: `snake_case` strings as defined in §4 — never hardcode these strings outside `PluginProcessor.cpp`. Use a `namespace ParameterIDs { const juce::String ... }` block.
- **No raw `new`/`delete`** — use `std::make_unique` or `juce::OwnedArray`. Avoid stack-allocating `PluginProcessor` or `HalationEngine` — they contain ~12MB of DSP buffers and will SIGSEGV on Windows (1MB default stack).
- **No `using namespace juce`** in headers — always qualify (`juce::String`, etc.)
- **Header guards**: `#pragma once` only
- File pairs: every `.h` has a matching `.cpp`. No implementation in headers except trivial getters.
- Uses `.clang-format` with Allman-style braces, 4-space indentation, no column limit.

### Code Quality
Always resolve any compile warnings encountered during builds. Warnings should be treated as errors and fixed before considering a task complete.

Note: LSP/clangd often reports false positive diagnostic errors (like "undeclared identifier", "file not found") because it doesn't have full context of the JUCE module system. Ignore these unless the actual build fails.

JUCE modules include common standard library headers (`<vector>`, `<algorithm>`, `<string>`, `<memory>`, etc.) so you don't need to add those explicitly in JUCE code. Adding them is harmless but redundant.

---

## 9. Preset System

Presets store the full plugin state via APVTS `copyState()` / `applyState()`. File format is JUCE's ValueTree serialised to XML, saved to `~/Library/Audio/Presets/Ament Audio/HALATION/` on macOS.

### Factory Presets (ship with plugin, read-only)
| Name | Description |
|---|---|
| Bloom — Clean | 4 paths, Fifths preset, zero feedback, wide stagger |
| Wall of Fifths | 4 paths, Fifths, medium feedback |
| Shimmer Cloud | 8 paths, HarmonicSeries, medium-high feedback, high chaos |
| Dark Drone | 4 paths, Octaves, high feedback, negative tilt, high damping |
| Cluster Frost | 6 paths, Chromatic, low feedback, high chaos |
| Init | Flat state: 4 paths, Unison, all defaults |

Implement a `PresetManager` class that handles load/save/list. The UI shows a simple prev/next arrow nav plus a name display — no full browser in v1.0.

---

## 10. Testing (Catch2)

Tests live in `tests/`. All required v1.0 tests are written and passing on Linux, macOS, and Windows CI.

### Test files (all complete)
- `PluginBasics.cpp` — dummy sanity + plugin name check
- `PitchShifterTests.cpp` — silence in → silence out; unity ratio keeps energy at input frequency (Goertzel); octave-up shifts energy to double the frequency (Goertzel)
- `SpectralTiltTests.cpp` — flat tilt near-unity gain; positive tilt increases HF; negative tilt reduces HF; damping attenuates HF
- `HalationEngineTests.cpp` — prepares/resets without crashing; no NaN/inf at any feedback value 0–0.99 over 300 blocks; num_paths changes mid-stream produce no NaN
- `ParameterTests.cpp` — all parameter IDs exist in APVTS; global defaults match §4; per-path defaults match Fifths preset + level=1.0 + spread pan

### Key testing lessons learned
- **Always heap-allocate `PluginProcessor` and `HalationEngine` in tests.** Both contain `PathProcessor[8]`, each with two `std::array<float, 192000>` members (~1.5MB each). Stack-allocating these causes SIGSEGV on Windows (1MB default stack) even though Linux/macOS tolerate it. Use `std::make_unique<>()` everywhere.
- **PitchShifter output amplitude is unreliable to assert on.** The phase vocoder overlap-add normalization produces very low absolute RMS (~0.001). Use Goertzel frequency analysis to check *which* frequency has energy instead of asserting output level.
- **Benchmarks are all wrapped in `#ifndef CI`.** The `PluginProcessor` is ~12MB; creating many instances for warmup iterations causes memory exhaustion on CI runners. All three benchmarks (constructor, destructor, editor) are skipped in CI via a `CI=1` compile definition set in `CMakeLists.txt` when the `CI` env var is present.

Run tests: `cmake --build Builds --config Release && ctest --test-dir Builds --verbose --output-on-failure`

---

## 11. CI / GitHub Actions

CI runs on Linux (ubuntu-22.04), macOS (macos-14), and Windows. All three platforms are currently green.

### Known CI quirks
- **Signing secrets**: Steps that require `DEV_ID_APP_CERT` check the job-level env var `HAVE_SIGNING_CERT` (set to `${{ secrets.DEV_ID_APP_CERT != '' }}`). GitHub Actions disallows `secrets.*` directly in step `if:` expressions. Without secrets configured, signing/notarization steps are skipped cleanly.
- **`cmake/` is a git submodule** (`sudara/cmake-includes`). Never edit files inside `cmake/` — changes cannot be committed from the parent repo. Add any project-level CMake customizations in the main `CMakeLists.txt` after the `include(...)` calls.
- **CI compile definition**: `CMakeLists.txt` passes `-DCI=1` to the Benchmarks target when the `CI` env var is set (standard on GitHub Actions runners).

---

## 12. Build Status

All phases complete as of March 2026:

| Phase | Description | Status |
|---|---|---|
| 1 | Source scaffold, APVTS, CMakeLists.txt | ✅ |
| 2 | Live DSP: smoothing, DC blocking, chaos LFO, limiter | ✅ |
| 3 | Interval preset wiring, path reset on num_paths change | ✅ |
| 4 | Preset navigation strip wired into editor | ✅ |
| 5 | Full UI: PathRowComponent, BloomVisualizer, global knobs, mix strip | ✅ |
| 6 | Catch2 tests: all four test files, green on all 3 CI platforms | ✅ |

### What remains for v1.0 ship
- **AU validation**: run `auval -v aufx Hala AmnA` locally on macOS to confirm AU compliance
- **Code signing**: configure Apple Developer secrets in GitHub repo settings (`DEV_ID_APP_CERT`, `DEV_ID_APP_PASSWORD`, `DEV_ID_INSTALLER_CERT`, `DEV_ID_INSTALLER_PASSWORD`, `DEVELOPER_ID_APPLICATION`, `DEVELOPER_ID_INSTALLER`, `NOTARIZATION_USERNAME`, `NOTARIZATION_PASSWORD`, `TEAM_ID`)
- **Factory presets**: implement the 6 factory presets listed in §9 as embedded XML in `PresetManager`
- **CPU profiling**: verify ≤15% CPU at 44.1kHz / 512 samples / 8 paths / full feedback on M1

---

## 13. What Claude Code Should Do First

When starting a new session on this project, read this file in full, then:

1. Run `git log --oneline` to confirm current state
2. Check `source/` if the task involves DSP or UI — confirm the relevant files exist
3. Ask which task to focus on before writing any code

Do not generate all files at once speculatively. Build one class at a time, compile-check mentally against the JUCE API, then move to the next. When uncertain about a JUCE API detail, say so rather than hallucinating a method name.
