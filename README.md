# Fresh Aero
A dynamic two-band high-frequency exciter built with C++20 and JUCE 9. Available as a 64-bit VST3 and standalone application for Windows.

Instead of acting like a static EQ, it reacts dynamically to the audio and smoothly boosts high frequencies without adding harshness.

The "Mid Air" band focuses on 3.2 kHz to 5 kHz to bring vocals forward.
The "High Air" band focuses on 8.5 kHz to 11 kHz to add openness.

### Features
- Dynamic envelope followers that duck harsh transients.
- Link button to keep the EQ balance proportional when it compresses.
- Output trim from -12 dB to +3 dB to compensate for volume bumps.
- Allocation-free and thread-safe DSP.

DSP performance is highly optimized. Processing a stereo track at 48 kHz takes about 1.36% of a single CPU core.

### Presets
It comes with a few starting points. If you move a knob after loading one, it will automatically switch to "Custom".

- Neutral: Transparent bypass.
- Vocal Presence: Pushes the Mid Air band for vocals.
- Vocal Air: Focuses on extreme high frequencies for breathiness.
- Drum Detail: Aggressive Mid Air for snares, High Air for cymbals.
- Acoustic Clarity: Balanced enhancement with linked dynamics.
- Mix Open: Gentle linked enhancement for the master bus.

### Installation
The compiled bundle is in `build\FreshAero_artefacts\VST3\`. Just copy the `Fresh Aero.vst3` folder into `C:\Program Files\Common Files\VST3` and rescan your DAW.

### Compiling
This uses MinGW-w64 GCC 16.2.0 and CMake 4.4+.

You can just run the included script in PowerShell:
```powershell
.\build.ps1 -Clean -Target All -Jobs 16
```

Or do it manually:
```bash
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build --config Release -j 16
```
