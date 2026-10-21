#pragma once

#include "RadioEngine.h"
#include "SceneEngine.h"

// Todo el procesamiento de CapCom en orden: radio (transmision, roger, squelch...) -> escena -> volumen final.
// La usan el plugin en tiempo real y el exportador de la app.
// Los sonidos sueltos (sirenas, ambientes, tonos de telefono) van aparte, en la biblioteca de WAV.
class RadioChain
{
public:
    void prepare (double sampleRate, int maxBlockSize, int numChannels);
    void reset();
    void process (juce::AudioBuffer<float>& buffer, const RadioParams& p);

    bool isTransmitting() const noexcept { return radio.isTransmitting(); }
    float getLastPeak() const noexcept   { return lastPeak; }
    float getGainReductionDb() const noexcept { return radio.getGainReductionDb(); }

    // Escuchar una banda: filtra la salida para oir solo esa zona (lo controla la interfaz).
    void setSolo (bool on, float freq, float q, int type)
    {
        soloFreq.store (freq); soloQ.store (q); soloType.store (type); soloOn.store (on);
    }
    bool isSoloOn() const noexcept { return soloOn.load(); }

private:
    RadioEngine radio;
    SceneEngine scene;
    juce::SmoothedValue<float> outGain;

    std::atomic<bool> soloOn { false };
    std::atomic<float> soloFreq { 1000 }, soloQ { 1 };
    std::atomic<int> soloType { 0 };
    std::array<juce::dsp::IIR::Filter<float>, 2> soloFilters;
    float curSoloFreq = -1, curSoloQ = -1;
    int curSoloType = -1;
    double sampleRate = 48000;
    float lastPeak = 0;
};
