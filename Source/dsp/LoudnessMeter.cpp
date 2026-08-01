#include "LoudnessMeter.h"

namespace livellatore
{

void LoudnessMeter::prepare (double newSampleRate, int newNumChannels)
{
    sampleRate = newSampleRate;
    numChannels = newNumChannels;

    makeKWeightingFilters (sampleRate);

    preFilters.clear();
    rlbFilters.clear();
    for (int ch = 0; ch < numChannels; ++ch)
    {
        preFilters.emplace_back();
        preFilters.back().coefficients = preCoeffs;
        rlbFilters.emplace_back();
        rlbFilters.back().coefficients = rlbCoeffs;
    }

    reset();
}

void LoudnessMeter::reset()
{
    for (auto& f : preFilters) f.reset();
    for (auto& f : rlbFilters) f.reset();
    window.clear();
    windowSumSquares = 0.0;
    windowNumSamples = 0;
    currentLufs = -100.0f;
}

void LoudnessMeter::setWindowSeconds (float seconds)
{
    windowSeconds = juce::jmax (0.05f, seconds);
}

// Coefficienti derivati dalle formule di ITU-R BS.1770-4, Annex 1
// (le stesse usate da libebur128/ffmpeg ebur128), calcolate qui a runtime
// per essere indipendenti dal sample rate.
void LoudnessMeter::makeKWeightingFilters (double fs)
{
    // Stage 1: high-shelf boost (~ +4dB sopra ~1.68kHz)
    {
        const double f0 = 1681.9744509555319;
        const double G  = 3.99984385397;
        const double Q  = 0.7071752369554193;

        const double K  = std::tan (juce::MathConstants<double>::pi * f0 / fs);
        const double Vh = std::pow (10.0, G / 20.0);
        const double Vb = std::pow (Vh, 0.4996667741545416);

        const double a0 = 1.0 + K / Q + K * K;
        const double b0 = (Vh + Vb * K / Q + K * K) / a0;
        const double b1 = 2.0 * (K * K - Vh) / a0;
        const double b2 = (Vh - Vb * K / Q + K * K) / a0;
        const double a1 = 2.0 * (K * K - 1.0) / a0;
        const double a2 = (1.0 - K / Q + K * K) / a0;

        preCoeffs = new juce::dsp::IIR::Coefficients<float> (
            (float) b0, (float) b1, (float) b2, 1.0f, (float) a1, (float) a2);
    }

    // Stage 2: RLB high-pass (~ -3dB a ~38Hz)
    {
        const double f0 = 38.13547087602444;
        const double Q  = 0.5003270373238773;
        const double K  = std::tan (juce::MathConstants<double>::pi * f0 / fs);

        const double a0 = 1.0 + K / Q + K * K;
        const double a1 = 2.0 * (K * K - 1.0) / a0;
        const double a2 = (1.0 - K / Q + K * K) / a0;

        rlbCoeffs = new juce::dsp::IIR::Coefficients<float> (
            1.0f, -2.0f, 1.0f, 1.0f, (float) a1, (float) a2);
    }
}

void LoudnessMeter::pushBlock (const juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    if (numSamples == 0)
        return;

    double blockSumSquares = 0.0;

    const int channelsToUse = juce::jmin (numChannels, buffer.getNumChannels());
    for (int ch = 0; ch < channelsToUse; ++ch)
    {
        const float* in = buffer.getReadPointer (ch);
        auto& pre = preFilters[(size_t) ch];
        auto& rlb = rlbFilters[(size_t) ch];

        for (int i = 0; i < numSamples; ++i)
        {
            float y = pre.processSample (in[i]);
            y = rlb.processSample (y);
            blockSumSquares += (double) y * (double) y;
        }
    }

    window.push_back ({ blockSumSquares, numSamples * channelsToUse });
    windowSumSquares += blockSumSquares;
    windowNumSamples += numSamples * channelsToUse;

    const long long maxWindowSamples = (long long) (windowSeconds * sampleRate * channelsToUse);
    while (windowNumSamples > maxWindowSamples && ! window.empty())
    {
        windowSumSquares -= window.front().sumOfSquares;
        windowNumSamples -= window.front().numSamples;
        window.pop_front();
    }

    if (windowNumSamples <= 0)
    {
        currentLufs = -100.0f;
        return;
    }

    const double meanSquare = windowSumSquares / (double) windowNumSamples;
    if (meanSquare <= 1.0e-10)
        currentLufs = -100.0f;
    else
        currentLufs = (float) (-0.691 + 10.0 * std::log10 (meanSquare));
}

} // namespace livellatore
