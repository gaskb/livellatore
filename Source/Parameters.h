#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace livellatore
{

namespace ParamID
{
    static constexpr auto inputGain  = "inputGain";
    static constexpr auto targetLufs = "targetLufs";
    static constexpr auto attack     = "attack";
    static constexpr auto release    = "release";
    static constexpr auto maxCorrectionEnabled = "maxCorrectionEnabled";
    static constexpr auto maxCorrection = "maxCorrection";
    static constexpr auto gateThreshold = "gateThreshold";
    static constexpr auto dialogueMode = "dialogueMode";
    static constexpr auto limiter    = "limiterThreshold";
    static constexpr auto outputGain = "outputGain";
}

inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using Range = juce::NormalisableRange<float>;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamID::inputGain, 1 }, "Input Gain",
        Range { -24.0f, 24.0f, 0.01f }, 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamID::targetLufs, 1 }, "Target Level",
        Range { -36.0f, -6.0f, 0.1f }, -16.0f,
        juce::AudioParameterFloatAttributes().withLabel ("LUFS")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamID::attack, 1 }, "Attack",
        Range { 10.0f, 2000.0f, 1.0f, 0.4f }, 200.0f,
        juce::AudioParameterFloatAttributes().withLabel ("ms")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamID::release, 1 }, "Release",
        Range { 50.0f, 5000.0f, 1.0f, 0.4f }, 800.0f,
        juce::AudioParameterFloatAttributes().withLabel ("ms")));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ParamID::maxCorrectionEnabled, 1 }, "Limit Rider Range", false));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamID::maxCorrection, 1 }, "Max Correction",
        Range { 0.0f, 20.0f, 0.1f }, 6.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamID::gateThreshold, 1 }, "Gate Threshold",
        Range { -90.0f, -20.0f, 0.1f }, -60.0f,
        juce::AudioParameterFloatAttributes().withLabel ("LUFS")));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ParamID::dialogueMode, 1 }, "Dialogue Mode", false));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamID::limiter, 1 }, "Limiter",
        Range { -12.0f, 0.0f, 0.1f }, -0.3f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamID::outputGain, 1 }, "Output Gain",
        Range { -24.0f, 24.0f, 0.01f }, 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    return { params.begin(), params.end() };
}

} // namespace livellatore
