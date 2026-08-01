#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace livellatore
{

/**
 * LookAndFeel ispirato allo stile di TriLeveler (vedi temp/ nella root del
 * repo): sfondo scuro, slider orizzontali "a barra" con thumb circolare,
 * accenti verde acqua.
 */
class LivellatoreLookAndFeel : public juce::LookAndFeel_V4
{
public:
    LivellatoreLookAndFeel();

    void drawLinearSlider (juce::Graphics&, int x, int y, int width, int height,
                            float sliderPos, float minSliderPos, float maxSliderPos,
                            const juce::Slider::SliderStyle, juce::Slider&) override;

    static const juce::Colour backgroundColour;
    static const juce::Colour trackColour;
    static const juce::Colour accentColour;
    static const juce::Colour textColour;
};

} // namespace livellatore
