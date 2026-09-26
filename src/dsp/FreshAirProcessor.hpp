#pragma once

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
  void prepare(double sampleRate, std::size_t channels, double rampSeconds = 0.02);
  void reset() noexcept;
  void setPresence(double normalizedAmount) noexcept;
  void setAir(double normalizedAmount) noexcept;
  void setOutputTrimDb(double decibels) noexcept;
  void setLinkBands(bool linked) noexcept { linkBands_ = linked; }
  void setBypass(bool bypassed) noexcept;
  [[nodiscard]] double presence() const noexcept { return targetPresence_; }
  [[nodiscard]] double air() const noexcept { return targetAir_; }
  [[nodiscard]] double outputTrimDb() const noexcept { return targetOutputTrimDb_; }
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
  void setSmoothed(SmoothedValue& value, double next) noexcept;
  static double runFilter(double input, const Coefficients& coefficients,
    FilterState& state) noexcept;
  [[nodiscard]] double detectorCoefficient(double frequencyHz) const noexcept;
  double followEnvelope(double input, double& envelope, double attackSeconds,
    double releaseSeconds) const noexcept;

  double sampleRate_{44100.0};
  std::size_t rampSamples_{882};
  double targetPresence_{};
  double targetAir_{};
  double targetOutputTrimDb_{};
  SmoothedValue presence_{};
  SmoothedValue air_{};
  SmoothedValue outputTrimDb_{};
  SmoothedValue wetMix_{1.0, 1.0, 0.0, 0};
  double presenceEnvelope_{};
  double airEnvelope_{};
  bool linkBands_{};
  std::vector<ChannelState> states_;
};
}
