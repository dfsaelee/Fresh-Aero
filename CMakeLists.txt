cmake_minimum_required(VERSION 3.22)
project(G3XFreshAir VERSION 0.1.0 LANGUAGES C CXX)

option(G3X_BUILD_PLUGIN "Build JUCE plugin targets" ON)
option(G3X_BUILD_TESTS "Build DSP tests" ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(MINGW)
  add_link_options(-static -static-libgcc -static-libstdc++)
endif()

add_library(g3x_fresh_air_dsp STATIC src/dsp/FreshAirProcessor.cpp)
target_include_directories(g3x_fresh_air_dsp PUBLIC src)
if(MSVC)
  target_compile_options(g3x_fresh_air_dsp PRIVATE /W4 /permissive-)
else()
  target_compile_options(g3x_fresh_air_dsp PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(G3X_BUILD_PLUGIN)
  include(FetchContent)
  FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 9.0.1 GIT_SHALLOW TRUE)
  FetchContent_GetProperties(JUCE)
  if(NOT juce_POPULATED)
    FetchContent_Populate(JUCE)
    if(MINGW)
      set(JUCE_SOURCE_DIR "${juce_SOURCE_DIR}")
      include(${CMAKE_SOURCE_DIR}/cmake/PatchJuceForMinGW.cmake)
    endif()
    add_subdirectory(${juce_SOURCE_DIR} ${juce_BINARY_DIR})
  endif()
  juce_add_plugin(G3XFreshAir COMPANY_NAME G3X BUNDLE_ID com.g3x.freshair
    PLUGIN_MANUFACTURER_CODE G3xx PLUGIN_CODE Gfar FORMATS VST3 Standalone
    PRODUCT_NAME "G3X Fresh Air" IS_SYNTH FALSE NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE IS_MIDI_EFFECT FALSE COPY_PLUGIN_AFTER_BUILD FALSE)
  target_sources(G3XFreshAir PRIVATE
    src/plugin/PluginProcessor.cpp
    src/plugin/PluginProcessor.hpp
    src/plugin/PluginEditor.cpp
    src/plugin/PluginEditor.hpp)
  target_include_directories(G3XFreshAir PRIVATE src)
  target_compile_definitions(G3XFreshAir PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)
  target_link_libraries(G3XFreshAir PRIVATE g3x_fresh_air_dsp juce::juce_audio_utils)
  if(MINGW)
    target_link_libraries(G3XFreshAir PUBLIC wininet winmm ws2_32 wsock32 version shlwapi dwrite d2d1 dxgi d3d11 dcomp dbghelp shcore uuid dxguid ksuser dsound strmiids imm32 comctl32 dwmapi ole32 oleaut32 sapi)
  endif()
  target_link_libraries(G3XFreshAir PRIVATE juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
endif()

if(G3X_BUILD_TESTS)
  enable_testing()
  add_executable(g3x_fresh_air_tests tests/FreshAirProcessorTests.cpp)
  target_link_libraries(g3x_fresh_air_tests PRIVATE g3x_fresh_air_dsp)
  add_test(NAME g3x_fresh_air_tests COMMAND g3x_fresh_air_tests)
endif()
