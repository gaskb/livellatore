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
    gateOpen = true;
    attackActive = false;
}

float GainRider::computeGainDb (float currentLufs, double deltaTimeSeconds) noexcept
{
    // Gate con isteresi: apre/chiude a soglie diverse per evitare di
    // riaprire e richiudere ad ogni blocco quando il segnale oscilla
    // proprio intorno alla soglia impostata dall'utente. Il pavimento
    // interno del LoudnessMeter (-100 LUFS) è sempre sotto il range
    // consentito per la soglia (min -90, vedi Parameters.h), quindi il
    // vero silenzio digitale chiude sempre il gate senza bisogno di un
    // controllo separato.
    const float openThreshold  = gateThresholdLufs + gateHysteresisDb * 0.5f;
    const float closeThreshold = gateThresholdLufs - gateHysteresisDb * 0.5f;

    if (gateOpen && currentLufs < closeThreshold)
        gateOpen = false;
    else if (! gateOpen && currentLufs > openThreshold)
        gateOpen = true;

    if (! gateOpen)
    {
        // Gate chiuso: non inseguire un target contro rumore di fondo o
        // silenzio, lascia che il gain rilassi verso 0. "Gate speed" non
        // è un controllo separato in questa versione: deriva dal release
        // esistente (vedi commento in testa al file).
        const float releaseCoeff = (float) std::exp (-deltaTimeSeconds / (releaseMs / 1000.0));
        smoothedGainDb = smoothedGainDb * releaseCoeff;
        return smoothedGainDb;
    }

    float desiredCorrectionDb = targetLufs - currentLufs;
    desiredCorrectionDb = juce::jlimit (-maxCorrectionDb, maxCorrectionDb, desiredCorrectionDb);

    // Dead-band con isteresi sulla scelta attack/release (issue #3): sotto
    // switchDeadbandDb di differenza si mantiene lo stato precedente
    // invece di rivalutarlo, per non alternare rapidamente le due costanti
    // di tempo quando il segnale dithera vicino al punto di incrocio.
    const float correctionDeltaDb = std::abs (desiredCorrectionDb) - std::abs (smoothedGainDb);
    if (correctionDeltaDb > switchDeadbandDb)
        attackActive = true;
    else if (correctionDeltaDb < -switchDeadbandDb)
        attackActive = false;

    const double timeConstantSeconds = (attackActive ? attackMs : releaseMs) / 1000.0;
    const float coeff = (float) std::exp (-deltaTimeSeconds / timeConstantSeconds);

    smoothedGainDb = desiredCorrectionDb + coeff * (smoothedGainDb - desiredCorrectionDb);
    return smoothedGainDb;
}

} // namespace livellatore
