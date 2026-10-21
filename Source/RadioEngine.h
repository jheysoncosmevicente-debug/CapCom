#pragma once

#include <juce_dsp/juce_dsp.h>
#include "Parameters.h"

// Cadena de radio: entrada -> cinta (wow/flutter) -> paso-altos -> paso-bajos -> medios -> compresor -> drive
// -> bits/muestreo -> soplido + estatica + squelch -> filtro final -> fallas de codec -> cortes
// -> tonos Quindar -> mezcla -> salida.
// La usa el plugin en tiempo real y tambien el exportador de WAV de la app independiente.
class RadioEngine
{
public:
    static constexpr int maxChannels = 2;
    static constexpr int maxStages = 4; // 48 dB/oct = 4 biquads

    void prepare (double sampleRate, int maxBlockSize, int numChannels);
    void reset();
    void process (juce::AudioBuffer<float>& buffer, const RadioParams& p);

    bool isTransmitting() const noexcept { return gateOpen; }
    float getLastPeak() const noexcept   { return lastPeak; }
    // Cuanto bajo el compresor en el ultimo bloque (dB, 0 o negativo).
    float getGainReductionDb() const noexcept { return lastGrDb; }

    // Coeficientes de una banda libre del ecualizador (tambien los usa la interfaz para dibujar).
    static juce::dsp::IIR::Coefficients<float>::Ptr bandCoefs (const RadioParams::Band& b, double sampleRate);

    // Respuesta en frecuencia (dB) de la parte de filtros/EQ, para dibujar la curva.
    static float responseDb (const RadioParams& p, double freq, double sampleRate);

private:
    using Coefs = juce::dsp::IIR::Coefficients<float>;
    using CoefArray = juce::ReferenceCountedArray<Coefs>;

    struct FilterGroup
    {
        std::array<juce::dsp::IIR::Filter<float>, maxStages> stages;
        int numStages = 0;
        float process (float x) noexcept
        {
            for (int i = 0; i < numStages; ++i)
                x = stages[(size_t) i].processSample (x);
            return x;
        }
        void reset() { for (auto& s : stages) s.reset(); }
    };

    struct Channel
    {
        FilterGroup hp, lp, postHp, postLp;
        juce::dsp::IIR::Filter<float> mid;
        std::array<juce::dsp::IIR::Filter<float>, numEqBands> band;
        float heldSample = 0, holdPhase = 1;

        // Fallas de codec: ultimos milisegundos de audio, para repetir un "paquete".
        std::vector<float> glitchBuf;
        int glitchWrite = 0, glitchRead = 0, glitchFramePos = 0;
    };

    // Valores que cambian muestra a muestra y comparten todos los canales.
    struct SampleState
    {
        float driveGain, driveNorm, hissAmp, squelchEnv, crackleVal, dropGain;
        float tapeDelay;      // en muestras; < 0 = cinta apagada
        float glitchMix;      // 0 = senal normal, 1 = paquete repetido/silencio
        bool glitchStart;
    };

    void updateFilters (const RadioParams& p, bool force);
    static void assign (FilterGroup& g, const CoefArray& c, bool resetState);
    float shape (float x, int type) const noexcept;
    float processChannel (Channel& c, int ch, float x, const RadioParams& p, const SampleState& s);
    void startQuindar (float freq);
    void startRoger (int type);
    float nextRogerSample() noexcept;

    double sr = 48000;
    int numCh = 2;
    std::array<Channel, maxChannels> chans;
    juce::dsp::Compressor<float> compressor;
    float makeupGain = 1;
    juce::Random rng;

    // Valores con los que se disenaron los filtros (para no redisenarlos en cada bloque).
    float curHp = -1, curLp = -1, curMidF = -1, curMidG = -100, curMidQ = -1;
    int curHpOrder = -1, curLpOrder = -1;
    std::array<RadioParams::Band, numEqBands> curBands {};
    bool bandsReady = false;

    // Medicion de compresion (canal 0): energia antes y despues del compresor.
    float grIn = 0, grOut = 0, lastGrDb = 0;

    juce::SmoothedValue<float> inGainS, mixS, hissS, wowS, flutterS;

    // Deteccion de voz para el squelch y el "ruido solo al hablar".
    float env = 0, envAtt = 0, envRel = 0;
    bool gateOpen = false;
    int belowCount = 0, holdSamples = 0;
    float carrier = 0, carrierAtt = 0, carrierRel = 0;

    // Rafaga de squelch ("kshh").
    int burstPos = 0, burstLen = 0, burstFadeOutStart = 0;
    float burstGain = 0;
    bool burstClick = false;

    // Estatica y cortes de senal.
    float crackleEnv = 0;
    int dropRemaining = 0;
    float dropTarget = 1, dropGain = 1, dropSmooth = 0;

    // Tonos Quindar (NASA): 2525 Hz al abrir la transmision, 2475 Hz al cerrarla.
    int quindarPos = 0, quindarLen = 0;
    float quindarFreq = 0, quindarPhase = 0;

    // Fallas de codec (celular).
    int glitchRemaining = 0, glitchFrameLen = 0;
    bool glitchRepeat = false;
    float glitchMix = 0, glitchSmooth = 0;

    // Cinta: linea de retardo modulada por los LFO de wow (lentos) y flutter (rapidos).
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> tapeLine;
    std::array<double, 4> tapePhase {};

    // Roger beep: patron de tonos al soltar (y opcionalmente al apretar) el boton.
    int rogerType = 0, rogerPos = -1, rogerLen = 0;
    float rogerPhase = 0, rogerAmp = 0;

    // Modo manual: el boton "Transmitir" abre y cierra el transmisor; fuera de el no pasa la voz.
    float keyGain = 1, keySmooth = 0;


    float lastPeak = 0;
};
