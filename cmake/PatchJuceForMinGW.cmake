# =============================================================================
# PatchJuceForMinGW.cmake — Automatically patch JUCE 9.x for MinGW-w64 builds
#
# Usage: After FetchContent_MakeAvailable(JUCE), set JUCE_SOURCE_DIR and include
#        this file. It applies all required MinGW patches idempotently.
#
# Requires: JUCE_SOURCE_DIR to be set (e.g. ${juce_SOURCE_DIR})
# =============================================================================

if(NOT DEFINED JUCE_SOURCE_DIR)
  message(FATAL_ERROR "JUCE_SOURCE_DIR must be set before including PatchJuceForMinGW.cmake")
endif()

# Sentinel file — if present, patches were already applied
set(_G3X_SENTINEL "${JUCE_SOURCE_DIR}/.g3x_patched")

if(EXISTS "${_G3X_SENTINEL}")
  message(STATUS "G3X: JUCE MinGW patches already applied (sentinel found)")
  return()
endif()

message(STATUS "G3X: Applying JUCE MinGW compatibility patches...")

# ---------------------------------------------------------------------------
# Helper: replace text in a file (no-op if file doesn't exist or old text absent)
# ---------------------------------------------------------------------------
function(_g3x_patch_file filepath old_text new_text)
  if(NOT EXISTS "${filepath}")
    message(STATUS "  SKIP (not found): ${filepath}")
    return()
  endif()
  file(READ "${filepath}" _content)
  string(FIND "${_content}" "${old_text}" _pos)
  if(_pos EQUAL -1)
    message(STATUS "  SKIP (already patched): ${filepath}")
    return()
  endif()
  string(REPLACE "${old_text}" "${new_text}" _content "${_content}")
  file(WRITE "${filepath}" "${_content}")
  message(STATUS "  PATCHED: ${filepath}")
endfunction()

# Helper: append text to a file if marker is not already present
function(_g3x_append_file filepath marker text)
  if(NOT EXISTS "${filepath}")
    return()
  endif()
  file(READ "${filepath}" _content)
  string(FIND "${_content}" "${marker}" _pos)
  if(NOT _pos EQUAL -1)
    message(STATUS "  SKIP (marker found): ${filepath}")
    return()
  endif()
  file(APPEND "${filepath}" "${text}")
  message(STATUS "  APPENDED: ${filepath}")
endfunction()

# Helper: prepend text to a file if marker is not already present
function(_g3x_prepend_file filepath marker text)
  if(NOT EXISTS "${filepath}")
    return()
  endif()
  file(READ "${filepath}" _content)
  string(FIND "${_content}" "${marker}" _pos)
  if(NOT _pos EQUAL -1)
    message(STATUS "  SKIP (marker found): ${filepath}")
    return()
  endif()
  file(WRITE "${filepath}" "${text}${_content}")
  message(STATUS "  PREPENDED: ${filepath}")
endfunction()

# ===========================================================================
# Patch 1: Remove MinGW rejection error
# ===========================================================================
_g3x_patch_file(
  "${JUCE_SOURCE_DIR}/modules/juce_core/system/juce_TargetPlatform.h"
  "#error \"MinGW is not supported. Please use an alternative compiler.\""
  "// MinGW support enabled by Fresh Aero build system"
)

# ===========================================================================
# Patch 2: Force 64-bit detection for MinGW
# ===========================================================================
_g3x_append_file(
  "${JUCE_SOURCE_DIR}/modules/juce_core/system/juce_TargetPlatform.h"
  "G3X: Force 64-bit"
  "
// G3X: Force 64-bit detection for MinGW
#if defined(__MINGW64__) || defined(_WIN64)
  #undef JUCE_32BIT
  #define JUCE_64BIT 1
#endif
"
)

