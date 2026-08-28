#pragma once

#include <juce_data_structures/juce_data_structures.h>
#include <vector>

namespace livellatore
{

/**
 * Preset di fabbrica: nome + valori grezzi dei parametri (ParamID -> valore,
 * vedi Parameters.h). Punti di partenza ragionevoli (per le coppie
 * attack/release vedi GainRider::presetVoice/presetMasterBus), NON
 * validati su materiale reale (stesso limite documentato nella chiusura
 * di #3).
 */
struct FactoryPreset
{
    juce::String name;
    std::vector<std::pair<juce::String, float>> paramValues;
};

/**
 * Gestione preset utente come file XML su disco (issue #5).
 *
 * Meccanismo scelto rispetto alle alternative valutate:
 *  - program list nativo VST3/AU: comportamento diverso per formato e per
 *    host, più complesso da testare in isolamento;
 *  - un concetto di "program" costruito sopra APVTS: APVTS non lo
 *    supporta nativamente, andrebbe comunque implementato con un
 *    meccanismo equivalente a questo sopra.
 * File XML in una cartella standard per-utente riusa lo stesso ValueTree
 * già serializzato da getStateInformation/setStateInformation ed è
 * uniforme fra i tre formati (VST3/AU/Standalone).
 *
 * Non conosce APVTS/AudioProcessor direttamente: riceve/restituisce solo
 * juce::ValueTree, per restare testabile senza istanziare un plugin
 * completo (stessa filosofia di Source/dsp/).
 */
class PresetManager
{
public:
    /** La cartella è iniettabile per i test; in produzione si usa
     * makeDefaultPresetDirectory. Viene creata se non esiste già. */
    explicit PresetManager (juce::File directoryToUse);

    static juce::File makeDefaultPresetDirectory (const juce::String& companyName,
                                                   const juce::String& productName);

    juce::File getPresetDirectory() const noexcept { return presetDirectory; }

    /** Nomi dei preset utente salvati su disco (senza estensione), ordine alfabetico. */
    juce::StringArray getUserPresetNames() const;

    /** Salva/sovrascrive un preset utente. Ritorna false se la scrittura fallisce. */
    bool savePreset (const juce::String& name, const juce::ValueTree& state) const;

    /** Elimina un preset utente. Ritorna false se il file non esiste o non è cancellabile. */
    bool deletePreset (const juce::String& name) const;

    /** ValueTree invalida (ValueTree()) se il preset non esiste o non è leggibile. */
    juce::ValueTree loadPreset (const juce::String& name) const;

    static const std::vector<FactoryPreset>& getFactoryPresets();

private:
    juce::File fileForPreset (const juce::String& name) const;

    juce::File presetDirectory;
};

} // namespace livellatore
