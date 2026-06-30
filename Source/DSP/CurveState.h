#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "SpectralEngine.h"
#include <vector>
#include <cmath>

namespace Specdrift
{

/** Single point on the drawable curve. X = log-frequency position 0..1, Y = blur speed 0..1 (0=max blur, 1=no blur). */
struct CurvePoint
{
    float x{ 0.5f };
    float y{ 0.5f };
};

/**
 * Curve state: sorted points, linear interpolation between them.
 * Generates a lookup table of NUM_BINS values for the spectral engine.
 */
class CurveState
{
public:
    CurveState() = default;

    /** Points sorted by x. Do not modify directly; use add/remove/move so we stay sorted. */
    const std::vector<CurvePoint>& getPoints() const { return points; }

    /** Add a point (inserted in x order). Returns index. */
    int addPoint(float x, float y);

    /** Remove point at index. */
    void removePoint(int index);

    /** Move point at index to new position. */
    void movePoint(int index, float x, float y);

    /** Evaluate curve at log-frequency position 0..1. Returns speed 0..1. */
    float evaluateAt(float logFreqPos) const;

    /** Fill lookup table for NUM_BINS using current sample rate. */
    void generateLookupTable(std::vector<float>& out, double sampleRate) const;

    /** Serialize to JSON string for getStateInformation. */
    juce::String toJson() const;

    /** Restore from JSON (setStateInformation). Returns true if parsed. */
    bool fromJson(const juce::String& json);

    /** Clear all points. */
    void clear();

    bool isEmpty() const { return points.empty(); }

private:
    static float binToLogFreqPos(int bin, double sampleRate);
    std::vector<CurvePoint> points;
};

} // namespace Specdrift
