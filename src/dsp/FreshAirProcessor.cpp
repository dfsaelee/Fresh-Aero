#include "dsp/FreshAirProcessor.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>

namespace g3x {
PresenceCurve mapPresenceCurve(double amount) noexcept {
  const auto value = std::isfinite(amount) ? std::clamp(amount, 0.0, 1.0) : 0.0;
  return {3200.0 + 1800.0 * value, 6.0 * std::pow(value, 1.45), 0.62};
}

AirCurve mapAirCurve(double amount) noexcept {
  const auto value = std::isfinite(amount) ? std::clamp(amount, 0.0, 1.0) : 0.0;
  return {8500.0 + 2500.0 * value, 10.0 * std::pow(value, 1.55), 0.72};
}

void FreshAirProcessor::prepare(double sampleRate, std::size_t channels, double rampSeconds) {
  sampleRate_ = std::isfinite(sampleRate) ? std::clamp(sampleRate, 8000.0, 768000.0) : 44100.0;
  const auto ramp = std::isfinite(rampSeconds) ? std::clamp(rampSeconds, 0.0, 1.0) : 0.02;
  rampSamples_ = std::max<std::size_t>(1, static_cast<std::size_t>(sampleRate_ * ramp));
  states_.assign(channels, {});
  presence_ = {targetPresence_, targetPresence_, 0.0, 0};
  air_ = {targetAir_, targetAir_, 0.0, 0};
  outputTrimDb_ = {targetOutputTrimDb_, targetOutputTrimDb_, 0.0, 0};
  wetMix_.current = wetMix_.target;
  wetMix_.step = 0.0;
  wetMix_.remaining = 0;
  presenceEnvelope_ = 0.0;
  airEnvelope_ = 0.0;
}

void FreshAirProcessor::reset() noexcept {
  for (auto& state : states_) state = {};
  presenceEnvelope_ = 0.0;
  airEnvelope_ = 0.0;
}

void FreshAirProcessor::setPresence(double amount) noexcept {
  targetPresence_ = std::isfinite(amount) ? std::clamp(amount, 0.0, 1.0) : 0.0;
  setSmoothed(presence_, targetPresence_);
}

void FreshAirProcessor::setAir(double amount) noexcept {
  targetAir_ = std::isfinite(amount) ? std::clamp(amount, 0.0, 1.0) : 0.0;
  setSmoothed(air_, targetAir_);
}

void FreshAirProcessor::setOutputTrimDb(double decibels) noexcept {
  targetOutputTrimDb_ = std::isfinite(decibels) ? std::clamp(decibels, -12.0, 3.0) : 0.0;
  setSmoothed(outputTrimDb_, targetOutputTrimDb_);
}

void FreshAirProcessor::setBypass(bool bypassed) noexcept {
  setSmoothed(wetMix_, bypassed ? 0.0 : 1.0);
}

void FreshAirProcessor::setSmoothed(SmoothedValue& value, double next) noexcept {
  if (next == value.target) return;
  value.target = next;
  value.remaining = rampSamples_;
  value.step = (value.target - value.current) / static_cast<double>(value.remaining);
}

double FreshAirProcessor::advance(SmoothedValue& value) noexcept {
  if (value.remaining > 0) {
    value.current += value.step;
    if (--value.remaining == 0) value.current = value.target;
  }
  return value.current;
}

FreshAirProcessor::Coefficients FreshAirProcessor::presenceCoefficients(double amount) const noexcept {
  const auto curve = mapPresenceCurve(amount);
  const auto frequency = std::min(curve.frequencyHz, sampleRate_ * 0.42);
  const auto a = std::pow(10.0, curve.gainDb / 40.0);
  const auto omega = 2.0 * std::numbers::pi * frequency / sampleRate_;
  const auto alpha = std::sin(omega) / (2.0 * curve.q);
  const auto cosine = std::cos(omega);
  const auto a0 = 1.0 + alpha / a;
  return {
    (1.0 + alpha * a) / a0,
    (-2.0 * cosine) / a0,
    (1.0 - alpha * a) / a0,
    (-2.0 * cosine) / a0,
    (1.0 - alpha / a) / a0
  };
}

FreshAirProcessor::Coefficients FreshAirProcessor::airCoefficients(double amount) const noexcept {
  const auto curve = mapAirCurve(amount);
  const auto frequency = std::min(curve.frequencyHz, sampleRate_ * 0.42);
  const auto a = std::pow(10.0, curve.gainDb / 40.0);
  const auto omega = 2.0 * std::numbers::pi * frequency / sampleRate_;
  const auto cosine = std::cos(omega);
  const auto sine = std::sin(omega);
  const auto sqrtA = std::sqrt(a);
  const auto alpha = sine * 0.5
    * std::sqrt((a + 1.0 / a) * (1.0 / curve.shelfSlope - 1.0) + 2.0);
  const auto a0 = (a + 1.0) - (a - 1.0) * cosine + 2.0 * sqrtA * alpha;
  return {
    a * ((a + 1.0) + (a - 1.0) * cosine + 2.0 * sqrtA * alpha) / a0,
    -2.0 * a * ((a - 1.0) + (a + 1.0) * cosine) / a0,
    a * ((a + 1.0) + (a - 1.0) * cosine - 2.0 * sqrtA * alpha) / a0,
    2.0 * ((a - 1.0) - (a + 1.0) * cosine) / a0,
    ((a + 1.0) - (a - 1.0) * cosine - 2.0 * sqrtA * alpha) / a0
  };
}

double FreshAirProcessor::runFilter(double input, const Coefficients& coefficients,
    FilterState& state) noexcept {
  const auto output = coefficients.b0 * input + state.z1;
  state.z1 = coefficients.b1 * input - coefficients.a1 * output + state.z2;
  state.z2 = coefficients.b2 * input - coefficients.a2 * output;
  if (!std::isfinite(output) || !std::isfinite(state.z1) || !std::isfinite(state.z2)) {
    state = {};
    return 0.0;
  }
  return output;
}

double FreshAirProcessor::detectorCoefficient(double frequencyHz) const noexcept {
  return 1.0 - std::exp(-2.0 * std::numbers::pi * frequencyHz / sampleRate_);
}

double FreshAirProcessor::followEnvelope(double input, double& envelope,
    double attackSeconds, double releaseSeconds) const noexcept {
  const auto time = input > envelope ? attackSeconds : releaseSeconds;
  const auto coefficient = std::exp(-1.0 / (std::max(0.0001, time) * sampleRate_));
  envelope = coefficient * envelope + (1.0 - coefficient) * input;
  return envelope;
}

void FreshAirProcessor::process(float* const* channels, std::size_t channelCount,
    std::size_t sampleCount) noexcept {
  channelCount = std::min(channelCount, states_.size());
  const auto presenceDetectorCoefficient = detectorCoefficient(1800.0);
  const auto airDetectorCoefficient = detectorCoefficient(7500.0);
  for (std::size_t sample = 0; sample < sampleCount; ++sample) {
    auto presenceEnergy = 0.0;
    auto airEnergy = 0.0;
    for (std::size_t channel = 0; channel < channelCount; ++channel) {
      if (channels == nullptr || channels[channel] == nullptr) continue;
      const auto input = std::isfinite(channels[channel][sample])
        ? static_cast<double>(channels[channel][sample]) : 0.0;
      auto& state = states_[channel];
      state.presenceDetectorLowpass += presenceDetectorCoefficient
        * (input - state.presenceDetectorLowpass);
      state.airDetectorLowpass += airDetectorCoefficient
        * (input - state.airDetectorLowpass);
      presenceEnergy = std::max(presenceEnergy, std::abs(input - state.presenceDetectorLowpass));
      airEnergy = std::max(airEnergy, std::abs(input - state.airDetectorLowpass));
    }
    const auto presenceEnvelope = followEnvelope(presenceEnergy, presenceEnvelope_, 0.006, 0.12);
    const auto airEnvelope = followEnvelope(airEnergy, airEnvelope_, 0.003, 0.18);
    auto presenceReduction = 0.30 * std::clamp((presenceEnvelope - 0.08) / 0.42, 0.0, 1.0);
    auto airReduction = 0.45 * std::clamp((airEnvelope - 0.045) / 0.30, 0.0, 1.0);
    if (linkBands_) presenceReduction = airReduction = std::max(presenceReduction, airReduction);

    const auto presenceAmount = advance(presence_) * (1.0 - presenceReduction);
    const auto airAmount = advance(air_) * (1.0 - airReduction);
    const auto presenceCoefficientsForSample = presenceCoefficients(presenceAmount);
    const auto airCoefficientsForSample = airCoefficients(airAmount);
    const auto trim = std::pow(10.0, advance(outputTrimDb_) / 20.0);
    const auto wet = advance(wetMix_);
    for (std::size_t channel = 0; channel < channelCount; ++channel) {
      if (channels == nullptr || channels[channel] == nullptr) continue;
      const auto input = std::isfinite(channels[channel][sample])
        ? static_cast<double>(channels[channel][sample]) : 0.0;
      auto& state = states_[channel];
      const auto presenceOutput = runFilter(input, presenceCoefficientsForSample, state.presence);
      const auto processed = runFilter(presenceOutput, airCoefficientsForSample, state.air) * trim;
      const auto output = input + wet * (processed - input);
      if (std::abs(output) <= static_cast<double>(std::numeric_limits<float>::max())) {
        channels[channel][sample] = static_cast<float>(output);
      } else {
        state = {};
        channels[channel][sample] = 0.0F;
      }
    }
  }
}
}