# ===========================================================================
# Patch 3: Add <intrin.h> for __cpuid intrinsic
# ===========================================================================
_g3x_prepend_file(
  "${JUCE_SOURCE_DIR}/modules/juce_core/native/juce_SystemStats_windows.cpp"
  "intrin.h"
  "#include <intrin.h>
"
)

# ===========================================================================
# Patch 4: Fix __uuidof(device) in DirectX wrapper
# ===========================================================================
_g3x_patch_file(
  "${JUCE_SOURCE_DIR}/modules/juce_graphics/native/juce_DirectX_windows.cpp"
  "__uuidof (device)"
  "__uuidof (IDXGIDevice)"
)

# ===========================================================================
# Patch 5: Fix __uuidof in Direct2D HwndContext (device + surface)
# ===========================================================================
_g3x_patch_file(
  "${JUCE_SOURCE_DIR}/modules/juce_gui_basics/native/juce_Direct2DHwndContext_windows.cpp"
  "__uuidof (device)"
  "__uuidof (IDXGIDevice)"
)
_g3x_patch_file(
  "${JUCE_SOURCE_DIR}/modules/juce_gui_basics/native/juce_Direct2DHwndContext_windows.cpp"
  "__uuidof (surface)"
  "__uuidof (IDXGISurface)"
)

# ===========================================================================
# Patch 6: Fix missing CaretPosition enums (MinGW headers lack them)
# These are already integer literals in JUCE 9.0.1, but older checkouts may
# use CaretPosition_BeginningOfLine etc. — patch if present.
# ===========================================================================
_g3x_patch_file(
  "${JUCE_SOURCE_DIR}/modules/juce_gui_basics/native/accessibility/juce_UIATextProvider_windows.h"
  "CaretPosition_BeginningOfLine"
  "2"
)
_g3x_patch_file(
  "${JUCE_SOURCE_DIR}/modules/juce_gui_basics/native/accessibility/juce_UIATextProvider_windows.h"
  "CaretPosition_EndOfLine"
  "1"
)
_g3x_patch_file(
  "${JUCE_SOURCE_DIR}/modules/juce_gui_basics/native/accessibility/juce_UIATextProvider_windows.h"
  "CaretPosition_Unknown"
  "0"
)

# ===========================================================================
# Patch 7: Fix missing NS_E_NO_MORE_SAMPLES macro
# ===========================================================================
_g3x_patch_file(
  "${JUCE_SOURCE_DIR}/modules/juce_audio_formats/codecs/juce_WindowsMediaAudioFormat.cpp"
  "NS_E_NO_MORE_SAMPLES"
  "((HRESULT)0xC00D0FEAL)"
)

