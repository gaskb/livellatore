#pragma once

#include "PluginProcessor.h"
#include "ui/LivellatoreLookAndFeel.h"
#include "ui/VuMeterComponent.h"
#include "ui/LevelSliderComponent.h"

namespace livellatore
{

class LivellatoreAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit LivellatoreAudioProcessorEditor (LivellatoreAudioProcessor&);
    ~LivellatoreAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    LivellatoreAudioProcessor& processorRef;
    LivellatoreLookAndFeel lookAndFeel;

    VuMeterComponent inputMeter { "IN", -60.0f, 6.0f };
    VuMeterComponent outputMeter { "OUT", -60.0f, 6.0f };
    VuMeterComponent riderActivityMeter { "RIDER", -24.0f, 24.0f };

    LevelSliderComponent inputGainSlider, targetLevelSlider, attackSlider,
                          releaseSlider, limiterSlider, outputGainSlider;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LivellatoreAudioProcessorEditor)
};

} // namespace livellatore
