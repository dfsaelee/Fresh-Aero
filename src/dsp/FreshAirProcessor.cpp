#include "dsp/FreshAirProcessor.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>

namespace g3x {
namespace {
[[nodiscard]] inline double sanitize(double value) noexcept {
  if (!std::isfinite(value) || std::abs(value) < 1.0e-15) {
    return 0.0;
  }
  return value;
}
} // namespace

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
  states_.assign(std::max<std::size_t>(channels, 2), {});

  const auto p = targetPresence_.load(std::memory_order_relaxed);
  const auto a = targetAir_.load(std::memory_order_relaxed);
  const auto t = targetOutputTrimDb_.load(std::memory_order_relaxed);
  const auto b = targetBypass_.load(std::memory_order_relaxed);

  presence_ = {p, p, 0.0, 0};
  air_ = {a, a, 0.0, 0};
  outputTrimDb_ = {t, t, 0.0, 0};
  wetMix_ = {b ? 0.0 : 1.0, b ? 0.0 : 1.0, 0.0, 0};

  presenceEnvelope_ = 0.0;
  airEnvelope_ = 0.0;
  presenceReduction_.store(0.0, std::memory_order_relaxed);
  airReduction_.store(0.0, std::memory_order_relaxed);
  outputPeak_.store(0.0F, std::memory_order_relaxed);
  outputRms_.store(0.0F, std::memory_order_relaxed);
  outputClipped_.store(false, std::memory_order_relaxed);
}

void FreshAirProcessor::reset() noexcept {
  for (auto& state : states_) state = {};
  presenceEnvelope_ = 0.0;
  airEnvelope_ = 0.0;
  presenceReduction_.store(0.0, std::memory_order_relaxed);
  airReduction_.store(0.0, std::memory_order_relaxed);

  presence_.current = presence_.target;
  presence_.step = 0.0;
  presence_.remaining = 0;

  air_.current = air_.target;
  air_.step = 0.0;
  air_.remaining = 0;

  outputTrimDb_.current = outputTrimDb_.target;
  outputTrimDb_.step = 0.0;
  outputTrimDb_.remaining = 0;

  wetMix_.current = wetMix_.target;
  wetMix_.step = 0.0;
  wetMix_.remaining = 0;

  outputPeak_.store(0.0F, std::memory_order_relaxed);
  outputRms_.store(0.0F, std::memory_order_relaxed);
  outputClipped_.store(false, std::memory_order_relaxed);
}

void FreshAirProcessor::setPresence(double amount) noexcept {
  const auto clamped = std::isfinite(amount) ? std::clamp(amount, 0.0, 1.0) : 0.0;
  targetPresence_.store(clamped, std::memory_order_release);
  if (rampSamples_ <= 1) {
    presence_.target = clamped;
    presence_.current = clamped;
    presence_.step = 0.0;
    presence_.remaining = 0;
  }
}

void FreshAirProcessor::setAir(double amount) noexcept {
  const auto clamped = std::isfinite(amount) ? std::clamp(amount, 0.0, 1.0) : 0.0;
  targetAir_.store(clamped, std::memory_order_release);
  if (rampSamples_ <= 1) {
    air_.target = clamped;
    air_.current = clamped;
    air_.step = 0.0;
    air_.remaining = 0;
  }
}

void FreshAirProcessor::setOutputTrimDb(double decibels) noexcept {
  const auto clamped = std::isfinite(decibels) ? std::clamp(decibels, -12.0, 3.0) : 0.0;
  targetOutputTrimDb_.store(clamped, std::memory_order_release);
  if (rampSamples_ <= 1) {
    outputTrimDb_.target = clamped;
    outputTrimDb_.current = clamped;
    outputTrimDb_.step = 0.0;
    outputTrimDb_.remaining = 0;
  }
}

void FreshAirProcessor::setLinkBands(bool linked) noexcept {
  linkBands_.store(linked, std::memory_order_release);
}

