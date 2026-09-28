#pragma once

#include "plugin/AeroPainters.h"

#include <vector>

namespace aero {

class AeroScene {
public:
  void rebuild(juce::Rectangle<int> bounds);
  void drawBackground(juce::Graphics&) const;
  void drawDynamicDecorations(juce::Graphics&, float phase) const;
  [[nodiscard]] const juce::Image& frostedImage() const noexcept { return frostedImage_; }
  void repaintDynamicRegions(juce::Component&, float oldPhase, float newPhase) const;
  void logContrastForLabel(const juce::String& labelName, juce::Rectangle<int> labelBounds) const;

private:
  struct BubbleSeed {
    float xFraction{};
    float yFraction{};
    float radius{};
    float speed{};
    float alpha{};
  };

  [[nodiscard]] juce::Point<float> bubblePosition(const BubbleSeed&, float phase) const;
  [[nodiscard]] juce::Rectangle<int> shimmerBounds(float xFraction) const;
  void drawStaticScene(juce::Graphics&);

  juce::Rectangle<int> bounds_;
  juce::Image sceneImage_;
  juce::Image frostedImage_;
  std::vector<BubbleSeed> bubbles_;
};

} // namespace aero
