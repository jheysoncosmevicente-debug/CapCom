#include "SceneEngine.h"

namespace
{
    using Design = juce::dsp::FilterDesign<float>;
    using Coefs = juce::dsp::IIR::Coefficients<float>;

    float clampF (float f, double sr) { return juce::jlimit (20.0f, (float) (sr * 0.45), f); }

    struct Peak { float freq, gainDb, q; };

    // Modelos de aparatos (indice = deviceChoices). Frecuencias a "Tono del aparato" = 0.
    struct DeviceModel
    {
        float hp; int hpOrder; float lp; int lpOrder;
        std::array<Peak, 3> peaks;
        float boxMs, boxFb;     // resonancia de la caja/bocina
        float breakup;          // cuanto satura el cono (0-1) con "Saturacion del parlante" al 100 %
        float levelDb;          // compensacion de volumen
    };

    const std::array<DeviceModel, 11> devices {{
        { 20, 2, 20000, 2, {{ { 1000, 0, 1 }, { 1000, 0, 1 }, { 1000, 0, 1 } }}, 0, 0, 0, 0 },                        // Ninguno
        { 300, 4, 5000, 4,  {{ { 1000, 4, 1.5f }, { 2600, 6, 2.0f }, { 4200, -3, 2 } }}, 0.8f, 0.40f, 0.50f, -2 },   // Radio de patrulla
        { 350, 4, 4500, 4,  {{ { 800, 4, 1.5f }, { 2200, 6, 2.5f }, { 3200, -4, 2 } }}, 0.6f, 0.35f, 0.35f, -2 },    // Radio de transistores
        { 180, 2, 6500, 2,  {{ { 450, 3, 1.0f }, { 1300, -2, 1.0f }, { 3000, 5, 2 } }}, 1.2f, 0.30f, 0.15f, -1 },    // TV antigua
        { 500, 4, 9000, 2,  {{ { 900, 5, 2.0f }, { 2500, 3, 1.5f }, { 5000, 4, 2 } }},  0.4f, 0.25f, 0.15f, -2 },    // Laptop
        { 700, 4, 8000, 2,  {{ { 1100, 6, 2.5f }, { 3000, 5, 2.0f }, { 5500, 4, 3 } }}, 0.3f, 0.30f, 0.40f, -3 },    // Celular (altavoz)
        { 90, 2, 10000, 2,  {{ { 120, 5, 1.2f }, { 400, 3, 1.0f }, { 2500, -3, 1.5f } }}, 2.2f, 0.40f, 0.15f, -3 },  // Parlante de auto
        { 600, 6, 4500, 4,  {{ { 1200, 8, 3.0f }, { 2400, 6, 3.0f }, { 3600, 5, 4 } }}, 0.5f, 0.55f, 0.80f, -6 },    // Megafono
        { 250, 4, 7000, 2,  {{ { 1500, 4, 1.2f }, { 3500, 3, 2.0f }, { 600, -2, 1 } }}, 1.5f, 0.30f, 0.25f, -2 },    // Altavoz de techo
        { 400, 4, 3800, 4,  {{ { 1000, 6, 2.0f }, { 2000, 5, 3.0f }, { 3000, -3, 2 } }}, 0.5f, 0.45f, 0.55f, -4 },   // Portero electrico
        { 1200, 4, 12000, 2, {{ { 3000, 6, 1.5f }, { 6000, 5, 2.0f }, { 9000, 3, 2 } }}, 0.1f, 0.20f, 0.0f, -6 },    // Audifonos ajenos
    }};

    // Obstaculos (indice = obstacleChoices). Los "de caja" van antes del espacio; las paredes, despues.
    struct ObstacleModel
    {
        float lp; int lpOrder; float hp;
        Peak peak;
        float combMs, combFb, gainDb;
        bool beforeRoom;
    };

