#pragma once

#include <atomic>
#include <juce_audio_processors/juce_audio_processors.h>
#include "dsp/FreshAirProcessor.hpp"

class G3XFreshAirAudioProcessor final : public juce::AudioProcessor {
public:
  G3XFreshAirAudioProcessor();
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
  int getNumPrograms() override { return 1; }
  int getCurrentProgram() override { return 0; }
  void setCurrentProgram(int) override {}
  const juce::String getProgramName(int) override { return {}; }
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
  std::atomic<float> peak_{0.0F};
  std::atomic<float> rms_{0.0F};
  std::atomic<bool> clipped_{false};
};
