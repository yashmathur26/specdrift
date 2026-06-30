#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_graphics/juce_graphics.h>
#include "PluginProcessor.h"
#include "UI/OrbKnobComponent.h"
#include "UI/FooterSliderComponent.h"
#include "UI/SpectrogramComponent.h"

class SpecdriftEditor : public juce::AudioProcessorEditor
{
public:
    explicit SpecdriftEditor(SpecdriftProcessor&);
    ~SpecdriftEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    SpecdriftProcessor& processorRef;

    std::unique_ptr<OrbKnobComponent> blurOrb;
    std::unique_ptr<OrbKnobComponent> tiltOrb;
    std::unique_ptr<OrbKnobComponent> randomOrb;
    std::unique_ptr<OrbKnobComponent> gateOrb;
    std::unique_ptr<OrbKnobComponent> mixOrb;

    std::unique_ptr<Specdrift::SpectrogramComponent> spectrogramComponent;
    juce::TextButton spectralViewButton;

    std::unique_ptr<OrbKnobComponent> lowCutOrb;
    std::unique_ptr<OrbKnobComponent> highCutOrb;

    juce::TextButton knobsModeButton;
    juce::TextButton curveModeButton;
    juce::TextButton clearCurveButton;

    // Tab buttons (expanded section)
    juce::TextButton eqTabButton;

    struct InvisibleButtonLAF : public juce::LookAndFeel_V4
    {
        void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour&,
                                  bool, bool) override {}
        void drawButtonText(juce::Graphics&, juce::TextButton&, bool, bool) override {}
    };
    InvisibleButtonLAF invisibleButtonLAF;

    std::unique_ptr<FooterSliderComponent> inputGainSlider;
    std::unique_ptr<FooterSliderComponent> outputGainSlider;

    bool spectralViewExpanded{ false };
    int activeTab{ 0 };

    static constexpr int kWidth = 720;
    static constexpr int kHeaderHeight = 46;
    static constexpr int kOrbsHeight = 154;
    static constexpr int kDividerHeight = 1;
    static constexpr int kSpectrogramHeight = 220;
    static constexpr int kCurveBarHeight = 22;
    static constexpr int kTabBarHeight = 28;
    static constexpr int kTabContentHeight = 110;
    static constexpr int kBarHeight = 26;
    static constexpr int kFooterHeight = 38;

    static constexpr int kTotalHeightCollapsed = kHeaderHeight + kOrbsHeight + kDividerHeight + kBarHeight + kFooterHeight;
    static constexpr int kTotalHeightExpanded  = kTotalHeightCollapsed + kSpectrogramHeight + kCurveBarHeight + kTabBarHeight + kTabContentHeight;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpecdriftEditor)
};
