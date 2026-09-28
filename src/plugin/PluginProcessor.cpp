#include "plugin/PluginProcessor.hpp"
#include "plugin/PluginEditor.hpp"

#include <algorithm>
#include <array>
#include <cmath>

FreshAeroAudioProcessor::FreshAeroAudioProcessor()
  : AudioProcessor(BusesProperties()
      .withInput("Input", juce::AudioChannelSet::stereo(), true)
      .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
    state(*this, nullptr, "state", createParameterLayout()) {
  presenceParam_ = state.getRawParameterValue("presenceAmount");
  airParam_ = state.getRawParameterValue("airAmount");
  linkBandsParam_ = state.getRawParameterValue("linkBands");
  outputTrimDbParam_ = state.getRawParameterValue("outputTrimDb");
  bypassParam_ = state.getRawParameterValue("bypass");
}

juce::AudioProcessorValueTreeState::ParameterLayout
FreshAeroAudioProcessor::createParameterLayout() {
  juce::AudioProcessorValueTreeState::ParameterLayout layout;
  layout.add(std::make_unique<juce::AudioParameterFloat>(
    juce::ParameterID{"presenceAmount", 1}, "Presence",
    juce::NormalisableRange<float>{0.0F, 100.0F, 0.1F}, 0.0F, "%"));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
    juce::ParameterID{"airAmount", 1}, "Air",
    juce::NormalisableRange<float>{0.0F, 100.0F, 0.1F}, 0.0F, "%"));
  layout.add(std::make_unique<juce::AudioParameterBool>(
    juce::ParameterID{"linkBands", 1}, "Link bands", false));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
    juce::ParameterID{"outputTrimDb", 1}, "Output trim",
    juce::NormalisableRange<float>{-12.0F, 3.0F, 0.1F}, 0.0F, "dB"));
  layout.add(std::make_unique<juce::AudioParameterBool>(
    juce::ParameterID{"bypass", 1}, "Bypass", false));
  return layout;
}

void FreshAeroAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
  juce::ignoreUnused(samplesPerBlock);
  dsp_.prepare(sampleRate, static_cast<std::size_t>(std::max(1, getTotalNumOutputChannels())));
  updateParameters();
  clearClipIndicator();
}

void FreshAeroAudioProcessor::releaseResources() { dsp_.reset(); }

bool FreshAeroAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
  const auto input = layouts.getMainInputChannelSet();
  const auto output = layouts.getMainOutputChannelSet();
  if (input != output) return false;
  return input == juce::AudioChannelSet::mono() || input == juce::AudioChannelSet::stereo();
}

void FreshAeroAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
  juce::ScopedNoDenormals guard;

  const auto totalNumInputChannels = getTotalNumInputChannels();
  const auto totalNumOutputChannels = getTotalNumOutputChannels();
  for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
    buffer.clear(i, 0, buffer.getNumSamples());

  if (buffer.getNumChannels() == 0 || buffer.getNumSamples() == 0) return;

  updateParameters();

  const auto numChannels = std::min(buffer.getNumChannels(), 2);
  std::array<float*, 2> channels{};
  for (int ch = 0; ch < numChannels; ++ch)
    channels[static_cast<std::size_t>(ch)] = buffer.getWritePointer(ch);

  dsp_.process(channels.data(), static_cast<std::size_t>(numChannels),
    static_cast<std::size_t>(buffer.getNumSamples()));
  updateMeters(buffer);
}

void FreshAeroAudioProcessor::updateParameters() noexcept {
  if (presenceParam_ != nullptr)
    dsp_.setPresence(presenceParam_->load(std::memory_order_relaxed) / 100.0F);
  if (airParam_ != nullptr)
    dsp_.setAir(airParam_->load(std::memory_order_relaxed) / 100.0F);
  if (linkBandsParam_ != nullptr)
    dsp_.setLinkBands(linkBandsParam_->load(std::memory_order_relaxed) >= 0.5F);
  if (outputTrimDbParam_ != nullptr)
    dsp_.setOutputTrimDb(outputTrimDbParam_->load(std::memory_order_relaxed));
  if (bypassParam_ != nullptr)
    dsp_.setBypass(bypassParam_->load(std::memory_order_relaxed) >= 0.5F);
}

void FreshAeroAudioProcessor::updateMeters(const juce::AudioBuffer<float>& buffer) noexcept {
  auto peak = 0.0F;
  double sumSquares = 0.0;
  for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
    peak = std::max(peak, buffer.getMagnitude(channel, 0, buffer.getNumSamples()));
    const auto* samples = buffer.getReadPointer(channel);
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
      const auto val = samples[sample];
      if (std::isfinite(val))
        sumSquares += static_cast<double>(val) * val;
    }
  }
  const auto sampleCount = buffer.getNumChannels() * buffer.getNumSamples();
  auto rms = sampleCount > 0 ? static_cast<float>(std::sqrt(sumSquares / sampleCount)) : 0.0F;
  if (!std::isfinite(peak)) peak = 0.0F;
  if (!std::isfinite(rms)) rms = 0.0F;

  peak_.store(peak, std::memory_order_relaxed);
  rms_.store(rms, std::memory_order_relaxed);
  if (peak >= 1.0F) clipped_.store(true, std::memory_order_relaxed);
}

void FreshAeroAudioProcessor::setCurrentProgram(int index) {
  if (index < 0 || index >= static_cast<int>(kPresets.size())) return;
  currentProgram_ = index;
  const auto& p = kPresets[static_cast<std::size_t>(index)];
  if (auto* param = state.getParameter("presenceAmount"))
    param->setValueNotifyingHost(param->convertTo0to1(p.presence));
  if (auto* param = state.getParameter("airAmount"))
    param->setValueNotifyingHost(param->convertTo0to1(p.air));
  if (auto* param = state.getParameter("outputTrimDb"))
    param->setValueNotifyingHost(param->convertTo0to1(p.outputTrimDb));
  if (auto* param = state.getParameter("linkBands"))
    param->setValueNotifyingHost(param->convertTo0to1(p.linkBands ? 1.0F : 0.0F));
}

const juce::String FreshAeroAudioProcessor::getProgramName(int index) {
  if (index >= 0 && index < static_cast<int>(kPresets.size()))
    return kPresets[static_cast<std::size_t>(index)].name;
  return {};
}

juce::AudioProcessorEditor* FreshAeroAudioProcessor::createEditor() {
  return new FreshAeroEditor(*this);
}

void FreshAeroAudioProcessor::getStateInformation(juce::MemoryBlock& destination) {
  auto stateCopy = state.copyState();
  stateCopy.setProperty("currentProgram", currentProgram_, nullptr);
  if (auto xml = stateCopy.createXml()) copyXmlToBinary(*xml, destination);
}

void FreshAeroAudioProcessor::setStateInformation(const void* data, int size) {
  if (auto xml = getXmlFromBinary(data, size)) {
    auto vt = juce::ValueTree::fromXml(*xml);
    if (vt.isValid()) {
      state.replaceState(vt);
      if (vt.hasProperty("currentProgram"))
        currentProgram_ = static_cast<int>(vt.getProperty("currentProgram"));
    }
  }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
  return new FreshAeroAudioProcessor();
}
