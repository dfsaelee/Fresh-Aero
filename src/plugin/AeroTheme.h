#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace aero {

inline constexpr float referenceWidth = 680.0F;
inline constexpr float cardRadius = 22.0F;
inline constexpr float grooveWidth = 8.0F;
inline constexpr float outerBorderWidth = 2.0F;
inline constexpr float innerBorderWidth = 1.0F;

inline const juce::Colour skyTop{0xff3fa9f5};
inline const juce::Colour skyHorizon{0xffd6f1ff};
inline const juce::Colour glassTint{0x42ffffff};
inline const juce::Colour glassEdge{0xd9ffffff};
inline const juce::Colour hillFar{0xff6dbe5a};
inline const juce::Colour hillNear{0xff3e9b2f};
inline const juce::Colour limeGlow{0xffb6f03c};
inline const juce::Colour cyanGlow{0xff5fe0ff};
inline const juce::Colour textPrimary{0xffffffff};
inline const juce::Colour textShadow{0x990b3d6b};
inline const juce::Colour darkBlue{0xff195b84};
inline const juce::Colour oceanTeal{0xff1c6e88};
inline const juce::Colour oceanDeep{0xff124e6d};
inline const juce::Colour glassOcean{0x99185f78};
inline const juce::Colour outerOutline{0xff15557f};
inline const juce::Colour bevelBlue{0xff74c8f2};
inline const juce::Colour rimBlue{0xff3a9ed3};
inline const juce::Colour knobLight{0xffeaf7ff};
inline const juce::Colour knobRim{0xff68bfe9};
inline const juce::Colour pillTop{0xffdff5ff};
inline const juce::Colour pillBottom{0xff69bee9};
inline const juce::Colour pillEdge{0xff3288bf};
inline const juce::Colour clipCoral{0xffff5c73};
inline const juce::Colour disabledBlue{0xff6f93aa};
inline const juce::Colour shadow{0x40000000};

[[nodiscard]] inline float uiScaleForWidth(int width) noexcept {
  return static_cast<float>(width) / referenceWidth;
}

[[nodiscard]] inline float scaled(float value, float uiScale) noexcept {
  return value * uiScale;
}

[[nodiscard]] inline juce::Font titleFont(float uiScale) {
  return juce::FontOptions{scaled(21.0F, uiScale), juce::Font::bold};
}

[[nodiscard]] inline juce::Font labelFont(float uiScale) {
  return juce::FontOptions{scaled(11.0F, uiScale), juce::Font::bold};
}

} // namespace aero
