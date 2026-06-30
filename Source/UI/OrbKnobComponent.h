#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_graphics/juce_graphics.h>

class SpecdriftProcessor;

//==============================================================================
/** Custom LookAndFeel for orb-style knobs — no text (drawn separately). */
class OrbLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void setOrbColour(juce::Colour c) { orbColour = c; }
    /** For High Cut: 20k at start of rotation (7 o'clock), clockwise = more cut */
    void setFreqKnobReversed(bool reversed) { freqKnobReversed = reversed; }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider&) override;

private:
    juce::Colour orbColour{0xff4488ff};
    bool freqKnobReversed{false};
};

//==============================================================================
/** Orb knob with radial gradient, 270-degree arc, and label drawn in paint(). */
class OrbKnobComponent : public juce::Component
{
public:
    OrbKnobComponent(SpecdriftProcessor& processor,
                     const juce::String& paramID,
                     const juce::String& labelText,
                     juce::Colour orbColour,
                     float defaultValue);
    ~OrbKnobComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    OrbLookAndFeel orbLookAndFeel;
    juce::Slider slider;
    juce::String labelText;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OrbKnobComponent)
};
