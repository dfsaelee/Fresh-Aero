# G3X Fresh Air: VST3 troubleshooting notes

## Current status

The build configuration does create a conventional Windows VST3 bundle:

`G3X Fresh Air.vst3/Contents/x86_64-win/G3X Fresh Air.vst3`

The repository contains packaging checks and DSP tests, but no successful DAW scan/load result. The prior handoff contains no completed diagnosis.

## Most likely issue: the wrong file is being tested and patched

The temporary loader programs (`test_load*.cpp`) try to load the bundle root:

`...\\G3X Fresh Air\\G3X Fresh Air.vst3`

That is a **directory**, not the VST3 module. The actual binary is nested in `Contents\\x86_64-win`. Therefore a `LoadLibrary` failure from these probes does not establish that the plugin module itself is broken.

The manual DLL-renaming scripts have the same layout problem: they look in the folder surrounding the bundle, rather than beside the actual module. If runtime DLLs are needed, they must be found through the loader's search path—normally adjacent to the module inside `Contents\\x86_64-win`—or avoided by static linking.

## Smallest next checks (in this order)

1. Remove the manually modified installed copy and copy a fresh, complete `.vst3` bundle from the release build output. Do not copy only the internal binary.
2. Put it in exactly one scanned VST3 location, preferably `C:\\Program Files\\Common Files\\VST3\\`. Then trigger a full rescan/restart in the DAW.
3. Check the internal module, not the bundle directory:

   ```powershell
   $module = 'C:\\Program Files\\Common Files\\VST3\\G3X Fresh Air.vst3\\Contents\\x86_64-win\\G3X Fresh Air.vst3'
   & .\\tools\\mingw64\\bin\\objdump.exe -p $module | Select-String 'DLL Name:'
   ```

   It should not list `libstdc++-6.dll`, `libgcc_s_seh-1.dll`, or `libwinpthread-1.dll`. The current `build.ps1` already flags the first two; add `libwinpthread` to that check if it appears.
4. In the DAW's scanner log, distinguish **not discovered** (wrong folder/cache) from **discovered then rejected/crashed** (module/runtime/ABI problem). The exact scanner message is the next high-value evidence.

## High-probability build risk

JUCE 9 explicitly rejects MinGW upstream. This project overrides that restriction and applies 15 compatibility patches directly to JUCE, including graphics and VST3-helper changes. That can produce a binary which compiles but is not reliably host-compatible.

The clean control experiment is an unpatched **MSVC x64 Release** build of the same source, with the normal JUCE VST3 target. If that scans, the DSP/plugin code is likely fine and the MinGW patch/runtime chain is the fault domain. If it does not, focus on the DAW log and VST3 metadata instead.

## Secondary checks if the module loads but the DAW still rejects it

- Verify `Contents\\Resources\\moduleinfo.json` exists in the installed bundle and was generated from the current build.
- Clear the DAW's plugin cache before rescanning; hosts commonly remember a prior failed scan.
- Confirm the DAW and plugin are both x64.
- Test in a second host (for example REAPER) to separate a host-specific cache/configuration problem from a module problem.
- Build and run the Standalone target. A Standalone crash points to JUCE/runtime/UI initialization; a healthy Standalone plus a failed VST3 scan narrows it to VST3 packaging, scanner expectations, or ABI.

## What not to infer yet

Passing DSP tests or package-layout tests does not prove that a DAW can load the VST3. Conversely, the existing root-bundle `LoadLibrary` tests are not valid VST3 module tests. The next diagnostic should record the DAW scanner error and the import list from the nested module before changing source code again.
