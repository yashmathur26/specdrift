#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace Specdrift
{

/** Parameter IDs for APVTS */
namespace ParamID
{
    inline constexpr const char* blur   = "blur";
    inline constexpr const char* tilt   = "tilt";
    inline constexpr const char* random = "random";
    inline constexpr const char* gate   = "gate";
    inline constexpr const char* mix    = "mix";
    inline constexpr const char* inputGain  = "inputGain";
    inline constexpr const char* outputGain = "outputGain";
    inline constexpr const char* lowCut  = "lowCut";
    inline constexpr const char* highCut = "highCut";
}

/** Creates the APVTS layout for Specdrift.
 *  See SPECDRIFT_ROADMAP.md Parameters section.
 */
juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

} // namespace Specdrift
