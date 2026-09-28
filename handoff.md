# Handoff: MinGW Build Loop & VST3 Scanning Fixes

## 1. Goal Achieved
We completely broke the debug build loop! The project can now be cleanly built from scratch in one step using MinGW-w64 GCC 16, without any manual file modifications. The resulting `.vst3` bundle is fully self-contained and statically linked.

## 2. Key Changes Made

* **Automated JUCE Patching:** Created `cmake/PatchJuceForMinGW.cmake`, a pure-CMake script that automatically applies 16 MinGW compatibility patches to JUCE's source code. 
* **CMake Lifecycle Fix:** Changed `CMakeLists.txt` to use `FetchContent_Populate()` followed by our patch script, and *then* `add_subdirectory(JUCE)`. This ensures patches are applied *before* JUCE attempts to build its internal `juceaide` tool.
* **C++ Type-Checking Fix:** Replaced a broken `if (false && FAILED(...))` workaround with a proper `#if 0` block in `juce_Direct2DGraphicsContext_windows.cpp` so GCC 16 doesn't fail on the missing Windows SDK signature.
* **Full Static Linking:** Added `-static` to the global CMake linker flags to ensure `libwinpthread-1.dll`, `libstdc++-6.dll`, and `libgcc_s_seh-1.dll` are fully statically compiled into the VST3.
* **REAPER VST3 Scanner Fix (TaskDialogIndirect):** Modified `juce_NativeMessageBox_windows.cpp` to load `TaskDialogIndirect` *dynamically* via `GetProcAddress`. Previously, this was a hard dependency on `comctl32.dll` v6. REAPER's background scanner (and the Standalone app) lacked the application manifest required to load v6, causing Windows to instantly abort the plugin load with a `0xC0000139` (STATUS_ENTRYPOINT_NOT_FOUND) error during REAPER's scan phase.
* **Build Script Updates:** Updated `build.ps1` to verify the absence of `libwinpthread`, and corrected its pathing to find the `.vst3` bundle in the `G3XFreshAir_artefacts` directory (MinGW does not use MSVC's `Release/` subdirectory).

## 3. Current Status
* The `.\build.ps1 -Clean` command is 100% green.
* The `vst3_helper.exe` successfully loads the compiled plugin and generates a valid `moduleinfo.json` manifest.
* DSP tests pass.
* The VST3 DLL has exactly zero MinGW runtime dependencies.

## 4. Next Steps for You (DAW Testing)
Because we dynamically patched `TaskDialogIndirect`, REAPER's scanner will no longer crash trying to map the DLL!

1. Open File Explorer and go to `C:\Program Files\Common Files\VST3\`.
2. Delete the current `G3X Fresh Air.vst3` **file** (the single file you copied earlier).
3. Go to your project build folder: `build\G3XFreshAir_artefacts\VST3\`.
4. Copy the entire `G3X Fresh Air.vst3` **folder** (which contains the `Contents` subfolder and `moduleinfo.json`).
5. Paste it into `C:\Program Files\Common Files\VST3\`.
6. Open REAPER and trigger a rescan. It should now successfully pick up the plugin!
