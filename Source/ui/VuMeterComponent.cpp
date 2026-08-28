#include "VuMeterComponent.h"
#include "LivellatoreLookAndFeel.h"

namespace livellatore
{

VuMeterComponent::VuMeterComponent (juce::String label, float minDb, float maxDb, bool bipolarMeter)
    : labelText (std::move (label)), rangeMinDb (minDb), rangeMaxDb (maxDb), bipolar (bipolarMeter)
{
    displayedLevelDb = bipolar ? 0.0f : -100.0f;
    startTimerHz (30);
}

void VuMeterComponent::timerCallback()
{
    const float target = targetLevelDb.load();

    if (bipolar)
    {
        // Il gain del rider e' gia' "lisciato" a monte da attack/release:
        // uno smoothing simmetrico evita che il ballistic asimmetrico
        // (pensato per livelli di picco, sale subito/scende piano) distorca
        // una lettura che puo' muoversi in entrambe le direzioni.
        displayedLevelDb += (target - displayedLevelDb) * 0.3f;
    }
    else if (target > displayedLevelDb)
    {
        // Ballistics: sale subito, scende con un piccolo decay per leggibilità.
        displayedLevelDb = target;
    }
    else
    {
        displayedLevelDb += (target - displayedLevelDb) * 0.3f;
    }

    repaint();
}

void VuMeterComponent::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    auto labelBounds = bounds.removeFromBottom (16.0f);
    auto meterBounds = bounds.reduced (2.0f);

    g.setColour (LivellatoreLookAndFeel::trackColour);
    g.fillRoundedRectangle (meterBounds, 3.0f);

    if (bipolar)
    {
        const float clamped = juce::jlimit (rangeMinDb, rangeMaxDb, displayedLevelDb);
        const float zeroNormalised = juce::jlimit (0.0f, 1.0f, (0.0f - rangeMinDb) / (rangeMaxDb - rangeMinDb));
        const float valueNormalised = juce::jlimit (0.0f, 1.0f, (clamped - rangeMinDb) / (rangeMaxDb - rangeMinDb));

        const float zeroY = meterBounds.getBottom() - zeroNormalised * meterBounds.getHeight();
        const float valueY = meterBounds.getBottom() - valueNormalised * meterBounds.getHeight();

        auto fillBounds = juce::Rectangle<float>::leftTopRightBottom (
            meterBounds.getX(), juce::jmin (zeroY, valueY), meterBounds.getRight(), juce::jmax (zeroY, valueY));

        g.setColour (clamped >= 0.0f ? LivellatoreLookAndFeel::accentColour : juce::Colours::orange);
        g.fillRoundedRectangle (fillBounds, 3.0f);

        g.setColour (LivellatoreLookAndFeel::textColour.withAlpha (0.6f));
        g.drawLine (meterBounds.getX(), zeroY, meterBounds.getRight(), zeroY, 1.5f);
    }
    else
    {
        const float normalised = juce::jlimit (0.0f, 1.0f,
            (displayedLevelDb - rangeMinDb) / (rangeMaxDb - rangeMinDb));
        auto fillBounds = meterBounds.withTop (meterBounds.getBottom() - normalised * meterBounds.getHeight());

        juce::ColourGradient gradient (juce::Colours::green, meterBounds.getBottomLeft(),
                                        juce::Colours::red, meterBounds.getTopLeft(), false);
        gradient.addColour (0.7, LivellatoreLookAndFeel::accentColour);
        g.setGradientFill (gradient);
        g.fillRoundedRectangle (fillBounds, 3.0f);
    }

    g.setColour (LivellatoreLookAndFeel::textColour);
    g.setFont (juce::Font (juce::FontOptions (11.0f)));
    g.drawText (labelText, labelBounds, juce::Justification::centred);
}

} // namespace livellatore
