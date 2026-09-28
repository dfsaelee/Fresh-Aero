#pragma once

#include "plugin/AeroTheme.h"

namespace aero {

enum class PillState { normal, hover, pressed, disabled };

struct ArcParams {
  juce::Point<float> centre;
  float radius{};
  float startAngle{};
  float endAngle{};
  float value{};
};

void drawSoftShadow(juce::Graphics&, const juce::Path&, juce::Point<int> offset,
  int radius, float alpha);
void drawControlledGlow(juce::Graphics&, const juce::Path&, juce::Colour, int radius, float alpha);
void drawGlossHighlight(juce::Graphics&, juce::Rectangle<float>, float topFraction,
  float alphaTop, float alphaBottom);
void drawBottomReflection(juce::Graphics&, juce::Rectangle<float>, juce::Colour, float alpha);
void drawGlassPanel(juce::Graphics&, const juce::Path&, juce::Rectangle<float> sceneBounds,
  const juce::Image& frostedScene);
void drawGlassTitleBar(juce::Graphics&, juce::Rectangle<float>, float uiScale);
void drawGroove(juce::Graphics&, const ArcParams&);
void drawGlowArc(juce::Graphics&, const ArcParams&, juce::Colour, float glowAlpha);
void drawKnobBody(juce::Graphics&, juce::Rectangle<float>);
void drawPill(juce::Graphics&, juce::Rectangle<float>, PillState, float uiScale);
void drawStatusLight(juce::Graphics&, juce::Point<float>, bool isOn, juce::Colour, float uiScale);
void drawGlassTube(juce::Graphics&, juce::Rectangle<float>, float level, bool clipLit, float uiScale);
void drawLabelWithBacking(juce::Graphics&, const juce::String&, juce::Rectangle<int>,
  juce::Justification, const juce::Font&, bool useBacking = true);
void drawBubble(juce::Graphics&, juce::Point<float>, float radius, float alpha = 1.0F);
void drawCloudCluster(juce::Graphics&, juce::Point<float>, float scale, float alpha);
void drawHill(juce::Graphics&, const juce::Path&, juce::Rectangle<float>,
  juce::Colour topColour, juce::Colour bottomColour, juce::Colour ridgeColour);

} // namespace aero
