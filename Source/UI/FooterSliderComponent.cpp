#include "FooterSliderComponent.h"
#include "Core/PluginProcessor.h"
#include "Core/Parameters.h"

//==============================================================================
void FooterSliderLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                                               float sliderPos, float /*minSliderPos*/, float /*maxSliderPos*/,
                                               const juce::Slider::SliderStyle, juce::Slider&)
{
    // HTML: track 1.5px height, fill rgba(139,92,246,0.7), thumb 6px white 90%
    auto trackHeight = 1.5f;
    auto trackY = static_cast<float>(y) + (static_cast<float>(height) - trackHeight) * 0.5f;
    auto track = juce::Rectangle<float>(static_cast<float>(x), trackY,
                                        static_cast<float>(width), trackHeight);
    
    // Track background — rgba(80,100,150,0.3)
    g.setColour(juce::Colour(80, 100, 150).withAlpha(0.3f));
    g.fillRoundedRectangle(track, 1.0f);

    // Fill — rgba(139,92,246,0.7)
    auto fillWidth = sliderPos - static_cast<float>(x);
    if (fillWidth > 0.0f)
    {
        auto fillRect = track.withWidth(fillWidth);
        g.setColour(juce::Colour(139, 92, 246).withAlpha(0.7f));
        g.fillRoundedRectangle(fillRect, 1.0f);
    }

    // Thumb — 6px white 90%
    auto thumbSize = 6.0f;
    auto thumbX = sliderPos - thumbSize * 0.5f;
    auto thumbY = static_cast<float>(y) + (static_cast<float>(height) - thumbSize) * 0.5f;
    g.setColour(juce::Colours::white.withAlpha(0.9f));
    g.fillEllipse(thumbX, thumbY, thumbSize, thumbSize);
}

//==============================================================================
FooterSliderComponent::FooterSliderComponent(SpecdriftProcessor& processor,
                                             const juce::String& paramID,
                                             const juce::String& labelText)
    : nameText(labelText)
{
    slider.setLookAndFeel(&lookAndFeel);
    slider.setSliderStyle(juce::Slider::LinearHorizontal);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    slider.setRange(0.0, 1.0, 0.001);
    slider.setValue(1.0);
    slider.setDoubleClickReturnValue(true, 0.5);
    slider.onValueChange = [this] { repaint(); };
    addAndMakeVisible(slider);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.getAPVTS(), paramID, slider);
}

FooterSliderComponent::~FooterSliderComponent()
{
    slider.setLookAndFeel(nullptr);
}

void FooterSliderComponent::paint(juce::Graphics& g)
{
    auto r = getLocalBounds();
    auto nameArea = r.removeFromLeft(32);  // Wide enough for "OUT"
    auto valueArea = r.removeFromRight(36);

    // HTML: 8px, color #6a6a9a
    g.setColour(juce::Colour(0xff6a6a9a));
    // Slight vertical inset so descenders (e.g. bottom of "U") aren't clipped
    auto nameSafe = nameArea.reduced(0, 2);
    g.drawText(nameText, nameSafe, juce::Justification::centredRight, false);

    int pct = juce::roundToInt(static_cast<float>(slider.getValue()) * 100.0f);
    g.drawText(juce::String(pct) + "%", valueArea, juce::Justification::centredLeft, false);
}

void FooterSliderComponent::resized()
{
    auto r = getLocalBounds();
    r.removeFromLeft(36);   // Space for "IN"/"OUT" label + gap
    r.removeFromRight(40);  // Space for "100%" value + gap
    slider.setBounds(r);
}
