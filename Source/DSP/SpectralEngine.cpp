#include "SpectralEngine.h"
#include "SpectrogramFIFO.h"
#include <cmath>
#include <algorithm>

namespace Specdrift
{

namespace
{
    constexpr float MIN_SPEED = 0.001f;
    constexpr float MAX_SPEED = 1.0f;
}

SpectralEngine::SpectralEngine() : fft(11)
{
    randomSeeds.resize(static_cast<size_t>(NUM_BINS), 0.0f);
    std::mt19937 gen(12345678u);
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    for (int i = 0; i < NUM_BINS; ++i)
        randomSeeds[static_cast<size_t>(i)] = dist(gen);
}

float SpectralEngine::getBinMagnitude(const float* fftData, int bin)
{
    if (bin <= 0 || bin >= NUM_BINS - 1)
        return std::abs(fftData[bin * 2]);
    float re = fftData[bin * 2];
    float im = fftData[bin * 2 + 1];
    return std::sqrt(re * re + im * im);
}

void SpectralEngine::setBinMagnitude(float* fftData, int bin, float newMag)
{
    if (bin <= 0 || bin >= NUM_BINS - 1)
    {
        float oldRe = fftData[bin * 2];
        fftData[bin * 2] = (oldRe >= 0.0f ? newMag : -newMag);
        return;
    }
    float re = fftData[bin * 2];
    float im = fftData[bin * 2 + 1];
    float mag = std::sqrt(re * re + im * im);
    if (mag < 1.0e-10f)
    {
        fftData[bin * 2]     = newMag;
        fftData[bin * 2 + 1] = 0.0f;
        return;
    }
    float scale = newMag / mag;
    fftData[bin * 2]     = re * scale;
    fftData[bin * 2 + 1] = im * scale;
}

void SpectralEngine::prepare(double sampleRate, int maxSamplesPerBlock)
{
    juce::ignoreUnused(sampleRate);

    maxBlockSize = maxSamplesPerBlock;

    ringSize = FFT_SIZE + maxBlockSize * 2;
    inputRingLeft.resize(static_cast<size_t>(ringSize), 0.0f);
    inputRingRight.resize(static_cast<size_t>(ringSize), 0.0f);
    outputRingLeft.resize(static_cast<size_t>(ringSize), 0.0f);
    outputRingRight.resize(static_cast<size_t>(ringSize), 0.0f);

    windowedInput.resize(static_cast<size_t>(FFT_SIZE), 0.0f);
    fftData.resize(static_cast<size_t>(2 * FFT_SIZE), 0.0f);
    hannWindow.resize(static_cast<size_t>(FFT_SIZE), 0.0f);

    previousMagnitudesLeft.resize(static_cast<size_t>(NUM_BINS), 0.0f);
    previousMagnitudesRight.resize(static_cast<size_t>(NUM_BINS), 0.0f);
    speedLookup.resize(static_cast<size_t>(NUM_BINS), 0.0f);
    displayMagnitudes.resize(static_cast<size_t>(NUM_BINS), 0.0f);

    for (int i = 0; i < FFT_SIZE; ++i)
    {
        float n = static_cast<float>(i);
        hannWindow[static_cast<size_t>(i)] = 0.5f * (1.0f - std::cos(2.0f * static_cast<float>(M_PI) * n / static_cast<float>(FFT_SIZE)));
    }

    reset();
}

void SpectralEngine::reset()
{
    std::fill(inputRingLeft.begin(), inputRingLeft.end(), 0.0f);
    std::fill(inputRingRight.begin(), inputRingRight.end(), 0.0f);
    std::fill(outputRingLeft.begin(), outputRingLeft.end(), 0.0f);
    std::fill(outputRingRight.begin(), outputRingRight.end(), 0.0f);
    std::fill(previousMagnitudesLeft.begin(), previousMagnitudesLeft.end(), 0.0f);
    std::fill(previousMagnitudesRight.begin(), previousMagnitudesRight.end(), 0.0f);

    smoothedBlur   = targetBlur;
    smoothedTilt   = targetTilt;
    smoothedRandom = targetRandom;
    smoothedGate   = targetGate;

    inputWritePos       = 0;
    outputReadPos       = 0;
    outputWritePos      = 0;
    samplesUntilNextHop = FFT_SIZE;
    latencyRemaining    = FFT_SIZE;
}

void SpectralEngine::process(juce::AudioBuffer<float>& buffer, float blur, float tilt, float random, float gate,
                            const float* curveLookup, bool useCurve)
{
    targetBlur   = std::max(0.0f, std::min(1.0f, blur));
    targetTilt   = std::max(-1.0f, std::min(1.0f, tilt));
    targetRandom = std::max(0.0f, std::min(1.0f, random));
    targetGate   = std::max(0.0f, std::min(1.0f, gate));

    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    if (numChannels < 2)
        return;

    const float* leftIn  = buffer.getReadPointer(0);
    const float* rightIn = buffer.getReadPointer(1);
    float* leftOut       = buffer.getWritePointer(0);
    float* rightOut      = buffer.getWritePointer(1);

    for (int i = 0; i < numSamples; ++i)
    {
        inputRingLeft[static_cast<size_t>(inputWritePos)]  = leftIn[i];
        inputRingRight[static_cast<size_t>(inputWritePos)] = rightIn[i];
        inputWritePos = (inputWritePos + 1) % ringSize;

        samplesUntilNextHop--;
        if (samplesUntilNextHop <= 0)
        {
            samplesUntilNextHop = HOP_SIZE;
            processHop(curveLookup, useCurve);
        }

        if (latencyRemaining > 0)
        {
            leftOut[i]  = 0.0f;
            rightOut[i] = 0.0f;
            latencyRemaining--;
        }
        else
        {
            leftOut[i]  = outputRingLeft[static_cast<size_t>(outputReadPos)];
            rightOut[i] = outputRingRight[static_cast<size_t>(outputReadPos)];
            // Clear after read so the next overlap-add write to this slot doesn't accumulate on old data
            outputRingLeft[static_cast<size_t>(outputReadPos)]  = 0.0f;
            outputRingRight[static_cast<size_t>(outputReadPos)] = 0.0f;
            outputReadPos = (outputReadPos + 1) % ringSize;
        }
    }
}

void SpectralEngine::processHop(const float* curveLookup, bool useCurve)
{
    smoothedBlur   += SMOOTH_COEF * (targetBlur   - smoothedBlur);
    smoothedTilt   += SMOOTH_COEF * (targetTilt   - smoothedTilt);
    smoothedRandom += SMOOTH_COEF * (targetRandom - smoothedRandom);
    smoothedGate   += SMOOTH_COEF * (targetGate   - smoothedGate);

    if (useCurve && curveLookup != nullptr)
    {
        for (int i = 0; i < NUM_BINS; ++i)
        {
            float speed = curveLookup[i] + smoothedRandom * randomSeeds[static_cast<size_t>(i)] * 0.5f;
            speedLookup[static_cast<size_t>(i)] = std::max(MIN_SPEED, std::min(MAX_SPEED, speed));
        }
    }
    else
    {
        const float invBins = 1.0f / static_cast<float>(NUM_BINS - 1);
        for (int i = 0; i < NUM_BINS; ++i)
        {
            float pos = static_cast<float>(i) * invBins;
            float speed = (1.0f - smoothedBlur) + smoothedTilt * (pos - 0.5f) + smoothedRandom * randomSeeds[static_cast<size_t>(i)];
            speedLookup[static_cast<size_t>(i)] = std::max(MIN_SPEED, std::min(MAX_SPEED, speed));
        }
    }

    int readStart = (inputWritePos - FFT_SIZE + ringSize) % ringSize;
    if (readStart < 0)
        readStart += ringSize;

    // --- Left channel ---
    for (int j = 0; j < FFT_SIZE; ++j)
    {
        int idx = (readStart + j) % ringSize;
        windowedInput[static_cast<size_t>(j)] = inputRingLeft[static_cast<size_t>(idx)] * hannWindow[static_cast<size_t>(j)];
    }
    for (int j = 0; j < FFT_SIZE; ++j)
        fftData[static_cast<size_t>(j)] = windowedInput[static_cast<size_t>(j)];
    std::fill(fftData.begin() + FFT_SIZE, fftData.end(), 0.0f);

    fft.performRealOnlyForwardTransform(fftData.data(), true);

    float maxMag = 0.0f;
    for (int b = 0; b < NUM_BINS; ++b)
    {
        float mag = getBinMagnitude(fftData.data(), b);
        if (mag > maxMag)
            maxMag = mag;
    }
    // When input is silent, fast-decay blur state so we don't get a long ringing tail
    if (maxMag < SILENCE_THRESHOLD)
    {
        for (int b = 0; b < NUM_BINS; ++b)
        {
            previousMagnitudesLeft[static_cast<size_t>(b)]  *= SILENCE_DECAY;
            previousMagnitudesRight[static_cast<size_t>(b)] *= SILENCE_DECAY;
        }
    }
    float threshold = (maxMag > 1.0e-12f) ? (smoothedGate * maxMag * 0.3f) : 0.0f;
    float threshLow  = threshold * (1.0f - GATE_KNEE);
    float threshHigh = threshold * (1.0f + GATE_KNEE);
    if (threshHigh <= threshLow)
        threshHigh = threshLow + 1.0e-6f;

    for (int b = 0; b < NUM_BINS; ++b)
    {
        float mag = getBinMagnitude(fftData.data(), b);
        float prev = previousMagnitudesLeft[static_cast<size_t>(b)];
        float speed = speedLookup[static_cast<size_t>(b)];
        float newMag = prev + speed * (mag - prev);
        previousMagnitudesLeft[static_cast<size_t>(b)] = newMag;

        if (threshold > 1.0e-12f)
        {
            if (newMag <= threshLow)
                newMag = 0.0f;
            else if (newMag < threshHigh)
                newMag *= (newMag - threshLow) / (threshHigh - threshLow);
        }
        setBinMagnitude(fftData.data(), b, newMag);
    }

    // Phase 4: push display-smoothed magnitudes to spectrogram FIFO (left channel)
    if (spectrogramFIFO != nullptr)
    {
        for (int b = 0; b < NUM_BINS; ++b)
        {
            float current = previousMagnitudesLeft[static_cast<size_t>(b)];
            float smoothed = displayMagnitudes[static_cast<size_t>(b)];
            if (current > smoothed)
                displayMagnitudes[static_cast<size_t>(b)] = smoothed + DISPLAY_ATTACK * (current - smoothed);
            else
                displayMagnitudes[static_cast<size_t>(b)] = smoothed + DISPLAY_RELEASE * (current - smoothed);
        }
        spectrogramFIFO->push(displayMagnitudes.data());
    }

    fft.performRealOnlyInverseTransform(fftData.data());

    int outStart = outputWritePos;
    for (int j = 0; j < FFT_SIZE; ++j)
    {
        int idx = (outStart + j) % ringSize;
        outputRingLeft[static_cast<size_t>(idx)] += fftData[static_cast<size_t>(j)] * hannWindow[static_cast<size_t>(j)] * COLA_GAIN;
    }

    // --- Right channel ---
    for (int j = 0; j < FFT_SIZE; ++j)
    {
        int idx = (readStart + j) % ringSize;
        windowedInput[static_cast<size_t>(j)] = inputRingRight[static_cast<size_t>(idx)] * hannWindow[static_cast<size_t>(j)];
    }
    for (int j = 0; j < FFT_SIZE; ++j)
        fftData[static_cast<size_t>(j)] = windowedInput[static_cast<size_t>(j)];
    std::fill(fftData.begin() + FFT_SIZE, fftData.end(), 0.0f);

    fft.performRealOnlyForwardTransform(fftData.data(), true);

    maxMag = 0.0f;
    for (int b = 0; b < NUM_BINS; ++b)
    {
        float mag = getBinMagnitude(fftData.data(), b);
        if (mag > maxMag)
            maxMag = mag;
    }
    threshold = (maxMag > 1.0e-12f) ? (smoothedGate * maxMag * 0.3f) : 0.0f;
    threshLow  = threshold * (1.0f - GATE_KNEE);
    threshHigh = threshold * (1.0f + GATE_KNEE);
    if (threshHigh <= threshLow)
        threshHigh = threshLow + 1.0e-6f;

    for (int b = 0; b < NUM_BINS; ++b)
    {
        float mag = getBinMagnitude(fftData.data(), b);
        float prev = previousMagnitudesRight[static_cast<size_t>(b)];
        float speed = speedLookup[static_cast<size_t>(b)];
        float newMag = prev + speed * (mag - prev);
        previousMagnitudesRight[static_cast<size_t>(b)] = newMag;

        if (threshold > 1.0e-12f)
        {
            if (newMag <= threshLow)
                newMag = 0.0f;
            else if (newMag < threshHigh)
                newMag *= (newMag - threshLow) / (threshHigh - threshLow);
        }
        setBinMagnitude(fftData.data(), b, newMag);
    }

    fft.performRealOnlyInverseTransform(fftData.data());

    for (int j = 0; j < FFT_SIZE; ++j)
    {
        int idx = (outStart + j) % ringSize;
        outputRingRight[static_cast<size_t>(idx)] += fftData[static_cast<size_t>(j)] * hannWindow[static_cast<size_t>(j)] * COLA_GAIN;
    }

    outputWritePos = (outputWritePos + HOP_SIZE) % ringSize;
}

} // namespace Specdrift