    const std::array<ObstacleModel, 9> obstacles {{
        { 20000, 2, 20, { 1000, 0, 1 }, 0, 0, 0, false },          // Ninguno
        { 1100, 4, 60, { 250, 3, 1.2f }, 0, 0, -7, false },         // Puerta cerrada
        { 450, 4, 50, { 110, 4, 1.5f }, 0, 0, -13, false },         // Pared
        { 220, 6, 40, { 70, 4, 1.5f }, 0, 0, -20, false },          // Pared gruesa
        { 1800, 4, 150, { 900, 3, 2.0f }, 0, 0, -9, false },        // Ventana cerrada
        { 3500, 2, 150, { 650, 8, 4.0f }, 1.1f, 0.55f, -4, true },  // Dentro de una caja
        { 650, 4, 60, { 170, 6, 2.0f }, 3.0f, 0.45f, -11, true },   // Maletera de auto
        { 550, 6, 40, { 1000, 0, 1 }, 0, 0, -9, true },             // Bajo una manta
        { 1400, 4, 120, { 400, 3, 1.5f }, 0.6f, 0.30f, -8, true },  // Bolsillo / mochila
    }};

    // Espacios (indice = roomChoices).
    struct RoomModel
    {
        float size, damping, width;   // cola (Freeverb)
        float erScaleMs, preMs;       // reflexiones tempranas
        float wetHp, wetLp;
        float slapMs, slapFb;         // ecos discretos (calle, pasillo)
        float erGain, revGain;
    };

    const std::array<RoomModel, 10> rooms {{
        { 0, 0, 0, 0, 0, 20, 20000, 0, 0, 0, 0 },                       // Ninguno
        { 0.25f, 0.55f, 0.6f, 6, 2, 150, 7000, 0, 0, 0.7f, 0.5f },       // Cuarto pequeno
        { 0.45f, 0.45f, 0.8f, 10, 6, 120, 8000, 0, 0, 0.6f, 0.6f },      // Sala
        { 0.35f, 0.10f, 0.7f, 5, 2, 200, 10000, 0, 0, 0.8f, 0.8f },      // Bano
        { 0.55f, 0.35f, 0.5f, 14, 8, 150, 8000, 22, 0.35f, 0.7f, 0.6f }, // Pasillo
        { 0.70f, 0.30f, 0.9f, 12, 10, 120, 7500, 0, 0, 0.6f, 0.8f },     // Escalera
        { 0.80f, 0.25f, 1.0f, 20, 15, 100, 7000, 0, 0, 0.6f, 0.9f },     // Garaje
        { 0.30f, 0.60f, 1.0f, 30, 25, 150, 6000, 110, 0.35f, 0.4f, 0.3f }, // Calle
        { 0.90f, 0.30f, 1.0f, 35, 30, 100, 6500, 0, 0, 0.5f, 1.0f },     // Galpon
        { 0.97f, 0.45f, 1.0f, 40, 40, 80, 5000, 0, 0, 0.4f, 1.1f },      // Iglesia
    }};

    constexpr std::array<float, 6> erPatternL { 1.0f, 1.7f, 2.3f, 3.1f, 3.9f, 4.6f };
    constexpr std::array<float, 6> erPatternR { 1.2f, 1.9f, 2.6f, 3.3f, 4.2f, 4.9f };

    float tuneFactor (const RadioParams& p) { return std::pow (2.0f, p.deviceTune / 100.0f); }

    void addAll (std::vector<Coefs::Ptr>& out, const juce::ReferenceCountedArray<Coefs>& arr)
    {
        for (auto* c : arr) out.push_back (c);
    }
}

//==============================================================================
SceneEngine::CoefList SceneEngine::deviceCoefs (const RadioParams& p, double sr)
{
    CoefList c;
    if (p.device <= 0 || p.device >= (int) devices.size()) return c;
    const auto& d = devices[(size_t) p.device];
    const float t = tuneFactor (p);
    addAll (c, Design::designIIRHighpassHighOrderButterworthMethod (clampF (d.hp * t, sr), sr, d.hpOrder));
    addAll (c, Design::designIIRLowpassHighOrderButterworthMethod (clampF (d.lp * t, sr), sr, d.lpOrder));
    for (const auto& pk : d.peaks)
        if (std::abs (pk.gainDb) > 0.01f)
            c.push_back (Coefs::makePeakFilter (sr, clampF (pk.freq * t, sr), pk.q, juce::Decibels::decibelsToGain (pk.gainDb)));
    return c;
}

