#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <random>
#include <vector>

namespace Specdrift
{

/** Constants from SPECDRIFT_ROADMAP */
constexpr int FFT_SIZE   = 2048;
constexpr int HOP_SIZE   = 512;
constexpr int NUM_BINS   = FFT_SIZE / 2 + 1;  // 1025
constexpr float OVERLAP_FACTOR = static_cast<float>(FFT_SIZE) / static_cast<float>(HOP_SIZE);  // 4.0

/** COLA gain for Hann window at 75% overlap: 1/1.5 */
constexpr float COLA_GAIN = 1.0f / 1.5f;

/** Parameter smoothing coefficient per hop (lerp toward target). */
constexpr float SMOOTH_COEF = 0.1f;

/** Soft knee width for gate (fraction of threshold). */
constexpr float GATE_KNEE = 0.3f;

/** When input frame max magnitude is below this, treat as silence and fast-decay blur state to avoid ringing tail. */
constexpr float SILENCE_THRESHOLD = 1e-5f;
/** Factor applied per hop when silent (previous *= SILENCE_DECAY) so tail dies in a few hops. */
constexpr float SILENCE_DECAY = 0.05f;

/** Display smoothing for spectrogram: fast attack, slow release (Phase 4). */
constexpr float DISPLAY_ATTACK  = 0.6f;
constexpr float DISPLAY_RELEASE = 0.05f;

/**
 * SpectralEngine: FFT → per-bin blur + gate → IFFT → overlap-add
 *
 * Phase 2: Blur (speed limit per bin), gate (soft knee), parameters from host.
 * All buffers pre-allocated in prepare(). Zero allocations on audio thread.
 */
class SpectralEngine
{
public:
    SpectralEngine();
    ~SpectralEngine() = default;

    /** Call before processing. Resizes all buffers. */
    void prepare(double sampleRate, int maxBlockSize);

/** Process stereo buffer. In-place. Parameters: blur, tilt, random, gate (0-1).
 *  When curveLookup != nullptr and useCurve, per-bin speed comes from curveLookup + random; else from blur/tilt/random. */
void process(juce::AudioBuffer<float>& buffer, float blur, float tilt, float random, float gate,
            const float* curveLookup, bool useCurve);

    /** Reset state (e.g. on sample rate change). */
    void reset();

    /** Latency in samples (FFT_SIZE for overlap-add). */
    static constexpr int getLatencySamples() { return FFT_SIZE; }

    /** Optional: set for spectrogram UI. Audio thread pushes display-smoothed magnitudes each hop. */
    void setSpectrogramFIFO(class SpectrogramFIFO* fifo) { spectrogramFIFO = fifo; }

private:
    void processHop(const float* curveLookup, bool useCurve);
    /** Get magnitude of bin i from fftData (real FFT layout: bin i = [2*i], [2*i+1]). */
    static float getBinMagnitude(const float* fftData, int bin);
    /** Set magnitude of bin i preserving phase. */
    static void setBinMagnitude(float* fftData, int bin, float newMag);

    juce::dsp::FFT fft{11};  // 2^11 = 2048

    // Ring buffers (stereo)
    std::vector<float> inputRingLeft;
    std::vector<float> inputRingRight;
    std::vector<float> outputRingLeft;
    std::vector<float> outputRingRight;

    int inputWritePos{0};
    int outputReadPos{0};
    int outputWritePos{0};
    int ringSize{0};
    int latencyRemaining{0};

    // Work buffers
    std::vector<float> windowedInput;
    std::vector<float> fftData;
    std::vector<float> hannWindow;

    // Phase 2a: Blur state
    std::vector<float> previousMagnitudesLeft;
    std::vector<float> previousMagnitudesRight;
    std::vector<float> randomSeeds;
    std::vector<float> speedLookup;

    // Smoothed parameters (lerp per hop)
    float smoothedBlur{0.0f};
    float smoothedTilt{0.0f};
    float smoothedRandom{0.0f};
    float smoothedGate{0.0f};

    // Current parameter targets (set each process() call)
    float targetBlur{0.0f};
    float targetTilt{0.0f};
    float targetRandom{0.0f};
    float targetGate{0.0f};

    int samplesUntilNextHop{0};
    int maxBlockSize{0};

    // Phase 4: spectrogram (optional)
    class SpectrogramFIFO* spectrogramFIFO{nullptr};
    std::vector<float> displayMagnitudes;
};

} // namespace Specdrift
