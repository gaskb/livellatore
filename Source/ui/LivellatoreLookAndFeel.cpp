#include "LivellatoreLookAndFeel.h"

namespace livellatore
{

const juce::Colour LivellatoreLookAndFeel::backgroundColour { 0xff1a1f1e };
const juce::Colour LivellatoreLookAndFeel::trackColour { 0xff2e3634 };
const juce::Colour LivellatoreLookAndFeel::accentColour { 0xff4fd8b8 };
const juce::Colour LivellatoreLookAndFeel::textColour { 0xffd7ded9 };

LivellatoreLookAndFeel::LivellatoreLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, backgroundColour);
    setColour (juce::Slider::trackColourId, trackColour);
    setColour (juce::Slider::thumbColourId, accentColour);
    setColour (juce::Label::textColourId, textColour);
}

void LivellatoreLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                                                float sliderPos, float minSliderPos, float maxSliderPos,
                                                const juce::Slider::SliderStyle style, juce::Slider& slider)
{
    juce::ignoreUnused (minSliderPos, maxSliderPos, style);

    auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height);
    const float trackHeight = juce::jmin (8.0f, bounds.getHeight() * 0.35f);
    auto trackBounds = bounds.withSizeKeepingCentre (bounds.getWidth(), trackHeight);

    g.setColour (trackColour);
    g.fillRoundedRectangle (trackBounds, trackHeight * 0.5f);

    const float filledWidth = juce::jlimit (0.0f, bounds.getWidth(), sliderPos - bounds.getX());
    auto filledBounds = trackBounds.withWidth (filledWidth);
    g.setColour (accentColour);
    g.fillRoundedRectangle (filledBounds, trackHeight * 0.5f);

    const float thumbRadius = juce::jmin (bounds.getHeight() * 0.5f, 9.0f);
    g.setColour (slider.isEnabled() ? accentColour.brighter (0.3f) : trackColour);
    g.fillEllipse (sliderPos - thumbRadius, bounds.getCentreY() - thumbRadius,
                    thumbRadius * 2.0f, thumbRadius * 2.0f);
    g.setColour (backgroundColour);
    g.drawEllipse (sliderPos - thumbRadius, bounds.getCentreY() - thumbRadius,
                    thumbRadius * 2.0f, thumbRadius * 2.0f, 1.5f);
}

} // namespace livellatore
