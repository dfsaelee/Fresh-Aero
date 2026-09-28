#include "plugin/AeroPainters.h"

namespace aero {
namespace {

juce::Path makeArc(const ArcParams& params, float endAngle) {
  juce::Path path;
  path.addCentredArc(params.centre.x, params.centre.y, params.radius, params.radius,
    0.0F, params.startAngle, endAngle, true);
  return path;
}

juce::ColourGradient makeOceanGradient(juce::Rectangle<float> bounds, juce::Colour highlight,
    juce::Colour transition, juce::Colour main, juce::Colour lower) {
  juce::ColourGradient gradient{highlight, bounds.getCentreX(), bounds.getY(), lower,
    bounds.getCentreX(), bounds.getBottom(), false};
  gradient.addColour(0.27, transition);
  gradient.addColour(0.68, main);
  return gradient;
}

void drawCurvedSurfaceGloss(juce::Graphics& graphics, juce::Rectangle<float> bounds, float alpha) {
  juce::Path gloss;
  const auto left = bounds.getX() + bounds.getWidth() * 0.15F;
  const auto right = bounds.getRight() - bounds.getWidth() * 0.15F;
  const auto top = bounds.getY() + bounds.getHeight() * 0.05F;
  const auto lower = bounds.getY() + bounds.getHeight() * 0.31F;
  gloss.startNewSubPath(left, top);
  gloss.lineTo(right, top);
  gloss.cubicTo(right - bounds.getWidth() * 0.03F, lower, bounds.getCentreX() + bounds.getWidth() * 0.16F,
    lower + bounds.getHeight() * 0.03F, bounds.getCentreX(), lower);
  gloss.cubicTo(bounds.getCentreX() - bounds.getWidth() * 0.16F, lower + bounds.getHeight() * 0.03F,
    left + bounds.getWidth() * 0.03F, lower, left, top);
  gloss.closeSubPath();
  graphics.setGradientFill(juce::ColourGradient{textPrimary.withAlpha(alpha), bounds.getCentreX(), top,
    textPrimary.withAlpha(0.0F), bounds.getCentreX(), lower, false});
  graphics.fillPath(gloss);
}

} // namespace

void drawSoftShadow(juce::Graphics& graphics, const juce::Path& path, juce::Point<int> offset,
    int radius, float alpha) {
  juce::DropShadow{shadow.withAlpha(alpha), radius, offset}.drawForPath(graphics, path);
}

void drawControlledGlow(juce::Graphics& graphics, const juce::Path& path, juce::Colour colour,
    int radius, float alpha) {
  juce::DropShadow{colour.withAlpha(alpha), radius, {0, 0}}.drawForPath(graphics, path);
}

void drawGlossHighlight(juce::Graphics& graphics, juce::Rectangle<float> bounds,
    float topFraction, float alphaTop, float alphaBottom) {
  auto highlight = bounds;
  highlight.setHeight(bounds.getHeight() * topFraction);
  graphics.setGradientFill(juce::ColourGradient{textPrimary.withAlpha(alphaTop), highlight.getCentreX(), highlight.getY(),
    textPrimary.withAlpha(alphaBottom), highlight.getCentreX(), highlight.getBottom(), false});
  graphics.fillRoundedRectangle(highlight, highlight.getHeight() * 0.5F);
}

void drawBottomReflection(juce::Graphics& graphics, juce::Rectangle<float> bounds,
    juce::Colour colour, float alpha) {
  auto reflection = bounds.reduced(bounds.getWidth() * 0.12F, bounds.getHeight() * 0.08F);
  reflection.setY(bounds.getBottom() - bounds.getHeight() * 0.24F);
  reflection.setHeight(bounds.getHeight() * 0.12F);
  graphics.setGradientFill(juce::ColourGradient{colour.withAlpha(alpha), reflection.getCentreX(), reflection.getY(),
    colour.withAlpha(0.0F), reflection.getCentreX(), reflection.getBottom(), false});
  graphics.fillEllipse(reflection);
}

void drawGlassPanel(juce::Graphics& graphics, const juce::Path& cardPath,
    juce::Rectangle<float> sceneBounds, const juce::Image& frostedScene) {
  drawControlledGlow(graphics, cardPath, cyanGlow, 14, 0.08F);
  drawSoftShadow(graphics, cardPath, {0, 6}, 18, 0.25F);

  graphics.saveState();
  graphics.reduceClipRegion(cardPath);
  if (frostedScene.isValid()) {
    const auto transform = juce::AffineTransform::scale(
      sceneBounds.getWidth() / static_cast<float>(frostedScene.getWidth()),
      sceneBounds.getHeight() / static_cast<float>(frostedScene.getHeight()));
    graphics.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
    graphics.drawImageTransformed(frostedScene, transform, false);
  }
  graphics.setColour(glassOcean);
  graphics.fillPath(cardPath);
  graphics.setGradientFill(makeOceanGradient(sceneBounds, textPrimary.withAlpha(0.23F),
    skyHorizon.withAlpha(0.15F), cyanGlow.withAlpha(0.16F), oceanDeep.withAlpha(0.29F)));
  graphics.fillPath(cardPath);
  juce::Path diagonalSheen;
  diagonalSheen.addQuadrilateral(sceneBounds.getX() - sceneBounds.getWidth() * 0.08F, sceneBounds.getY(),
    sceneBounds.getX() + sceneBounds.getWidth() * 0.38F, sceneBounds.getY(),
    sceneBounds.getX() + sceneBounds.getWidth() * 0.14F, sceneBounds.getBottom(),
    sceneBounds.getX() - sceneBounds.getWidth() * 0.26F, sceneBounds.getBottom());
  graphics.setGradientFill(juce::ColourGradient{textPrimary.withAlpha(0.12F), sceneBounds.getX(), sceneBounds.getY(),
    textPrimary.withAlpha(0.0F), sceneBounds.getRight(), sceneBounds.getBottom(), false});
  graphics.fillPath(diagonalSheen);
  // One broad curved reflection keeps the pane glassy without reading as a second panel.
  graphics.setColour(textPrimary.withAlpha(0.12F));
  graphics.fillEllipse(sceneBounds.withTrimmedBottom(sceneBounds.getHeight() * 0.55F)
    .withTrimmedLeft(-sceneBounds.getWidth() * 0.12F).withTrimmedRight(-sceneBounds.getWidth() * 0.12F));
  drawBottomReflection(graphics, sceneBounds, cyanGlow, 0.10F);
  graphics.restoreState();

  graphics.setColour(outerOutline.withAlpha(0.76F));
  graphics.strokePath(cardPath, juce::PathStrokeType{1.35F});
  const auto rimTransform = juce::AffineTransform::scale(0.978F, 0.978F,
    sceneBounds.getCentreX(), sceneBounds.getCentreY());
  graphics.setColour(glassEdge.withAlpha(0.60F));
  graphics.strokePath(cardPath, juce::PathStrokeType{innerBorderWidth}, rimTransform);
}

void drawGlassTitleBar(juce::Graphics& graphics, juce::Rectangle<float> bounds, float uiScale) {
  const auto radius = scaled(14.0F, uiScale);
  juce::Path titlePath;
  titlePath.addRoundedRectangle(bounds, radius);
  graphics.saveState();
  graphics.reduceClipRegion(titlePath);
  graphics.setGradientFill(makeOceanGradient(bounds, textPrimary.withAlpha(0.20F),
    skyHorizon.withAlpha(0.11F), cyanGlow.withAlpha(0.08F), oceanTeal.withAlpha(0.18F)));
  graphics.fillPath(titlePath);
  graphics.setGradientFill(juce::ColourGradient{textPrimary.withAlpha(0.13F), bounds.getCentreX(), bounds.getY(),
    textPrimary.withAlpha(0.0F), bounds.getCentreX(), bounds.getY() + bounds.getHeight() * 0.42F, false});
  graphics.fillPath(titlePath);
  graphics.restoreState();
}

void drawGroove(juce::Graphics& graphics, const ArcParams& params) {
  const auto groove = makeArc(params, params.endAngle);
  graphics.setColour(outerOutline.withAlpha(0.68F));
  graphics.strokePath(groove, juce::PathStrokeType{grooveWidth + 2.0F, juce::PathStrokeType::curved,
    juce::PathStrokeType::rounded});
  graphics.setColour(oceanDeep.withAlpha(0.78F));
  graphics.strokePath(groove, juce::PathStrokeType{grooveWidth, juce::PathStrokeType::curved,
    juce::PathStrokeType::rounded});
  graphics.setColour(textPrimary.withAlpha(0.22F));
  graphics.strokePath(groove, juce::PathStrokeType{innerBorderWidth, juce::PathStrokeType::curved,
    juce::PathStrokeType::rounded}, juce::AffineTransform::translation(0.0F, 0.72F));
}

void drawGlowArc(juce::Graphics& graphics, const ArcParams& params, juce::Colour colour, float glowAlpha) {
  const auto valueEnd = params.startAngle + (params.endAngle - params.startAngle) * params.value;
  const auto arc = makeArc(params, valueEnd);
  graphics.setColour(colour.withAlpha(glowAlpha * 0.34F));
  graphics.strokePath(arc, juce::PathStrokeType{grooveWidth + 7.0F, juce::PathStrokeType::curved,
    juce::PathStrokeType::rounded});
  graphics.setColour(colour.withAlpha(glowAlpha * 0.86F));
  graphics.strokePath(arc, juce::PathStrokeType{grooveWidth + 6.0F, juce::PathStrokeType::curved,
    juce::PathStrokeType::rounded});
  graphics.setGradientFill(juce::ColourGradient{colour.brighter(0.18F), params.centre.x, params.centre.y - params.radius,
    colour.darker(0.12F), params.centre.x, params.centre.y + params.radius, false});
  graphics.strokePath(arc, juce::PathStrokeType{grooveWidth - 2.0F, juce::PathStrokeType::curved,
    juce::PathStrokeType::rounded});
}

void drawKnobBody(juce::Graphics& graphics, juce::Rectangle<float> bounds) {
  juce::Path knobPath;
  knobPath.addEllipse(bounds);
  drawSoftShadow(graphics, knobPath, {0, 4}, 5, 0.30F);
  graphics.setColour(outerOutline.withAlpha(0.78F));
  graphics.fillEllipse(bounds);
  auto body = bounds.reduced(1.55F);
  graphics.setGradientFill(makeOceanGradient(body, knobLight.withAlpha(0.96F),
    skyHorizon.withAlpha(0.94F), cyanGlow.withAlpha(0.93F), oceanTeal.withAlpha(0.98F)));
  graphics.fillEllipse(body);
  graphics.setColour(glassEdge.withAlpha(0.68F));
  graphics.drawEllipse(body.reduced(1.0F), innerBorderWidth);

  auto gloss = body.reduced(body.getWidth() * 0.08F);
  gloss.setHeight(body.getHeight() * 0.43F);
  graphics.setGradientFill(juce::ColourGradient{textPrimary.withAlpha(0.66F), gloss.getCentreX(), gloss.getY(),
    textPrimary.withAlpha(0.04F), gloss.getCentreX(), gloss.getBottom(), false});
  graphics.fillEllipse(gloss);
  drawBottomReflection(graphics, body, cyanGlow, 0.20F);
}

void drawPill(juce::Graphics& graphics, juce::Rectangle<float> bounds, PillState state, float uiScale) {
  const auto radius = bounds.getHeight() * 0.50F;
  juce::Path pillPath;
  pillPath.addRoundedRectangle(bounds, radius);
  const auto disabled = state == PillState::disabled;
  const auto pressed = state == PillState::pressed;
  const auto mainTop = disabled ? disabledBlue : (pressed ? cyanGlow : textPrimary.withAlpha(0.93F));
  const auto transition = disabled ? disabledBlue : (pressed ? pillBottom : skyHorizon.withAlpha(0.90F));
  const auto main = disabled ? disabledBlue.darker(0.10F) : (pressed ? pillEdge : pillBottom);
  const auto mainBottom = disabled ? disabledBlue.darker(0.18F) : (pressed ? oceanTeal : pillEdge);

  drawSoftShadow(graphics, pillPath, {0, 2}, juce::roundToInt(scaled(4.0F, uiScale)), disabled ? 0.08F : 0.16F);
  graphics.setColour(outerOutline.withAlpha(disabled ? 0.48F : 0.76F));
  graphics.fillRoundedRectangle(bounds, radius);
  const auto surface = bounds.reduced(scaled(1.7F, uiScale));
  juce::Path surfacePath;
  surfacePath.addRoundedRectangle(surface, radius * 0.84F);
  graphics.saveState();
  graphics.reduceClipRegion(surfacePath);
  graphics.setGradientFill(makeOceanGradient(surface, mainTop, transition, main, mainBottom));
  graphics.fillRoundedRectangle(surface, radius * 0.84F);
  drawCurvedSurfaceGloss(graphics, surface, pressed ? 0.14F : 0.38F);
  // Low-alpha inset shadows add depth while keeping the surface visually simple.
  graphics.setGradientFill(juce::ColourGradient{oceanDeep.withAlpha(0.0F), surface.getCentreX(), surface.getY(),
    oceanDeep.withAlpha(0.16F), surface.getCentreX(), surface.getBottom(), false});
  graphics.fillRoundedRectangle(surface, radius * 0.84F);
  graphics.setGradientFill(juce::ColourGradient{textPrimary.withAlpha(0.13F), surface.getCentreX(), surface.getY(),
    textPrimary.withAlpha(0.0F), surface.getCentreX(), surface.getY() + surface.getHeight() * 0.28F, false});
  graphics.fillRoundedRectangle(surface, radius * 0.84F);
  drawBottomReflection(graphics, surface, cyanGlow, disabled ? 0.08F : 0.16F);
  graphics.restoreState();
  graphics.setColour(glassEdge.withAlpha(disabled ? 0.42F : 0.60F));
  graphics.drawRoundedRectangle(surface, radius * 0.84F, innerBorderWidth);
  if (state == PillState::hover) {
    drawControlledGlow(graphics, pillPath, cyanGlow, 8, 0.24F);
    graphics.setColour(cyanGlow.withAlpha(0.72F));
    graphics.drawRoundedRectangle(bounds.expanded(scaled(1.5F, uiScale)), radius, scaled(1.0F, uiScale));
  }
}

void drawStatusLight(juce::Graphics& graphics, juce::Point<float> centre, bool isOn,
    juce::Colour colour, float uiScale) {
  const auto radius = scaled(5.0F, uiScale);
  const auto bounds = juce::Rectangle<float>{centre.x - radius, centre.y - radius, radius * 2.0F, radius * 2.0F};
  const auto base = isOn ? colour : disabledBlue.darker(0.25F);
  if (isOn) {
    graphics.setColour(colour.withAlpha(0.16F));
    graphics.fillEllipse(bounds.expanded(radius * 0.75F));
  }
  graphics.setGradientFill(juce::ColourGradient{textPrimary.withAlpha(isOn ? 0.92F : 0.35F), bounds.getX(), bounds.getY(),
    base.darker(0.25F), bounds.getRight(), bounds.getBottom(), true});
  graphics.fillEllipse(bounds);
  graphics.setColour(darkBlue.withAlpha(0.56F));
  graphics.drawEllipse(bounds, innerBorderWidth);
  graphics.setColour(textPrimary.withAlpha(isOn ? 0.84F : 0.38F));
  graphics.fillEllipse(bounds.withSizeKeepingCentre(radius * 0.52F, radius * 0.34F)
    .translated(-radius * 0.24F, -radius * 0.32F));
}

void drawGlassTube(juce::Graphics& graphics, juce::Rectangle<float> bounds, float level,
    bool clipLit, float uiScale) {
  const auto radius = bounds.getHeight() * 0.5F;
  juce::Path tubePath;
  tubePath.addRoundedRectangle(bounds, radius);
  drawSoftShadow(graphics, tubePath, {0, 2}, juce::roundToInt(scaled(4.0F, uiScale)), 0.14F);
  graphics.setColour(outerOutline.withAlpha(0.74F));
  graphics.fillRoundedRectangle(bounds, radius);
  const auto tube = bounds.reduced(scaled(1.45F, uiScale));
  graphics.setGradientFill(makeOceanGradient(tube, textPrimary.withAlpha(0.44F),
    skyHorizon.withAlpha(0.26F), cyanGlow.withAlpha(0.22F), oceanDeep.withAlpha(0.72F)));
  graphics.fillRoundedRectangle(tube, tube.getHeight() * 0.5F);
  graphics.setColour(glassEdge.withAlpha(0.54F));
  graphics.drawRoundedRectangle(tube, tube.getHeight() * 0.5F, innerBorderWidth);
  auto fill = tube.reduced(scaled(1.0F, uiScale));
  fill.setWidth(fill.getWidth() * juce::jlimit(0.0F, 1.0F, level));
  if (fill.getWidth() > 0.0F) {
    graphics.setColour(limeGlow.withAlpha(0.28F));
    graphics.fillRoundedRectangle(fill.expanded(scaled(3.0F, uiScale), scaled(2.0F, uiScale)),
      fill.getHeight() * 0.5F);
    graphics.setGradientFill(juce::ColourGradient{limeGlow, fill.getX(), fill.getY(), cyanGlow,
      fill.getRight(), fill.getBottom(), false});
    graphics.fillRoundedRectangle(fill, fill.getHeight() * 0.5F);
  }
  auto highlight = tube.reduced(scaled(1.0F, uiScale));
  highlight.setHeight(highlight.getHeight() * 0.30F);
  graphics.setColour(textPrimary.withAlpha(0.48F));
  graphics.fillRoundedRectangle(highlight, highlight.getHeight() * 0.5F);
  drawBottomReflection(graphics, tube, cyanGlow, 0.32F);
  if (clipLit) {
    const auto clip = tube.reduced(scaled(1.0F, uiScale)).removeFromRight(scaled(8.0F, uiScale));
    graphics.setColour(clipCoral);
    graphics.fillRoundedRectangle(clip, clip.getHeight() * 0.5F);
  }
}

void drawLabelWithBacking(juce::Graphics& graphics, const juce::String& text,
    juce::Rectangle<int> bounds, juce::Justification justification, const juce::Font& font, bool useBacking) {
  const auto floatBounds = bounds.toFloat();
  if (useBacking) {
    graphics.setColour(darkBlue.withAlpha(0.84F));
    graphics.fillRoundedRectangle(floatBounds.reduced(2.0F, 1.0F), floatBounds.getHeight() * 0.45F);
  }
  graphics.setFont(font);
  graphics.setColour(textShadow);
  graphics.drawFittedText(text, bounds.translated(1, 1), justification, 1);
  graphics.setColour(textPrimary);
  graphics.drawFittedText(text, bounds, justification, 1);
}

void drawBubble(juce::Graphics& graphics, juce::Point<float> centre, float radius, float alpha) {
  const auto bounds = juce::Rectangle<float>{centre.x - radius, centre.y - radius, radius * 2.0F, radius * 2.0F};
  for (int glow = 3; glow > 0; --glow) {
    const auto expansion = static_cast<float>(glow) * radius * 0.36F;
    graphics.setColour(cyanGlow.withAlpha(alpha * 0.10F / static_cast<float>(glow)));
    graphics.fillEllipse(bounds.expanded(expansion));
  }
  graphics.setGradientFill(juce::ColourGradient{textPrimary.withAlpha(0.60F * alpha), centre.x - radius * 0.22F,
    centre.y - radius * 0.28F, cyanGlow.withAlpha(0.04F * alpha), bounds.getRight(), bounds.getBottom(), true});
  graphics.fillEllipse(bounds);
  graphics.setColour(textPrimary.withAlpha(0.86F * alpha));
  graphics.drawEllipse(bounds.reduced(radius * 0.08F), 1.15F);
  graphics.setColour(textPrimary.withAlpha(0.96F * alpha));
  graphics.fillEllipse(bounds.withSizeKeepingCentre(radius * 0.42F, radius * 0.27F)
    .translated(-radius * 0.22F, -radius * 0.26F));
  graphics.setColour(cyanGlow.withAlpha(0.68F * alpha));
  graphics.fillEllipse(bounds.withSizeKeepingCentre(radius * 0.18F, radius * 0.13F)
    .translated(radius * 0.20F, radius * 0.24F));
}

void drawCloudCluster(juce::Graphics& graphics, juce::Point<float> origin, float scale, float alpha) {
  const auto cloudColour = textPrimary.withAlpha(alpha);
  const auto drawSoftEllipse = [&graphics, cloudColour](juce::Rectangle<float> bounds) {
    for (int pass = 2; pass >= 0; --pass) {
      const auto expansion = static_cast<float>(pass) * 2.0F;
      graphics.setColour(cloudColour.withAlpha(cloudColour.getFloatAlpha() / static_cast<float>((pass + 1) * 2)));
      graphics.fillEllipse(bounds.expanded(expansion));
    }
  };
  drawSoftEllipse({origin.x, origin.y, 52.0F * scale, 22.0F * scale});
  drawSoftEllipse({origin.x + 24.0F * scale, origin.y - 12.0F * scale, 42.0F * scale, 32.0F * scale});
  drawSoftEllipse({origin.x + 54.0F * scale, origin.y - 4.0F * scale, 58.0F * scale, 27.0F * scale});
}

void drawHill(juce::Graphics& graphics, const juce::Path& path, juce::Rectangle<float> bounds,
    juce::Colour topColour, juce::Colour bottomColour, juce::Colour ridgeColour) {
  drawSoftShadow(graphics, path, {0, 3}, 5, 0.16F);
  graphics.setGradientFill(juce::ColourGradient{topColour, bounds.getCentreX(), bounds.getY(),
    bottomColour, bounds.getCentreX(), bounds.getBottom(), false});
  graphics.fillPath(path);
  graphics.setColour(ridgeColour.withAlpha(0.68F));
  graphics.strokePath(path, juce::PathStrokeType{innerBorderWidth + 0.35F});
}

} // namespace aero
