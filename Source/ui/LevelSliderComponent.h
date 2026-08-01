#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace livellatore
{

/** Slider orizzontale con etichetta e readout numerico, stile TriLeveler. */
class LevelSliderComponent : public juce::Component
{
public:
    LevelSliderComponent (juce::AudioProcessorValueTreeState& state,
                           const juce::String& paramID,
                           const juce::String& displayName);

    void resized() override;

private:
    juce::Label nameLabel;
    juce::Slider slider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};

} // namespace livellatore
