#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "plugin/AeroLookAndFeel.h"
#include "plugin/AeroScene.h"
#include "plugin/PluginProcessor.hpp"

class FreshAeroEditor final : public juce::AudioProcessorEditor, private juce::Timer {
public:
  explicit FreshAeroEditor(FreshAeroAudioProcessor&);
  ~FreshAeroEditor() override;

  void paint(juce::Graphics&) override;
  void resized() override;
  void mouseDown(const juce::MouseEvent&) override;

  void setReducedMotion(bool shouldReduceMotion);

private:
  using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
  using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

  void timerCallback() override;
  void applyPreset(int presetId);
  void configureMacro(juce::Slider&, const juce::String&, const juce::String&);
  void updateThemeState();
  void drawBadge(juce::Graphics&) const;
  void drawMeter(juce::Graphics&) const;
  void recordPaintDuration(double elapsedMilliseconds);

  FreshAeroAudioProcessor& processor_;
  AeroLookAndFeel lookAndFeel_;
  aero::AeroScene scene_;
  juce::Slider presenceSlider_;
  juce::Slider airSlider_;
  juce::Slider outputSlider_;
  juce::ToggleButton linkButton_{"LINK"};
  juce::ToggleButton bypassButton_{"BYPASS"};
  juce::ComboBox presetBox_;
  juce::Label titleLabel_;
  juce::Label subtitleLabel_;
  std::unique_ptr<SliderAttachment> presenceAttachment_;
  std::unique_ptr<SliderAttachment> airAttachment_;
  std::unique_ptr<SliderAttachment> outputAttachment_;
  std::unique_ptr<ButtonAttachment> linkAttachment_;
  std::unique_ptr<ButtonAttachment> bypassAttachment_;
  juce::Path cardPath_;
  juce::Rectangle<float> cardBounds_;
  juce::Rectangle<float> titleBarBounds_;
  juce::Rectangle<float> badgeBounds_;
  juce::Rectangle<int> meterBounds_;
  juce::Rectangle<int> meterHitBounds_;
  float uiScale_{1.0F};
  float displayedPeak_{};
  float displayedRms_{};
  float animationPhase_{};
  bool isApplyingPreset_{false};
  bool reducedMotion_{false};
  bool lastBypassed_{false};
#if JUCE_DEBUG
  double paintTotalMilliseconds_{};
  double worstPaintMilliseconds_{};
  int paintSampleCount_{};
#endif
};
