#include "plugin/PluginEditor.hpp"

namespace {

float meterLevel(float gain) {
  return juce::jlimit(0.0F, 1.0F,
    juce::jmap(juce::Decibels::gainToDecibels(gain, -60.0F), -60.0F, 0.0F, 0.0F, 1.0F));
}

} // namespace

FreshAeroEditor::FreshAeroEditor(FreshAeroAudioProcessor& audioProcessor)
  : AudioProcessorEditor(audioProcessor), processor_(audioProcessor) {
  setLookAndFeel(&lookAndFeel_);
  setOpaque(true);
  setResizable(true, true);
  setResizeLimits(560, 400, 900, 680);
  setSize(680, 480);

  titleLabel_.setComponentID("aero-title");
  titleLabel_.setText("FRESH AERO", juce::dontSendNotification);
  titleLabel_.setFont(aero::titleFont(uiScale_));
  titleLabel_.setJustificationType(juce::Justification::centred);
  addAndMakeVisible(titleLabel_);
  subtitleLabel_.setComponentID("aero-subtitle");
  subtitleLabel_.setText("DYNAMIC PRESENCE + OPEN AIR", juce::dontSendNotification);
  subtitleLabel_.setFont(aero::labelFont(uiScale_));
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
  outputSlider_.setColour(juce::Slider::textBoxTextColourId, aero::darkBlue);
  outputSlider_.setColour(juce::Slider::textBoxBackgroundColourId, aero::pillTop);
  outputSlider_.setColour(juce::Slider::textBoxOutlineColourId, aero::pillEdge);
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

  const auto markCustom = [this] {
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
  resized();
  updateThemeState();
  startTimerHz(30);
}

FreshAeroEditor::~FreshAeroEditor() {
  stopTimer();
  presenceAttachment_.reset();
  airAttachment_.reset();
  outputAttachment_.reset();
  linkAttachment_.reset();
  bypassAttachment_.reset();
  setLookAndFeel(nullptr);
}

void FreshAeroEditor::setReducedMotion(bool shouldReduceMotion) {
  if (reducedMotion_ == shouldReduceMotion) return;
  reducedMotion_ = shouldReduceMotion;
  scene_.repaintDynamicRegions(*this, animationPhase_, animationPhase_);
}

void FreshAeroEditor::configureMacro(juce::Slider& slider, const juce::String& name,
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
  slider.setColour(juce::Slider::textBoxTextColourId, aero::darkBlue);
  slider.setColour(juce::Slider::textBoxBackgroundColourId, aero::pillTop);
  slider.setColour(juce::Slider::textBoxOutlineColourId, aero::pillEdge);
  addAndMakeVisible(slider);
}

void FreshAeroEditor::paint(juce::Graphics& graphics) {
  const auto paintStarted = juce::Time::getMillisecondCounterHiRes();
  scene_.drawBackground(graphics);
  aero::drawGlassPanel(graphics, cardPath_, getLocalBounds().toFloat(), scene_.frostedImage());
  aero::drawGlassTitleBar(graphics, titleBarBounds_, uiScale_);
  drawBadge(graphics);
  drawMeter(graphics);
  recordPaintDuration(juce::Time::getMillisecondCounterHiRes() - paintStarted);
}

void FreshAeroEditor::drawBadge(juce::Graphics& graphics) const {
  const auto dotRadius = aero::scaled(3.5F, uiScale_);
  aero::drawStatusLight(graphics, {badgeBounds_.getX() + dotRadius, badgeBounds_.getCentreY()}, true,
    aero::limeGlow, uiScale_);
  graphics.setFont(aero::labelFont(uiScale_));
  const auto textBounds = badgeBounds_.toNearestInt().withTrimmedLeft(juce::roundToInt(dotRadius * 2.0F));
  graphics.setColour(aero::textShadow);
  graphics.drawFittedText("FRESH AERO", textBounds.translated(1, 1), juce::Justification::centred, 1);
  graphics.setColour(aero::textPrimary);
  graphics.drawFittedText("FRESH AERO", textBounds, juce::Justification::centred, 1);
}

void FreshAeroEditor::drawMeter(juce::Graphics& graphics) const {
  aero::drawGlassTube(graphics, meterBounds_.toFloat(), meterLevel(displayedRms_), processor_.outputClipped(), uiScale_);
  const auto peakX = static_cast<float>(meterBounds_.getX()) + static_cast<float>(meterBounds_.getWidth()) * meterLevel(displayedPeak_);
  graphics.setColour(aero::textPrimary.withAlpha(0.88F));
  graphics.fillRect(peakX - 1.0F, static_cast<float>(meterBounds_.getY()), 2.0F,
    static_cast<float>(meterBounds_.getHeight()));
  aero::drawStatusLight(graphics, {static_cast<float>(meterBounds_.getRight()) + aero::scaled(13.0F, uiScale_),
    static_cast<float>(meterBounds_.getCentreY())}, processor_.outputClipped(), aero::clipCoral, uiScale_);
  aero::drawLabelWithBacking(graphics, "OUTPUT  RMS / PEAK | CLICK TO RESET CLIP",
    meterBounds_.withY(meterBounds_.getY() - juce::roundToInt(aero::scaled(18.0F, uiScale_)))
      .withHeight(juce::roundToInt(aero::scaled(14.0F, uiScale_))),
    juce::Justification::centred, aero::labelFont(uiScale_), true);
}

void FreshAeroEditor::resized() {
  uiScale_ = aero::uiScaleForWidth(getWidth());
  cardBounds_ = getLocalBounds().toFloat().reduced(aero::scaled(18.0F, uiScale_));
  cardPath_.clear();
  cardPath_.addRoundedRectangle(cardBounds_, aero::scaled(aero::cardRadius, uiScale_));
  titleBarBounds_ = cardBounds_.reduced(aero::scaled(9.0F, uiScale_));
  titleBarBounds_.setHeight(aero::scaled(62.0F, uiScale_));
  badgeBounds_ = {cardBounds_.getRight() - aero::scaled(104.0F, uiScale_), cardBounds_.getY() + aero::scaled(20.0F, uiScale_),
    aero::scaled(82.0F, uiScale_), aero::scaled(20.0F, uiScale_)};

  auto area = getLocalBounds().reduced(juce::roundToInt(aero::scaled(34.0F, uiScale_)));
  titleLabel_.setFont(aero::titleFont(uiScale_));
  titleLabel_.setBounds(area.removeFromTop(juce::roundToInt(aero::scaled(30.0F, uiScale_))));
  subtitleLabel_.setFont(aero::labelFont(uiScale_));
  subtitleLabel_.setBounds(area.removeFromTop(juce::roundToInt(aero::scaled(20.0F, uiScale_))));
  area.removeFromTop(juce::roundToInt(aero::scaled(4.0F, uiScale_)));
  presetBox_.setBounds(area.removeFromTop(juce::roundToInt(aero::scaled(34.0F, uiScale_)))
    .withSizeKeepingCentre(juce::roundToInt(aero::scaled(220.0F, uiScale_)), juce::roundToInt(aero::scaled(34.0F, uiScale_))));
  area.removeFromTop(juce::roundToInt(aero::scaled(8.0F, uiScale_)));

  auto footer = area.removeFromBottom(juce::roundToInt(aero::scaled(70.0F, uiScale_)));
  auto controls = area;
  const auto third = controls.getWidth() / 3;
  presenceSlider_.setBounds(controls.removeFromLeft(third).reduced(juce::roundToInt(aero::scaled(8.0F, uiScale_))));
  outputSlider_.setBounds(controls.removeFromLeft(third).reduced(juce::roundToInt(aero::scaled(38.0F, uiScale_)),
    juce::roundToInt(aero::scaled(22.0F, uiScale_))));
  airSlider_.setBounds(controls.reduced(juce::roundToInt(aero::scaled(8.0F, uiScale_))));

  auto buttons = footer.removeFromTop(juce::roundToInt(aero::scaled(32.0F, uiScale_)))
    .withSizeKeepingCentre(juce::roundToInt(aero::scaled(220.0F, uiScale_)), juce::roundToInt(aero::scaled(32.0F, uiScale_)));
  linkButton_.setBounds(buttons.removeFromLeft(juce::roundToInt(aero::scaled(100.0F, uiScale_))));
  buttons.removeFromLeft(juce::roundToInt(aero::scaled(20.0F, uiScale_)));
  bypassButton_.setBounds(buttons);
  meterBounds_ = {juce::roundToInt(cardBounds_.getX() + aero::scaled(36.0F, uiScale_)),
    footer.getY() + juce::roundToInt(aero::scaled(18.0F, uiScale_)),
    juce::roundToInt(cardBounds_.getWidth() - aero::scaled(72.0F, uiScale_)), juce::roundToInt(aero::scaled(10.0F, uiScale_))};
  meterHitBounds_ = meterBounds_.expanded(juce::roundToInt(aero::scaled(14.0F, uiScale_)),
    juce::roundToInt(aero::scaled(22.0F, uiScale_)));

  scene_.rebuild(getLocalBounds());
  scene_.logContrastForLabel("title", titleLabel_.getBounds());
  scene_.logContrastForLabel("subtitle", subtitleLabel_.getBounds());
  scene_.logContrastForLabel("meter", meterBounds_.withY(meterBounds_.getY() - juce::roundToInt(aero::scaled(18.0F, uiScale_))));
}

void FreshAeroEditor::mouseDown(const juce::MouseEvent& event) {
  if (meterHitBounds_.contains(event.getPosition())) {
    processor_.clearClipIndicator();
    repaint(meterHitBounds_);
  }
}

void FreshAeroEditor::updateThemeState() {
  const auto bypassed = bypassButton_.getToggleState();
  if (lastBypassed_ == bypassed) return;
  lastBypassed_ = bypassed;
  lookAndFeel_.setBypassed(bypassed);
  presenceSlider_.repaint();
  airSlider_.repaint();
  outputSlider_.repaint();
}

void FreshAeroEditor::timerCallback() {
  if (!isShowing()) return;
  updateThemeState();
  displayedPeak_ = juce::jmax(processor_.outputPeak(), displayedPeak_ * 0.88F);
  displayedRms_ = juce::jmax(processor_.outputRms(), displayedRms_ * 0.94F);
  repaint(meterHitBounds_);
}

void FreshAeroEditor::recordPaintDuration(double elapsedMilliseconds) {
#if JUCE_DEBUG
  paintTotalMilliseconds_ += elapsedMilliseconds;
  worstPaintMilliseconds_ = juce::jmax(worstPaintMilliseconds_, elapsedMilliseconds);
  if (++paintSampleCount_ < 120) return;
  const auto average = paintTotalMilliseconds_ / static_cast<double>(paintSampleCount_);
  if (average > 2.0 || worstPaintMilliseconds_ > 8.0)
    DBG("Aero paint budget exceeded: avg=" + juce::String(average, 2) + "ms worst="
      + juce::String(worstPaintMilliseconds_, 2) + "ms");
  paintTotalMilliseconds_ = 0.0;
  worstPaintMilliseconds_ = 0.0;
  paintSampleCount_ = 0;
#else
  juce::ignoreUnused(elapsedMilliseconds);
#endif
}

void FreshAeroEditor::applyPreset(int presetId) {
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
