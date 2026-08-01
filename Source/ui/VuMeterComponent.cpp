#include "VuMeterComponent.h"
#include "LivellatoreLookAndFeel.h"

namespace livellatore
{

VuMeterComponent::VuMeterComponent (juce::String label, float minDb, float maxDb)
    : labelText (std::move (label)), rangeMinDb (minDb), rangeMaxDb (maxDb)
{
    startTimerHz (30);
}

void VuMeterComponent::timerCallback()
{
    const float target = targetLevelDb.load();
    // Ballistics: sale subito, scende con un piccolo decay per leggibilità.
    if (target > displayedLevelDb)
        displayedLevelDb = target;
    else
        displayedLevelDb += (target - displayedLevelDb) * 0.3f;

    repaint();
}

void VuMeterComponent::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    auto labelBounds = bounds.removeFromBottom (16.0f);
    auto meterBounds = bounds.reduced (2.0f);

    g.setColour (LivellatoreLookAndFeel::trackColour);
    g.fillRoundedRectangle (meterBounds, 3.0f);

    const float normalised = juce::jlimit (0.0f, 1.0f,
        (displayedLevelDb - rangeMinDb) / (rangeMaxDb - rangeMinDb));
    auto fillBounds = meterBounds.withTop (meterBounds.getBottom() - normalised * meterBounds.getHeight());

    juce::ColourGradient gradient (juce::Colours::green, meterBounds.getBottomLeft(),
                                    juce::Colours::red, meterBounds.getTopLeft(), false);
    gradient.addColour (0.7, LivellatoreLookAndFeel::accentColour);
    g.setGradientFill (gradient);
    g.fillRoundedRectangle (fillBounds, 3.0f);

    g.setColour (LivellatoreLookAndFeel::textColour);
    g.setFont (juce::Font (juce::FontOptions (11.0f)));
    g.drawText (labelText, labelBounds, juce::Justification::centred);
}

} // namespace livellatore
