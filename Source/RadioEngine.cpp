#include "RadioEngine.h"

namespace
{
    using Design = juce::dsp::FilterDesign<float>;

    float clampFreq (float f, double sr, float lo) { return juce::jlimit (lo, (float) (sr * 0.45), f); }

    // Filtro final tras la distorsion: un poco mas abierto que el principal para no apilar pendiente.
    float postHpFreq (const RadioParams& p, double sr) { return clampFreq (p.hpFreq * 0.9f, sr, 10.0f); }
    float postLpFreq (const RadioParams& p, double sr) { return clampFreq (p.lpFreq * 1.08f, sr, 100.0f); }
    constexpr int postOrder = 4;

    float compAmount (float pct) { return juce::jlimit (0.0f, 1.0f, pct / 100.0f); }

    // Cinta: retardo central y profundidad maxima (en segundos) del wow y del flutter.
    // Wow al 100 % ~ 0.6 % de desafinacion; flutter al 100 % ~ 0.4 %.
    constexpr double tapeBaseDelay = 0.0045, wowMaxDepth = 0.0028, flutterMaxDepth = 0.00015;
    constexpr std::array<double, 4> tapeLfoHz { 0.50, 0.83, 6.3, 9.7 }; // wow, wow, flutter, flutter

    constexpr double quindarSeconds = 0.25;

    // Roger beeps: tramos de tono {inicio, fin (s), frecuencia inicial, frecuencia final}.
    struct ToneSegment { float start, end, f0, f1; };
    const std::array<std::vector<ToneSegment>, 6> rogerPatterns {{
        {},                                                              // Ninguno
        { { 0.0f, 0.12f, 1000, 1000 } },                                  // Bip corto
        { { 0.0f, 0.08f, 1200, 1200 }, { 0.14f, 0.22f, 1200, 1200 } },   // Doble bip
        { { 0.0f, 0.06f, 1800, 1800 }, { 0.06f, 0.13f, 1300, 1300 } },   // Motorola (dos tonos)
        { { 0.0f, 0.035f, 1950, 1950 }, { 0.05f, 0.085f, 2400, 2400 } }, // Nextel
        { { 0.0f, 0.15f, 1800, 800 } },                                   // Militar (barrido)
    }};

    float rogerSeconds (int type)
    {
        const auto& pat = rogerPatterns[(size_t) juce::jlimit (0, (int) rogerPatterns.size() - 1, type)];
        return pat.empty() ? 0.0f : pat.back().end;
    }
}

void RadioEngine::prepare (double sampleRate, int maxBlockSize, int numChannels)
{
    sr = sampleRate;
    numCh = juce::jlimit (1, maxChannels, numChannels);

    compressor.prepare ({ sr, (juce::uint32) maxBlockSize, (juce::uint32) maxChannels });
    compressor.setAttack (3.0f);
    compressor.setRelease (60.0f);

    for (auto* s : { &inGainS, &mixS, &hissS, &wowS, &flutterS })
        s->reset (sr, 0.03);

    tapeLine.prepare ({ sr, (juce::uint32) maxBlockSize, (juce::uint32) maxChannels });
    tapeLine.setMaximumDelayInSamples ((int) std::ceil ((tapeBaseDelay + wowMaxDepth + flutterMaxDepth) * sr) + 8);

    for (auto& c : chans)
        c.glitchBuf.assign ((size_t) (0.06 * sr) + 1, 0.0f); // paquetes de hasta 30 ms
    glitchSmooth = std::exp (-1.0f / (float) (0.001 * sr));

    envAtt = std::exp (-1.0f / (float) (0.001 * sr));
    envRel = std::exp (-1.0f / (float) (0.050 * sr));
    carrierAtt = std::exp (-1.0f / (float) (0.005 * sr));
    carrierRel = std::exp (-1.0f / (float) (0.250 * sr));
    holdSamples = (int) (0.45 * sr); // mantiene el "boton" apretado en las pausas entre frases
    dropSmooth = std::exp (-1.0f / (float) (0.002 * sr));
    keySmooth = std::exp (-1.0f / (float) (0.004 * sr));

    reset();
}

