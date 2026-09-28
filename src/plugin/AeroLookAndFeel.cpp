#include "plugin/AeroLookAndFeel.h"

#include <cmath>

namespace {

juce::Colour colourForSlider(const juce::Slider& slider) {
  if (slider.getName() == "Air") return aero::cyanGlow;
  return aero::limeGlow;
}

} // namespace

AeroLookAndFeel::AeroLookAndFeel() {
  setColour(juce::ComboBox::backgroundColourId, aero::pillTop);
  setColour(juce::ComboBox::textColourId, aero::darkBlue);
  setColour(juce::ComboBox::outlineColourId, aero::pillEdge);
  setColour(juce::PopupMenu::backgroundColourId, aero::pillTop);
  setColour(juce::PopupMenu::textColourId, aero::darkBlue);
  setColour(juce::PopupMenu::highlightedBackgroundColourId, aero::cyanGlow.withAlpha(0.35F));
  setColour(juce::PopupMenu::highlightedTextColourId, aero::darkBlue);
}

const juce::Image& AeroLookAndFeel::knobBodyForDiameter(int diameter) const {
  const auto key = juce::jmax(1, diameter);
  if (const auto found = knobBodies_.find(key); found != knobBodies_.end()) return found->second;

  const auto inset = 9;
  auto image = juce::Image{juce::Image::ARGB, key + inset * 2, key + inset * 2, true};
  juce::Graphics graphics{image};
  aero::drawKnobBody(graphics, juce::Rectangle<float>{static_cast<float>(inset), static_cast<float>(inset - 3),
    static_cast<float>(key), static_cast<float>(key)});
  return knobBodies_.emplace(key, std::move(image)).first->second;
}

aero::PillState AeroLookAndFeel::pillState(const juce::Component& component, bool highlighted, bool pressed) const {
  if (!component.isEnabled()) return aero::PillState::disabled;
  if (pressed) return aero::PillState::pressed;
  if (highlighted || component.isMouseOverOrDragging()) return aero::PillState::hover;
  return aero::PillState::normal;
}

void AeroLookAndFeel::drawRotarySlider(juce::Graphics& graphics, int x, int y, int width, int height,
    float position, float startAngle, float endAngle, juce::Slider& slider) {
  const auto root = juce::Rectangle<float>{static_cast<float>(x), static_cast<float>(y),
    static_cast<float>(width), static_cast<float>(height)};
  const auto scale = aero::uiScaleForWidth(width * 3);
  auto dialArea = root.withTrimmedTop(aero::scaled(19.0F, scale)).withTrimmedBottom(aero::scaled(32.0F, scale));
  const auto dialSize = juce::jmax(24.0F, juce::jmin(dialArea.getWidth(), dialArea.getHeight()));
  dialArea = dialArea.withSizeKeepingCentre(dialSize, dialSize);
  const auto centre = dialArea.getCentre();
  const auto radius = dialSize * 0.47F;
  auto colour = colourForSlider(slider);
  if (bypassed_) colour = colour.interpolatedWith(aero::disabledBlue, 0.55F);
  const aero::ArcParams arc{centre, radius, startAngle, endAngle, position};
  aero::drawGroove(graphics, arc);
  aero::drawGlowArc(graphics, arc, colour, bypassed_ ? 0.10F : 0.38F);

  const auto knobDiameter = juce::roundToInt(radius * 1.58F);
  const auto knobBounds = juce::Rectangle<float>{static_cast<float>(knobDiameter), static_cast<float>(knobDiameter)}
    .withCentre(centre);
  const auto& knobImage = knobBodyForDiameter(knobDiameter);
  graphics.drawImageAt(knobImage, juce::roundToInt(knobBounds.getX()) - 9,
    juce::roundToInt(knobBounds.getY()) - 6, false);

  const auto angle = startAngle + position * (endAngle - startAngle);
  const auto indicatorDistance = radius * 0.46F;
  const auto indicatorCentre = juce::Point<float>{centre.x + std::sin(angle) * indicatorDistance,
    centre.y - std::cos(angle) * indicatorDistance};
  const auto indicatorSize = aero::scaled(8.0F, scale);
  const auto indicator = juce::Rectangle<float>{indicatorSize, indicatorSize}.withCentre(indicatorCentre);
  graphics.setColour(aero::textPrimary.withAlpha(0.94F));
  graphics.fillEllipse(indicator.expanded(aero::scaled(1.0F, scale)));
  graphics.setColour(colour.brighter(0.12F));
  graphics.fillEllipse(indicator);
  graphics.setColour(aero::textPrimary.withAlpha(0.82F));
  graphics.fillEllipse(indicator.withSizeKeepingCentre(indicatorSize * 0.36F, indicatorSize * 0.24F)
    .translated(-indicatorSize * 0.12F, -indicatorSize * 0.16F));
  if (slider.hasKeyboardFocus(true)) {
    graphics.setColour(aero::cyanGlow.withAlpha(0.92F));
    graphics.drawEllipse(knobBounds.expanded(aero::scaled(4.0F, scale)), aero::scaled(2.0F, scale));
  }
  aero::drawLabelWithBacking(graphics, slider.getName().toUpperCase(),
    juce::Rectangle<int>{x, y + 2, width, juce::roundToInt(aero::scaled(16.0F, scale))},
    juce::Justification::centred, aero::labelFont(scale), true);
}

