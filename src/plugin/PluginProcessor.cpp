#include "plugin/PluginProcessor.hpp"
#include "plugin/PluginEditor.hpp"

#include <algorithm>
#include <array>
#include <cmath>

G3XFreshAirAudioProcessor::G3XFreshAirAudioProcessor()
  : AudioProcessor(BusesProperties()
      .withInput("Input", juce::AudioChannelSet::stereo(), true)
      .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
    state(*this, nullptr, "state", createParameterLayout()) {}

juce::AudioProcessorValueTreeState::ParameterLayout
G3XFreshAirAudioProcessor::createParameterLayout() {
  juce::AudioProcessorValueTreeState::ParameterLayout layout;
  layout.add(std::make_unique<juce::AudioParameterFloat>(
    juce::ParameterID{"presenceAmount", 1}, "Presence",
    juce::NormalisableRange<float>{0.0F, 100.0F, 0.1F}, 0.0F));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
    juce::ParameterID{"airAmount", 1}, "Air",
    juce::NormalisableRange<float>{0.0F, 100.0F, 0.1F}, 0.0F));
  layout.add(std::make_unique<juce::AudioParameterBool>(
    juce::ParameterID{"linkBands", 1}, "Link bands", false));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
    juce::ParameterID{"outputTrimDb", 1}, "Output trim",
    juce::NormalisableRange<float>{-12.0F, 3.0F, 0.1F}, 0.0F, "dB"));
  layout.add(std::make_unique<juce::AudioParameterBool>(
    juce::ParameterID{"bypass", 1}, "Bypass", false));
  return layout;
}

void G3XFreshAirAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
  juce::ignoreUnused(samplesPerBlock);
  dsp_.prepare(sampleRate, static_cast<std::size_t>(getTotalNumOutputChannels()));
  updateParameters();
  clearClipIndicator();
}

void G3XFreshAirAudioProcessor::releaseResources() { dsp_.reset(); }

bool G3XFreshAirAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
  const auto input = layouts.getMainInputChannelSet();
  return input == layouts.getMainOutputChannelSet()
    && (input == juce::AudioChannelSet::mono() || input == juce::AudioChannelSet::stereo());
}

void G3XFreshAirAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
  juce::ScopedNoDenormals guard;
  updateParameters();
  std::array<float*, 2> channels{
    buffer.getWritePointer(0), buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr};
  dsp_.process(channels.data(), static_cast<std::size_t>(buffer.getNumChannels()),
    static_cast<std::size_t>(buffer.getNumSamples()));
  updateMeters(buffer);
}

void G3XFreshAirAudioProcessor::updateParameters() noexcept {
  dsp_.setPresence(*state.getRawParameterValue("presenceAmount") / 100.0F);
  dsp_.setAir(*state.getRawParameterValue("airAmount") / 100.0F);
  dsp_.setLinkBands(*state.getRawParameterValue("linkBands") >= 0.5F);
  dsp_.setOutputTrimDb(*state.getRawParameterValue("outputTrimDb"));
  dsp_.setBypass(*state.getRawParameterValue("bypass") >= 0.5F);
}

void G3XFreshAirAudioProcessor::updateMeters(const juce::AudioBuffer<float>& buffer) noexcept {
  auto peak = 0.0F;
  double sumSquares = 0.0;
  for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
    peak = std::max(peak, buffer.getMagnitude(channel, 0, buffer.getNumSamples()));
    const auto* samples = buffer.getReadPointer(channel);
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
      sumSquares += static_cast<double>(samples[sample]) * samples[sample];
  }
  const auto sampleCount = buffer.getNumChannels() * buffer.getNumSamples();
  const auto rms = sampleCount > 0 ? static_cast<float>(std::sqrt(sumSquares / sampleCount)) : 0.0F;
  peak_.store(peak, std::memory_order_relaxed);
  rms_.store(rms, std::memory_order_relaxed);
  if (peak >= 1.0F) clipped_.store(true, std::memory_order_relaxed);
}

juce::AudioProcessorEditor* G3XFreshAirAudioProcessor::createEditor() {
  return new G3XFreshAirEditor(*this);
}

void G3XFreshAirAudioProcessor::getStateInformation(juce::MemoryBlock& destination) {
  if (auto xml = state.copyState().createXml()) copyXmlToBinary(*xml, destination);
}

void G3XFreshAirAudioProcessor::setStateInformation(const void* data, int size) {
  if (auto xml = getXmlFromBinary(data, size)) state.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
  return new G3XFreshAirAudioProcessor();
}
