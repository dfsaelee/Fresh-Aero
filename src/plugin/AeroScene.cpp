#include "plugin/AeroScene.h"

#include <cmath>

namespace aero {
namespace {

juce::Path makeFarHill(juce::Rectangle<float> bounds) {
  juce::Path hill;
  hill.startNewSubPath(bounds.getX() - 8.0F, bounds.getHeight() * 0.73F);
  hill.cubicTo(bounds.getWidth() * 0.18F, bounds.getHeight() * 0.53F,
    bounds.getWidth() * 0.43F, bounds.getHeight() * 0.80F,
    bounds.getWidth() * 0.60F, bounds.getHeight() * 0.66F);
  hill.cubicTo(bounds.getWidth() * 0.77F, bounds.getHeight() * 0.55F,
    bounds.getWidth() * 0.91F, bounds.getHeight() * 0.71F,
    bounds.getRight() + 8.0F, bounds.getHeight() * 0.61F);
  hill.lineTo(bounds.getRight() + 8.0F, bounds.getBottom() + 8.0F);
  hill.lineTo(bounds.getX() - 8.0F, bounds.getBottom() + 8.0F);
  hill.closeSubPath();
  return hill;
}

juce::Path makeNearHill(juce::Rectangle<float> bounds) {
  juce::Path hill;
  hill.startNewSubPath(bounds.getX() - 8.0F, bounds.getHeight() * 0.82F);
  hill.cubicTo(bounds.getWidth() * 0.18F, bounds.getHeight() * 0.65F,
    bounds.getWidth() * 0.37F, bounds.getHeight() * 0.92F,
    bounds.getWidth() * 0.58F, bounds.getHeight() * 0.76F);
  hill.cubicTo(bounds.getWidth() * 0.78F, bounds.getHeight() * 0.61F,
    bounds.getWidth() * 0.95F, bounds.getHeight() * 0.82F,
    bounds.getRight() + 8.0F, bounds.getHeight() * 0.71F);
  hill.lineTo(bounds.getRight() + 8.0F, bounds.getBottom() + 8.0F);
  hill.lineTo(bounds.getX() - 8.0F, bounds.getBottom() + 8.0F);
  hill.closeSubPath();
  return hill;
}

[[maybe_unused]] float contrastRatio(float first, float second) {
  const auto light = juce::jmax(first, second);
  const auto dark = juce::jmin(first, second);
  return (light + 0.05F) / (dark + 0.05F);
}

} // namespace

void AeroScene::rebuild(juce::Rectangle<int> bounds) {
  bounds_ = bounds;
  if (bounds_.isEmpty()) return;

  bubbles_.clear();
  juce::Random random{0x0a3e0};
  for (int index = 0; index < 7; ++index) {
    bubbles_.push_back({
      0.05F + random.nextFloat() * 0.90F,
      0.10F + random.nextFloat() * 0.82F,
      5.0F + random.nextFloat() * 10.0F,
      0.025F + random.nextFloat() * 0.045F,
      0.36F + random.nextFloat() * 0.36F
    });
  }

  sceneImage_ = juce::Image{juce::Image::ARGB, bounds_.getWidth(), bounds_.getHeight(), true};
  juce::Graphics sceneGraphics{sceneImage_};
  drawStaticScene(sceneGraphics);

  const auto lowWidth = juce::jmax(1, bounds_.getWidth() / 8);
  const auto lowHeight = juce::jmax(1, bounds_.getHeight() / 8);
  frostedImage_ = juce::Image{juce::Image::ARGB, lowWidth, lowHeight, true};
  juce::Graphics frostedGraphics{frostedImage_};
  frostedGraphics.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
  frostedGraphics.drawImageTransformed(sceneImage_, juce::AffineTransform::scale(1.0F / 8.0F), false);
}

void AeroScene::drawStaticScene(juce::Graphics& graphics) {
  const auto bounds = bounds_.toFloat();
  const auto horizon = bounds.getY() + bounds.getHeight() * 0.65F;
  graphics.setGradientFill(juce::ColourGradient{skyTop, bounds.getCentreX(), bounds.getY(),
    skyHorizon, bounds.getCentreX(), horizon, false});
  graphics.fillRect(bounds);
  graphics.setGradientFill(juce::ColourGradient{textPrimary.withAlpha(0.60F), bounds.getWidth() * 0.22F,
    bounds.getHeight() * 0.18F, textPrimary.withAlpha(0.0F), bounds.getWidth() * 0.22F,
    bounds.getWidth() * 0.60F, true});
  graphics.fillEllipse(bounds.getWidth() * -0.10F, bounds.getHeight() * -0.42F,
    bounds.getWidth() * 1.20F, bounds.getWidth() * 1.20F);

  const auto horizonY = bounds.getHeight() * 0.64F;
  const auto waterLight = juce::Rectangle<float>{-bounds.getWidth() * 0.12F, horizonY - bounds.getHeight() * 0.10F,
    bounds.getWidth() * 1.24F, bounds.getHeight() * 0.42F};
  juce::ColourGradient waterBloom{skyHorizon.withAlpha(0.36F), waterLight.getCentreX(), horizonY,
    oceanTeal.withAlpha(0.01F), waterLight.getCentreX(), waterLight.getBottom(), false};
  waterBloom.addColour(0.38, cyanGlow.withAlpha(0.22F));
  graphics.setGradientFill(waterBloom);
  graphics.fillEllipse(waterLight);
  // Irregular glints suggest water without turning the scene into horizontal stripes.
  graphics.setColour(textPrimary.withAlpha(0.13F));
  graphics.fillEllipse({bounds.getWidth() * 0.18F, horizonY + bounds.getHeight() * 0.045F,
    bounds.getWidth() * 0.24F, bounds.getHeight() * 0.026F});
  graphics.fillEllipse({bounds.getWidth() * 0.56F, horizonY + bounds.getHeight() * 0.12F,
    bounds.getWidth() * 0.19F, bounds.getHeight() * 0.018F});

  drawCloudCluster(graphics, {bounds.getWidth() * 0.03F, bounds.getHeight() * 0.25F}, 1.08F, 0.34F);
  drawCloudCluster(graphics, {bounds.getWidth() * 0.48F, bounds.getHeight() * 0.17F}, 0.78F, 0.28F);
  drawCloudCluster(graphics, {bounds.getWidth() * 0.76F, bounds.getHeight() * 0.34F}, 0.90F, 0.38F);

  const auto farHill = makeFarHill(bounds);
  drawHill(graphics, farHill, bounds, hillFar.brighter(0.12F), hillFar.darker(0.16F), textPrimary);
  const auto nearHill = makeNearHill(bounds);
  drawHill(graphics, nearHill, bounds, hillNear.brighter(0.16F), hillNear.darker(0.20F), limeGlow);
  for (const auto& bubble : bubbles_)
    drawBubble(graphics, bubblePosition(bubble, 0.0F), bubble.radius, bubble.alpha);
}

void AeroScene::drawBackground(juce::Graphics& graphics) const {
  if (sceneImage_.isValid()) graphics.drawImageAt(sceneImage_, bounds_.getX(), bounds_.getY(), false);
}

juce::Point<float> AeroScene::bubblePosition(const BubbleSeed& bubble, float phase) const {
  const auto travel = std::fmod(bubble.yFraction - phase * bubble.speed + 1.15F, 1.15F);
  return {bounds_.getX() + bubble.xFraction * static_cast<float>(bounds_.getWidth()),
    bounds_.getY() + travel * static_cast<float>(bounds_.getHeight())};
}

void AeroScene::drawDynamicDecorations(juce::Graphics& graphics, float phase) const {
  if (bounds_.isEmpty()) return;
  const auto scale = uiScaleForWidth(bounds_.getWidth());
  const auto drawShimmer = [&graphics, scale, phase](juce::Point<float> centre, juce::Colour colour) {
    const auto pulse = 0.5F + 0.5F * std::sin(phase * 1.2F);
    for (int ring = 0; ring < 2; ++ring) {
      const auto radius = scaled(38.0F + static_cast<float>(ring) * 14.0F + pulse * 7.0F, scale);
      graphics.setColour(colour.withAlpha(0.08F - static_cast<float>(ring) * 0.025F));
      graphics.drawEllipse(centre.x - radius, centre.y - radius, radius * 2.0F, radius * 2.0F,
        scaled(1.5F, scale));
    }
  };
  drawShimmer({bounds_.getWidth() * 0.20F, bounds_.getHeight() * 0.52F}, limeGlow);
  drawShimmer({bounds_.getWidth() * 0.80F, bounds_.getHeight() * 0.52F}, cyanGlow);
}

juce::Rectangle<int> AeroScene::shimmerBounds(float xFraction) const {
  const auto scale = uiScaleForWidth(bounds_.getWidth());
  const auto shimmerRadius = scaled(68.0F, scale);
  return juce::Rectangle<float>{bounds_.getWidth() * xFraction - shimmerRadius,
    bounds_.getHeight() * 0.52F - shimmerRadius, shimmerRadius * 2.0F, shimmerRadius * 2.0F}
    .toNearestInt().getIntersection(bounds_);
}

void AeroScene::repaintDynamicRegions(juce::Component& component, float oldPhase, float newPhase) const {
  juce::ignoreUnused(oldPhase, newPhase);
  component.repaint(shimmerBounds(0.20F));
  component.repaint(shimmerBounds(0.80F));
}

void AeroScene::logContrastForLabel(const juce::String& labelName, juce::Rectangle<int> labelBounds) const {
#if JUCE_DEBUG
  if (!sceneImage_.isValid()) return;
  const auto sample = labelBounds.getIntersection(bounds_);
  if (sample.isEmpty()) return;
  float totalLuminance{};
  int sampleCount{};
  const auto xStep = juce::jmax(1, sample.getWidth() / 10);
  const auto yStep = juce::jmax(1, sample.getHeight() / 4);
  for (int y = sample.getY(); y < sample.getBottom(); y += yStep) {
    for (int x = sample.getX(); x < sample.getRight(); x += xStep) {
      totalLuminance += sceneImage_.getPixelAt(x - bounds_.getX(), y - bounds_.getY()).getPerceivedBrightness();
      ++sampleCount;
    }
  }
  if (sampleCount == 0) return;
  const auto sceneLuminance = totalLuminance / static_cast<float>(sampleCount);
  const auto greyscale = juce::Colour{static_cast<juce::uint8>(sceneLuminance * 255.0F),
    static_cast<juce::uint8>(sceneLuminance * 255.0F), static_cast<juce::uint8>(sceneLuminance * 255.0F)};
  const auto backedLuminance = greyscale.interpolatedWith(darkBlue, 0.84F).getPerceivedBrightness();
  if (contrastRatio(textPrimary.getPerceivedBrightness(), backedLuminance) < 4.5F)
    DBG("Aero contrast below 4.5:1 for " + labelName);
#else
  juce::ignoreUnused(labelName, labelBounds);
#endif
}

} // namespace aero