SceneEngine::CoefList SceneEngine::obstacleCoefs (int obstacle, double sr)
{
    CoefList c;
    if (obstacle <= 0 || obstacle >= (int) obstacles.size()) return c;
    const auto& o = obstacles[(size_t) obstacle];
    addAll (c, Design::designIIRHighpassHighOrderButterworthMethod (clampF (o.hp, sr), sr, 2));
    addAll (c, Design::designIIRLowpassHighOrderButterworthMethod (clampF (o.lp, sr), sr, o.lpOrder));
    if (std::abs (o.peak.gainDb) > 0.01f)
        c.push_back (Coefs::makePeakFilter (sr, clampF (o.peak.freq, sr), o.peak.q, juce::Decibels::decibelsToGain (o.peak.gainDb)));
    return c;
}

SceneEngine::CoefList SceneEngine::distanceCoefs (float distance, double sr)
{
    CoefList c;
    const float d = juce::jlimit (0.0f, 1.0f, distance / 100.0f);
    if (d < 0.005f) return c;
    // El aire se come los agudos: de 20 kHz (al lado) a ~3.5 kHz (muy lejos).
    const float f = 20000.0f * std::pow (3500.0f / 20000.0f, d);
    addAll (c, Design::designIIRLowpassHighOrderButterworthMethod (clampF (f, sr), sr, 2));
    return c;
}

//==============================================================================
void SceneEngine::Colour::allocate (int maxCombSamples)
{
    for (auto& b : combBuf) b.assign ((size_t) juce::jmax (1, maxCombSamples), 0.0f);
}

void SceneEngine::Colour::configure (const CoefList& coefs, int combSamples, float fb, float linearGain, bool resetState)
{
    numFilters = juce::jmin (maxFilters, (int) coefs.size());
    for (auto& chFilters : filters)
        for (int i = 0; i < numFilters; ++i)
        {
            chFilters[(size_t) i].coefficients = coefs[(size_t) i];
            if (resetState) chFilters[(size_t) i].reset();
        }
    combLen = juce::jlimit (0, (int) combBuf[0].size(), combSamples);
    combFb = combLen > 0 ? fb : 0.0f;
    gain = linearGain;
    active = numFilters > 0 || combLen > 0 || std::abs (gain - 1.0f) > 1.0e-4f;
    if (resetState) reset();
}

float SceneEngine::Colour::process (int ch, float x) noexcept
{
    auto& f = filters[(size_t) ch];
    for (int i = 0; i < numFilters; ++i)
        x = f[(size_t) i].processSample (x);

    if (combLen > 0)
    {
        auto& buf = combBuf[(size_t) ch];
        auto& pos = combPos[(size_t) ch];
        const float y = x + combFb * buf[(size_t) pos];
        buf[(size_t) pos] = y;
        pos = (pos + 1) % combLen;
        x = y * (1.0f - combFb * 0.5f);
    }
    return x * gain;
}

void SceneEngine::Colour::reset()
{
    for (auto& chFilters : filters)
        for (auto& f : chFilters) f.reset();
    for (auto& b : combBuf) std::fill (b.begin(), b.end(), 0.0f);
    combPos = {};
}

//==============================================================================
void SceneEngine::prepare (double sampleRate, int maxBlockSize, int numChannels)
{
    sr = sampleRate;
    numCh = juce::jlimit (1, maxChannels, numChannels);
    maxBlock = juce::jmax (1, maxBlockSize);

    const int maxComb = (int) (0.006 * sr) + 2;
    for (auto* c : { &deviceColour, &boxColour, &wallColour, &distanceColour })
        c->allocate (maxComb);

    preRing.allocate ((int) (0.06 * sr) + 2);
    erRing.allocate ((int) (0.25 * sr) + 2);
    slapRing.allocate ((int) (0.15 * sr) + 2);
    revBuf.setSize (maxChannels, maxBlock);
    erBuf.setSize (maxChannels, maxBlock);
    reverb.setSampleRate (sr);

    reset();
}

void SceneEngine::reset()
{
    for (auto* c : { &deviceColour, &boxColour, &wallColour, &distanceColour })
        c->reset();
    preRing.reset(); erRing.reset(); slapRing.reset();
    for (auto& f : wetHp) f.reset();
    for (auto& f : wetLp) f.reset();
    reverb.reset();
    curDevice = curObstacle = curRoom = -1;
    curTune = -999; curDrive = curDistance = curRoomMix = -1;
}