void RadioEngine::reset()
{
    for (auto& c : chans)
    {
        c.hp.reset(); c.lp.reset(); c.postHp.reset(); c.postLp.reset();
        c.mid.reset();
        c.heldSample = 0; c.holdPhase = 1;
        std::fill (c.glitchBuf.begin(), c.glitchBuf.end(), 0.0f);
        c.glitchWrite = c.glitchRead = c.glitchFramePos = 0;
    }
    tapeLine.reset();
    tapePhase = {};
    quindarPos = quindarLen = 0; quindarPhase = 0;
    rogerPos = -1; rogerLen = 0; rogerPhase = 0;
    keyGain = 1;
    glitchRemaining = 0; glitchMix = 0; glitchRepeat = false;
    compressor.reset();
    curHp = curLp = curMidF = curMidQ = -1; curMidG = -100;
    curHpOrder = curLpOrder = -1;
    bandsReady = false;
    for (auto& c : chans) for (auto& f : c.band) f.reset();
    grIn = grOut = 0; lastGrDb = 0;
    env = 0; gateOpen = false; belowCount = 0; carrier = 0;
    burstPos = burstLen = 0; burstGain = 0;
    crackleEnv = 0; dropRemaining = 0; dropTarget = dropGain = 1;
    inGainS.setCurrentAndTargetValue (1);
    mixS.setCurrentAndTargetValue (1);    hissS.setCurrentAndTargetValue (0);
    wowS.setCurrentAndTargetValue (0);    flutterS.setCurrentAndTargetValue (0);
}

void RadioEngine::startQuindar (float freq)
{
    quindarFreq = freq;
    quindarPos = 0;
    quindarLen = (int) (quindarSeconds * sr);
    quindarPhase = 0;
}

void RadioEngine::startRoger (int type)
{
    rogerType = type;
    rogerLen = (int) (rogerSeconds (type) * sr);
    rogerPos = rogerLen > 0 ? 0 : -1;
    rogerPhase = 0;
}

float RadioEngine::nextRogerSample() noexcept
{
    if (rogerPos < 0 || rogerPos >= rogerLen) return 0.0f;
    const float t = (float) rogerPos / (float) sr;
    ++rogerPos;
    for (const auto& seg : rogerPatterns[(size_t) rogerType])
        if (t >= seg.start && t < seg.end)
        {
            const float u = (t - seg.start) / (seg.end - seg.start);
            const float freq = seg.f0 + (seg.f1 - seg.f0) * u;
            const float edge = std::min (t - seg.start, seg.end - t);
            const float fade = std::min (1.0f, edge / 0.003f);
            rogerPhase += juce::MathConstants<float>::twoPi * freq / (float) sr;
            if (rogerPhase > juce::MathConstants<float>::twoPi) rogerPhase -= juce::MathConstants<float>::twoPi;
            return rogerAmp * fade * std::sin (rogerPhase);
        }
    return 0.0f;
}

void RadioEngine::assign (FilterGroup& g, const CoefArray& c, bool resetState)
{
    g.numStages = juce::jmin (maxStages, c.size());
    for (int i = 0; i < g.numStages; ++i)
    {
        g.stages[(size_t) i].coefficients = c[i];
        if (resetState) g.stages[(size_t) i].reset();
    }
}

