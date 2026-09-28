# G3X Fresh Air

a dynamic two-band high-frequency exciter built with c++20 and juce 9. available as a 64-bit vst3 and standalone for windows.

instead of acting like a static eq, it reacts dynamically to the audio and smoothly boosts high frequencies without adding harshness.

the "mid air" band focuses on 3.2 kHz to 5 kHz to bring vocals forward.
the "high air" band focuses on 8.5 kHz to 11 kHz to add openness.

### features
- dynamic envelope followers that duck harsh transients
- link button to keep the eq balance proportional when it compresses
- output trim from -12 db to +3 db to compensate for volume bumps
- allocation-free and thread-safe dsp 

dsp performance is pretty solid. processing a stereo track at 48kHz takes about 1.36% of a single cpu core.

### presets
it comes with a few starting points. if you move a knob after loading one, it will automatically switch to "custom".

- neutral: transparent bypass
- vocal presence: pushes the mid air band for vocals
- vocal air: focuses on extreme high frequencies for breathiness
- drum detail: aggressive mid air for snares, high air for cymbals
- acoustic clarity: balanced enhancement with linked dynamics
- mix open: gentle linked enhancement for the master bus

### installation
the compiled bundle is in `build\G3XFreshAir_artefacts\VST3\`. just copy the `G3X Fresh Air.vst3` folder into `C:\Program Files\Common Files\VST3` and rescan your daw.

### compiling
this uses mingw-w64 gcc 16.2.0 and cmake 4.4+.

you can just run the included script in powershell:
```powershell
.\build.ps1 -Clean -Target All -Jobs 16
```

or do it manually:
```bash
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build --config Release -j 16
ctest --test-dir build --build-config Release --output-on-failure
```
