# HALATION — Setup & Build Guide
**Ament Audio | JUCE 8 | macOS**

---

## Prerequisites

| Tool | Version | Install |
|---|---|---|
| macOS | 13 Ventura+ | — |
| Xcode | 15+ | App Store |
| Xcode Command Line Tools | latest | `xcode-select --install` |
| CMake | 3.22+ | `brew install cmake` |
| Git | any recent | bundled with Xcode CLT |
| Ninja (optional, faster builds) | any | `brew install ninja` |

Verify:
```bash
cmake --version    # should be 3.22+
xcodebuild -version
git --version
```

---

## 1. Clone the Repo

```bash
git clone https://github.com/KelseyProgrammer/HALATION.git
cd HALATION
```

---

## 2. Initialize JUCE Submodule

JUCE is included as a git submodule. You must populate it before CMake will configure.

```bash
git submodule update --init --recursive
```

This fetches JUCE into `JUCE/`. It's ~600MB so give it a minute on first clone.

Verify it worked:
```bash
ls JUCE/modules   # should list juce_audio_basics, juce_dsp, etc.
```

---

## 3. Configure with CMake

### Option A — Xcode (recommended for debugging)
```bash
cmake -B build -G Xcode
```
Then open in Xcode:
```bash
open build/HALATION.xcodeproj
```
Select the `HALATION_VST3` or `HALATION_Standalone` scheme and hit Run.

### Option B — Ninja (faster iteration, no IDE)
```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug
```

### Option C — Default Makefiles
```bash
cmake -B build
cmake --build build
```

**First configure takes 2–5 minutes** — CMake is compiling JUCE modules. Subsequent builds are incremental and fast.

---

## 4. Build Targets

| Target | What it builds |
|---|---|
| `HALATION_VST3` | VST3 plugin bundle |
| `HALATION_AU` | Audio Unit plugin bundle |
| `HALATION_Standalone` | Standalone app (fastest for testing) |
| `HALATION_Tests` | Catch2 test runner |

Build a specific target:
```bash
cmake --build build --target HALATION_Standalone
```

---

## 5. Run the Plugin

### Standalone (quickest)
After build, the standalone app is at:
```
build/HALATION_artefacts/Debug/Standalone/HALATION.app
```
Double-click or:
```bash
open build/HALATION_artefacts/Debug/Standalone/HALATION.app
```

### Copy VST3 for DAW testing
```bash
cp -r build/HALATION_artefacts/Debug/VST3/HALATION.vst3 \
      ~/Library/Audio/Plug-Ins/VST3/
```
Then rescan plugins in your DAW (Logic / Ableton / Reaper).

### Copy AU for Logic/GarageBand
```bash
cp -r build/HALATION_artefacts/Debug/AU/HALATION.component \
      ~/Library/Audio/Plug-Ins/Components/
```
Restart Logic or run:
```bash
auval -v aufx Hala AmnA   # validate the AU
```

> **Note:** `COPY_PLUGIN_AFTER_BUILD` is set to `FALSE` in `CMakeLists.txt` by default. You can set it to `TRUE` to auto-copy on every build, but this requires the build to run without sandboxing issues. Fine to enable once you've confirmed the build works.

---

## 6. Run Tests

```bash
cmake --build build --target HALATION_Tests
./build/Tests/HALATION_Tests
```

With verbose output:
```bash
./build/Tests/HALATION_Tests -v high
```

Run a single test tag:
```bash
./build/Tests/HALATION_Tests "[pitch_shifter]"
```

---

## 7. Project Structure

