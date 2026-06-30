#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "Core/PluginProcessor.h"
#include "DSP/SpectralEngine.h"
#include "DSP/CurveState.h"
#include <vector>

namespace Specdrift
{

/** Phase 4 + 5: Live spectrogram and drawable curve overlay (per-bin blur speed). */
class SpectrogramComponent : public juce::Component,
                             private juce::Timer
{
public:
    explicit SpectrogramComponent(SpecdriftProcessor& processor);
    ~SpectrogramComponent() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void visibilityChanged() override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;

private:
    void timerCallback() override;
    /** Spectrogram area (excludes frequency labels). */
    juce::Rectangle<int> getSpectrogramBounds() const;
    /** Convert pixel in spectrogram area to curve coords (x,y in 0..1). */
    void pixelToCurveCoords(int px, int py, float& outX, float& outY) const;
    /** Convert curve coords (0..1) to pixel in spectrogram area. */
    void curveToPixel(float x, float y, float& outPx, float& outPy) const;
    /** Index of point near (px, py), or -1. */
    int hitTestPoint(int px, int py) const;

    static float pixelYToFreqHz(float normY);
    static float freqHzToBinFloat(float freqHz, double sampleRate);
    static float freqHzToNormX(float freqHz);
    juce::Colour magnitudeToColour(float magnitude);
    void ensureImageAndLUT();
    void pushColumn(const float* magnitudes);

    SpecdriftProcessor& processorRef;
    juce::Image spectrogramImage;
    std::vector<float> frameBuffer;
    std::vector<juce::Colour> colormapLUT;
    float displayMaxMagnitude{0.001f};
    int writeColumn{0};
    static constexpr int kImageWidth  = 512;
    static constexpr int kImageHeight = 256;
    static constexpr int kTimerHz     = 30;
    static constexpr int kFreqLabelHeight = 18;

    int draggingPointIndex{ -1 };
    static constexpr int kPointHitRadius = 10;
};

} // namespace Specdrift
