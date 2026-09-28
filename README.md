# G3X Fresh Air

G3X Fresh Air is a dynamic two-band high-frequency exciter and presence processor plugin. It is designed to add clarity, articulation, and brilliance to audio signals using just two intuitive macro controls.

Built with C++20 and JUCE 9, the plugin is available as a 64-bit VST3 plugin and a Standalone application for Windows.

## What It Does

Rather than acting as a static EQ, Fresh Air utilizes dynamic, program-dependent processing. It reacts to the transients and volume of the incoming audio to smoothly enhance the upper frequencies without introducing harshness or brittleness.

- **Presence (Mid Air):** A broad, moving band focused between 3.2 kHz and 5 kHz. Increasing this control brings vocals forward, adds articulation to melodies, and enhances the definition of the mid-range without affecting the low end.
- **Air (High Air):** A moving high-shelf filter focused between 8.5 kHz and 11 kHz. Increasing this control adds openness, brilliance, and sheen to the extreme high frequencies (like hi-hats and vocal breathiness) without becoming piercing.

When both controls are set to 0%, the plugin is completely transparent and mathematically neutral, passing the audio through unaltered.

## Features

- **Dynamic Processing:** Built-in envelope followers and detectors react to the audio dynamically.
- **Link Bands:** A toggle that links the dynamic reduction of both bands, preserving the static proportion you set while ensuring the overall high-frequency enhancement remains balanced.
- **Output Trim (-12 dB to +3 dB):** Essential for level-matching. Since enhancing high frequencies increases the perceived and actual volume, the trim control allows you to compensate so you aren't fooled by "louder is better."
- **Smooth Bypass:** A crossfaded bypass that guarantees zero clicks or pops when toggled.
- **Precision Metering:** Accurate Peak and RMS output meters with clipping retention.
- **Hardened DSP:** Protected against NaNs, denormals (subnormals), and clipping. The DSP is thread-safe, allocation-free on the audio thread, and strictly guards against sample-rate Nyquist reflections.
- **DAW Preset Synchronization:** Factory presets are fully synchronized between the plugin GUI and your DAW's preset manager.

## Presets (Modes)

Fresh Air comes with several factory presets (sometimes referred to as "modes") that set the parameters for specific use-cases. **Only one mode can be active at a time.** When you select a mode, it instantly sets both bands, the link toggle, and the trim to optimal starting points.

- **Neutral (Default):** 0% Presence, 0% Air. Completely transparent bypass mode.
- **Vocal Presence:** Pushes the Mid Air (38%) to bring vocals to the front of the mix, with a touch of High Air (18%). Unlinked, allowing the transients to breathe independently.
- **Vocal Air:** Focuses heavily on extreme high frequencies (62% High Air) for breathiness and pop-vocal sheen. Linked mode is enabled to keep the bands dynamically proportional.
- **Drum Detail:** Aggressive Mid Air (52%) to enhance snare crack and tom articulation, paired with moderate High Air (34%) for cymbal brightness.
- **Acoustic Clarity:** Balanced enhancement (42% Mid, 30% High) with Linked dynamics. Perfect for adding string definition to acoustic guitars without harsh picking transients.
- **Mix Open:** Gentle, linked enhancement across both bands to add subtle life and "air" to an entire master bus or instrument group.
## Installation (Windows x64)

The team has successfully compiled the final release build. 

1. Locate the compiled `G3X Fresh Air.vst3` bundle (typically found in `build\G3XFreshAir_artefacts\VST3\`).
2. Copy the entire `G3X Fresh Air.vst3` folder into your system VST3 directory:
   `C:\Program Files\Common Files\VST3`
3. Rescan your plugins in your DAW (Ableton, FL Studio, Reaper, etc.).

## Building from Source

To build the project yourself, you will need **CMake** and a compatible C++ compiler (like MSVC or MinGW-w64 GCC 16+). The build process will automatically fetch the required JUCE 9.0.1 framework.

```bash
# Generate the build environment
cmake -S . -B build

# Compile the Release target
cmake --build build --config Release

# Run the DSP Unit Tests
ctest --test-dir build --build-config Release --output-on-failure
```
