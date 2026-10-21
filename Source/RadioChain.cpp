#include "RadioChain.h"

void RadioChain::prepare (double sampleRate, int maxBlockSize, int numChannels)
{
    radio.prepare (sampleRate, maxBlockSize, numChannels);
    scene.prepare (sampleRate, maxBlockSize, numChannels);
    this->sampleRate = sampleRate;
    curSoloFreq = -1;
    for (auto& f : soloFilters) f.reset();
    outGain.reset (sampleRate, 0.03);
    outGain.setCurrentAndTargetValue (1.0f);
}

void RadioChain::reset()
{
    radio.reset();
    scene.reset();
}

void RadioChain::process (juce::AudioBuffer<float>& buffer, const RadioParams& p)
{
    radio.process (buffer, p);
    scene.process (buffer, p);

    // Volumen final, despues de la escena: subirlo no satura el parlante simulado.
    const int n = buffer.getNumSamples();
    outGain.setTargetValue (juce::Decibels::decibelsToGain (p.outGainDb));
    if (outGain.isSmoothing())
        for (int i = 0; i < n; ++i)
        {
            const float g = outGain.getNextValue();
            for (int c = 0; c < buffer.getNumChannels(); ++c) buffer.getWritePointer (c)[i] *= g;
        }
    else
        buffer.applyGain (outGain.getTargetValue());

    if (soloOn.load())
    {
        const float f = juce::jlimit (20.0f, (float) (sampleRate * 0.45), soloFreq.load());
        const float q = juce::jlimit (0.3f, 10.0f, soloQ.load());
        const int type = soloType.load();
        if (std::abs (f - curSoloFreq) > 0.5f || std::abs (q - curSoloQ) > 0.01f || type != curSoloType)
        {
            using C = juce::dsp::IIR::Coefficients<float>;
            auto c = type == 1 ? C::makeLowPass (sampleRate, f) : (type == 2 ? C::makeHighPass (sampleRate, f) : C::makeBandPass (sampleRate, f, q));
            for (auto& filt : soloFilters) { filt.coefficients = c; if (type != curSoloType) filt.reset(); }
            curSoloFreq = f; curSoloQ = q; curSoloType = type;
        }
        for (int c = 0; c < juce::jmin (2, buffer.getNumChannels()); ++c)
        {
            auto* d = buffer.getWritePointer (c);
            for (int i = 0; i < n; ++i) d[i] = soloFilters[(size_t) c].processSample (d[i]);
        }
    }

    float peak = 0;
    for (int c = 0; c < buffer.getNumChannels(); ++c)
        peak = std::max (peak, buffer.getMagnitude (c, 0, n));
    lastPeak = peak;
}
