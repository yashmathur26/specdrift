#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SpecdriftProcessor::SpecdriftProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "SpecdriftState", Specdrift::createParameterLayout())
{
}

SpecdriftProcessor::~SpecdriftProcessor() {}

//==============================================================================
const juce::String SpecdriftProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SpecdriftProcessor::acceptsMidi() const { return false; }
bool SpecdriftProcessor::producesMidi() const { return false; }
bool SpecdriftProcessor::isMidiEffect() const { return false; }
double SpecdriftProcessor::getTailLengthSeconds() const { return 0.0; }

int SpecdriftProcessor::getNumPrograms() { return 1; }
int SpecdriftProcessor::getCurrentProgram() { return 0; }
void SpecdriftProcessor::setCurrentProgram(int index) { juce::ignoreUnused(index); }
const juce::String SpecdriftProcessor::getProgramName(int index)
{
    juce::ignoreUnused(index);
    return "Default";
}
void SpecdriftProcessor::changeProgramName(int index, const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

//==============================================================================
void SpecdriftProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    spectralEngine.prepare(sampleRate, samplesPerBlock);
    setLatencySamples(Specdrift::SpectralEngine::getLatencySamples());
    spectrogramFIFO.prepare(Specdrift::NUM_BINS, 64);
    spectralEngine.setSpectrogramFIFO(&spectrogramFIFO);

    updateCurveLookup();

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = 1;

    eqHighpassCoefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 20.0f);
    eqLowpassCoefficients  = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 20000.0f);
    eqHighpassLeft.coefficients  = eqHighpassCoefficients;
    eqHighpassRight.coefficients = eqHighpassCoefficients;
    eqLowpassLeft.coefficients  = eqLowpassCoefficients;
    eqLowpassRight.coefficients = eqLowpassCoefficients;
    eqHighpassLeft.prepare(spec);
    eqHighpassRight.prepare(spec);
    eqLowpassLeft.prepare(spec);
    eqLowpassRight.prepare(spec);

    updateEqFilters();

    dryDelayLeft.resize(static_cast<size_t>(Specdrift::FFT_SIZE), 0.0f);
    dryDelayRight.resize(static_cast<size_t>(Specdrift::FFT_SIZE), 0.0f);
    delayedDryLeft.resize(static_cast<size_t>(samplesPerBlock), 0.0f);
    delayedDryRight.resize(static_cast<size_t>(samplesPerBlock), 0.0f);
    dryDelayWritePos = 0;
}

void SpecdriftProcessor::releaseResources()
{
    spectralEngine.reset();
}

bool SpecdriftProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
    return true;
}

void SpecdriftProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);

    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    if (totalNumInputChannels == 1 && totalNumOutputChannels >= 2)
        buffer.copyFrom(1, 0, buffer, 0, 0, buffer.getNumSamples());

    const float inputGain  = apvts.getRawParameterValue(Specdrift::ParamID::inputGain)->load();
    const float outputGain = apvts.getRawParameterValue(Specdrift::ParamID::outputGain)->load();
    const float mix        = apvts.getRawParameterValue(Specdrift::ParamID::mix)->load();

    const int numSamples = buffer.getNumSamples();

    buffer.applyGain(inputGain);

    const float* leftIn  = buffer.getReadPointer(0);
    const float* rightIn = buffer.getReadPointer(1);

    // Always update dry delay so that when user raises mix from 0, we have correct delay‑compensated dry
    for (int i = 0; i < numSamples; ++i)
    {
        delayedDryLeft[static_cast<size_t>(i)]  = dryDelayLeft[static_cast<size_t>(dryDelayWritePos)];
        delayedDryRight[static_cast<size_t>(i)] = dryDelayRight[static_cast<size_t>(dryDelayWritePos)];
        dryDelayLeft[static_cast<size_t>(dryDelayWritePos)]  = leftIn[i];
        dryDelayRight[static_cast<size_t>(dryDelayWritePos)] = rightIn[i];
        dryDelayWritePos = (dryDelayWritePos + 1) % Specdrift::FFT_SIZE;
    }

    // Mix 0 = pure dry. Bypass spectral engine and EQ.
    if (mix < 0.001f)
    {
        smoothedMix = 0.0f;
        buffer.applyGain(outputGain);
        return;
    }

    const float blur   = apvts.getRawParameterValue(Specdrift::ParamID::blur)->load();
    const float tilt   = -apvts.getRawParameterValue(Specdrift::ParamID::tilt)->load();  // Flipped: -100 right, +100 left
    const float random = apvts.getRawParameterValue(Specdrift::ParamID::random)->load();
    const float gate   = apvts.getRawParameterValue(Specdrift::ParamID::gate)->load();

    const float* curvePtr = nullptr;
    bool useCurve = false;
    {
        std::lock_guard<std::mutex> lock(curveMutex);
        if (curveEnabled && curveLookup.size() >= static_cast<size_t>(Specdrift::NUM_BINS))
        {
            curveLookupCopy = curveLookup;
            curvePtr = curveLookupCopy.data();
            useCurve = true;
        }
    }

    if (totalNumInputChannels >= 2)
        spectralEngine.process(buffer, blur, tilt, random, gate, curvePtr, useCurve);

    // Low Cut: highpass on wet path (before mix) so blur has no bass; dry keeps original bass
    updateEqFilters();
    {
        juce::dsp::AudioBlock<float> block(buffer);
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto channelBlock = block.getSingleChannelBlock(static_cast<size_t>(ch));
            juce::dsp::ProcessContextReplacing<float> ctx(channelBlock);
            if (ch == 0) eqHighpassLeft.process(ctx);
            else         eqHighpassRight.process(ctx);
        }
    }

    float* leftOut  = buffer.getWritePointer(0);
    float* rightOut = buffer.getWritePointer(1);

    // Dry/wet mix
    for (int i = 0; i < numSamples; ++i)
    {
        smoothedMix += MIX_SMOOTH_COEF * (mix - smoothedMix);
        const float dryGain = 1.0f - smoothedMix;
        const float wetGain = smoothedMix;
        leftOut[i]  = delayedDryLeft[static_cast<size_t>(i)] * dryGain + leftOut[i] * wetGain;
        rightOut[i] = delayedDryRight[static_cast<size_t>(i)] * dryGain + rightOut[i] * wetGain;
    }

    // High Cut: lowpass on final output
    {
        juce::dsp::AudioBlock<float> block(buffer);
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto channelBlock = block.getSingleChannelBlock(static_cast<size_t>(ch));
            juce::dsp::ProcessContextReplacing<float> ctx(channelBlock);
            if (ch == 0) eqLowpassLeft.process(ctx);
            else         eqLowpassRight.process(ctx);
        }
    }

    buffer.applyGain(outputGain);
}

void SpecdriftProcessor::updateCurveLookup()
{
    std::lock_guard<std::mutex> lock(curveMutex);
    curveState.generateLookupTable(curveLookup, currentSampleRate);
}

void SpecdriftProcessor::updateEqFilters()
{
    float lowCutHz = 20.0f;
    float highCutHz = 20000.0f;
    if (auto* p = apvts.getParameter(Specdrift::ParamID::lowCut))
    {
        if (auto* fp = dynamic_cast<juce::AudioParameterFloat*>(p))
            lowCutHz = fp->get();
    }
    if (auto* p = apvts.getParameter(Specdrift::ParamID::highCut))
    {
        if (auto* fp = dynamic_cast<juce::AudioParameterFloat*>(p))
            highCutHz = fp->get();
    }
    lowCutHz = juce::jlimit(20.0f, 20000.0f, lowCutHz);
    highCutHz = juce::jlimit(20.0f, 20000.0f, highCutHz);
    eqHighpassCoefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(currentSampleRate, lowCutHz);
    eqLowpassCoefficients  = juce::dsp::IIR::Coefficients<float>::makeLowPass(currentSampleRate, highCutHz);
    eqHighpassLeft.coefficients  = eqHighpassCoefficients;
    eqHighpassRight.coefficients = eqHighpassCoefficients;
    eqLowpassLeft.coefficients  = eqLowpassCoefficients;
    eqLowpassRight.coefficients = eqLowpassCoefficients;
}

//==============================================================================
bool SpecdriftProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* SpecdriftProcessor::createEditor()
{
    return new SpecdriftEditor(*this);
}

//==============================================================================
void SpecdriftProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();

    juce::ValueTree curveStateTree("CurveState");
    curveStateTree.setProperty("curve_enabled", curveEnabled, nullptr);
    curveStateTree.setProperty("curve_points", curveState.toJson(), nullptr);
    state.appendChild(curveStateTree, nullptr);

    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void SpecdriftProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr)
    {
        auto state = juce::ValueTree::fromXml(*xmlState);
        apvts.replaceState(state);

        juce::ValueTree curveStateTree = state.getChildWithName("CurveState");
        if (curveStateTree.isValid())
        {
            curveEnabled = curveStateTree.getProperty("curve_enabled", false);
            juce::String pointsJson = curveStateTree.getProperty("curve_points", juce::String()).toString();
            if (pointsJson.isNotEmpty())
                curveState.fromJson(pointsJson);
            updateCurveLookup();
        }
    }
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SpecdriftProcessor();
}