void RadioEngine::updateFilters (const RadioParams& p, bool force)
{
    auto moved = [] (float a, float b) { return std::abs (a - b) > 0.002f * std::max (1.0f, std::abs (b)); };

    // Suaviza los barridos de frecuencia entre bloques para evitar chasquidos.
    auto glide = [force] (float cur, float target) { return (force || cur < 0) ? target : cur + (target - cur) * 0.35f; };

    const float hp = glide (curHp, p.hpFreq), lp = glide (curLp, p.lpFreq);

    if (force || moved (hp, curHp) || p.hpOrder != curHpOrder)
    {
        const bool orderChanged = p.hpOrder != curHpOrder;
        RadioParams q = p; q.hpFreq = hp;
        auto main = Design::designIIRHighpassHighOrderButterworthMethod (clampFreq (hp, sr, 10.0f), sr, p.hpOrder);
        auto post = Design::designIIRHighpassHighOrderButterworthMethod (postHpFreq (q, sr), sr, postOrder);
        for (auto& c : chans) { assign (c.hp, main, orderChanged); assign (c.postHp, post, orderChanged); }
        curHp = hp; curHpOrder = p.hpOrder;
    }

    if (force || moved (lp, curLp) || p.lpOrder != curLpOrder)
    {
        const bool orderChanged = p.lpOrder != curLpOrder;
        RadioParams q = p; q.lpFreq = lp;
        auto main = Design::designIIRLowpassHighOrderButterworthMethod (clampFreq (lp, sr, 100.0f), sr, p.lpOrder);
        auto post = Design::designIIRLowpassHighOrderButterworthMethod (postLpFreq (q, sr), sr, postOrder);
        for (auto& c : chans) { assign (c.lp, main, orderChanged); assign (c.postLp, post, orderChanged); }
        curLp = lp; curLpOrder = p.lpOrder;
    }

    if (force || moved (p.midFreq, curMidF) || moved (p.midGainDb + 100, curMidG + 100) || moved (p.midQ, curMidQ))
    {
        auto c = Coefs::makePeakFilter (sr, clampFreq (p.midFreq, sr, 20.0f), p.midQ,
                                        juce::Decibels::decibelsToGain (p.midGainDb));
        for (auto& ch : chans) ch.mid.coefficients = c;
        curMidF = p.midFreq; curMidG = p.midGainDb; curMidQ = p.midQ;
    }

    // Bandas libres: solo se recalculan las que cambiaron.
    for (int b = 0; b < numEqBands; ++b)
    {
        const auto& nb = p.bands[(size_t) b];
        auto& cb = curBands[(size_t) b];
        const bool changed = ! bandsReady || ! cb.on || nb.type != cb.type || moved (nb.freq, cb.freq)
                             || std::abs (nb.gainDb - cb.gainDb) > 0.01f || moved (nb.q, cb.q);
        if (changed && nb.on)
        {
            auto c = bandCoefs (nb, sr);
            for (auto& ch : chans)
            {
                ch.band[(size_t) b].coefficients = c;
                if (nb.type != cb.type || ! cb.on) ch.band[(size_t) b].reset();
            }
            cb = nb;
        }
        cb.on = nb.on;
    }
    bandsReady = true;

    const float a = compAmount (p.comp);
    const float thr = -30.0f * a, ratio = 1.0f + 11.0f * a;
    compressor.setThreshold (thr);
    compressor.setRatio (ratio);
    makeupGain = juce::Decibels::decibelsToGain (-thr * (1.0f - 1.0f / ratio) * 0.6f);
}

float RadioEngine::shape (float x, int type) const noexcept
{
    switch (type)
    {
        case 1:  return juce::jlimit (-1.0f, 1.0f, x);                        // Duro
        case 2:  return std::tanh (x + 0.35f) - std::tanh (0.35f);           // Asimetrico (el filtro final quita el DC)
        default: return std::tanh (x);                                        // Suave
    }
}

