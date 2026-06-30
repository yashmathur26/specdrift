#include "SpectrogramComponent.h"
#include <cmath>

namespace Specdrift
{

namespace
{
    constexpr float kLogFreqMin = 1.30103f;   // log10(20)
    constexpr float kLogFreqMax = 4.30103f;   // log10(20000)
}

SpectrogramComponent::SpectrogramComponent(SpecdriftProcessor& processor)
    : processorRef(processor)
{
    frameBuffer.resize(static_cast<size_t>(NUM_BINS), 0.0f);
    ensureImageAndLUT();
    // Timer started in visibilityChanged() when component is shown
}

void SpectrogramComponent::paint(juce::Graphics& g)
{
    ensureImageAndLUT();
    auto bounds = getLocalBounds();
    auto vizArea = bounds.removeFromBottom(kFreqLabelHeight);
    auto drawBounds = bounds.toFloat();
    float cornerRadius = 6.0f;

    // Dark inset background with rounded corners
    g.setColour(juce::Colour(0xff08081a));
    g.fillRoundedRectangle(drawBounds, cornerRadius);

    if (spectrogramImage.isValid())
    {
        // Clip to rounded rectangle
        juce::Path clip;
        clip.addRoundedRectangle(drawBounds, cornerRadius);
        g.saveState();
        g.reduceClipRegion(clip);

        const int imgW = spectrogramImage.getWidth();
        const int imgH = spectrogramImage.getHeight();
        int rightPortion = imgW - writeColumn;
        if (rightPortion > 0)
        {
            int destX = static_cast<int>(drawBounds.getX());
            int destW = static_cast<int>(drawBounds.getWidth() * static_cast<float>(rightPortion) / static_cast<float>(imgW));
            int destH = static_cast<int>(drawBounds.getHeight());
            g.drawImage(spectrogramImage,
                        destX, static_cast<int>(drawBounds.getY()), destW, destH,
                        writeColumn, 0, rightPortion, imgH);
        }
        if (writeColumn > 0)
        {
            int rightW = static_cast<int>(drawBounds.getWidth() * static_cast<float>(imgW - writeColumn) / static_cast<float>(imgW));
            int destX = static_cast<int>(drawBounds.getX()) + rightW;
            int destW = static_cast<int>(drawBounds.getWidth()) - rightW;
            int destH = static_cast<int>(drawBounds.getHeight());
            g.drawImage(spectrogramImage,
                        destX, static_cast<int>(drawBounds.getY()), destW, destH,
                        0, 0, writeColumn, imgH);
        }

        g.restoreState();
    }

    g.setColour(juce::Colour(139, 92, 246).withAlpha(0.08f));
    g.drawRoundedRectangle(drawBounds.reduced(0.5f), cornerRadius, 1.0f);

    // Phase 5: Curve overlay (when curve has points)
    auto& curveState = processorRef.getCurveState();
    const auto& points = curveState.getPoints();
    if (!points.empty())
    {
        juce::Path path;
        float px0, py0;
        curveToPixel(points[0].x, points[0].y, px0, py0);
        path.startNewSubPath(px0, py0);
        for (size_t i = 1; i < points.size(); ++i)
        {
            float pxi, pyi;
            curveToPixel(points[i].x, points[i].y, pxi, pyi);
            path.lineTo(pxi, pyi);
        }
        g.setColour(juce::Colour(139, 92, 246).withAlpha(0.35f));
        g.strokePath(path, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        for (size_t i = 0; i < points.size(); ++i)
        {
            float pxi, pyi;
            curveToPixel(points[i].x, points[i].y, pxi, pyi);
            g.setColour(i == static_cast<size_t>(draggingPointIndex) ? juce::Colour(0xffa78bfa) : juce::Colour(0xff8b5cf6));
            g.fillEllipse(pxi - 4.0f, pyi - 4.0f, 8.0f, 8.0f);
            g.setColour(juce::Colour(139, 92, 246).withAlpha(0.6f));
            g.drawEllipse(pxi - 4.0f, pyi - 4.0f, 8.0f, 8.0f, 1.0f);
        }
    }

    // Frequency axis labels
    g.setFont(juce::Font(juce::FontOptions(9.0f)));
    g.setColour(juce::Colour(0xff505078));
    const float freqHzVals[] = { 20.0f, 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f, 20000.0f };
    const char* labels[] = { "20", "50", "100", "200", "500", "1k", "2k", "5k", "10k", "20k" };
    const int numLabels = 10;
    const int labelW = 30;
    for (int i = 0; i < numLabels; ++i)
    {
        float normX = freqHzToNormX(freqHzVals[i]);
        int x = static_cast<int>(normX * static_cast<float>(vizArea.getWidth()));
        if (i == numLabels - 1)
        {
            int right = vizArea.getRight();
            g.drawText(labels[i], right - labelW, vizArea.getY(), labelW, vizArea.getHeight(), juce::Justification::centredRight, false);
        }
        else if (i == 0)
        {
            g.drawText(labels[i], vizArea.getX(), vizArea.getY(), labelW, vizArea.getHeight(), juce::Justification::centredLeft, false);
        }
        else
        {
            x = juce::jlimit(0, vizArea.getWidth() - labelW, x - labelW / 2);
            g.drawText(labels[i], x, vizArea.getY(), labelW, vizArea.getHeight(), juce::Justification::centred, false);
        }
    }
}

void SpectrogramComponent::visibilityChanged()
{
    if (isVisible())
        startTimerHz(kTimerHz);
    else
        stopTimer();
}

juce::Rectangle<int> SpectrogramComponent::getSpectrogramBounds() const
{
    auto r = getLocalBounds();
    r.removeFromBottom(kFreqLabelHeight);
    return r;
}

void SpectrogramComponent::pixelToCurveCoords(int px, int py, float& outX, float& outY) const
{
    auto spec = getSpectrogramBounds();
    if (spec.isEmpty())
    {
        outX = outY = 0.5f;
        return;
    }
    outX = (px - spec.getX()) / static_cast<float>(juce::jmax(1, spec.getWidth()));
    outX = juce::jlimit(0.0f, 1.0f, outX);
    outY = 1.0f - (py - spec.getY()) / static_cast<float>(juce::jmax(1, spec.getHeight()));
    outY = juce::jlimit(0.0f, 1.0f, outY);
}

void SpectrogramComponent::curveToPixel(float x, float y, float& outPx, float& outPy) const
{
    auto spec = getSpectrogramBounds().toFloat();
    if (spec.isEmpty())
    {
        outPx = outPy = 0.0f;
        return;
    }
    outPx = spec.getX() + x * spec.getWidth();
    outPy = spec.getY() + (1.0f - y) * spec.getHeight();
}

int SpectrogramComponent::hitTestPoint(int px, int py) const
{
    auto& curveState = processorRef.getCurveState();
    const auto& points = curveState.getPoints();
    for (size_t i = 0; i < points.size(); ++i)
    {
        float pxi, pyi;
        curveToPixel(points[i].x, points[i].y, pxi, pyi);
        float dx = px - pxi;
        float dy = py - pyi;
        if (dx * dx + dy * dy <= static_cast<float>(kPointHitRadius * kPointHitRadius))
            return static_cast<int>(i);
    }
    return -1;
}

void SpectrogramComponent::mouseDown(const juce::MouseEvent& e)
{
    auto spec = getSpectrogramBounds();
    if (!spec.contains(e.getPosition()))
        return;
    int idx = hitTestPoint(e.getPosition().getX(), e.getPosition().getY());
    if (idx >= 0)
    {
        draggingPointIndex = idx;
        return;
    }
    if (e.mods.isRightButtonDown())
        return;
    float cx, cy;
    pixelToCurveCoords(e.getPosition().getX(), e.getPosition().getY(), cx, cy);
    processorRef.getCurveState().addPoint(cx, cy);
    processorRef.updateCurveLookup();
    draggingPointIndex = static_cast<int>(processorRef.getCurveState().getPoints().size()) - 1;
    repaint();
}

void SpectrogramComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (draggingPointIndex < 0)
        return;
    auto spec = getSpectrogramBounds();
    if (spec.isEmpty())
        return;
    float cx, cy;
    pixelToCurveCoords(e.getPosition().getX(), e.getPosition().getY(), cx, cy);
    processorRef.getCurveState().movePoint(draggingPointIndex, cx, cy);
    processorRef.updateCurveLookup();
    repaint();
}

void SpectrogramComponent::mouseDoubleClick(const juce::MouseEvent& e)
{
    int idx = hitTestPoint(e.getPosition().getX(), e.getPosition().getY());
    if (idx >= 0)
    {
        processorRef.getCurveState().removePoint(idx);
        processorRef.updateCurveLookup();
        if (draggingPointIndex == idx)
            draggingPointIndex = -1;
        else if (draggingPointIndex > idx)
            draggingPointIndex--;
        repaint();
    }
}

void SpectrogramComponent::mouseUp(const juce::MouseEvent&)
{
    draggingPointIndex = -1;
}

void SpectrogramComponent::resized()
{
}

float SpectrogramComponent::pixelYToFreqHz(float normY)
{
    // normY 0 = bottom = 20 Hz, 1 = top = 20 kHz (log scale)
    float logFreq = kLogFreqMin + normY * (kLogFreqMax - kLogFreqMin);
    return std::pow(10.0f, logFreq);
}

float SpectrogramComponent::freqHzToBinFloat(float freqHz, double sampleRate)
{
    if (sampleRate <= 0.0)
        return 0.0f;
    float bin = static_cast<float>(freqHz * FFT_SIZE / sampleRate);
    return juce::jlimit(0.0f, static_cast<float>(NUM_BINS - 1), bin);
}

float SpectrogramComponent::freqHzToNormX(float freqHz)
{
    float logFreq = std::log10(juce::jlimit(20.0f, 20000.0f, freqHz));
    return (logFreq - kLogFreqMin) / (kLogFreqMax - kLogFreqMin);
}

juce::Colour SpectrogramComponent::magnitudeToColour(float magnitude)
{
    magnitude = juce::jlimit(0.0f, 1.0f, magnitude);
    const int idx = juce::jlimit(0, 255, static_cast<int>(magnitude * 255.0f));
    return colormapLUT.empty() ? juce::Colours::black : colormapLUT[static_cast<size_t>(idx)];
}

void SpectrogramComponent::ensureImageAndLUT()
{
    if (spectrogramImage.getWidth() != kImageWidth || spectrogramImage.getHeight() != kImageHeight)
    {
        spectrogramImage = juce::Image(juce::Image::ARGB, kImageWidth, kImageHeight, true);
        juce::Graphics g(spectrogramImage);
        g.fillAll(juce::Colour(0xff0a0a12));
    }
    if (colormapLUT.size() != 256)
    {
        colormapLUT.resize(256);
        for (int i = 0; i < 256; ++i)
        {
            float t = static_cast<float>(i) / 255.0f;
            float r, g, b;
            if (t < 0.2f)
            {
                float x = t / 0.2f;
                r = 8.0f + (15.0f - 8.0f) * x;
                g = 8.0f + (30.0f - 8.0f) * x;
                b = 18.0f + (70.0f - 18.0f) * x;
            }
            else if (t < 0.45f)
            {
                float x = (t - 0.2f) / 0.25f;
                r = 15.0f + (20.0f - 15.0f) * x;
                g = 30.0f + (90.0f - 30.0f) * x;
                b = 70.0f + (110.0f - 70.0f) * x;
            }
            else if (t < 0.7f)
            {
                float x = (t - 0.45f) / 0.25f;
                r = 20.0f + (50.0f - 20.0f) * x;
                g = 90.0f + (190.0f - 90.0f) * x;
                b = 110.0f + (180.0f - 110.0f) * x;
            }
            else
            {
                float x = (t - 0.7f) / 0.3f;
                r = 50.0f + (220.0f - 50.0f) * x;
                g = 190.0f + (255.0f - 190.0f) * x;
                b = 180.0f + (250.0f - 180.0f) * x;
            }
            colormapLUT[static_cast<size_t>(i)] = juce::Colour::fromRGB(
                static_cast<uint8_t>(juce::jlimit(0, 255, static_cast<int>(r))),
                static_cast<uint8_t>(juce::jlimit(0, 255, static_cast<int>(g))),
                static_cast<uint8_t>(juce::jlimit(0, 255, static_cast<int>(b))));
        }
    }
}

void SpectrogramComponent::pushColumn(const float* magnitudes)
{
    if (magnitudes == nullptr || !spectrogramImage.isValid())
        return;
    const double sampleRate = processorRef.getCurrentSampleRate();
    const int h = spectrogramImage.getHeight();
    const int w = spectrogramImage.getWidth();

    for (int row = 0; row < h; ++row)
    {
        float normY = static_cast<float>(h - 1 - row) / static_cast<float>(h);
        float freqHz = pixelYToFreqHz(normY);
        float binFloat = freqHzToBinFloat(freqHz, sampleRate);
        int binLow = juce::jlimit(0, NUM_BINS - 2, static_cast<int>(binFloat));
        int binHigh = binLow + 1;
        float frac = binFloat - static_cast<float>(binLow);
        float mag = magnitudes[binLow] * (1.0f - frac) + magnitudes[binHigh] * frac;
        if (mag > displayMaxMagnitude)
            displayMaxMagnitude = mag;
        displayMaxMagnitude *= 0.9995f;
        if (displayMaxMagnitude < 0.001f)
            displayMaxMagnitude = 0.001f;
        float norm = mag / displayMaxMagnitude;
        juce::Colour c = magnitudeToColour(norm);
        spectrogramImage.setPixelAt(writeColumn, row, c);
    }
    writeColumn = (writeColumn + 1) % w;
}

void SpectrogramComponent::timerCallback()
{
    auto& fifo = processorRef.getSpectrogramFIFO();
    if (fifo.getBinsPerFrame() != NUM_BINS)
        return;
    while (fifo.pull(frameBuffer))
        pushColumn(frameBuffer.data());
    repaint();
}

} // namespace Specdrift
