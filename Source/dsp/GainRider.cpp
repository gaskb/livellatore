#include "GainRider.h"

namespace livellatore
{

void GainRider::prepare (double newSampleRate)
{
    sampleRate = newSampleRate;
    reset();
}

void GainRider::reset()
{
    smoothedGainDb = 0.0f;
}

float GainRider::computeGainDb (float currentLufs, double deltaTimeSeconds) noexcept
{
    if (currentLufs <= -99.0f)
    {
        // Silenzio: non inseguire un target contro il rumore di fondo,
        // lascia che il gain rilassi verso 0 con il release.
        const float releaseCoeff = (float) std::exp (-deltaTimeSeconds / (releaseMs / 1000.0));
        smoothedGainDb = smoothedGainDb * releaseCoeff;
        return smoothedGainDb;
    }

    float desiredCorrectionDb = targetLufs - currentLufs;
    desiredCorrectionDb = juce::jlimit (-maxCorrectionDb, maxCorrectionDb, desiredCorrectionDb);

    const bool needsMoreCorrection = std::abs (desiredCorrectionDb) > std::abs (smoothedGainDb);
    const double timeConstantSeconds = (needsMoreCorrection ? attackMs : releaseMs) / 1000.0;
    const float coeff = (float) std::exp (-deltaTimeSeconds / timeConstantSeconds);

    smoothedGainDb = desiredCorrectionDb + coeff * (smoothedGainDb - desiredCorrectionDb);
    return smoothedGainDb;
}

} // namespace livellatore