void AeroLookAndFeel::drawComboBox(juce::Graphics& graphics, int width, int height, bool isButtonDown,
    int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox& box) {
  const auto bounds = juce::Rectangle<float>{0.0F, 0.0F, static_cast<float>(width), static_cast<float>(height)}.reduced(1.0F);
  const auto scale = aero::uiScaleForWidth(width * 3);
  aero::drawPill(graphics, bounds, pillState(box, box.hasKeyboardFocus(true), isButtonDown), scale);
  const auto arrowBounds = juce::Rectangle<float>{static_cast<float>(buttonX), static_cast<float>(buttonY),
    static_cast<float>(buttonW), static_cast<float>(buttonH)}.reduced(aero::scaled(10.0F, scale));
  juce::Path arrow;
  arrow.addTriangle(arrowBounds.getX(), arrowBounds.getY() + 1.0F, arrowBounds.getRight(), arrowBounds.getY() + 1.0F,
    arrowBounds.getCentreX(), arrowBounds.getBottom());
  graphics.setColour(aero::darkBlue);
  graphics.fillPath(arrow);
  if (box.hasKeyboardFocus(true)) {
    graphics.setColour(aero::cyanGlow.withAlpha(0.80F));
    graphics.drawRoundedRectangle(bounds.expanded(2.0F), bounds.getHeight() * 0.5F, 2.0F);
  }
}

juce::Font AeroLookAndFeel::getComboBoxFont(juce::ComboBox& box) {
  return aero::labelFont(aero::uiScaleForWidth(box.getWidth() * 3));
}

void AeroLookAndFeel::positionComboBoxText(juce::ComboBox& box, juce::Label& label) {
  label.setBounds(juce::roundToInt(aero::scaled(12.0F, aero::uiScaleForWidth(box.getWidth() * 3))), 1,
    box.getWidth() - juce::roundToInt(aero::scaled(40.0F, aero::uiScaleForWidth(box.getWidth() * 3))), box.getHeight() - 2);
  label.setJustificationType(juce::Justification::centred);
  label.setColour(juce::Label::textColourId, aero::darkBlue);
  label.setFont(getComboBoxFont(box));
}

void AeroLookAndFeel::drawPopupMenuBackground(juce::Graphics& graphics, int width, int height) {
  const auto bounds = juce::Rectangle<float>{0.0F, 0.0F, static_cast<float>(width), static_cast<float>(height)}.reduced(1.0F);
  aero::drawPill(graphics, bounds, aero::PillState::normal, aero::uiScaleForWidth(width * 3));
}

