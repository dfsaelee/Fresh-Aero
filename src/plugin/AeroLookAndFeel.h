#pragma once

#include "plugin/AeroPainters.h"

#include <map>

class AeroLookAndFeel final : public juce::LookAndFeel_V4 {
public:
  AeroLookAndFeel();

  void setBypassed(bool bypassed) noexcept { bypassed_ = bypassed; }

  void drawRotarySlider(juce::Graphics&, int x, int y, int width, int height,
    float position, float startAngle, float endAngle, juce::Slider&) override;
  void drawComboBox(juce::Graphics&, int width, int height, bool isButtonDown,
    int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox&) override;
  juce::Font getComboBoxFont(juce::ComboBox&) override;
  void positionComboBoxText(juce::ComboBox&, juce::Label&) override;
  void drawPopupMenuBackground(juce::Graphics&, int width, int height) override;
  void drawPopupMenuItem(juce::Graphics&, const juce::Rectangle<int>& area,
    bool isSeparator, bool isActive, bool isHighlighted, bool isTicked, bool hasSubMenu,
    const juce::String& text, const juce::String& shortcutKeyText,
    const juce::Drawable* icon, const juce::Colour* textColour) override;
  void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour&,
    bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
  void drawButtonText(juce::Graphics&, juce::TextButton&, bool shouldDrawButtonAsHighlighted,
    bool shouldDrawButtonAsDown) override;
  void drawToggleButton(juce::Graphics&, juce::ToggleButton&,
    bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
  void drawLabel(juce::Graphics&, juce::Label&) override;

private:
  [[nodiscard]] const juce::Image& knobBodyForDiameter(int diameter) const;
  [[nodiscard]] aero::PillState pillState(const juce::Component&, bool highlighted, bool pressed) const;

  mutable std::map<int, juce::Image> knobBodies_;
  bool bypassed_{false};
};