void RadioEngine::process (juce::AudioBuffer<float>& buffer, const RadioParams& p)
{
    const int n = buffer.getNumSamples();
    const int bufCh = juce::jmin (buffer.getNumChannels(), maxChannels);
    if (n == 0 || bufCh == 0) return;

    updateFilters (p, curHpOrder < 0);

    // En mono se suma a un canal, se procesa una vez y se copia.
    const bool monoMode = p.mono && bufCh > 1;
    if (monoMode)
    {
        buffer.addFrom (0, 0, buffer, 1, 0, n);
        buffer.applyGain (0, 0, n, 0.5f);
    }
    const int procCh = monoMode ? 1 : bufCh;

    inGainS.setTargetValue (juce::Decibels::decibelsToGain (p.inGainDb));
    mixS.setTargetValue (p.mix / 100.0f);
    hissS.setTargetValue (p.hissDb <= -89.5f ? 0.0f : juce::Decibels::decibelsToGain (p.hissDb) * 1.73f);

    const float driveGain = juce::Decibels::decibelsToGain (p.driveDb);
    const float driveNorm = p.driveDb > 0.05f ? 0.5f / std::max (1.0e-4f, std::abs (shape (0.5f * driveGain, p.driveType))) : 1.0f;
    const float thrLin = juce::Decibels::decibelsToGain (p.squelchThrDb);
    const float closeLin = thrLin * 0.5f; // histeresis de 6 dB
    const float squelchAmp = std::pow (p.squelch / 100.0f, 2.0f) * 0.5f;
    const float crackleProb = p.crackle / 100.0f * 0.0015f;
    const float crackleAmp = std::sqrt (p.crackle / 100.0f);
    const float dropRate = p.dropouts / 100.0f * 2.0f; // cortes por segundo, maximo
    const float glitchRate = p.glitch / 100.0f * 1.5f;  // fallas de codec por segundo, maximo
    const float quindarAmp = juce::Decibels::decibelsToGain (p.quindarDb);
    const int quindarFade = std::max (1, (int) (0.004 * sr));
    const bool manual = p.txMode == 1;
    rogerAmp = juce::Decibels::decibelsToGain (p.rogerDb);
    const float rogerGap = rogerSeconds (p.roger);
    wowS.setTargetValue (p.wow / 100.0f);
    flutterS.setTargetValue (p.flutter / 100.0f);

    auto* const* data = buffer.getArrayOfWritePointers();
    float peak = 0;
    grIn = grOut = 0;

    for (int i = 0; i < n; ++i)
    {
        const float inG = inGainS.getNextValue();
        const float mix = mixS.getNextValue(), hissBase = hissS.getNextValue();

        // --- Deteccion de voz (sobre la senal de entrada) ---
        float detect = 0;
        for (int c = 0; c < procCh; ++c) detect = std::max (detect, std::abs (data[c][i] * inG));
        env = detect > env ? detect + (env - detect) * envAtt : detect + (env - detect) * envRel;

        // En automatico abre la voz; en manual, el boton "Transmitir".
        const bool wantOpen = manual ? p.txKey : env > thrLin;
        if (! gateOpen && wantOpen)
        {
            gateOpen = true; belowCount = 0;
            burstLen = (int) (0.12 * sr); burstFadeOutStart = (int) (0.06 * sr);
            burstPos = 0; burstGain = squelchAmp * 0.8f; burstClick = false;
            if (p.quindar) startQuindar (2525.0f);
            if (p.roger > 0 && p.rogerStart) startRoger (p.roger);
        }
        else if (gateOpen)
        {
            belowCount = manual ? (p.txKey ? 0 : holdSamples + 1) : (env < closeLin ? belowCount + 1 : 0);
            if (belowCount > holdSamples)
            {
                gateOpen = false;
                // Primero el roger beep y despues la cola de squelch.
                const int rogerSamples = p.roger > 0 ? (int) ((rogerGap + 0.03f) * sr) : 0;
                burstLen = (int) (0.32 * sr); burstFadeOutStart = (int) (0.06 * sr);
                burstPos = -rogerSamples; burstGain = squelchAmp; burstClick = true;
                if (p.quindar) startQuindar (2475.0f);
                if (p.roger > 0) startRoger (p.roger);
            }
        }
        const float cTarget = gateOpen ? 1.0f : 0.0f;
        carrier = cTarget + (carrier - cTarget) * (cTarget > carrier ? carrierAtt : carrierRel);

        // --- Envolvente de la rafaga de squelch ---
        float squelchEnv = 0;
        if (burstPos < 0)
            ++burstPos; // esperando a que termine el roger beep
        else if (burstPos < burstLen && burstGain > 0)
        {
            const float fadeIn = std::min (1.0f, (float) burstPos / (float) (0.004 * sr));
            const float fadeOut = burstPos < burstFadeOutStart ? 1.0f
                                : 1.0f - (float) (burstPos - burstFadeOutStart) / (float) (burstLen - burstFadeOutStart);
            squelchEnv = burstGain * fadeIn * fadeOut * fadeOut;
            if (burstClick && burstPos == 0) squelchEnv = burstGain * 4.0f; // clic al soltar el boton
            ++burstPos;
        }

        // --- Estatica ---
        if (crackleProb > 0 && rng.nextFloat() < crackleProb * (p.hissGate ? carrier : 1.0f))
            crackleEnv = (rng.nextBool() ? 1.0f : -1.0f) * (0.08f + 0.4f * rng.nextFloat()) * crackleAmp;
        const float crackleVal = crackleEnv;
        crackleEnv *= 0.55f;

        // --- Cortes de senal ---
        if (dropRemaining > 0)
        {
            if (--dropRemaining == 0) dropTarget = 1;
        }
        else if (dropRate > 0 && gateOpen && rng.nextFloat() < dropRate / (float) sr)
        {
            dropRemaining = (int) ((0.02 + 0.13 * rng.nextFloat()) * sr);
            dropTarget = 0.3f * rng.nextFloat();
        }
        dropGain = dropTarget + (dropGain - dropTarget) * dropSmooth;

        // --- Fallas de codec: repite un paquete de 12-30 ms o lo silencia ---
        bool glitchStart = false;
        if (glitchRemaining > 0)
            --glitchRemaining;
        else if (glitchRate > 0 && gateOpen && rng.nextFloat() < glitchRate / (float) sr)
        {
            glitchRemaining = (int) ((0.03 + 0.09 * rng.nextFloat()) * sr);
            glitchFrameLen = (int) ((0.012 + 0.018 * rng.nextFloat()) * sr);
            glitchRepeat = rng.nextFloat() < 0.65f;
            glitchStart = true;
        }
        const float gTarget = glitchRemaining > 0 ? 1.0f : 0.0f;
        glitchMix = gTarget + (glitchMix - gTarget) * glitchSmooth;

        // --- Cinta: retardo modulado (wow + flutter) ---
        float tapeDelay = -1.0f;
        const float wowD = wowS.getNextValue(), flutD = flutterS.getNextValue();
        if (wowD > 0 || flutD > 0)
        {
            const auto s = [this] (int k) { return (float) std::sin (juce::MathConstants<double>::twoPi * tapePhase[(size_t) k]); };
            tapeDelay = (float) sr * ((float) tapeBaseDelay
                                      + (float) wowMaxDepth * wowD * (0.65f * s (0) + 0.35f * s (1))
                                      + (float) flutterMaxDepth * flutD * (0.75f * s (2) + 0.25f * s (3)));
            for (size_t k = 0; k < tapePhase.size(); ++k)
            {
                tapePhase[k] += tapeLfoHz[k] / sr;
                if (tapePhase[k] >= 1.0) tapePhase[k] -= 1.0;
            }
        }

        // --- Tonos Quindar ---
        float quindarVal = 0;
        if (quindarPos < quindarLen)
        {
            const float fade = (float) std::min ({ quindarPos, quindarLen - quindarPos, quindarFade }) / (float) quindarFade;
            quindarVal = quindarAmp * fade * std::sin (quindarPhase);
            quindarPhase += juce::MathConstants<float>::twoPi * quindarFreq / (float) sr;
            if (quindarPhase > juce::MathConstants<float>::twoPi) quindarPhase -= juce::MathConstants<float>::twoPi;
            ++quindarPos;
        }

        // Boton manual: fuera de la transmision no pasa nada de lo que entra al transmisor.
        const float keyTarget = manual ? (gateOpen ? 1.0f : 0.0f) : 1.0f;
        keyGain = keyTarget + (keyGain - keyTarget) * keySmooth;

        const float rogerVal = nextRogerSample();

        const SampleState st { driveGain, driveNorm, hissBase * (p.hissGate ? carrier : 1.0f), squelchEnv, crackleVal,
                               dropGain, tapeDelay, glitchMix, glitchStart };

        for (int c = 0; c < procCh; ++c)
        {
            const float dry = data[c][i];
            const float wet = processChannel (chans[(size_t) c], c, dry * inG * keyGain + rogerVal, p, st) + quindarVal;
            const float out = dry * (1.0f - mix) + wet * mix; // el volumen final lo aplica RadioChain, despues de la escena
            data[c][i] = out;
            peak = std::max (peak, std::abs (out));
        }
    }

    if (monoMode)
        for (int c = 1; c < bufCh; ++c)
            buffer.copyFrom (c, 0, buffer, 0, 0, n);

    lastPeak = peak;
    lastGrDb = grIn > 1.0e-9f ? juce::jmin (0.0f, 10.0f * std::log10 (std::max (grOut, 1.0e-12f) / grIn)) : 0.0f;
}

