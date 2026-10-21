#pragma once

#include <juce_dsp/juce_dsp.h>
#include "Parameters.h"

// Pone la senal dentro de una escena: aparato que la reproduce -> objeto que lo tapa (caja, maletera...)
// -> distancia -> espacio (reverb) -> pared/puerta/ventana entre el espacio y quien escucha.
// Los aparatos son modelos sinteticos (filtros + resonancia de caja + saturacion del cono), sin latencia.
class SceneEngine
{
public:
    static constexpr int maxChannels = 2;

    void prepare (double sampleRate, int maxBlockSize, int numChannels);
    void reset();
    void process (juce::AudioBuffer<float>& buffer, const RadioParams& p);

    // Respuesta (dB) de aparato + obstaculo + distancia, para sumarla a la curva de la interfaz.
    static float responseDb (const RadioParams& p, double freq, double sampleRate);

    static bool isActive (const RadioParams& p) { return p.device > 0 || p.obstacle > 0 || p.distance > 0.5f || p.room > 0; }

private:
    using Filter = juce::dsp::IIR::Filter<float>;
    using Coefs = juce::dsp::IIR::Coefficients<float>;
    using CoefList = std::vector<Coefs::Ptr>;

    static constexpr int maxFilters = 10;

    // Filtros fijos en cascada (paso-altos, paso-bajos y resonancias) mas un peine corto de "caja".
    struct Colour
    {
        std::array<std::array<Filter, maxFilters>, maxChannels> filters;
        std::array<std::vector<float>, maxChannels> combBuf;
        std::array<int, maxChannels> combPos {};
        int numFilters = 0, combLen = 0;
        float combFb = 0, gain = 1;
        bool active = false;

        void allocate (int maxCombSamples);
        void configure (const CoefList& coefs, int combSamples, float fb, float linearGain, bool resetState);
        float process (int ch, float x) noexcept;
        void reset();
    };

    // Un circulo de muestras simple para predelay, reflexiones tempranas y ecos.
    struct Ring
    {
        std::array<std::vector<float>, maxChannels> buf;
        int pos = 0, size = 1;
        void allocate (int samples) { size = juce::jmax (1, samples); for (auto& b : buf) b.assign ((size_t) size, 0.0f); pos = 0; }
        void reset() { for (auto& b : buf) std::fill (b.begin(), b.end(), 0.0f); pos = 0; }
        float read (int ch, int delay) const noexcept { return buf[(size_t) ch][(size_t) ((pos - delay + size * 2) % size)]; }
        void write (int ch, float x) noexcept { buf[(size_t) ch][(size_t) pos] = x; }
        void advance() noexcept { pos = (pos + 1) % size; }
    };

    void updateSettings (const RadioParams& p, bool force);
    void processRoom (juce::AudioBuffer<float>& buffer, int start, int n, int channels);

    static CoefList deviceCoefs (const RadioParams& p, double sr);
    static CoefList obstacleCoefs (int obstacle, double sr);
    static CoefList distanceCoefs (float distance, double sr);

    double sr = 48000;
    int numCh = 2, maxBlock = 512;

    Colour deviceColour, boxColour, wallColour, distanceColour;
    float deviceDriveGain = 1, deviceDriveNorm = 1;
    float distanceGain = 1;

    // Espacio
    juce::Reverb reverb;
    Ring preRing, erRing, slapRing;
    std::array<Filter, maxChannels> wetHp, wetLp;
    juce::AudioBuffer<float> revBuf, erBuf;
    std::array<int, 6> erTapsL {}, erTapsR {};
    float erGain = 0, revGain = 0, slapFb = 0, roomWet = 0;
    int preDelaySamples = 0, slapSamples = 0;
    bool roomActive = false;

    // Ultimos valores aplicados, para recalcular solo cuando algo cambia.
    int curDevice = -1, curObstacle = -1, curRoom = -1;
    float curTune = -999, curDrive = -1, curDistance = -1, curRoomMix = -1;
};
