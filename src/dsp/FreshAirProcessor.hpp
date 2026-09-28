#pragma once

#include <atomic>
#include <cstddef>
#include <vector>

namespace g3x {
struct PresenceCurve {
  double frequencyHz{};
  double gainDb{};
  double q{};
};

struct AirCurve {
  double frequencyHz{};
  double gainDb{};
  double shelfSlope{};
};

[[nodiscard]] PresenceCurve mapPresenceCurve(double amount) noexcept;
[[nodiscard]] AirCurve mapAirCurve(double amount) noexcept;

class FreshAirProcessor {
public:
  FreshAirProcessor() = default;
  ~FreshAirProcessor() = default;

  FreshAirProcessor(const FreshAirProcessor&) = delete;
  FreshAirProcessor& operator=(const FreshAirProcessor&) = delete;
  FreshAirProcessor(FreshAirProcessor&&) noexcept = default;
  FreshAirProcessor& operator=(FreshAirProcessor&&) noexcept = default;

  void prepare(double sampleRate, std::size_t channels, double rampSeconds = 0.02);
  void reset() noexcept;
  void setPresence(double normalizedAmount) noexcept;
  void setAir(double normalizedAmount) noexcept;
  void setOutputTrimDb(double decibels) noexcept;
  void setLinkBands(bool linked) noexcept;
  void setBypass(bool bypassed) noexcept;

  [[nodiscard]] double presence() const noexcept { return targetPresence_.load(std::memory_order_relaxed); }
  [[nodiscard]] double air() const noexcept { return targetAir_.load(std::memory_order_relaxed); }
  [[nodiscard]] double outputTrimDb() const noexcept { return targetOutputTrimDb_.load(std::memory_order_relaxed); }
  [[nodiscard]] bool linkBands() const noexcept { return linkBands_.load(std::memory_order_relaxed); }
  [[nodiscard]] bool isBypassed() const noexcept { return targetBypass_.load(std::memory_order_relaxed); }

  [[nodiscard]] float outputPeak() const noexcept { return outputPeak_.load(std::memory_order_relaxed); }
  [[nodiscard]] float outputRms() const noexcept { return outputRms_.load(std::memory_order_relaxed); }
  [[nodiscard]] bool outputClipped() const noexcept { return outputClipped_.load(std::memory_order_relaxed); }
  void clearClipIndicator() noexcept { outputClipped_.store(false, std::memory_order_relaxed); }

  [[nodiscard]] double presenceEnvelope() const noexcept { return presenceEnvelope_; }
  [[nodiscard]] double airEnvelope() const noexcept { return airEnvelope_; }
  [[nodiscard]] double presenceReduction() const noexcept { return presenceReduction_.load(std::memory_order_relaxed); }
  [[nodiscard]] double airReduction() const noexcept { return airReduction_.load(std::memory_order_relaxed); }

  void process(float* const* channels, std::size_t channelCount,
    std::size_t sampleCount) noexcept;

private:
  struct FilterState { double z1{}, z2{}; };
  struct ChannelState {
    FilterState presence, air;
    double presenceDetectorLowpass{}, airDetectorLowpass{};
  };
  struct Coefficients { double b0{1.0}, b1{}, b2{}, a1{}, a2{}; };
  struct SmoothedValue {
    double current{}, target{}, step{};
    std::size_t remaining{};
  };

  [[nodiscard]] Coefficients presenceCoefficients(double amount) const noexcept;
  [[nodiscard]] Coefficients airCoefficients(double amount) const noexcept;
  static double advance(SmoothedValue& value) noexcept;
  void updateSmoothed(SmoothedValue& value, double next) noexcept;
  static double runFilter(double input, const Coefficients& coefficients,
    FilterState& state) noexcept;
  [[nodiscard]] double detectorCoefficient(double frequencyHz) const noexcept;
  double followEnvelope(double input, double& envelope, double attackSeconds,
    double releaseSeconds) const noexcept;

  double sampleRate_{44100.0};
  std::size_t rampSamples_{882};

  std::atomic<double> targetPresence_{0.0};
  std::atomic<double> targetAir_{0.0};
  std::atomic<double> targetOutputTrimDb_{0.0};
  std::atomic<bool> linkBands_{false};
  std::atomic<bool> targetBypass_{false};

  SmoothedValue presence_{};
  SmoothedValue air_{};
  SmoothedValue outputTrimDb_{};
  SmoothedValue wetMix_{1.0, 1.0, 0.0, 0};

  double presenceEnvelope_{};
  double airEnvelope_{};
  std::atomic<double> presenceReduction_{0.0};
  std::atomic<double> airReduction_{0.0};

  std::atomic<float> outputPeak_{0.0F};
  std::atomic<float> outputRms_{0.0F};
  std::atomic<bool> outputClipped_{false};

  std::vector<ChannelState> states_{2};
};
}