# ===========================================================================
# Patch 8: Move GetDpiForWindow extern to file-global scope
# The original has `extern "C" ... GetDpiForWindow` inside a function body.
# We move it to global scope after #pragma once.
# ===========================================================================
set(_SCALE_FILE "${JUCE_SOURCE_DIR}/modules/juce_audio_plugin_client/detail/juce_PluginScaleFactorUtilities.h")
if(EXISTS "${_SCALE_FILE}")
  file(READ "${_SCALE_FILE}" _scale_content)
  # Check if we already patched (global extern present after #pragma once)
  string(FIND "${_scale_content}" "// G3X: GetDpiForWindow" _scale_pos)
  if(_scale_pos EQUAL -1)
    # Remove any existing extern "C" ... GetDpiForWindow line inside functions
    string(REPLACE "extern \"C\" UINT WINAPI GetDpiForWindow(HWND hwnd);" "" _scale_content "${_scale_content}")
    string(REPLACE "extern \"C\" UINT WINAPI GetDpiForWindow (HWND hwnd);" "" _scale_content "${_scale_content}")
    # Insert after #pragma once
    string(REPLACE "#pragma once" "#pragma once

// G3X: GetDpiForWindow moved to global scope for MinGW compatibility
extern \"C\" UINT WINAPI GetDpiForWindow(HWND hwnd);
" _scale_content "${_scale_content}")
    file(WRITE "${_SCALE_FILE}" "${_scale_content}")
    message(STATUS "  PATCHED: ${_SCALE_FILE}")
  else()
    message(STATUS "  SKIP (already patched): ${_SCALE_FILE}")
  endif()
endif()

# ===========================================================================
# Patch 9: Fix juceaide CMakeLists.txt — add static linking + Windows SDK libs
# ===========================================================================
set(_JUCEAIDE_FILE "${JUCE_SOURCE_DIR}/extras/Build/juceaide/CMakeLists.txt")
if(EXISTS "${_JUCEAIDE_FILE}")
  file(READ "${_JUCEAIDE_FILE}" _aide_content)
  string(FIND "${_aide_content}" "wininet" _aide_pos)
  if(_aide_pos EQUAL -1)
    # Add Windows SDK libs and static linking to target_link_libraries
    string(REPLACE
      "juce::juce_recommended_warning_flags)"
      "juce::juce_recommended_warning_flags wininet winmm wsock32 ws2_32 version shlwapi dwrite d2d1 dxgi d3d11 dcomp dbghelp shcore uuid dxguid)
    target_link_options(juceaide PRIVATE -static -static-libgcc -static-libstdc++)"
      _aide_content "${_aide_content}")
    file(WRITE "${_JUCEAIDE_FILE}" "${_aide_content}")
    message(STATUS "  PATCHED: ${_JUCEAIDE_FILE}")
  else()
    message(STATUS "  SKIP (already patched): ${_JUCEAIDE_FILE}")
  endif()
endif()

# ===========================================================================
# Patch 10: Fix pointer_sized_uint cast in HashMap
# ===========================================================================
_g3x_patch_file(
  "${JUCE_SOURCE_DIR}/modules/juce_core/containers/juce_HashMap.h"
  "(pointer_sized_uint)"
  "(size_t)"
)

# ===========================================================================
# Patch 11: Fix pointer_sized_int cast in ImageCache
# ===========================================================================
_g3x_patch_file(
  "${JUCE_SOURCE_DIR}/modules/juce_graphics/images/juce_ImageCache.cpp"
  "(pointer_sized_int)"
  "(size_t)"
)

# ===========================================================================
# Patch 12: Disable broken Direct2D custom rendering params
# ===========================================================================
set(_D2D_FILE "${JUCE_SOURCE_DIR}/modules/juce_graphics/native/juce_Direct2DGraphicsContext_windows.cpp")
if(EXISTS "${_D2D_FILE}")
  file(READ "${_D2D_FILE}" _d2d_content)
  string(FIND "${_d2d_content}" "#if 0 // G3X" _d2d_pos)
  if(_d2d_pos EQUAL -1)
    # We replace the entire if (FAILED(...)) block with an #if 0 block
    string(REGEX REPLACE
      "if \\(FAILED \\(factory->CreateCustomRenderingParams[^}]*return \\{\\};\r?\n[ \t]*}"
      "#if 0 // G3X\n        \\0\n#endif"
      _d2d_content "${_d2d_content}"
    )
    file(WRITE "${_D2D_FILE}" "${_d2d_content}")
    message(STATUS "  PATCHED: ${_D2D_FILE} (CreateCustomRenderingParams)")
  else()
    message(STATUS "  SKIP (already patched): ${_D2D_FILE}")
  endif()
endif()

# ===========================================================================
# Patch 13: Fix missing D2D1_SATURATION_PROP_SATURATION enum
# ===========================================================================
_g3x_patch_file(
  "${JUCE_SOURCE_DIR}/modules/juce_graphics/native/juce_Direct2DImage_windows.cpp"
  "D2D1_SATURATION_PROP_SATURATION"
  "0"
)

# ===========================================================================
# Patch 14: Fix vst3_helper — add static linking so it doesn't need MinGW DLLs
# ===========================================================================
set(_VST3_HELPER_FILE "${JUCE_SOURCE_DIR}/extras/Build/CMake/juce_vst3_helper/CMakeLists.txt")
if(EXISTS "${_VST3_HELPER_FILE}")
  file(READ "${_VST3_HELPER_FILE}" _vh_content)
  string(FIND "${_vh_content}" "static-libgcc" _vh_pos)
  if(_vh_pos EQUAL -1)
    string(REPLACE
      "target_compile_features(\${helper_name} PRIVATE cxx_std_17)"
      "target_compile_features(\${helper_name} PRIVATE cxx_std_17)

# G3X: Static link so vst3_helper doesn't depend on MinGW runtime DLLs
if(MINGW)
  target_link_options(\${helper_name} PRIVATE -static-libgcc -static-libstdc++)
endif()"
      _vh_content "${_vh_content}")
    file(WRITE "${_VST3_HELPER_FILE}" "${_vh_content}")
    message(STATUS "  PATCHED: ${_VST3_HELPER_FILE}")
  else()
    message(STATUS "  SKIP (already patched): ${_VST3_HELPER_FILE}")
  endif()
endif()

# ===========================================================================
# Patch 15: Add <cstring> / <string.h> includes to juce_core source files
# ===========================================================================
file(GLOB_RECURSE _JUCE_CORE_FILES
  "${JUCE_SOURCE_DIR}/modules/juce_core/*.h"
  "${JUCE_SOURCE_DIR}/modules/juce_core/*.cpp"
)
set(_cstring_count 0)
foreach(_file IN LISTS _JUCE_CORE_FILES)
  file(READ "${_file}" _fc)
  string(FIND "${_fc}" "#include <cstring>" _cpos)
  if(_cpos EQUAL -1)
    file(WRITE "${_file}" "#ifdef __cplusplus\n#include <cstring>\n#endif\n#include <string.h>\n${_fc}")
    math(EXPR _cstring_count "${_cstring_count} + 1")
  endif()
endforeach()
message(STATUS "  Added <cstring> includes to ${_cstring_count} juce_core files")

# ===========================================================================
# Write sentinel — all patches applied
# ===========================================================================
# ===========================================================================
# Patch 16: Dynamically load TaskDialogIndirect to avoid missing entry point
# ===========================================================================
set(_MSG_FILE "${JUCE_SOURCE_DIR}/modules/juce_gui_basics/native/juce_NativeMessageBox_windows.cpp")
if(EXISTS "${_MSG_FILE}")
  file(READ "${_MSG_FILE}" _msg_content)
  string(FIND "${_msg_content}" "// G3X Dynamic TaskDialogIndirect" _msg_pos)
  if(_msg_pos EQUAL -1)
    string(REGEX REPLACE
      "TaskDialogIndirect \\(&config, &buttonIndex, nullptr, nullptr\\);"
      "// G3X Dynamic TaskDialogIndirect\n                using TaskDialogIndirectFunc = HRESULT (WINAPI *)(const TASKDIALOGCONFIG*, int*, int*, BOOL*);\n                if (HMODULE comctl = LoadLibraryA(\"comctl32.dll\")) {\n                    if (auto proc = (TaskDialogIndirectFunc)GetProcAddress(comctl, \"TaskDialogIndirect\")) {\n                        proc(&config, &buttonIndex, nullptr, nullptr);\n                    }\n                    FreeLibrary(comctl);\n                }"
      _msg_content "${_msg_content}"
    )
    file(WRITE "${_MSG_FILE}" "${_msg_content}")
    message(STATUS "  PATCHED: ${_MSG_FILE} (TaskDialogIndirect)")
  else()
    message(STATUS "  SKIP (already patched): ${_MSG_FILE}")
  endif()
endif()

file(WRITE "${_G3X_SENTINEL}" "Patched for MinGW by Fresh Aero build system\nTimestamp: ${CMAKE_CURRENT_LIST_FILE}\n")
message(STATUS "G3X: All JUCE MinGW patches applied successfully")