void SceneEngine::updateSettings (const RadioParams& p, bool force)
{
    const bool deviceChanged = force || p.device != curDevice || std::abs (p.deviceTune - curTune) > 0.5f;
    if (deviceChanged)
    {
        const auto& d = devices[(size_t) juce::jlimit (0, (int) devices.size() - 1, p.device)];
        deviceColour.configure (deviceCoefs (p, sr), p.device > 0 ? (int) (d.boxMs * 0.001 * sr / tuneFactor (p)) : 0,
                                d.boxFb, juce::Decibels::decibelsToGain (d.levelDb), p.device != curDevice);
        curDevice = p.device; curTune = p.deviceTune; curDrive = -1;
    }

    if (force || std::abs (p.deviceDrive - curDrive) > 0.1f)
    {
        const auto& d = devices[(size_t) juce::jlimit (0, (int) devices.size() - 1, p.device)];
        const float amount = d.breakup * p.deviceDrive / 100.0f;
        deviceDriveGain = 1.0f + 14.0f * amount;
        deviceDriveNorm = 0.5f / std::tanh (0.5f * deviceDriveGain);
        curDrive = p.deviceDrive;
    }

    if (force || p.obstacle != curObstacle)
    {
        const int idx = juce::jlimit (0, (int) obstacles.size() - 1, p.obstacle);
        const auto& o = obstacles[(size_t) idx];
        const auto coefs = obstacleCoefs (idx, sr);
        const int comb = (int) (o.combMs * 0.001 * sr);
        const float g = juce::Decibels::decibelsToGain (o.gainDb);
        boxColour.configure (o.beforeRoom ? coefs : CoefList {}, o.beforeRoom ? comb : 0, o.combFb, o.beforeRoom ? g : 1.0f, true);
        wallColour.configure (o.beforeRoom ? CoefList {} : coefs, o.beforeRoom ? 0 : comb, o.combFb, o.beforeRoom ? 1.0f : g, true);
        curObstacle = p.obstacle;
    }

    if (force || std::abs (p.distance - curDistance) > 0.2f || std::abs (p.roomMix - curRoomMix) > 0.2f || p.room != curRoom)
    {
        const float d = juce::jlimit (0.0f, 1.0f, p.distance / 100.0f);
        distanceColour.configure (distanceCoefs (p.distance, sr), 0, 0, 1.0f, false);
        distanceGain = juce::Decibels::decibelsToGain (-14.0f * std::pow (d, 0.8f));
        roomWet = juce::jmin (1.3f, p.roomMix / 100.0f * (0.6f + 0.9f * d));
        curDistance = p.distance; curRoomMix = p.roomMix;
    }

    if (force || p.room != curRoom)
    {
        const int idx = juce::jlimit (0, (int) rooms.size() - 1, p.room);
        const auto& r = rooms[(size_t) idx];
        roomActive = idx > 0;
        if (roomActive)
        {
            juce::Reverb::Parameters rp;
            rp.roomSize = r.size; rp.damping = r.damping; rp.width = r.width;
            rp.wetLevel = 0.33f; rp.dryLevel = 0.0f; rp.freezeMode = 0.0f;
            reverb.setParameters (rp);

            preDelaySamples = juce::jmin (preRing.size - 1, (int) (r.preMs * 0.001 * sr));
            for (size_t k = 0; k < erTapsL.size(); ++k)
            {
                erTapsL[k] = juce::jlimit (1, erRing.size - 1, (int) (r.erScaleMs / 5.0f * erPatternL[k] * 0.001f * (float) sr));
                erTapsR[k] = juce::jlimit (1, erRing.size - 1, (int) (r.erScaleMs / 5.0f * erPatternR[k] * 0.001f * (float) sr));
            }
            slapSamples = juce::jmin (slapRing.size - 1, (int) (r.slapMs * 0.001 * sr));
            slapFb = r.slapFb;
            erGain = r.erGain; revGain = r.revGain;
            for (auto& f : wetHp) f.coefficients = Coefs::makeHighPass (sr, clampF (r.wetHp, sr));
            for (auto& f : wetLp) f.coefficients = Coefs::makeLowPass (sr, clampF (r.wetLp, sr));
            if (p.room != curRoom)
            {
                reverb.reset(); preRing.reset(); erRing.reset(); slapRing.reset();
                for (auto& f : wetHp) f.reset();
                for (auto& f : wetLp) f.reset();
            }
        }
        curRoom = p.room;
    }
}

