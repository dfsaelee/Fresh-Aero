#include "plugin/PluginEditor.hpp"

namespace {
const auto background = juce::Colour{0xff0d1518};
const auto panel = juce::Colour{0xff162328};
const auto presenceColour = juce::Colour{0xffffbf69};
const auto airColour = juce::Colour{0xff72e1f2};
const auto textColour = juce::Colour{0xffedf7f5};
const auto muted = juce::Colour{0xff82969b};
}

FreshAirLookAndFeel::FreshAirLookAndFeel() {
  setColour(juce::ComboBox::backgroundColourId, juce::Colour{0xff203138});
  setColour(juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
  setColour(juce::ComboBox::textColourId, textColour);
  setColour(juce::PopupMenu::backgroundColourId, panel);
  setColour(juce::PopupMenu::textColourId, textColour);
  setColour(juce::ToggleButton::textColourId, muted);
  setColour(juce::ToggleButton::tickColourId, airColour);
}

void FreshAirLookAndFeel::drawRotarySlider(juce::Graphics& graphics, int x, int y,
    int width, int height, float position, float startAngle, float endAngle,
    juce::Slider& slider) {
  auto bounds = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y),
    static_cast<float>(width), static_cast<float>(height)).reduced(13.0F);
  const auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5F;
  const auto centre = bounds.getCentre();
  const auto angle = startAngle + position * (endAngle - startAngle);
  const auto colour = slider.getName() == "Presence" ? presenceColour : airColour;

  juce::Path track;
  track.addCentredArc(centre.x, centre.y, radius, radius, 0.0F, startAngle, endAngle, true);
  graphics.setColour(juce::Colour{0xff2a3c43});
  graphics.strokePath(track, juce::PathStrokeType(6.0F,
    juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
  juce::Path value;
  value.addCentredArc(centre.x, centre.y, radius, radius, 0.0F, startAngle, angle, true);
  graphics.setColour(colour);
  graphics.strokePath(value, juce::PathStrokeType(6.0F,
    juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

  const auto knob = bounds.withSizeKeepingCentre(radius * 1.55F, radius * 1.55F);
  graphics.setGradientFill(juce::ColourGradient{juce::Colour{0xff3c5057}, knob.getX(), knob.getY(),
    juce::Colour{0xff141e22}, knob.getRight(), knob.getBottom(), false});
  graphics.fillEllipse(knob);
  graphics.setColour(juce::Colour{0xff536a72});
  graphics.drawEllipse(knob, 1.5F);
  juce::Path pointer;
  pointer.addRoundedRectangle(-2.0F, -radius * 0.6F, 4.0F, radius * 0.24F, 2.0F);
  pointer.applyTransform(juce::AffineTransform::rotation(angle).translated(centre.x, centre.y));
  graphics.setColour(colour);
  graphics.fillPath(pointer);
}

G3XFreshAirEditor::G3XFreshAirEditor(G3XFreshAirAudioProcessor& processor)
  : AudioProcessorEditor(processor), processor_(processor) {
  setLookAndFeel(&lookAndFeel_);
  setOpaque(true);
  setResizable(true, true);
  setResizeLimits(560, 400, 900, 680);
  setSize(680, 480);

  titleLabel_.setText("G3X  /  FRESH AIR", juce::dontSendNotification);
  titleLabel_.setColour(juce::Label::textColourId, textColour);
  titleLabel_.setFont(juce::FontOptions{21.0F, juce::Font::bold});
  titleLabel_.setJustificationType(juce::Justification::centred);
  addAndMakeVisible(titleLabel_);
  subtitleLabel_.setText("DYNAMIC PRESENCE + OPEN AIR", juce::dontSendNotification);
  subtitleLabel_.setColour(juce::Label::textColourId, muted);
  subtitleLabel_.setFont(juce::FontOptions{11.0F});
  subtitleLabel_.setJustificationType(juce::Justification::centred);
  addAndMakeVisible(subtitleLabel_);

  configureMacro(presenceSlider_, "Presence", "Controls definition in the mid-high region");
  configureMacro(airSlider_, "Air", "Controls openness in the upper spectrum");
  outputSlider_.setName("Output");
  outputSlider_.setTitle("Output trim");
  outputSlider_.setDescription("Adjusts output level from minus twelve to plus three decibels");
  outputSlider_.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  outputSlider_.setRotaryParameters(juce::MathConstants<float>::pi * 1.2F,
    juce::MathConstants<float>::pi * 2.8F, true);
  outputSlider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 26);
  outputSlider_.setDoubleClickReturnValue(true, 0.0);
  outputSlider_.setTextValueSuffix(" dB");
  outputSlider_.setColour(juce::Slider::textBoxTextColourId, textColour);
  outputSlider_.setColour(juce::Slider::textBoxBackgroundColourId, panel);
  outputSlider_.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
  addAndMakeVisible(outputSlider_);

  linkButton_.setTitle("Link band dynamics");
  linkButton_.setDescription("Applies the strongest dynamic reduction to both bands");
  bypassButton_.setTitle("Effect bypass");
  bypassButton_.setDescription("Crossfades to the original signal");
  addAndMakeVisible(linkButton_);
  addAndMakeVisible(bypassButton_);

  presetBox_.setTitle("Preset");
  presetBox_.setDescription("Selects starting values for Presence, Air and output trim");
  for (std::size_t i = 0; i < kPresets.size(); ++i)
    presetBox_.addItem(kPresets[i].name, static_cast<int>(i + 1));
  presetBox_.setTextWhenNothingSelected("Custom");
  presetBox_.setSelectedId(processor_.getCurrentProgram() + 1, juce::dontSendNotification);
  presetBox_.onChange = [this] { applyPreset(presetBox_.getSelectedId()); };
  addAndMakeVisible(presetBox_);

  auto markCustom = [this] { 
    if (!isApplyingPreset_) presetBox_.setSelectedId(0, juce::dontSendNotification); 
  };
  presenceSlider_.onValueChange = markCustom;
  airSlider_.onValueChange = markCustom;
  outputSlider_.onValueChange = markCustom;
  linkButton_.onClick = markCustom;

  presenceAttachment_ = std::make_unique<SliderAttachment>(processor_.state,
    "presenceAmount", presenceSlider_);
  airAttachment_ = std::make_unique<SliderAttachment>(processor_.state, "airAmount", airSlider_);
  outputAttachment_ = std::make_unique<SliderAttachment>(processor_.state,
    "outputTrimDb", outputSlider_);
  linkAttachment_ = std::make_unique<ButtonAttachment>(processor_.state, "linkBands", linkButton_);
  bypassAttachment_ = std::make_unique<ButtonAttachment>(processor_.state, "bypass", bypassButton_);
  startTimerHz(30);
}

G3XFreshAirEditor::~G3XFreshAirEditor() {
  stopTimer();
  presenceAttachment_.reset();
  airAttachment_.reset();
  outputAttachment_.reset();
  linkAttachment_.reset();
  bypassAttachment_.reset();
  setLookAndFeel(nullptr);
}

void G3XFreshAirEditor::configureMacro(juce::Slider& slider, const juce::String& name,
    const juce::String& description) {
  slider.setName(name);
  slider.setTitle(name + " amount");
  slider.setDescription(description);
  slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  slider.setRotaryParameters(juce::MathConstants<float>::pi * 1.2F,
    juce::MathConstants<float>::pi * 2.8F, true);
  slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 78, 28);
  slider.setDoubleClickReturnValue(true, 0.0);
  slider.setTextValueSuffix(" %");
  slider.setColour(juce::Slider::textBoxTextColourId, textColour);
  slider.setColour(juce::Slider::textBoxBackgroundColourId, panel);
  slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
  addAndMakeVisible(slider);
}

void G3XFreshAirEditor::paint(juce::Graphics& graphics) {
  graphics.fillAll(background);
  const auto card = getLocalBounds().toFloat().reduced(18.0F);
  graphics.setColour(panel);
  graphics.fillRoundedRectangle(card, 18.0F);
  graphics.setColour(juce::Colour{0xff294047});
  graphics.drawRoundedRectangle(card, 18.0F, 1.0F);

  auto meter = juce::Rectangle<float>{54.0F, getHeight() - 56.0F,
    getWidth() - 108.0F, 10.0F};
  graphics.setColour(juce::Colour{0xff0a1114});
  graphics.fillRoundedRectangle(meter, 5.0F);
  const auto peak = juce::jlimit(0.0F, 1.0F,
    juce::jmap(juce::Decibels::gainToDecibels(displayedPeak_, -60.0F), -60.0F, 0.0F, 0.0F, 1.0F));
  const auto rms = juce::jlimit(0.0F, 1.0F,
    juce::jmap(juce::Decibels::gainToDecibels(displayedRms_, -60.0F), -60.0F, 0.0F, 0.0F, 1.0F));
  auto rmsFill = meter.reduced(2.0F);
  rmsFill.setWidth(rmsFill.getWidth() * rms);
  graphics.setColour(airColour.withAlpha(0.55F));
  graphics.fillRoundedRectangle(rmsFill, 3.0F);
  graphics.setColour(processor_.outputClipped() ? juce::Colour{0xffff6b6b} : textColour);
  graphics.fillRect(meter.getX() + meter.getWidth() * peak - 1.0F, meter.getY(), 2.0F, meter.getHeight());
  graphics.setFont(juce::FontOptions{10.0F, juce::Font::bold});
  graphics.drawText("OUTPUT  RMS / PEAK  ·  CLICK TO RESET CLIP",
    meter.withY(meter.getY() - 18.0F).withHeight(14.0F), juce::Justification::centred);
}

void G3XFreshAirEditor::resized() {
  auto area = getLocalBounds().reduced(34);
  titleLabel_.setBounds(area.removeFromTop(30));
  subtitleLabel_.setBounds(area.removeFromTop(20));
  area.removeFromTop(4);
  presetBox_.setBounds(area.removeFromTop(34).withSizeKeepingCentre(220, 34));
  area.removeFromTop(8);
  auto controls = area.removeFromTop(juce::jmax(190, area.getHeight() - 88));
  const auto third = controls.getWidth() / 3;
  presenceSlider_.setBounds(controls.removeFromLeft(third).reduced(8));
  outputSlider_.setBounds(controls.removeFromLeft(third).reduced(38, 34));
  airSlider_.setBounds(controls.reduced(8));
  auto buttons = area.removeFromTop(36).withSizeKeepingCentre(220, 32);
  linkButton_.setBounds(buttons.removeFromLeft(100));
  buttons.removeFromLeft(20);
  bypassButton_.setBounds(buttons);
}

void G3XFreshAirEditor::mouseDown(const juce::MouseEvent& event) {
  const auto meterZone = juce::Rectangle<int>{40, getHeight() - 84, getWidth() - 80, 56};
  if (meterZone.contains(event.getPosition())) {
    processor_.clearClipIndicator();
    repaint();
  }
}

void G3XFreshAirEditor::timerCallback() {
  displayedPeak_ = juce::jmax(processor_.outputPeak(), displayedPeak_ * 0.88F);
  displayedRms_ = juce::jmax(processor_.outputRms(), displayedRms_ * 0.94F);
  repaint();
}

void G3XFreshAirEditor::applyPreset(int presetId) {
  if (presetId < 1 || presetId > static_cast<int>(kPresets.size())) return;
  isApplyingPreset_ = true;
  processor_.setCurrentProgram(presetId - 1);
  const auto& preset = kPresets[static_cast<std::size_t>(presetId - 1)];
  presenceSlider_.setValue(preset.presence, juce::sendNotificationSync);
  airSlider_.setValue(preset.air, juce::sendNotificationSync);
  outputSlider_.setValue(preset.outputTrimDb, juce::sendNotificationSync);
  linkButton_.setToggleState(preset.linkBands, juce::sendNotificationSync);
  isApplyingPreset_ = false;
}
