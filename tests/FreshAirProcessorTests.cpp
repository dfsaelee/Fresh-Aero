#include "dsp/FreshAirProcessor.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <numbers>
#include <numeric>
#include <vector>

namespace {
int failures{};
void expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    ++failures;
  }
}

double measuredGain(double frequency, double presence, double air, double amplitude = 0.1,
    double outputTrimDb = 0.0, bool bypass = false, bool link = false) {
  constexpr double sampleRate = 48000.0;
  constexpr std::size_t size = 48000;
  g3x::FreshAirProcessor processor;
  processor.prepare(sampleRate, 1, 0.0);
  processor.setPresence(presence);
  processor.setAir(air);
  processor.setOutputTrimDb(outputTrimDb);
  processor.setBypass(bypass);
  processor.setLinkBands(link);
  std::vector<float> signal(size);
  for (std::size_t sample = 0; sample < size; ++sample)
    signal[sample] = static_cast<float>(amplitude * std::sin(
      2.0 * std::numbers::pi * frequency * static_cast<double>(sample) / sampleRate));
  const auto begin = signal.begin() + static_cast<std::ptrdiff_t>(size / 2);
  const auto inputRms = std::sqrt(std::inner_product(begin, signal.end(), begin, 0.0) / (size / 2));
  float* channels[]{signal.data()};
  processor.process(channels, 1, signal.size());
  const auto outputRms = std::sqrt(std::inner_product(begin, signal.end(), begin, 0.0) / (size / 2));
  return 20.0 * std::log10(outputRms / inputRms);
}
}

int main() {
  auto previousPresence = g3x::mapPresenceCurve(0.0);
  auto previousAir = g3x::mapAirCurve(0.0);
  for (int step = 1; step <= 100; ++step) {
    const auto value = static_cast<double>(step) / 100.0;
    const auto presence = g3x::mapPresenceCurve(value);
    const auto air = g3x::mapAirCurve(value);
    expect(presence.frequencyHz >= previousPresence.frequencyHz, "presence frequency must be monotonic");
    expect(presence.gainDb >= previousPresence.gainDb, "presence gain must be monotonic");
    expect(air.frequencyHz >= previousAir.frequencyHz, "air frequency must be monotonic");
    expect(air.gainDb >= previousAir.gainDb, "air gain must be monotonic");
    previousPresence = presence;
    previousAir = air;
  }

  for (const auto sampleRate : {44100.0, 48000.0, 96000.0, 192000.0}) {
    g3x::FreshAirProcessor processor;
    processor.prepare(sampleRate, 2, 0.0);
    processor.setPresence(1.0);
    processor.setAir(1.0);
    std::vector<float> left(4096), right(4096);
    left[0] = right[0] = 1.0F;
    float* channels[]{left.data(), right.data()};
    processor.process(channels, 2, left.size());
    expect(std::all_of(left.begin(), left.end(), [](float value) { return std::isfinite(value); }),
      "impulse response must remain finite");
    expect(left == right, "stereo channels must remain identical");
  }

  g3x::FreshAirProcessor neutral;
  neutral.prepare(48000.0, 1, 0.0);
  std::vector<float> signal{0.25F, -0.5F, 0.75F, 0.0F};
  const auto original = signal;
  float* mono[]{signal.data()};
  neutral.process(mono, 1, signal.size());
  for (std::size_t sample = 0; sample < signal.size(); ++sample)
    expect(std::abs(signal[sample] - original[sample]) < 1.0e-6F,
      "zero presence and air must be neutral");

  neutral.setPresence(std::numeric_limits<double>::quiet_NaN());
  neutral.setAir(std::numeric_limits<double>::infinity());
  expect(neutral.presence() == 0.0 && neutral.air() == 0.0,
    "non-finite automation must fall back to neutral");

  neutral.setPresence(2.0);
  neutral.setAir(-1.0);
  expect(neutral.presence() == 1.0 && neutral.air() == 0.0,
    "automation values must be clamped to the public range");

  signal.assign(64, std::numeric_limits<float>::max());
  mono[0] = signal.data();
  neutral.setAir(1.0);
  neutral.process(mono, 1, signal.size());
  std::fill(signal.begin(), signal.end(), 0.1F);
  neutral.process(mono, 1, signal.size());
  expect(std::all_of(signal.begin(), signal.end(), [](float value) { return std::isfinite(value); }),
    "processor must recover after finite overflow");

  expect(measuredGain(100.0, 1.0, 1.0) < 0.5, "low frequencies must remain stable");
  const auto presenceGain = measuredGain(4500.0, 1.0, 0.0);
  expect(presenceGain > 5.0 && presenceGain < 6.5, "presence must target the mid-high region");
  expect(measuredGain(4500.0, 0.0, 1.0) < 1.5, "air must not replace the presence band");
  const auto airGain = measuredGain(18000.0, 0.0, 1.0);
  expect(airGain > 7.0 && airGain < 10.5, "air must boost the upper spectrum");

  const auto quietAirGain = measuredGain(18000.0, 0.0, 1.0, 0.02);
  const auto loudAirGain = measuredGain(18000.0, 0.0, 1.0, 0.8);
  expect(quietAirGain > loudAirGain + 1.0,
    "dynamic air detector must reduce boost for strong high-frequency energy");
  const auto quietPresenceGain = measuredGain(4500.0, 1.0, 0.0, 0.02);
  const auto loudPresenceGain = measuredGain(4500.0, 1.0, 0.0, 0.8);
  expect(quietPresenceGain > loudPresenceGain + 0.3,
    "dynamic presence detector must reduce boost for strong mid-high energy");

  const auto trimmedGain = measuredGain(1000.0, 0.0, 0.0, 0.1, -6.0);
  expect(std::abs(trimmedGain + 6.0) < 0.15, "output trim must match its decibel value");
  const auto bypassedGain = measuredGain(18000.0, 1.0, 1.0, 0.1, -6.0, true);
  expect(std::abs(bypassedGain) < 0.05, "bypass must exclude processing and output trim");
  const auto linkedGain = measuredGain(18000.0, 1.0, 1.0, 0.8, 0.0, false, true);
  expect(std::isfinite(linkedGain), "linked band dynamics must remain finite");

  g3x::FreshAirProcessor defensive;
  defensive.prepare(std::numeric_limits<double>::infinity(), 2, -1.0);
  defensive.process(nullptr, 2, 0);
  float* nullChannels[]{nullptr, nullptr};
  defensive.process(nullChannels, 2, 16);

  if (failures == 0) std::cout << "All G3X Fresh Air DSP tests passed\n";
  return failures == 0 ? 0 : 1;
}