void SceneEngine::process (juce::AudioBuffer<float>& buffer, const RadioParams& p)
{
    const int total = buffer.getNumSamples();
    const int channels = juce::jmin (buffer.getNumChannels(), maxChannels);
    if (total == 0 || channels == 0) return;

    updateSettings (p, curDevice < 0);
    if (! isActive (p) && ! roomActive) return;

    auto* const* data = buffer.getArrayOfWritePointers();
    const bool driveOn = p.device > 0 && deviceDriveGain > 1.001f;

    for (int c = 0; c < channels; ++c)
    {
        for (int i = 0; i < total; ++i)
        {
            float x = data[c][i];
            if (p.device > 0)
            {
                if (driveOn) x = std::tanh (x * deviceDriveGain) * deviceDriveNorm;
                x = deviceColour.process (c, x);
            }
            if (boxColour.active) x = boxColour.process (c, x);
            if (distanceColour.active) x = distanceColour.process (c, x);
            data[c][i] = x * distanceGain;
        }
    }

    if (roomActive)
        for (int start = 0; start < total; start += maxBlock)
            processRoom (buffer, start, juce::jmin (maxBlock, total - start), channels);

    if (wallColour.active)
        for (int c = 0; c < channels; ++c)
            for (int i = 0; i < total; ++i)
                data[c][i] = wallColour.process (c, data[c][i]);
}

void SceneEngine::processRoom (juce::AudioBuffer<float>& buffer, int start, int n, int channels)
{
    auto* const* data = buffer.getArrayOfWritePointers();
    auto* const* rev = revBuf.getArrayOfWritePointers();
    auto* const* er = erBuf.getArrayOfWritePointers();

    for (int i = 0; i < n; ++i)
    {
        for (int c = 0; c < channels; ++c)
        {
            const float x = data[c][start + i];
            preRing.write (c, x);
            const float pre = preRing.read (c, preDelaySamples);
            erRing.write (c, pre);

            float early = 0, sign = 1, g = 1;
            const auto& taps = c == 0 ? erTapsL : erTapsR;
            for (int tap : taps) { early += sign * g * erRing.read (c, tap); g *= 0.8f; sign = -sign; }

            float slap = 0;
            if (slapSamples > 0)
            {
                slap = slapRing.read (c, slapSamples);
                slapRing.write (c, pre + slap * slapFb);
            }

            rev[c][i] = pre;
            er[c][i] = early * 0.35f + slap * 0.6f;
        }
        preRing.advance(); erRing.advance(); slapRing.advance();
    }

    if (channels > 1) reverb.processStereo (rev[0], rev[1], n);
    else              reverb.processMono (rev[0], n);

    for (int c = 0; c < channels; ++c)
        for (int i = 0; i < n; ++i)
        {
            float wet = erGain * er[c][i] + revGain * rev[c][i];
            wet = wetLp[(size_t) c].processSample (wetHp[(size_t) c].processSample (wet));
            data[c][start + i] += roomWet * wet;
        }
}

float SceneEngine::responseDb (const RadioParams& p, double f, double sampleRate)
{
    double mag = 1.0;
    auto apply = [&] (const CoefList& list) { for (auto& c : list) mag *= c->getMagnitudeForFrequency (f, sampleRate); };
    apply (deviceCoefs (p, sampleRate));
    apply (obstacleCoefs (p.obstacle, sampleRate));
    apply (distanceCoefs (p.distance, sampleRate));

    float db = (float) juce::Decibels::gainToDecibels (mag, -120.0);
    if (p.device > 0)
        db += devices[(size_t) juce::jlimit (0, (int) devices.size() - 1, p.device)].levelDb;
    if (p.obstacle > 0)
        db += obstacles[(size_t) juce::jlimit (0, (int) obstacles.size() - 1, p.obstacle)].gainDb;
    db += -14.0f * std::pow (juce::jlimit (0.0f, 1.0f, p.distance / 100.0f), 0.8f);
    return db;
}
