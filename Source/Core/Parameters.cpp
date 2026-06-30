#include "Parameters.h"

namespace Specdrift
{

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamID::blur, 1 },
        "Blur",
        0.0f, 1.0f, 0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamID::tilt, 1 },
        "Tilt",
        -1.0f, 1.0f, 0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamID::random, 1 },
        "Random",
        0.0f, 1.0f, 0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamID::gate, 1 },
        "Gate",
        0.0f, 1.0f, 0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamID::mix, 1 },
        "Mix",
        0.0f, 1.0f, 0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamID::inputGain, 1 },
        "Input Gain",
        0.0f, 1.0f, 1.0f));  // 0-100%, default 100%

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamID::outputGain, 1 },
        "Output Gain",
        0.0f, 1.0f, 1.0f));  // 0-100%, default 100%

    // EQ: Low Cut (highpass on wet), High Cut (lowpass on output) — 20 Hz–20 kHz log
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamID::lowCut, 1 },
        "Low Cut",
        juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.3f),
        20.0f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float value, int) {
            if (value >= 1000.0f) return juce::String(value / 1000.0f, 2) + " kHz";
            return juce::String(static_cast<int>(value)) + " Hz";
        }));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamID::highCut, 1 },
        "High Cut",
        juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.3f),
        20000.0f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float value, int) {
            if (value >= 1000.0f) return juce::String(value / 1000.0f, 2) + " kHz";
            return juce::String(static_cast<int>(value)) + " Hz";
        }));

    return layout;
}

} // namespace Specdrift
