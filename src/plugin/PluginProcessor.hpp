#pragma once

#include <array>
#include <atomic>
#include <juce_audio_processors/juce_audio_processors.h>
#include "dsp/FreshAirProcessor.hpp"

struct PresetInfo {
  const char* name;
  float presence;
  float air;
  float outputTrimDb;
  bool linkBands;
};

inline constexpr std::array<PresetInfo, 6> kPresets{{
  {"Neutral", 0.0f, 0.0f, 0.0f, false},
  {"Vocal Presence", 38.0f, 18.0f, -1.0f, false},
  {"Vocal Air", 25.0f, 62.0f, -2.0f, true},
  {"Drum Detail", 52.0f, 34.0f, -1.5f, false},
  {"Acoustic Clarity", 42.0f, 30.0f, -1.0f, true},
  {"Mix Open", 28.0f, 38.0f, -1.5f, true}
}};

class G3XFreshAirAudioProcessor final : public juce::AudioProcessor {
public:
  G3XFreshAirAudioProcessor();
  ~G3XFreshAirAudioProcessor() override = default;

  static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;
  bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
  void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override;
  juce::AudioProcessorEditor* createEditor() override;
  bool hasEditor() const override { return true; }
  const juce::String getName() const override { return "G3X Fresh Air"; }
  double getTailLengthSeconds() const override { return 0.0; }
  bool acceptsMidi() const override { return false; }
  bool producesMidi() const override { return false; }
  int getNumPrograms() override { return static_cast<int>(kPresets.size()); }
  int getCurrentProgram() override { return currentProgram_; }
  void setCurrentProgram(int) override;
  const juce::String getProgramName(int) override;
  void changeProgramName(int, const juce::String&) override {}
  void getStateInformation(juce::MemoryBlock& destination) override;
  void setStateInformation(const void* data, int size) override;

  [[nodiscard]] float outputPeak() const noexcept { return peak_.load(std::memory_order_relaxed); }
  [[nodiscard]] float outputRms() const noexcept { return rms_.load(std::memory_order_relaxed); }
  [[nodiscard]] bool outputClipped() const noexcept { return clipped_.load(std::memory_order_relaxed); }
  void clearClipIndicator() noexcept { clipped_.store(false, std::memory_order_relaxed); }
  juce::AudioProcessorValueTreeState state;

private:
  void updateParameters() noexcept;
  void updateMeters(const juce::AudioBuffer<float>& buffer) noexcept;

  g3x::FreshAirProcessor dsp_;
  std::atomic<float>* presenceParam_{nullptr};
  std::atomic<float>* airParam_{nullptr};
  std::atomic<float>* linkBandsParam_{nullptr};
  std::atomic<float>* outputTrimDbParam_{nullptr};
  std::atomic<float>* bypassParam_{nullptr};

  int currentProgram_{0};
  std::atomic<float> peak_{0.0F};
  std::atomic<float> rms_{0.0F};
  std::atomic<bool> clipped_{false};
};
