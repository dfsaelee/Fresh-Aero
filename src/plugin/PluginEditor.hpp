#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "plugin/PluginProcessor.hpp"

class FreshAirLookAndFeel final : public juce::LookAndFeel_V4 {
public:
  FreshAirLookAndFeel();
  void drawRotarySlider(juce::Graphics&, int x, int y, int width, int height,
    float position, float startAngle, float endAngle, juce::Slider&) override;
};

class G3XFreshAirEditor final : public juce::AudioProcessorEditor, private juce::Timer {
public:
  explicit G3XFreshAirEditor(G3XFreshAirAudioProcessor&);
  ~G3XFreshAirEditor() override;
  void paint(juce::Graphics&) override;
  void resized() override;
  void mouseDown(const juce::MouseEvent&) override;

private:
  using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
  using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
  void timerCallback() override;
  void applyPreset(int presetId);
  void configureMacro(juce::Slider&, const juce::String&, const juce::String&);

  G3XFreshAirAudioProcessor& processor_;
  FreshAirLookAndFeel lookAndFeel_;
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
  float displayedPeak_{};
  float displayedRms_{};
  bool isApplyingPreset_{false};
};
