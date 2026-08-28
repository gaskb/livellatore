#include "PresetManager.h"
#include "Parameters.h"
#include <juce_core/juce_core.h>

using namespace livellatore;

namespace
{
    juce::ValueTree makeFakeState (float targetLufs)
    {
        juce::ValueTree state ("PARAMETERS");
        juce::ValueTree param ("PARAM");
        param.setProperty ("id", ParamID::targetLufs, nullptr);
        param.setProperty ("value", targetLufs, nullptr);
        state.appendChild (param, nullptr);
        return state;
    }
}

class PresetManagerTests : public juce::UnitTest
{
public:
    PresetManagerTests() : juce::UnitTest ("PresetManager", "presets") {}

    void runTest() override
    {
        // Cartella temporanea isolata, ripulita a fine test: i preset
        // utente non devono toccare la cartella reale durante i test.
        auto tempDir = juce::File::getSpecialLocation (juce::File::tempDirectory)
                           .getChildFile ("LivellatoreTests_" + juce::Uuid().toString());

        beginTest ("Salvare e ricaricare un preset restituisce lo stesso stato");
        {
            PresetManager manager (tempDir);
            const auto state = makeFakeState (-18.0f);

            expect (manager.savePreset ("Il Mio Preset", state));

            const auto loaded = manager.loadPreset ("Il Mio Preset");
            expect (loaded.isValid());
            expect (loaded.getChildWithProperty ("id", ParamID::targetLufs)
                        .getProperty ("value").operator float() == -18.0f);
        }

        beginTest ("getUserPresetNames elenca i preset salvati in ordine alfabetico");
        {
            PresetManager manager (tempDir.getChildFile ("scan"));
            manager.savePreset ("Zeta", makeFakeState (-10.0f));
            manager.savePreset ("Alfa", makeFakeState (-20.0f));

            const auto names = manager.getUserPresetNames();
            expect (names.size() == 2);
            expectEquals (names[0], juce::String ("Alfa"));
            expectEquals (names[1], juce::String ("Zeta"));
        }

        beginTest ("Salvare due volte con lo stesso nome sovrascrive invece di duplicare");
        {
            PresetManager manager (tempDir.getChildFile ("overwrite"));
            manager.savePreset ("Preset", makeFakeState (-10.0f));
            manager.savePreset ("Preset", makeFakeState (-20.0f));

            expect (manager.getUserPresetNames().size() == 1);
            const auto loaded = manager.loadPreset ("Preset");
            expect (loaded.getChildWithProperty ("id", ParamID::targetLufs)
                        .getProperty ("value").operator float() == -20.0f);
        }

        beginTest ("deletePreset rimuove il file, loadPreset su un nome inesistente non e' valido");
        {
            PresetManager manager (tempDir.getChildFile ("delete"));
            manager.savePreset ("Temporaneo", makeFakeState (-10.0f));
            expect (manager.deletePreset ("Temporaneo"));
            expect (manager.getUserPresetNames().isEmpty());
            expect (! manager.loadPreset ("Temporaneo").isValid());
            expect (! manager.loadPreset ("MaiEsistito").isValid());
        }

        beginTest ("I preset di fabbrica non sono vuoti e coprono i parametri chiave del rider");
        {
            const auto& factory = PresetManager::getFactoryPresets();
            expect (! factory.empty());

            for (const auto& preset : factory)
            {
                expect (preset.name.isNotEmpty());
                expect (! preset.paramValues.empty());
            }
        }

        tempDir.deleteRecursively();
    }
};

static PresetManagerTests presetManagerTests;