void AeroLookAndFeel::drawPopupMenuItem(juce::Graphics& graphics, const juce::Rectangle<int>& area,
    bool isSeparator, bool isActive, bool isHighlighted, bool isTicked, bool hasSubMenu,
    const juce::String& text, const juce::String& shortcutKeyText, const juce::Drawable* icon,
    const juce::Colour* textColour) {
  juce::ignoreUnused(icon, textColour);
  if (isSeparator) {
    graphics.setColour(aero::rimBlue.withAlpha(0.35F));
    graphics.fillRect(area.reduced(8, area.getHeight() / 2).withHeight(1));
    return;
  }
  auto itemBounds = area.toFloat().reduced(2.0F, 1.0F);
  if (isHighlighted && isActive)
    aero::drawPill(graphics, itemBounds, aero::PillState::hover, aero::uiScaleForWidth(area.getWidth() * 3));
  const auto textArea = area.reduced(12, 0);
  graphics.setFont(aero::labelFont(aero::uiScaleForWidth(area.getWidth() * 3)));
  graphics.setColour(isActive ? aero::darkBlue : aero::disabledBlue);
  graphics.drawFittedText(text, textArea, juce::Justification::centredLeft, 1);
  if (isTicked) {
    graphics.setColour(aero::limeGlow.darker(0.30F));
    graphics.fillEllipse(static_cast<float>(area.getX() + 5), static_cast<float>(area.getCentreY() - 3), 6.0F, 6.0F);
  }
  if (hasSubMenu) {
    graphics.setColour(aero::rimBlue);
    graphics.drawSingleLineText(">", area.getRight() - 12, area.getCentreY() + 4, juce::Justification::centred);
  }
  if (shortcutKeyText.isNotEmpty()) {
    graphics.setColour(aero::darkBlue.withAlpha(0.70F));
    graphics.drawFittedText(shortcutKeyText, textArea, juce::Justification::centredRight, 1);
  }
}

void AeroLookAndFeel::drawButtonBackground(juce::Graphics& graphics, juce::Button& button, const juce::Colour&,
    bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) {
  aero::drawPill(graphics, button.getLocalBounds().toFloat().reduced(1.0F),
    pillState(button, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown),
    aero::uiScaleForWidth(button.getWidth() * 6));
}

void AeroLookAndFeel::drawButtonText(juce::Graphics& graphics, juce::TextButton& button,
    bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) {
  juce::ignoreUnused(shouldDrawButtonAsHighlighted);
  const auto scale = aero::uiScaleForWidth(button.getWidth() * 6);
  graphics.setFont(aero::labelFont(scale));
  graphics.setColour(aero::darkBlue);
  graphics.drawFittedText(button.getButtonText(), button.getLocalBounds().translated(0, shouldDrawButtonAsDown ? 1 : 0),
    juce::Justification::centred, 1);
}

void AeroLookAndFeel::drawToggleButton(juce::Graphics& graphics, juce::ToggleButton& button,
    bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) {
  const auto scale = aero::uiScaleForWidth(button.getWidth() * 6);
  aero::drawPill(graphics, button.getLocalBounds().toFloat().reduced(1.0F),
    pillState(button, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown), scale);
  const auto isBypass = button.getButtonText() == "BYPASS";
  aero::drawStatusLight(graphics, {aero::scaled(14.0F, scale), button.getHeight() * 0.5F}, button.getToggleState(),
    isBypass ? aero::clipCoral : aero::limeGlow, scale);
  graphics.setFont(aero::labelFont(scale));
  graphics.setColour(aero::darkBlue);
  graphics.drawFittedText(button.getButtonText(), button.getLocalBounds().withTrimmedLeft(14)
    .translated(0, shouldDrawButtonAsDown ? 1 : 0), juce::Justification::centred, 1);
  if (button.hasKeyboardFocus(true)) {
    graphics.setColour(aero::cyanGlow.withAlpha(0.86F));
    graphics.drawRoundedRectangle(button.getLocalBounds().toFloat().reduced(2.0F), button.getHeight() * 0.45F, 2.0F);
  }
}

void AeroLookAndFeel::drawLabel(juce::Graphics& graphics, juce::Label& label) {
  const auto isComboText = dynamic_cast<juce::ComboBox*>(label.getParentComponent()) != nullptr;
  if (isComboText) {
    graphics.setFont(label.getFont());
    graphics.setColour(aero::darkBlue);
    graphics.drawFittedText(label.getText(), label.getLocalBounds(), label.getJustificationType(), 1);
    return;
  }
  const auto isAeroLabel = label.getComponentID().startsWith("aero-");
  if (isAeroLabel) {
    aero::drawLabelWithBacking(graphics, label.getText(), label.getLocalBounds(), label.getJustificationType(),
      label.getFont(), true);
    return;
  }
  juce::LookAndFeel_V4::drawLabel(graphics, label);
}
