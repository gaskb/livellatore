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
    // Numero di versione (JucePlugin_VersionString, singola fonte di
    // verita' project(Livellatore VERSION X.Y.Z) nel CMakeLists.txt),
    // mostrato in-GUI invece che nella barra del titolo del sistema: e'
    // l'unico posto che funziona identico su Standalone/VST3/AU, dato che
    // la barra del titolo la disegna l'host (o, per la Standalone, il
    // bootstrap di JUCE stesso — non modificabile senza rimpiazzare
    // interamente StandaloneFilterApp, sproporzionato per un dettaglio
    // cosmetico).
    juce::Label versionLabel;
    juce::StringArray userPresetNamesById; // indicizzato da (itemId - userPresetIdBase)
    std::unique_ptr<juce::AlertWindow> savePresetDialog;

    VuMeterComponent inputMeter { "IN", -60.0f, 6.0f };
    VuMeterComponent outputMeter { "OUT", -60.0f, 6.0f };
    VuMeterComponent riderActivityMeter { "RIDER", -24.0f, 24.0f, true };
    // Quanto sta lavorando il limiter (issue #11): a riposo (0dB di
    // riduzione) il meter e' vuoto, si riempie quando interviene. Riusa il
    // meter unipolare standard passandogli il valore assoluto della GR
    // (sempre <= 0), invece di aggiungere una modalita' dedicata.
    VuMeterComponent limiterGrMeter { "LIM GR", 0.0f, 24.0f };

    // Readout numerici (issue #4): i meter grafici da soli non danno un
    // valore leggibile al volo, specie per la loudness corrente.
    juce::Label currentLoudnessLabel, riderGainLabel;

    LevelSliderComponent inputGainSlider, targetLevelSlider, attackSlider,
                          releaseSlider, gateThresholdSlider, limiterSlider, outputGainSlider;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LivellatoreAudioProcessorEditor)
};

} // namespace livellatore