void FreshAirProcessor::setBypass(bool bypassed) noexcept {
  targetBypass_.store(bypassed, std::memory_order_release);
  const auto targetWet = bypassed ? 0.0 : 1.0;
  if (rampSamples_ <= 1) {
    wetMix_.target = targetWet;
    wetMix_.current = targetWet;
    wetMix_.step = 0.0;
    wetMix_.remaining = 0;
  }
}

void FreshAirProcessor::updateSmoothed(SmoothedValue& value, double next) noexcept {
  if (std::abs(next - value.target) < 1.0e-9) return;
  value.target = next;
  if (rampSamples_ <= 1) {
    value.current = next;
    value.step = 0.0;
    value.remaining = 0;
  } else {
    value.remaining = rampSamples_;
    value.step = (value.target - value.current) / static_cast<double>(value.remaining);
  }
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
  const auto frequency = std::clamp(curve.frequencyHz, 10.0, sampleRate_ * 0.42);
  const auto a = std::pow(10.0, curve.gainDb / 40.0);
  const auto omega = 2.0 * std::numbers::pi * frequency / sampleRate_;
  const auto alpha = std::sin(omega) / (2.0 * std::max(0.001, curve.q));
  const auto cosine = std::cos(omega);
  const auto a0 = 1.0 + alpha / a;
  if (std::abs(a0) < 1.0e-12) return {1.0, 0.0, 0.0, 0.0, 0.0};
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
  const auto frequency = std::clamp(curve.frequencyHz, 10.0, sampleRate_ * 0.42);
  const auto a = std::pow(10.0, curve.gainDb / 40.0);
  const auto omega = 2.0 * std::numbers::pi * frequency / sampleRate_;
  const auto cosine = std::cos(omega);
  const auto sine = std::sin(omega);
  const auto sqrtA = std::sqrt(a);
  const auto slope = std::clamp(curve.shelfSlope, 0.01, 1.0);
  const auto alphaTerm = (a + 1.0 / a) * (1.0 / slope - 1.0) + 2.0;
  const auto alpha = sine * 0.5 * std::sqrt(std::max(0.0, alphaTerm));
  const auto a0 = (a + 1.0) - (a - 1.0) * cosine + 2.0 * sqrtA * alpha;
  if (std::abs(a0) < 1.0e-12) return {1.0, 0.0, 0.0, 0.0, 0.0};
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
  state.z1 = sanitize(coefficients.b1 * input - coefficients.a1 * output + state.z2);
  state.z2 = sanitize(coefficients.b2 * input - coefficients.a2 * output);
  if (!std::isfinite(output) || !std::isfinite(state.z1) || !std::isfinite(state.z2)) {
    state = {};
    return 0.0;
  }
  return sanitize(output);
}

double FreshAirProcessor::detectorCoefficient(double frequencyHz) const noexcept {
  const auto freq = std::clamp(frequencyHz, 10.0, sampleRate_ * 0.45);
  return 1.0 - std::exp(-2.0 * std::numbers::pi * freq / sampleRate_);
}

double FreshAirProcessor::followEnvelope(double input, double& envelope,
    double attackSeconds, double releaseSeconds) const noexcept {
  const auto time = input > envelope ? attackSeconds : releaseSeconds;
  const auto coefficient = std::exp(-1.0 / (std::max(0.0001, time) * sampleRate_));
  envelope = sanitize(coefficient * envelope + (1.0 - coefficient) * input);
  return envelope;
}

