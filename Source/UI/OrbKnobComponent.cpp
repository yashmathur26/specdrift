#include "OrbKnobComponent.h"
#include "Core/PluginProcessor.h"
#include "Core/Parameters.h"
#include <cmath>

//==============================================================================
void OrbLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                      float /*sliderPos*/, float /*rotaryStartAngle*/, float /*rotaryEndAngle*/,
                                      juce::Slider& slider)
{
    auto bounds = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y),
                                         static_cast<float>(width), static_cast<float>(height));
    auto centre = bounds.getCentre();
    auto orbRadius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;

    float value = static_cast<float>(slider.getValue());
    bool isTilt = (slider.getMinimum() < 0.0);
    bool isFreqKnob = (slider.getMaximum() > 100.0 && slider.getMinimum() >= 0.0);
    float displayVal;
    if (isTilt)
        displayVal = (value + 1.0f) * 0.5f;
    else if (isFreqKnob)
    {
        const float logMin = std::log10(20.0f);
        const float logMax = std::log10(20000.0f);
        float freq = juce::jlimit(20.0f, 20000.0f, value);
        displayVal = (std::log10(freq) - logMin) / (logMax - logMin);
        if (freqKnobReversed)
            displayVal = 1.0f - displayVal;  // High Cut: 20k at start, clockwise = more cut
    }
    else
        displayVal = value;

    // 1. Background circle — radial gradient
    float focalX = centre.x - orbRadius * 0.2f;
    float focalY = centre.y - orbRadius * 0.2f;
    juce::ColourGradient bgGradient(
        orbColour.withAlpha(0.4f), focalX, focalY,
        orbColour.withAlpha(0.02f), centre.x + orbRadius, centre.y + orbRadius,
        true);
    bgGradient.addColour(0.6, orbColour.withAlpha(0.1f));
    g.setGradientFill(bgGradient);
    g.fillEllipse(bounds);

    // 2. Inner ring
    float innerRingRadius = orbRadius * 0.45f;
    g.setColour(juce::Colours::white.withAlpha(0.08f));
    g.drawEllipse(centre.x - innerRingRadius, centre.y - innerRingRadius,
                  innerRingRadius * 2.0f, innerRingRadius * 2.0f, 1.5f);

    // 3. Outer subtle circle track
    float arcRadius = orbRadius * 0.945f;
    g.setColour(juce::Colour(80, 140, 255).withAlpha(0.08f));
    g.drawEllipse(centre.x - arcRadius, centre.y - arcRadius,
                  arcRadius * 2.0f, arcRadius * 2.0f, 1.5f);

    const float pi = juce::MathConstants<float>::pi;
    const float halfPi = pi / 2.0f;
    
    // Base angles + 180° flip to correct upside-down orientation
    const float clock5_base = pi / 3.0f + pi;           // 60° + 180° = 240°
    const float clock7_base = 2.0f * pi / 3.0f + pi;    // 120° + 180° = 300°
    const float clock12_base = 3.0f * pi / 2.0f + pi;   // 270° + 180° = 450° = 90°

    // Shifted angles for normal knobs (shift left = subtract 90°)
    const float clock5_normal = clock5_base - halfPi;   // 240° - 90° = 150°
    const float clock7_normal = clock7_base - halfPi;   // 300° - 90° = 210°
    
    // Shifted angles for tilt (shift right = add 90°)
    const float clock12_tilt = clock12_base + halfPi;   // 90° + 90° = 180°

    // 4. Background arc track
    {
        juce::Path trackPath;
        if (isTilt)
        {
            // Tilt track: from 7 o'clock to 5 o'clock through 12 (shifted right by 90°)
            float trackStart = clock7_base + halfPi;   // 120° + 90° = 210°
            float trackEnd = clock5_base + halfPi + 2.0f * pi;  // 60° + 90° + 360° = 510°
            trackPath.addCentredArc(centre.x, centre.y, arcRadius, arcRadius,
                                    0.0f, trackStart, trackEnd, true);
        }
        else
        {
            // Normal track: from 7 o'clock to 5 o'clock through 12 (shifted left by 90°)
            float trackStart = clock7_normal;  // 30°
            float trackEnd = clock5_normal + 2.0f * pi;  // -30° + 360° = 330°
            trackPath.addCentredArc(centre.x, centre.y, arcRadius, arcRadius,
                                    0.0f, trackStart, trackEnd, true);
        }
        g.setColour(orbColour.withAlpha(0.2f));
        g.strokePath(trackPath, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // 5. Value arc fill
    if (isTilt)
    {
        // Tilt: flipped so -100 on right, +100 on left (negate for arc)
        float v = -value;
        if (std::abs(v) > 0.001f)
        {
            juce::Path arcPath;
            float maxSweep = 150.0f * pi / 180.0f;  // 150°
            
            if (v > 0.0f)
            {
                // Clockwise from 12 toward right
                float sweep = maxSweep * v;
                float endAngle = clock12_tilt + sweep;
                arcPath.addCentredArc(centre.x, centre.y, arcRadius, arcRadius,
                                      0.0f, clock12_tilt, endAngle, true);
            }
            else
            {
                // Counterclockwise from 12 toward left
                float sweep = maxSweep * std::abs(v);
                float endAngle = clock12_tilt - sweep;
                arcPath.addCentredArc(centre.x, centre.y, arcRadius, arcRadius,
                                      0.0f, clock12_tilt, endAngle, true);
            }
            g.setColour(orbColour);
            g.strokePath(arcPath, juce::PathStrokeType(2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
    }
    else
    {
        // Normal knobs: fill from 7 o'clock clockwise (shifted left by 90°)
        if (displayVal > 0.001f)
        {
            juce::Path arcPath;
            float fullSweep = 300.0f * pi / 180.0f;  // 300°
            float endAngle = clock7_normal + fullSweep * displayVal;
            arcPath.addCentredArc(centre.x, centre.y, arcRadius, arcRadius,
                                  0.0f, clock7_normal, endAngle, true);
            g.setColour(orbColour);
            g.strokePath(arcPath, juce::PathStrokeType(2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
    }

    // 6. Value text
    g.setColour(juce::Colours::white);
    juce::String valueStr;
    if (isTilt)
    {
        int pct = juce::roundToInt(-value * 100.0f);  // Flipped display
        valueStr = (pct >= 0 ? "+" : "") + juce::String(pct);
    }
    else if (isFreqKnob)
    {
        float freq = juce::jlimit(20.0f, 20000.0f, value);
        if (freq >= 1000.0f)
            valueStr = juce::String(freq / 1000.0f, 1) + "k";
        else
            valueStr = juce::String(juce::roundToInt(freq));
    }
    else
    {
        valueStr = juce::String(juce::roundToInt(displayVal * 100.0f));
    }
    g.drawText(valueStr, bounds, juce::Justification::centred, false);
}

//==============================================================================
OrbKnobComponent::OrbKnobComponent(SpecdriftProcessor& processor,
                                   const juce::String& paramID,
                                   const juce::String& label,
                                   juce::Colour colour,
                                   float defaultValue)
    : labelText(label)
{
    orbLookAndFeel.setOrbColour(colour);
    if (paramID == Specdrift::ParamID::highCut)
        orbLookAndFeel.setFreqKnobReversed(true);  // 20k at start of rotation
    slider.setLookAndFeel(&orbLookAndFeel);
    slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);

    if (paramID == Specdrift::ParamID::tilt)
    {
        slider.setRange(-1.0, 1.0, 0.001);
        slider.setValue(0.0);
        slider.setDoubleClickReturnValue(true, 0.0);
    }
    else if (paramID == Specdrift::ParamID::lowCut || paramID == Specdrift::ParamID::highCut)
    {
        slider.setNormalisableRange(juce::NormalisableRange<double>(20.0, 20000.0, 1.0, 0.3));
        slider.setValue(paramID == Specdrift::ParamID::lowCut ? 20.0 : 20000.0);
        slider.setDoubleClickReturnValue(true, paramID == Specdrift::ParamID::lowCut ? 20.0 : 20000.0);
    }
    else
    {
        slider.setRange(0.0, 1.0, 0.001);
        slider.setValue(static_cast<double>(defaultValue));
        slider.setDoubleClickReturnValue(true, static_cast<double>(defaultValue));
    }
    
    // More drag needed for full range (higher = less sensitive)
    slider.setMouseDragSensitivity(150);
    
    addAndMakeVisible(slider);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.getAPVTS(), paramID, slider);
}

OrbKnobComponent::~OrbKnobComponent()
{
    slider.setLookAndFeel(nullptr);
}

void OrbKnobComponent::paint(juce::Graphics& g)
{
    auto labelArea = getLocalBounds().removeFromBottom(18);
    g.setColour(juce::Colour(0xff8a8ab8));
    g.drawText(labelText.toUpperCase(), labelArea, juce::Justification::centred, false);
}

void OrbKnobComponent::resized()
{
    auto bounds = getLocalBounds();
    auto labelHeight = 18;
    auto sliderArea = bounds.withTrimmedBottom(labelHeight);
    
    int orbSize = juce::jmin(sliderArea.getWidth(), sliderArea.getHeight());
    auto orbBounds = sliderArea.withSizeKeepingCentre(orbSize, orbSize);
    slider.setBounds(orbBounds);
}
