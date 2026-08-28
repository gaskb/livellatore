#include "dsp/LoudnessMeter.h"
#include "dsp/GainRider.h"
#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <complex>

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

    // Risposta in frequenza di un biquad in forma diretta normalizzata
    // (a0 = 1): H(z) = (b0 + b1 z^-1 + b2 z^-2) / (1 + a1 z^-1 + a2 z^-2).
    std::complex<double> biquadResponse (double b0, double b1, double b2,
                                          double a1, double a2,
                                          double frequency, double sampleRate)
    {
        const double w = juce::MathConstants<double>::twoPi * frequency / sampleRate;
        const std::complex<double> z1 = std::polar (1.0, -w);
        const std::complex<double> z2 = std::polar (1.0, -2.0 * w);
        const std::complex<double> num = b0 + b1 * z1 + b2 * z2;
        const std::complex<double> den = 1.0 + a1 * z1 + a2 * z2;
        return num / den;
    }

    // Ricalcolo indipendente (stessa formula di
    // LoudnessMeter::makeKWeightingFilters, ITU-R BS.1770-4 Annex 1) del
    // guadagno combinato pre-filter + RLB alla frequenza data. Serve a
    // validare che il filtro IIR realizzato a runtime (coefficienti float,
    // elaborazione per-sample) riproduca davvero la risposta K-weighting
    // attesa su tutto lo spettro, non solo vicino a 1kHz.
    double expectedKWeightingGainLinear (double frequency, double sampleRate)
    {
        double preB0, preB1, preB2, preA1, preA2;
        {
            const double f0 = 1681.9744509555319;
            const double G  = 3.99984385397;
            const double Q  = 0.7071752369554193;
            const double K  = std::tan (juce::MathConstants<double>::pi * f0 / sampleRate);
            const double Vh = std::pow (10.0, G / 20.0);
            const double Vb = std::pow (Vh, 0.4996667741545416);
            const double a0 = 1.0 + K / Q + K * K;
            preB0 = (Vh + Vb * K / Q + K * K) / a0;
            preB1 = 2.0 * (K * K - Vh) / a0;
            preB2 = (Vh - Vb * K / Q + K * K) / a0;
            preA1 = 2.0 * (K * K - 1.0) / a0;
            preA2 = (1.0 - K / Q + K * K) / a0;
        }

        double rlbB0, rlbB1, rlbB2, rlbA1, rlbA2;
        {
            const double f0 = 38.13547087602444;
            const double Q  = 0.5003270373238773;
            const double K  = std::tan (juce::MathConstants<double>::pi * f0 / sampleRate);
            const double a0 = 1.0 + K / Q + K * K;
            // Il numeratore del passa-alto RLB resta (1, -2, 1) senza
            // ulteriore normalizzazione per a0: e' il valore pubblicato in
            // BS.1770-4 Annex 1 (stesso usato da libebur128/ffmpeg ebur128/
            // pyloudnorm), a differenza dello shelving stage sopra.
            rlbB0 = 1.0;
            rlbB1 = -2.0;
            rlbB2 = 1.0;
            rlbA1 = 2.0 * (K * K - 1.0) / a0;
            rlbA2 = (1.0 - K / Q + K * K) / a0;
        }

        const auto preResponse = biquadResponse (preB0, preB1, preB2, preA1, preA2, frequency, sampleRate);
        const auto rlbResponse = biquadResponse (rlbB0, rlbB1, rlbB2, rlbA1, rlbA2, frequency, sampleRate);
        return std::abs (preResponse) * std::abs (rlbResponse);
    }

    // LUFS attesa per una sinusoide a piena scala (0dBFS di picco) dopo
    // K-weighting con guadagno lineare noto: RMS di una sinusoide piena
    // scala e' 1/sqrt(2) (-3.0103 dBFS), poi -0.691 e' l'offset K-weighting
    // di BS.1770.
    float expectedLufsForFullScaleSine (double frequency, double sampleRate)
    {
        const double gainLinear = expectedKWeightingGainLinear (frequency, sampleRate);
        const double meanSquare = (gainLinear * gainLinear) / 2.0;
        return (float) (-0.691 + 10.0 * std::log10 (meanSquare));
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

        beginTest ("Risposta K-weighting multi-frequenza corrisponde al design atteso (issue #1)");
        {
            // Copre basso (attenuato dal passa-alto RLB), medio (~piatto) e
            // alto (boost dello shelf): un bug di segno o di coefficiente
            // in una sola delle due bande non verrebbe rilevato testando
            // solo 1kHz.
            const double frequencies[] = { 60.0, 200.0, 1000.0, 3000.0, 8000.0 };

            for (const double frequency : frequencies)
            {
                LoudnessMeter meter;
                meter.prepare (testSampleRate, 2);
                meter.setWindowSeconds (3.0f);

                double phase = 0.0;
                for (int i = 0; i < 300; ++i)
                {
                    auto block = makeSineBlock (512, frequency, testSampleRate, phase);
                    meter.pushBlock (block);
                }

                const float measured = meter.getLoudnessLufs();
                const float expected = expectedLufsForFullScaleSine (frequency, testSampleRate);

                expectWithinAbsoluteError (measured, expected, 0.3f,
                    "A " + juce::String (frequency) + "Hz atteso " + juce::String (expected)
                        + " LUFS, ottenuto " + juce::String (measured));
            }
        }

        beginTest ("La finestra scorrevole dimentica un transiente forte dopo windowSeconds di silenzio (issue #1)");
        {
            // Segnale non stazionario: un burst forte seguito da silenzio.
            // Verifica che la finestra rettangolare "dimentichi" gradualmente
            // il burst (non un reset istantaneo) e converga al pavimento una
            // volta che il burst e' interamente uscito dalla finestra.
            LoudnessMeter meter;
            meter.prepare (testSampleRate, 2);
            meter.setWindowSeconds (1.0f);

            double phase = 0.0;
            for (int i = 0; i < 100; ++i) // ~1.06s di segnale forte, riempie la finestra
            {
                auto block = makeSineBlock (512, 1000.0, testSampleRate, phase);
                meter.pushBlock (block);
            }
            const float loudLufs = meter.getLoudnessLufs();
            expectGreaterThan (loudLufs, -10.0f);

            juce::AudioBuffer<float> silence (2, 512);
            silence.clear();

            // A meta' del periodo di svuotamento la finestra e' un mix di
            // burst residuo e silenzio: ne' al livello iniziale ne' al
            // pavimento.
            for (int i = 0; i < 50; ++i)
                meter.pushBlock (silence);
            const float midLufs = meter.getLoudnessLufs();
            expect (midLufs < loudLufs - 2.0f && midLufs > -90.0f,
                    "Atteso un valore intermedio, ottenuto " + juce::String (midLufs));

            // Dopo windowSeconds pieni di silenzio il burst e' uscito
            // interamente dalla finestra.
            for (int i = 0; i < 60; ++i)
                meter.pushBlock (silence);
            expectLessThan (meter.getLoudnessLufs(), -90.0f);
        }

        beginTest ("Dialogue Mode: la loudness resta stabile durante una pausa nel parlato, a differenza del default");
        {
            // Finestra da 3s (default), parlato breve (~0.43s) seguito da
            // una pausa lunga (~2.13s) MA che restando sotto i 3s totali
            // non fa uscire il parlato dalla finestra per eviction: cosi'
            // l'unica differenza fra i due meter e' il gating, non quanto
            // parlato resta fisicamente nella finestra.
            // Drop atteso senza gating: 10*log10(40/(40+200)) ~= -7.8dB.
            LoudnessMeter ungatedMeter, gatedMeter;
            for (auto* meter : { &ungatedMeter, &gatedMeter })
                meter->prepare (testSampleRate, 2);
            gatedMeter.setGatingEnabled (true);
            gatedMeter.setGateThresholdLufs (-40.0f);

            double phase1 = 0.0, phase2 = 0.0;
            for (int i = 0; i < 40; ++i)
            {
                auto block = makeSineBlock (512, 200.0, testSampleRate, phase1);
                ungatedMeter.pushBlock (block);

                auto block2 = makeSineBlock (512, 200.0, testSampleRate, phase2);
                gatedMeter.pushBlock (block2);
            }
            const float speechLufs = gatedMeter.getLoudnessLufs();
            expectWithinAbsoluteError (ungatedMeter.getLoudnessLufs(), speechLufs, 0.1f);

            juce::AudioBuffer<float> silence (2, 512);
            silence.clear();
            for (int i = 0; i < 200; ++i) // sotto la soglia di gate (-40 LUFS, il pavimento e' -100)
            {
                ungatedMeter.pushBlock (silence);
                gatedMeter.pushBlock (silence);
            }

            expect (ungatedMeter.getLoudnessLufs() < speechLufs - 5.0f,
                    "Senza Dialogue Mode la pausa dovrebbe diluire visibilmente la media, ottenuto "
                        + juce::String (ungatedMeter.getLoudnessLufs()) + " (parlato: " + juce::String (speechLufs) + ")");
            expectWithinAbsoluteError (gatedMeter.getLoudnessLufs(), speechLufs, 0.5f,
                "Con Dialogue Mode la loudness dovrebbe restare vicina a quella del parlato ("
                    + juce::String (speechLufs) + "), ottenuto " + juce::String (gatedMeter.getLoudnessLufs()));
        }

        beginTest ("Dialogue Mode: pausa piu' lunga dell'intera finestra scende comunque al pavimento");
        {
            LoudnessMeter meter;
            meter.prepare (testSampleRate, 2);
            meter.setWindowSeconds (1.0f);
            meter.setGatingEnabled (true);
            meter.setGateThresholdLufs (-40.0f);

            double phase = 0.0;
            for (int i = 0; i < 100; ++i)
            {
                auto block = makeSineBlock (512, 200.0, testSampleRate, phase);
                meter.pushBlock (block);
            }
            expectGreaterThan (meter.getLoudnessLufs(), -40.0f);

            juce::AudioBuffer<float> silence (2, 512);
            silence.clear();
            for (int i = 0; i < 120; ++i) // oltre la finestra di 1s: il parlato esce del tutto
                meter.pushBlock (silence);

            expectLessThan (meter.getLoudnessLufs(), -90.0f);
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

        beginTest ("Gate con isteresi: non chiude/riapre per piccole oscillazioni intorno alla soglia (issue #2)");
        {
            GainRider rider;
            rider.prepare (testSampleRate);
            rider.setTargetLufs (-16.0f);
            rider.setAttackMs (50.0f);
            rider.setReleaseMs (200.0f);
            rider.setGateThresholdLufs (-60.0f); // banda di isteresi: [-61.5, -58.5]

            for (int i = 0; i < 200; ++i)
                rider.computeGainDb (-20.0f, 0.01);
            expect (rider.isGateOpen(), "Il gate dovrebbe essere aperto ben sopra soglia");

            for (int i = 0; i < 50; ++i)
                rider.computeGainDb (-60.0f, 0.01); // dentro la banda di isteresi
            expect (rider.isGateOpen(), "Il gate non dovrebbe chiudersi dentro la banda di isteresi");

            for (int i = 0; i < 10; ++i)
                rider.computeGainDb (-70.0f, 0.01); // sotto la soglia di chiusura
            expect (! rider.isGateOpen(), "Il gate dovrebbe chiudersi sotto la soglia di chiusura");

            for (int i = 0; i < 10; ++i)
                rider.computeGainDb (-60.0f, 0.01); // dentro la banda: non deve riaprirsi da solo
            expect (! rider.isGateOpen(), "Il gate non dovrebbe riaprirsi dentro la banda di isteresi");

            for (int i = 0; i < 10; ++i)
                rider.computeGainDb (-20.0f, 0.01); // sopra la soglia di apertura
            expect (rider.isGateOpen(), "Il gate dovrebbe riaprirsi sopra la soglia di apertura");
        }

        beginTest ("Gate chiuso: il gain rilassa verso 0 invece di inseguire il rumore di fondo");
        {
            GainRider rider;
            rider.prepare (testSampleRate);
            rider.setTargetLufs (-16.0f);
            rider.setAttackMs (50.0f);
            rider.setReleaseMs (200.0f);
            rider.setGateThresholdLufs (-60.0f);

            for (int i = 0; i < 500; ++i)
                rider.computeGainDb (-26.0f, 0.01);
            expectGreaterThan (rider.getCurrentGainDb(), 8.0f);

            // Il segnale scende sotto la soglia del gate: anche se e' ben
            // lontano dal target, il rider non deve inseguirlo.
            float gainDb = 0.0f;
            for (int i = 0; i < 500; ++i)
                gainDb = rider.computeGainDb (-75.0f, 0.01);

            expect (! rider.isGateOpen());
            expectLessThan (std::abs (gainDb), 1.0f);
        }

        beginTest ("Dead-band anti-chattering: fluttuazioni di 0.1 LUFS non cambiano fase attack/release (issue #3)");
        {
            GainRider rider;
            rider.prepare (testSampleRate);
            rider.setTargetLufs (-16.0f);
            rider.setAttackMs (20.0f);
            rider.setReleaseMs (1000.0f);
            rider.setGateThresholdLufs (-90.0f); // gate sempre aperto in questo test

            for (int i = 0; i < 300; ++i)
                rider.computeGainDb (-22.0f, 0.01);

            int phaseFlips = 0;
            bool previousPhase = rider.isAttackPhase();
            for (int i = 0; i < 200; ++i)
            {
                // +-0.1 LUFS intorno a -22: sposta la correzione desiderata
                // di +-0.1dB, sotto lo switchDeadbandDb (0.3dB).
                const float dither = (i % 2 == 0) ? -22.1f : -21.9f;
                rider.computeGainDb (dither, 0.01);
                const bool phase = rider.isAttackPhase();
                if (phase != previousPhase)
                    ++phaseFlips;
                previousPhase = phase;
            }

            expect (phaseFlips == 0,
                    "Il dead-band dovrebbe impedire cambi di fase per fluttuazioni di 0.1 LUFS ("
                        + juce::String (phaseFlips) + " cambi rilevati)");
        }
    }
};

static LoudnessMeterTests loudnessMeterTests;
static GainRiderTests gainRiderTests;
