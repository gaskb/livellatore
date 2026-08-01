#include "dsp/LoudnessMeter.h"
#include "dsp/GainRider.h"
#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>

using namespace livellatore;

namespace
{
    constexpr double testSampleRate = 48000.0;

    juce::AudioBuffer<float> makeSineBlock (int numSamples, double frequency, double sampleRate,
                                             double& phase)
    {
        juce::AudioBuffer<float> buffer (2, numSamples);
        const double phaseIncrement = juce::MathConstants<double>::twoPi * frequency / sampleRate;
        for (int i = 0; i < numSamples; ++i)
        {
            const float sample = (float) std::sin (phase);
            phase += phaseIncrement;
            buffer.setSample (0, i, sample);
            buffer.setSample (1, i, sample);
        }
        return buffer;
    }
}

class LoudnessMeterTests : public juce::UnitTest
{
public:
    LoudnessMeterTests() : juce::UnitTest ("LoudnessMeter", "dsp") {}

    void runTest() override
    {
        beginTest ("Silenzio produce loudness al pavimento");
        {
            LoudnessMeter meter;
            meter.prepare (testSampleRate, 2);

            juce::AudioBuffer<float> silence (2, 512);
            silence.clear();
            for (int i = 0; i < 20; ++i)
                meter.pushBlock (silence);

            expectLessThan (meter.getLoudnessLufs(), -90.0f);
        }

        beginTest ("Sinusoide 1kHz a 0dBFS ha loudness prossima a -3 LUFS");
        {
            LoudnessMeter meter;
            meter.prepare (testSampleRate, 2);
            meter.setWindowSeconds (3.0f);

            double phase = 0.0;
            // Riempi la finestra (3s) di segnale stazionario
            for (int i = 0; i < 300; ++i)
            {
                auto block = makeSineBlock (512, 1000.0, testSampleRate, phase);
                meter.pushBlock (block);
            }

            const float lufs = meter.getLoudnessLufs();
            expect (lufs > -4.5f && lufs < -2.5f,
                    "Atteso circa -3 LUFS, ottenuto " + juce::String (lufs));
        }
    }
};

class GainRiderTests : public juce::UnitTest
{
public:
    GainRiderTests() : juce::UnitTest ("GainRider", "dsp") {}

    void runTest() override
    {
        beginTest ("Il gain converge verso la correzione necessaria (attack)");
        {
            GainRider rider;
            rider.prepare (testSampleRate);
            rider.setTargetLufs (-16.0f);
            rider.setAttackMs (50.0f);
            rider.setReleaseMs (500.0f);

            float gainDb = 0.0f;
            // Segnale a -26 LUFS: serve +10dB di correzione
            for (int i = 0; i < 500; ++i)
                gainDb = rider.computeGainDb (-26.0f, 0.01);

            expect (gainDb > 8.0f && gainDb <= 10.0f,
                    "Atteso gain vicino a +10dB, ottenuto " + juce::String (gainDb));
        }

        beginTest ("Il gain rilassa verso 0 quando la correzione non serve piu (release)");
        {
            GainRider rider;
            rider.prepare (testSampleRate);
            rider.setTargetLufs (-16.0f);
            rider.setAttackMs (50.0f);
            rider.setReleaseMs (200.0f);

            for (int i = 0; i < 500; ++i)
                rider.computeGainDb (-26.0f, 0.01);

            float gainDb = rider.getCurrentGainDb();
            expectGreaterThan (gainDb, 8.0f);

            // Ora il livello è tornato al target: il gain deve rilassare verso 0
            for (int i = 0; i < 500; ++i)
                gainDb = rider.computeGainDb (-16.0f, 0.01);

            expectLessThan (std::abs (gainDb), 1.0f);
        }
    }
};

static LoudnessMeterTests loudnessMeterTests;
static GainRiderTests gainRiderTests;
