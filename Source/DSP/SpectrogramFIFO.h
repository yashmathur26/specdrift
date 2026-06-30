#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "SpectralEngine.h"
#include <cstring>
#include <vector>

namespace Specdrift
{

/** Lock-free single-producer single-consumer FIFO for spectral frames (audio thread pushes, UI thread pulls). */
class SpectrogramFIFO
{
public:
    SpectrogramFIFO() = default;

    /** Call from processor prepareToPlay. numSlots = max frames to buffer (e.g. 64). */
    void prepare(int numBins, int numSlots)
    {
        const int total = numSlots * numBins;
        fifo.setTotalSize(total);
        buffer.resize(static_cast<size_t>(total), 0.0f);
        binsPerFrame = numBins;
    }

    /** Audio thread: push one frame of magnitudes. Returns true if pushed. */
    bool push(const float* frame)
    {
        if (frame == nullptr || binsPerFrame <= 0)
            return false;
        int start1, block1, start2, block2;
        fifo.prepareToWrite(binsPerFrame, start1, block1, start2, block2);
        if (block1 == 0 && block2 == 0)
            return false;
        auto* ptr = buffer.data();
        const int n = binsPerFrame;
        if (block1 >= n)
        {
            std::memcpy(ptr + start1, frame, static_cast<size_t>(n) * sizeof(float));
        }
        else
        {
            std::memcpy(ptr + start1, frame, static_cast<size_t>(block1) * sizeof(float));
            std::memcpy(ptr + start2, frame + block1, static_cast<size_t>(n - block1) * sizeof(float));
        }
        fifo.finishedWrite(block1 + block2);
        return true;
    }

    /** UI thread: pull one frame. Returns true if a frame was read. */
    bool pull(std::vector<float>& frame)
    {
        if (binsPerFrame <= 0 || frame.size() < static_cast<size_t>(binsPerFrame))
            return false;
        int start1, block1, start2, block2;
        fifo.prepareToRead(binsPerFrame, start1, block1, start2, block2);
        if (block1 == 0 && block2 == 0)
            return false;
        auto* ptr = buffer.data();
        const int n = binsPerFrame;
        if (block1 >= n)
        {
            std::memcpy(frame.data(), ptr + start1, static_cast<size_t>(n) * sizeof(float));
        }
        else
        {
            std::memcpy(frame.data(), ptr + start1, static_cast<size_t>(block1) * sizeof(float));
            std::memcpy(frame.data() + block1, ptr + start2, static_cast<size_t>(n - block1) * sizeof(float));
        }
        fifo.finishedRead(block1 + block2);
        return true;
    }

    int getBinsPerFrame() const { return binsPerFrame; }

private:
    juce::AbstractFifo fifo{ 0 };
    std::vector<float> buffer;
    int binsPerFrame{ 0 };
};

} // namespace Specdrift