void FreshAirProcessor::process(float* const* channels, std::size_t channelCount,
    std::size_t sampleCount) noexcept {
  if (channels == nullptr || channelCount == 0 || sampleCount == 0) return;
  channelCount = std::min(channelCount, states_.size());

  updateSmoothed(presence_, targetPresence_.load(std::memory_order_relaxed));
  updateSmoothed(air_, targetAir_.load(std::memory_order_relaxed));
  updateSmoothed(outputTrimDb_, targetOutputTrimDb_.load(std::memory_order_relaxed));
  updateSmoothed(wetMix_, targetBypass_.load(std::memory_order_relaxed) ? 0.0 : 1.0);

  const auto presenceDetectorCoefficient = detectorCoefficient(1800.0);
  const auto airDetectorCoefficient = detectorCoefficient(7500.0);
  const bool linkBands = linkBands_.load(std::memory_order_relaxed);

  float blockPeak = 0.0F;
  double sumSquares = 0.0;
  std::size_t validSampleCount = 0;
  double lastPresenceReduction = 0.0;
  double lastAirReduction = 0.0;

  for (std::size_t sample = 0; sample < sampleCount; ++sample) {
    auto presenceEnergy = 0.0;
    auto airEnergy = 0.0;
    for (std::size_t channel = 0; channel < channelCount; ++channel) {
      if (channels[channel] == nullptr) continue;
      const auto input = sanitize(static_cast<double>(channels[channel][sample]));
      auto& state = states_[channel];
      state.presenceDetectorLowpass = sanitize(state.presenceDetectorLowpass
        + presenceDetectorCoefficient * (input - state.presenceDetectorLowpass));
      state.airDetectorLowpass = sanitize(state.airDetectorLowpass
        + airDetectorCoefficient * (input - state.airDetectorLowpass));
      presenceEnergy = std::max(presenceEnergy, std::abs(input - state.presenceDetectorLowpass));
      airEnergy = std::max(airEnergy, std::abs(input - state.airDetectorLowpass));
    }
    const auto presenceEnvelope = followEnvelope(presenceEnergy, presenceEnvelope_, 0.006, 0.12);
    const auto airEnvelope = followEnvelope(airEnergy, airEnvelope_, 0.003, 0.18);
    auto presenceReduction = 0.30 * std::clamp((presenceEnvelope - 0.08) / 0.42, 0.0, 1.0);
    auto airReduction = 0.45 * std::clamp((airEnvelope - 0.045) / 0.30, 0.0, 1.0);
    if (linkBands) presenceReduction = airReduction = std::max(presenceReduction, airReduction);
    lastPresenceReduction = presenceReduction;
    lastAirReduction = airReduction;

    const auto presenceAmount = advance(presence_) * (1.0 - presenceReduction);
    const auto airAmount = advance(air_) * (1.0 - airReduction);
    const auto presenceCoefficientsForSample = presenceCoefficients(presenceAmount);
    const auto airCoefficientsForSample = airCoefficients(airAmount);
    const auto trim = std::pow(10.0, advance(outputTrimDb_) / 20.0);
    const auto wet = advance(wetMix_);

    for (std::size_t channel = 0; channel < channelCount; ++channel) {
      if (channels[channel] == nullptr) continue;
      const auto input = sanitize(static_cast<double>(channels[channel][sample]));
      auto& state = states_[channel];
      const auto presenceOutput = runFilter(input, presenceCoefficientsForSample, state.presence);
      const auto processed = runFilter(presenceOutput, airCoefficientsForSample, state.air) * trim;
      const auto output = input + wet * (processed - input);

      if (std::isfinite(output) && std::abs(output) <= static_cast<double>(std::numeric_limits<float>::max())) {
        const auto outFloat = static_cast<float>(sanitize(output));
        channels[channel][sample] = outFloat;
        const auto absVal = std::abs(outFloat);
        if (absVal > blockPeak) blockPeak = absVal;
        sumSquares += static_cast<double>(outFloat) * static_cast<double>(outFloat);
        ++validSampleCount;
      } else {
        state = {};
        channels[channel][sample] = 0.0F;
      }
    }
  }

  presenceReduction_.store(lastPresenceReduction, std::memory_order_relaxed);
  airReduction_.store(lastAirReduction, std::memory_order_relaxed);

  if (validSampleCount > 0) {
    const auto rms = static_cast<float>(std::sqrt(sumSquares / static_cast<double>(validSampleCount)));
    outputPeak_.store(blockPeak, std::memory_order_relaxed);
    outputRms_.store(rms, std::memory_order_relaxed);
    if (blockPeak >= 1.0F) outputClipped_.store(true, std::memory_order_relaxed);
  }
}
} // namespace g3x