```
HALATION/
├── CMakeLists.txt          ← single build config, edit this when adding files
├── CLAUDE.md               ← AI brief, architecture spec, all conventions
├── SETUP.md                ← this file
├── JUCE/                   ← git submodule, do not edit
├── cmake/                  ← HALATION cmake helpers
├── source/
│   ├── PluginProcessor.h/.cpp   ← APVTS, processBlock, host interface
│   ├── PluginEditor.h/.cpp      ← top-level JUCE Component, UI layout
│   ├── HalationEngine.h/.cpp    ← DSP orchestrator
│   ├── PathProcessor.h/.cpp     ← per-voice: pitch shifter + delay + feedback
│   ├── PitchShifter.h/.cpp      ← phase vocoder implementation
│   ├── SpectralTilt.h/.cpp      ← shelving EQ for feedback path
│   ├── IntervalPresets.h/.cpp   ← static preset tables
│   ├── PresetManager.h/.cpp     ← load/save/list user presets
│   └── HalationLookAndFeel.h/.cpp  ← custom JUCE LookAndFeel
└── tests/
    ├── PitchShifterTests.cpp
    ├── SpectralTiltTests.cpp
    ├── HalationEngineTests.cpp
    └── ParameterTests.cpp
```

---

## 8. Adding New Source Files

**Every new `.cpp` file must be manually added to `CMakeLists.txt`:**

```cmake
target_sources(HALATION PRIVATE
    source/PluginProcessor.cpp
    source/PluginEditor.cpp
    source/HalationEngine.cpp
    source/PathProcessor.cpp       # ← add new files here
    source/PitchShifter.cpp
    # ...
)
```

After editing `CMakeLists.txt`, re-run CMake configure:
```bash
cmake -B build   # or re-run from Xcode: Product → Clean Build Folder, then build
```

---

## 9. Updating JUCE

To pull the latest JUCE 8 patch:
```bash
cd JUCE
git pull origin master
cd ..
git add JUCE
git commit -m "Update JUCE submodule"
```

To update the cmake helpers:
```bash
git submodule update --remote cmake
git add cmake
git commit -m "Update cmake helpers"
```

Check the JUCE changelog before updating — occasionally there are breaking changes to `CMakeLists.txt` that require manual merging.

---

## 10. Common Issues

**CMake can't find JUCE modules**
→ You skipped `git submodule update --init`. Run it now.

**AU validation fails (`auval` errors)**
→ Make sure `PLUGIN_MANUFACTURER_CODE` is exactly 4 chars and matches what's in the binary. Clean build and re-copy the `.component`.

**Phase vocoder sounds metallic / artifacts**
→ This is normal at extreme pitch ratios (>±12 semitones). It's a known limitation of phase vocoders — acceptable for the shoegaze aesthetic. If it's severe at small ratios, check that the hop size is FFT/4 and the window is properly normalised.

**High CPU at 8 paths**
→ Each path runs a full 2048-point FFT at 44.1kHz. At 8 paths this is significant. If CPU exceeds ~20%, enable the performance mode (halve FFT size). This is a known tradeoff documented in `CLAUDE.md §6`.

**Plugin not appearing in DAW after copy**
→ Rescan in your DAW's plugin manager. For Logic, sometimes a full quit + relaunch is needed. Run `auval` to verify the AU is valid before blaming the DAW.

**`juce_dsp` not found during configure**
→ It's linked in `CMakeLists.txt` via `target_link_libraries`. If CMake still can't find it, make sure the JUCE submodule is fully initialized and you're not accidentally pointing at a partial JUCE install elsewhere on the system.

---

## 11. Release Builds

```bash
cmake -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release --config Release
```

Release builds enable LTO and optimization flags via `juce_recommended_lto_flags` and `juce_recommended_config_flags`. Expect 3–5x faster DSP vs. Debug.

For signed/notarized distribution, you will need an Apple Developer account and code signing certificates configured in `CMakeLists.txt` — this is out of scope for local development.

---

## 12. Claude Code Workflow

When using Claude Code on this project:

1. Always start a session by reading `CLAUDE.md` — it contains the full architecture spec, parameter IDs, naming conventions, and implementation notes
2. Build one class at a time; confirm it compiles before moving on
3. After adding any file, update `CMakeLists.txt` and re-configure CMake
4. Run `HALATION_Tests` after completing any DSP class
5. The Standalone target is the fastest way to hear results without a DAW

```bash
# Typical dev loop
cmake --build build --target HALATION_Standalone && \
open build/HALATION_artefacts/Debug/Standalone/HALATION.app
```
