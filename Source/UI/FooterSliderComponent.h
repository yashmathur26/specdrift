#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_graphics/juce_graphics.h>

class SpecdriftProcessor;

//==============================================================================
/** Custom LookAndFeel for footer horizontal sliders (IN/OUT gain). */
class FooterSliderLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          const juce::Slider::SliderStyle, juce::Slider&) override;
};

//==============================================================================
/** Footer slider: label + track + value drawn in paint(), no juce::Label children. */
class FooterSliderComponent : public juce::Component
{
public:
    FooterSliderComponent(SpecdriftProcessor& processor,
                          const juce::String& paramID,
                          const juce::String& labelText);
    ~FooterSliderComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    FooterSliderLookAndFeel lookAndFeel;
    juce::Slider slider;
    juce::String nameText;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FooterSliderComponent)
};