juce::dsp::IIR::Coefficients<float>::Ptr RadioEngine::bandCoefs (const RadioParams::Band& b, double sampleRate)
{
    const float f = clampFreq (b.freq, sampleRate, 20.0f);
    const float q = juce::jlimit (0.1f, 10.0f, b.q);
    const float g = juce::Decibels::decibelsToGain (b.gainDb);
    switch (b.type)
    {
        case 1:  return Coefs::makeLowShelf (sampleRate, f, q, g);
        case 2:  return Coefs::makeHighShelf (sampleRate, f, q, g);
        case 3:  return Coefs::makeNotch (sampleRate, f, q);
        default: return Coefs::makePeakFilter (sampleRate, f, q, g);
    }
}

float RadioEngine::processChannel (Channel& c, int ch, float x, const RadioParams& p, const SampleState& s)
{
    // La linea de retardo se alimenta siempre, para que al activar la cinta no salga audio viejo.
    tapeLine.pushSample (ch, x);
    const float delayed = tapeLine.popSample (ch, s.tapeDelay > 0 ? s.tapeDelay : (float) (tapeBaseDelay * sr));
    if (s.tapeDelay > 0) x = delayed;

    x = c.hp.process (x);
    x = c.lp.process (x);
    x = c.mid.processSample (x);

    for (int b = 0; b < numEqBands; ++b)
        if (p.bandActive (b))
            x = c.band[(size_t) b].processSample (x);

    if (p.comp > 0.5f)
    {
        const float before = x;
        x = compressor.processSample (ch, x);
        if (ch == 0) { grIn += before * before; grOut += x * x; }
        x *= makeupGain;
    }

    if (p.driveDb > 0.05f)
        x = shape (x * s.driveGain, p.driveType) * s.driveNorm;

    if (p.rateHz < (float) sr * 0.98f)
    {
        c.holdPhase += p.rateHz / (float) sr;
        if (c.holdPhase >= 1.0f) { c.holdPhase -= 1.0f; c.heldSample = x; }
        x = c.heldSample;
    }

    if (p.bits < 15.9f)
    {
        const float levels = std::pow (2.0f, p.bits - 1.0f);
        x = std::round (x * levels) / levels;
    }

    const float noise = rng.nextFloat() * 2.0f - 1.0f;
    x += noise * (s.hissAmp + s.squelchEnv) + s.crackleVal;

    x = c.postHp.process (x);
    x = c.postLp.process (x);

    // Fallas de codec. Mientras se repite un paquete el bufer queda congelado.
    const int size = (int) c.glitchBuf.size();
    if (s.glitchStart)
    {
        c.glitchRead = (c.glitchWrite - glitchFrameLen + size) % size;
        c.glitchFramePos = 0;
    }
    if (! (glitchRepeat && glitchRemaining > 0))
    {
        c.glitchBuf[(size_t) c.glitchWrite] = x;
        c.glitchWrite = (c.glitchWrite + 1) % size;
    }
    if (s.glitchMix > 1.0e-4f)
    {
        float alt = 0;
        if (glitchRepeat)
        {
            alt = 0.8f * c.glitchBuf[(size_t) ((c.glitchRead + c.glitchFramePos) % size)];
            c.glitchFramePos = (c.glitchFramePos + 1) % std::max (1, glitchFrameLen);
        }
        x += (alt - x) * s.glitchMix;
    }

    return x * s.dropGain;
}

