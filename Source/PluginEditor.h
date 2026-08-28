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

    /** Ricostruisce la lista del preset box (preset di fabbrica + separatore
     * + preset utente scansionati da disco) senza notificare onChange. */
    void refreshPresetBox (int idToSelect = 0);
    void showSavePresetDialog();

    LivellatoreAudioProcessor& processorRef;
    LivellatoreLookAndFeel lookAndFeel;

    juce::ComboBox presetBox;
    juce::TextButton savePresetButton { "+" };
    juce::StringArray userPresetNamesById; // indicizzato da (itemId - userPresetIdBase)
    std::unique_ptr<juce::AlertWindow> savePresetDialog;

    VuMeterComponent inputMeter { "IN", -60.0f, 6.0f };
    VuMeterComponent outputMeter { "OUT", -60.0f, 6.0f };
    VuMeterComponent riderActivityMeter { "RIDER", -24.0f, 24.0f };

    LevelSliderComponent inputGainSlider, targetLevelSlider, attackSlider,
                          releaseSlider, gateThresholdSlider, limiterSlider, outputGainSlider;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LivellatoreAudioProcessorEditor)
};

} // namespace livellatore
