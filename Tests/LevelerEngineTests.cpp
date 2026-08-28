#include "dsp/LevelerEngine.h"
#include "dsp/LoudnessMeter.h"
#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>

using namespace livellatore;

namespace
{
    constexpr double testSampleRate = 48000.0;

    void appendSine (juce::AudioBuffer<float>& dest, double frequency, double sampleRate,
                      double amplitude, double& phase)
    {
        const double phaseIncrement = juce::MathConstants<double>::twoPi * frequency / sampleRate;
        for (int i = 0; i < dest.getNumSamples(); ++i)
        {
            const float sample = (float) (amplitude * std::sin (phase));
            phase += phaseIncrement;
            for (int ch = 0; ch < dest.getNumChannels(); ++ch)
                dest.setSample (ch, i, sample);
        }
    }

    juce::dsp::ProcessSpec makeSpec()
    {
        return { testSampleRate, (juce::uint32) 512, (juce::uint32) 2 };
    }
}

/**
 * Test di integrazione sulla catena completa (LevelerEngine), non solo sui
 * singoli componenti DSP. Copre la parte oggettivamente misurabile di #8
 * (convergenza della loudness su materiale non stazionario, comportamento
 * del limiter su transienti che il rider non riesce a seguire) — NON il
 * confronto A/B percettivo contro TriLeveler su materiale audio reale, che
 * resta un giudizio umano non automatizzabile (vedi nota di chiusura di
 * #8): qui il "materiale" e' comunque sintetico, non registrazioni reali.
 */
class LevelerEngineTests : public juce::UnitTest
{
public:
    LevelerEngineTests() : juce::UnitTest ("LevelerEngine", "integration") {}

    void runTest() override
    {
        beginTest ("Converge la loudness di uscita verso il target su materiale con dinamica non stazionaria");
        {
            LevelerEngine engine;
            engine.prepare (makeSpec());
            engine.setTargetLufs (-16.0f);
            engine.setAttackMs (100.0f);
            engine.setReleaseMs (400.0f);
            engine.setLimiterThresholdDb (-0.3f);

            LoudnessMeter outputMeter;
            outputMeter.prepare (testSampleRate, 2);
            outputMeter.setWindowSeconds (3.0f);

            const int burstSamples = (int) (0.2 * testSampleRate);
            const int gapSamples = (int) (0.1 * testSampleRate);
            const int numCycles = 30;
            const double amplitude = 0.03; // segnale debole: richiede boost per arrivare al target

            double phase = 0.0;
            juce::AudioBuffer<float> block;

            for (int cycle = 0; cycle < numCycles; ++cycle)
            {
                for (int remaining = burstSamples; remaining > 0; )
                {
                    const int n = juce::jmin (512, remaining);
                    block.setSize (2, n, false, false, true);
                    appendSine (block, 220.0, testSampleRate, amplitude, phase);
                    engine.process (block);
                    outputMeter.pushBlock (block);
                    remaining -= n;
                }

                for (int remaining = gapSamples; remaining > 0; )
                {
                    const int n = juce::jmin (512, remaining);
                    block.setSize (2, n, false, false, true);
                    block.clear();
                    engine.process (block);
                    outputMeter.pushBlock (block);
                    remaining -= n;
                }
            }

            const float measuredLufs = outputMeter.getLoudnessLufs();
            expectWithinAbsoluteError (measuredLufs, -16.0f, 2.5f,
                "Atteso output vicino al target -16 LUFS su materiale non stazionario, ottenuto "
                    + juce::String (measuredLufs, 2));
        }

        beginTest ("Un click piu' rapido dell'attack del rider viene comunque contenuto dal limiter");
        {
            // Il gain rider lavora con tempi "musicali" (qui attack
            // deliberatamente lento, 500ms): non puo' reagire a un click di
            // pochi campioni. La rete di sicurezza e' il limiter (#9): la
            // catena completa non deve superare la sua soglia anche quando
            // il rider e' ancora a riposo (nessuna correzione applicata).
            LevelerEngine engine;
            engine.prepare (makeSpec());
            engine.setTargetLufs (-16.0f);
            engine.setAttackMs (500.0f);
            engine.setReleaseMs (500.0f);
            engine.setLimiterThresholdDb (-3.0f);

            juce::AudioBuffer<float> block (2, 512);
            block.clear();
            block.setSample (0, 10, 1.0f);
            block.setSample (1, 10, 1.0f);
            engine.process (block);

            // Silenzio a seguire, abbastanza lungo da far emergere dal
            // lookahead del limiter il campione ritardato del click.
            juce::AudioBuffer<float> tail (2, 4096);
            tail.clear();
            engine.process (tail);

            const float peakDb = juce::Decibels::gainToDecibels (
                juce::jmax (block.getMagnitude (0, block.getNumSamples()),
                            tail.getMagnitude (0, tail.getNumSamples())));

            expect (peakDb <= -3.0f + 0.2f,
                    "Atteso picco contenuto entro la soglia del limiter (-3dB), ottenuto "
                        + juce::String (peakDb, 3));
        }
    }
};

static LevelerEngineTests levelerEngineTests;
