#include "CurveState.h"
#include <algorithm>
#include <sstream>

namespace Specdrift
{

namespace
{
    constexpr float LOG_FREQ_MIN = 20.0f;
    constexpr float LOG_FREQ_MAX = 20000.0f;
    const float LOG_MIN = std::log10(LOG_FREQ_MIN);
    const float LOG_MAX = std::log10(LOG_FREQ_MAX);
}

float CurveState::binToLogFreqPos(int bin, double sampleRate)
{
    if (bin <= 0)
        return 0.0f;
    float freqHz = static_cast<float>(bin) * static_cast<float>(sampleRate) / static_cast<float>(FFT_SIZE);
    freqHz = std::max(LOG_FREQ_MIN, std::min(LOG_FREQ_MAX, freqHz));
    float logFreq = std::log10(freqHz);
    return (logFreq - LOG_MIN) / (LOG_MAX - LOG_MIN);
}

int CurveState::addPoint(float x, float y)
{
    x = std::max(0.0f, std::min(1.0f, x));
    y = std::max(0.0f, std::min(1.0f, y));
    CurvePoint pt{ x, y };
    auto it = std::lower_bound(points.begin(), points.end(), pt,
        [](const CurvePoint& a, const CurvePoint& b) { return a.x < b.x; });
    int index = static_cast<int>(std::distance(points.begin(), it));
    points.insert(it, pt);
    return index;
}

void CurveState::removePoint(int index)
{
    if (index >= 0 && index < static_cast<int>(points.size()))
        points.erase(points.begin() + index);
}

void CurveState::movePoint(int index, float x, float y)
{
    if (index < 0 || index >= static_cast<int>(points.size()))
        return;
    x = std::max(0.0f, std::min(1.0f, x));
    y = std::max(0.0f, std::min(1.0f, y));
    points[static_cast<size_t>(index)] = CurvePoint{ x, y };
    std::sort(points.begin(), points.end(), [](const CurvePoint& a, const CurvePoint& b) { return a.x < b.x; });
}

float CurveState::evaluateAt(float logFreqPos) const
{
    logFreqPos = std::max(0.0f, std::min(1.0f, logFreqPos));
    if (points.empty())
        return 0.5f;
    if (points.size() == 1)
        return points[0].y;
    if (logFreqPos <= points[0].x)
        return points[0].y;
    if (logFreqPos >= points.back().x)
        return points.back().y;
    for (size_t i = 0; i + 1 < points.size(); ++i)
    {
        float x0 = points[i].x;
        float x1 = points[i + 1].x;
        if (logFreqPos >= x0 && logFreqPos <= x1)
        {
            float t = (x1 - x0) > 1e-6f ? (logFreqPos - x0) / (x1 - x0) : 0.0f;
            return points[i].y + t * (points[i + 1].y - points[i].y);
        }
    }
    return points.back().y;
}

void CurveState::generateLookupTable(std::vector<float>& out, double sampleRate) const
{
    out.resize(static_cast<size_t>(NUM_BINS), 0.5f);
    for (int b = 0; b < NUM_BINS; ++b)
    {
        float logPos = binToLogFreqPos(b, sampleRate);
        float speed = evaluateAt(logPos);
        out[static_cast<size_t>(b)] = std::max(0.001f, std::min(1.0f, speed));
    }
}

void CurveState::clear()
{
    points.clear();
}

juce::String CurveState::toJson() const
{
    juce::String s = "{\"points\":[";
    for (size_t i = 0; i < points.size(); ++i)
    {
        if (i > 0)
            s += ",";
        s += "{\"x\":" + juce::String(points[i].x, 4) + ",\"y\":" + juce::String(points[i].y, 4) + "}";
    }
    s += "]}";
    return s;
}

bool CurveState::fromJson(const juce::String& json)
{
    points.clear();
    juce::var v;
    if (!juce::JSON::parse(json, v).wasOk() || !v.isObject())
        return false;
    juce::var pointsVar = v.getProperty("points", juce::var());
    if (!pointsVar.isArray())
        return true;
    juce::Array<juce::var>* arr = pointsVar.getArray();
    for (const auto& item : *arr)
    {
        if (!item.isObject())
            continue;
        float x = static_cast<float>(item.getProperty("x", 0.5).operator double());
        float y = static_cast<float>(item.getProperty("y", 0.5).operator double());
        x = std::max(0.0f, std::min(1.0f, x));
        y = std::max(0.0f, std::min(1.0f, y));
        points.push_back({ x, y });
    }
    std::sort(points.begin(), points.end(), [](const CurvePoint& a, const CurvePoint& b) { return a.x < b.x; });
    return true;
}

} // namespace Specdrift
