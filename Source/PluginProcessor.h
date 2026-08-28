#pragma once

#include "Parameters.h"
#include "PresetManager.h"
#include "dsp/LevelerEngine.h"
#include <juce_audio_processors/juce_audio_processors.h>

namespace livellatore
{

class LivellatoreAudioProcessor : public juce::AudioProcessor
{
public:
    LivellatoreAudioProcessor();
    ~LivellatoreAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    LevelerEngine& getEngine() noexcept { return engine; }

    /** Applica un preset di fabbrica (indice in PresetManager::getFactoryPresets()). */
    void applyFactoryPreset (int index);
    /** Carica un preset utente salvato su disco; false se non trovato/non valido. */
    bool loadUserPreset (const juce::String& name);
    /** Salva/sovrascrive lo stato corrente come preset utente. */
    bool saveCurrentStateAsPreset (const juce::String& name);

    PresetManager presetManager { PresetManager::makeDefaultPresetDirectory (JucePlugin_Manufacturer, JucePlugin_Name) };

private:
    void updateEngineParametersFromState();

    LevelerEngine engine;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LivellatoreAudioProcessor)
};

} // namespace livellatore
