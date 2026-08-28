#include "dsp/Limiter.h"
#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>

using namespace livellatore;

namespace
{
    constexpr double testSampleRate = 48000.0;

    juce::dsp::ProcessSpec makeSpec (int numChannels = 2)
    {
        return { testSampleRate, 512, (juce::uint32) numChannels };
    }
}

class LimiterTests : public juce::UnitTest
{
public:
    LimiterTests() : juce::UnitTest ("Limiter", "dsp") {}

    void runTest() override
    {
        beginTest ("Un impulso a 0dBFS non supera la soglia impostata (issue #9)");
        {
            // Prima del lookahead, misurato empiricamente che un impulso a
            // 0dBFS attraversava il vecchio wrapper su juce::dsp::Limiter
            // del tutto inalterato (0dB di riduzione anziche' -6dB) per
            // transienti larghi fino ad almeno 100 campioni: il suo "hard
            // clipper" e' fisso a 0dBFS, non alla soglia impostata.
            const float thresholdDb = -6.0f;

            Limiter limiter;
            limiter.prepare (makeSpec (1));
            limiter.setThresholdDb (thresholdDb);
            limiter.setReleaseMs (50.0f);

            const int bufferLength = limiter.getLatencySamples() * 3;
            juce::AudioBuffer<float> buffer (1, bufferLength);
            buffer.clear();
            buffer.setSample (0, 10, 1.0f);

            limiter.process (buffer);

            const float peakDb = juce::Decibels::gainToDecibels (buffer.getMagnitude (0, bufferLength));
            expectWithinAbsoluteError (peakDb, thresholdDb, 0.1f,
                "Atteso picco vicino a " + juce::String (thresholdDb) + "dB, ottenuto " + juce::String (peakDb, 3));
        }

        beginTest ("Transienti di larghezza crescente restano tutti entro la soglia (issue #9)");
        {
            const float thresholdDb = -6.0f;

            for (const int widthSamples : { 1, 2, 5, 10, 20, 50, 100, 200, 500 })
            {
                Limiter limiter;
                limiter.prepare (makeSpec (1));
                limiter.setThresholdDb (thresholdDb);
                limiter.setReleaseMs (50.0f);

                const int bufferLength = limiter.getLatencySamples() * 2 + widthSamples + 100;
                juce::AudioBuffer<float> buffer (1, bufferLength);
                buffer.clear();
                for (int i = 0; i < widthSamples; ++i)
                    buffer.setSample (0, 10 + i, 1.0f);

                limiter.process (buffer);

                const float peakDb = juce::Decibels::gainToDecibels (buffer.getMagnitude (0, bufferLength));
                expect (peakDb <= thresholdDb + 0.1f,
                        "Transiente largo " + juce::String (widthSamples) + " campioni: atteso picco <= "
                            + juce::String (thresholdDb) + "dB, ottenuto " + juce::String (peakDb, 3));
            }
        }

        beginTest ("getGainReductionDb e' sample-accurate, non un'approssimazione a picco di blocco");
        {
            const float thresholdDb = -6.0f;

            Limiter limiter;
            limiter.prepare (makeSpec (1));
            limiter.setThresholdDb (thresholdDb);
            limiter.setReleaseMs (50.0f);

            const int bufferLength = limiter.getLatencySamples() * 2;
            juce::AudioBuffer<float> buffer (1, bufferLength);
            buffer.clear();
            buffer.setSample (0, 5, 1.0f); // 0dBFS contro soglia -6dB -> -6dB di riduzione esatta

            limiter.process (buffer);

            expectWithinAbsoluteError (limiter.getGainReductionDb(), thresholdDb, 0.05f,
                "Attesa GR " + juce::String (thresholdDb) + "dB, ottenuta "
                    + juce::String (limiter.getGainReductionDb(), 3));
        }

        beginTest ("Nessuna riduzione quando il segnale resta sotto soglia");
        {
            Limiter limiter;
            limiter.prepare (makeSpec (1));
            limiter.setThresholdDb (-6.0f);
            limiter.setReleaseMs (50.0f);

            juce::AudioBuffer<float> buffer (1, 512);
            for (int i = 0; i < 512; ++i)
                buffer.setSample (0, i, 0.2f); // -14dBFS, ben sotto la soglia di -6dB

            limiter.process (buffer);

            expectWithinAbsoluteError (limiter.getGainReductionDb(), 0.0f, 0.01f);
        }

        beginTest ("Stereo-linked: un picco su un canale riduce anche l'altro canale");
        {
            const float thresholdDb = -6.0f;
            const float thresholdLinear = juce::Decibels::decibelsToGain (thresholdDb);

            Limiter limiter;
            limiter.prepare (makeSpec (2));
            limiter.setThresholdDb (thresholdDb);
            limiter.setReleaseMs (50.0f);

            const int bufferLength = limiter.getLatencySamples() * 3;
            juce::AudioBuffer<float> buffer (2, bufferLength);
            buffer.clear();
            buffer.setSample (0, 10, 1.0f);  // impulso solo a sinistra
            buffer.setSample (1, 10, 0.4f);  // segnale moderato a destra, da solo sotto soglia

            limiter.process (buffer);

            const int delayedIndex = 10 + limiter.getLatencySamples();
            const float rightOutput = buffer.getSample (1, delayedIndex);
            const float expectedRightOutput = 0.4f * thresholdLinear;

            expectWithinAbsoluteError (rightOutput, expectedRightOutput, 0.01f,
                "Il canale destro dovrebbe essere ridotto dal picco rilevato a sinistra (stereo-linked)");
        }

        beginTest ("Il gain rilascia gradualmente verso 1.0 dopo un transiente (no scatto istantaneo)");
        {
            const float thresholdDb = -6.0f;
            const float probeAmplitude = 0.3f; // sotto soglia (-6dB = ~0.501) da sola

            Limiter limiter;
            limiter.prepare (makeSpec (1));
            limiter.setThresholdDb (thresholdDb);
            limiter.setReleaseMs (100.0f);

            // Impulso a 0dBFS seguito da un segnale sostenuto sotto soglia
            // ("sonda"): misurando il rapporto uscita/ingresso della sonda
            // nel tempo si osserva il gain reale applicato campione per
            // campione, cosa che getGainReductionDb() (un minimo per
            // l'intero blocco) non permette di per se'.
            const int bufferLength = limiter.getLatencySamples() + (int) (testSampleRate * 0.05);
            juce::AudioBuffer<float> buffer (1, bufferLength);
            buffer.clear();
            buffer.setSample (0, 0, 1.0f);
            for (int i = 1; i < bufferLength; ++i)
                buffer.setSample (0, i, probeAmplitude);

            limiter.process (buffer);

            const int lookahead = limiter.getLatencySamples();
            const float gainRightAfterWindow = std::abs (buffer.getSample (0, lookahead + 1)) / probeAmplitude;
            const float gainLater = std::abs (buffer.getSample (0, bufferLength - 1)) / probeAmplitude;

            expectLessThan (gainRightAfterWindow, 1.0f);
            expectGreaterThan (gainLater, gainRightAfterWindow);
        }
    }
};

static LimiterTests limiterTests;
