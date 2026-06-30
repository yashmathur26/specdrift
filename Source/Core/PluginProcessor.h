#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "Parameters.h"
#include "DSP/SpectralEngine.h"
#include "DSP/SpectrogramFIFO.h"
#include "DSP/CurveState.h"
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <mutex>

//==============================================================================
class SpecdriftProcessor : public juce::AudioProcessor
{
public:
    SpecdriftProcessor();
    ~SpecdriftProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }
    const juce::AudioProcessorValueTreeState& getAPVTS() const { return apvts; }

    Specdrift::SpectrogramFIFO& getSpectrogramFIFO() { return spectrogramFIFO; }
    const Specdrift::SpectrogramFIFO& getSpectrogramFIFO() const { return spectrogramFIFO; }
    double getCurrentSampleRate() const { return currentSampleRate; }

    // Phase 5: Drawable curve (spectrogram overlay)
    Specdrift::CurveState& getCurveState() { return curveState; }
    const Specdrift::CurveState& getCurveState() const { return curveState; }
    void updateCurveLookup();
    bool getCurveEnabled() const { return curveEnabled; }
    void setCurveEnabled(bool v) { curveEnabled = v; }

private:
    juce::AudioProcessorValueTreeState apvts;
    Specdrift::SpectralEngine spectralEngine;
    Specdrift::SpectrogramFIFO spectrogramFIFO;
    double currentSampleRate{44100.0};

    // Phase 5: Curve
    Specdrift::CurveState curveState;
    std::vector<float> curveLookup;
    std::vector<float> curveLookupCopy;
    std::mutex curveMutex;
    bool curveEnabled{false};

    // EQ: Low Cut (highpass on wet path), High Cut (lowpass on output) — 2nd order Butterworth
    juce::dsp::IIR::Filter<float> eqHighpassLeft;
    juce::dsp::IIR::Filter<float> eqHighpassRight;
    juce::dsp::IIR::Filter<float> eqLowpassLeft;
    juce::dsp::IIR::Filter<float> eqLowpassRight;
    juce::dsp::IIR::Coefficients<float>::Ptr eqHighpassCoefficients;
    juce::dsp::IIR::Coefficients<float>::Ptr eqLowpassCoefficients;

    void updateEqFilters();

    // Dry delay line for mix (delay-compensate dry by FFT_SIZE samples)
    std::vector<float> dryDelayLeft;
    std::vector<float> dryDelayRight;
    std::vector<float> delayedDryLeft;
    std::vector<float> delayedDryRight;
    int dryDelayWritePos{0};

    float smoothedMix{0.0f};
    static constexpr float MIX_SMOOTH_COEF = 0.002f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpecdriftProcessor)
};
