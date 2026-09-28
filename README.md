# G3X Fresh Air

A dynamic two-band high-frequency exciter that adds clarity, articulation, and brilliance to audio signals. Built with C++20 and JUCE 9 for Windows (VST3 & Standalone).

## What It Does
Instead of a static EQ, Fresh Air reacts dynamically to your audio, boosting high frequencies smoothly without introducing harshness.
- **Mid Air:** Focuses on 3.2 kHz – 5 kHz to bring vocals forward and add definition.
- **High Air:** Focuses on 8.5 kHz – 11 kHz to add openness and brilliance.

## Features
- **Dynamic Processing:** Built-in envelope followers smoothly duck harsh transients.
- **Link Band Dynamics:** Links the dynamic reduction of both bands to preserve your EQ balance.
- **Output Trim:** Compensate for volume boosts (-12 dB to +3 dB).
- **Hardened DSP:** Allocation-free, thread-safe, and highly optimized processing.

## Performance (CPU Benchmark)
The DSP is heavily optimized. Processing a stereo track at 48kHz uses approximately **1.36% of a single CPU core**.

## Presets (Modes)
Comes with factory presets that set optimal starting points for different instruments. *Note: Moving a knob after loading a preset will automatically switch it to 'Custom'.*
- **Neutral:** Transparent bypass.
- **Vocal Presence:** Pushes the Mid Air band to bring vocals forward.
- **Vocal Air:** Focuses heavily on extreme high frequencies for breathiness.
- **Drum Detail:** Aggressive Mid Air for snare crack, paired with High Air for cymbals.
- **Acoustic Clarity:** Balanced enhancement for acoustic guitars with Linked dynamics.
- **Mix Open:** Gentle, linked enhancement for a master bus.

## Installation (Windows x64)
1. Locate the compiled `G3X Fresh Air.vst3` bundle in `build\G3XFreshAir_artefacts\VST3\`.
2. Copy it into your system VST3 directory: `C:\Program Files\Common Files\VST3`
3. Rescan plugins in your DAW.

## Building from Source
This project uses **MinGW-w64 GCC 16.2.0** and **CMake 4.4+**.

**Build Script (Recommended)**
```powershell
.\build.ps1 -Clean -Target All -Jobs 16
```

### Manual CMake Build
If you prefer to build manually, ensure your MinGW `bin` directory is in your `PATH` and run:

```bash
# Generate the build environment
cmake -S . -B build -G "MinGW Makefiles"

# Compile the Release target
cmake --build build --config Release -j 16

# Run the DSP Unit Tests
ctest --test-dir build --build-config Release --output-on-failure
```