float RadioEngine::responseDb (const RadioParams& p, double f, double sampleRate)
{
    double mag = 1.0;
    auto apply = [&] (const CoefArray& arr) { for (auto* c : arr) mag *= c->getMagnitudeForFrequency (f, sampleRate); };

    apply (Design::designIIRHighpassHighOrderButterworthMethod (clampFreq (p.hpFreq, sampleRate, 10.0f), sampleRate, p.hpOrder));
    apply (Design::designIIRLowpassHighOrderButterworthMethod (clampFreq (p.lpFreq, sampleRate, 100.0f), sampleRate, p.lpOrder));
    apply (Design::designIIRHighpassHighOrderButterworthMethod (postHpFreq (p, sampleRate), sampleRate, postOrder));
    apply (Design::designIIRLowpassHighOrderButterworthMethod (postLpFreq (p, sampleRate), sampleRate, postOrder));
    mag *= Coefs::makePeakFilter (sampleRate, clampFreq (p.midFreq, sampleRate, 20.0f), p.midQ,
                                  juce::Decibels::decibelsToGain (p.midGainDb))->getMagnitudeForFrequency (f, sampleRate);
    for (int b = 0; b < numEqBands; ++b)
        if (p.bandActive (b))
            mag *= bandCoefs (p.bands[(size_t) b], sampleRate)->getMagnitudeForFrequency (f, sampleRate);
    return (float) juce::Decibels::gainToDecibels (mag, -120.0);
}
