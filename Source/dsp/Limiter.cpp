#include "Limiter.h"

namespace livellatore
{

namespace
{
    constexpr float lookaheadMs = 5.0f;
}

void Limiter::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    numChannels = (int) spec.numChannels;
    lookaheadSamples = juce::jmax (1, (int) std::round (lookaheadMs * 0.001 * sampleRate));

    allocateBuffers();
    reset();
}

void Limiter::allocateBuffers()
{
    delayBuffers.assign ((size_t) numChannels, std::vector<float> ((size_t) lookaheadSamples, 0.0f));
    peakHistory.assign ((size_t) lookaheadSamples, 0.0f);
}

void Limiter::reset()
{
    for (auto& buf : delayBuffers)
        std::fill (buf.begin(), buf.end(), 0.0f);
    std::fill (peakHistory.begin(), peakHistory.end(), 0.0f);

    writeIndex = 0;
    currentGainLinear = 1.0f;
    currentGainReductionDb = 0.0f;
}

void Limiter::process (juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const int channelsToUse = juce::jmin (numChannels, buffer.getNumChannels());
    const float thresholdLinear = juce::Decibels::decibelsToGain (thresholdDb);

    // Quanto puo' salire currentGainLinear per campione quando la finestra
    // permette di rilassare la riduzione verso 1.0 (unita').
    const float releaseStepPerSample = 1.0f / (releaseMs * 0.001f * (float) sampleRate);

    float minGainLinear = 1.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        float combinedPeak = 0.0f;
        for (int ch = 0; ch < channelsToUse; ++ch)
            combinedPeak = juce::jmax (combinedPeak, std::abs (buffer.getSample (ch, i)));

        peakHistory[(size_t) writeIndex] = combinedPeak;

        // Scansione a forza bruta della finestra (lookahead tipicamente
        // poche centinaia di campioni): piu' che sufficiente qui, non
        // vale la pena della complessita' di una coda monotona per
        // il massimo scorrevole finche' il lookahead resta nell'ordine dei
        // millisecondi.
        float windowPeak = 0.0f;
        for (float peak : peakHistory)
            windowPeak = juce::jmax (windowPeak, peak);

        const float desiredGain = windowPeak > thresholdLinear ? thresholdLinear / windowPeak : 1.0f;

        if (desiredGain < currentGainLinear)
            currentGainLinear = desiredGain; // riduzione immediata: il lookahead ha gia' "avvisato"
        else
            currentGainLinear = juce::jmin (desiredGain, currentGainLinear + releaseStepPerSample);

        minGainLinear = juce::jmin (minGainLinear, currentGainLinear);

        // Leggi il campione ritardato PRIMA di sovrascrivere lo slot con
        // quello corrente: cosi' il buffer circolare realizza un ritardo
        // esatto di lookaheadSamples campioni.
        for (int ch = 0; ch < channelsToUse; ++ch)
        {
            auto& delayBuf = delayBuffers[(size_t) ch];
            const float delayedSample = delayBuf[(size_t) writeIndex];
            delayBuf[(size_t) writeIndex] = buffer.getSample (ch, i);
            buffer.setSample (ch, i, delayedSample * currentGainLinear);
        }

        writeIndex = (writeIndex + 1) % lookaheadSamples;
    }

    currentGainReductionDb = juce::Decibels::gainToDecibels (minGainLinear);
}

} // namespace livellatore
