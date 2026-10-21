#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

// IDs de parametros. No cambiarlos: los proyectos de DaVinci/Premiere guardan estos nombres.
namespace ParamID
{
    inline constexpr auto inGain      = "inGain";
    inline constexpr auto hpFreq      = "hpFreq";
    inline constexpr auto hpSlope     = "hpSlope";
    inline constexpr auto lpFreq      = "lpFreq";
    inline constexpr auto lpSlope     = "lpSlope";
    inline constexpr auto midFreq     = "midFreq";
    inline constexpr auto midGain     = "midGain";
    inline constexpr auto midQ        = "midQ";
    inline constexpr auto comp        = "comp";
    inline constexpr auto drive       = "drive";
    inline constexpr auto driveType   = "driveType";
    inline constexpr auto bits        = "bits";
    inline constexpr auto rate        = "rate";
    inline constexpr auto hiss        = "hiss";
    inline constexpr auto crackle     = "crackle";
    inline constexpr auto dropouts    = "dropouts";
    inline constexpr auto hissGate    = "hissGate";
    inline constexpr auto squelch     = "squelch";
    inline constexpr auto squelchThr  = "squelchThr";
    inline constexpr auto mix         = "mix";
    inline constexpr auto outGain     = "outGain";
    inline constexpr auto mono        = "mono";
    // v2: estilos NASA, telefono y cinta. Siempre agregar al final.
    inline constexpr auto quindar     = "quindar";
    inline constexpr auto quindarLvl  = "quindarLvl";
    inline constexpr auto glitch      = "glitch";
    inline constexpr auto wow         = "wow";
    inline constexpr auto flutter     = "flutter";
    // v3: escena (aparato, distancia, espacio, obstaculo, ambiente) y eventos (transmision, roger beep, tonos).
    inline constexpr auto txMode      = "txMode";
    inline constexpr auto txKey       = "txKey";
    inline constexpr auto roger       = "roger";
    inline constexpr auto rogerLvl    = "rogerLvl";
    inline constexpr auto rogerStart  = "rogerStart";
    inline constexpr auto device      = "device";
    inline constexpr auto deviceDrive = "deviceDrive";
    inline constexpr auto deviceTune  = "deviceTune";
    inline constexpr auto distance    = "distance";
    inline constexpr auto room        = "room";
    inline constexpr auto roomMix     = "roomMix";
    inline constexpr auto obstacle    = "obstacle";
    // Sin uso desde que ambientes y tonos pasaron a la biblioteca de WAV. Siguen registrados para no
    // romper proyectos guardados, pero el motor los ignora.
    inline constexpr auto ambSource   = "ambSource";
    inline constexpr auto ambLvl      = "ambLvl";
    inline constexpr auto ambDuck     = "ambDuck";
    inline constexpr auto ambPlace    = "ambPlace";
    inline constexpr auto toneType    = "toneType";
    inline constexpr auto toneRegion  = "toneRegion";
    inline constexpr auto toneOn      = "toneOn";
    inline constexpr auto toneLvl     = "toneLvl";
    // v5: interruptor de cada seccion (encendidos por defecto: no cambian el sonido).
    inline constexpr auto secDyn      = "secDynOn";
    inline constexpr auto secDirt     = "secDirtOn";
    inline constexpr auto secNoise    = "secNoiseOn";
    inline constexpr auto secTape     = "secTapeOn";
    inline constexpr auto secSquelch  = "secSquelchOn";
    inline constexpr auto secDevice   = "secDeviceOn";
    inline constexpr auto secDistance = "secDistanceOn";
    inline constexpr auto secRoom     = "secRoomOn";
    inline constexpr auto secObstacle = "secObstacleOn";
}

// v4: bandas libres del ecualizador grafico (b1..b5). IDs como "b1Freq", "b3Type"...
inline constexpr int numEqBands = 5;
inline juce::String bandParamID (int band, const char* what) { return "b" + juce::String (band + 1) + what; }
inline const juce::StringArray bandTypeChoices { "Campana", "Estante grave", "Estante agudo", "Muesca" };

inline const juce::StringArray slopeChoices { "12 dB/oct", "24 dB/oct", "36 dB/oct", "48 dB/oct" };
inline const juce::StringArray driveChoices { "Suave", "Duro", juce::String::fromUTF8 ("Asimétrico") };

inline juce::StringArray utf8Choices (std::initializer_list<const char*> items)
{
    juce::StringArray a;
    for (auto* i : items) a.add (juce::String::fromUTF8 (i));
    return a;
}

// El orden de estas listas es el valor guardado en los proyectos: solo agregar al final.
inline const juce::StringArray txModeChoices   = utf8Choices ({ "Automático (detecta voz)", "Manual (botón)" });
inline const juce::StringArray rogerChoices    = utf8Choices ({ "Ninguno", "Bip corto", "Doble bip", "Motorola (dos tonos)", "Nextel", "Militar (barrido)" });
inline const juce::StringArray deviceChoices   = utf8Choices ({ "Ninguno", "Radio de patrulla", "Radio de transistores", "TV antigua", "Laptop",
                                                                "Celular (altavoz)", "Parlante de auto", "Megáfono", "Altavoz de techo (PA)",
                                                                "Portero eléctrico", "Audífonos ajenos" });
inline const juce::StringArray roomChoices     = utf8Choices ({ "Ninguno", "Cuarto pequeño", "Sala / habitación", "Baño", "Pasillo", "Escalera",
                                                                "Garaje / estacionamiento", "Calle (exterior)", "Galpón / bodega", "Iglesia" });
inline const juce::StringArray obstacleChoices = utf8Choices ({ "Ninguno", "Puerta cerrada", "Pared (cuarto de al lado)", "Pared gruesa (otro piso)",
                                                                "Ventana cerrada", "Dentro de una caja", "Maletera de auto", "Bajo una manta",
                                                                "Bolsillo / mochila" });
inline const juce::StringArray ambChoices      = utf8Choices ({ "Ninguno", "Sirena lejana", "Lluvia", "Ciudad / tráfico", "Zumbido eléctrico", "Archivo propio" });
inline const juce::StringArray ambPlaceChoices = utf8Choices ({ "Dentro de la transmisión", "Alrededor (en la escena)" });
inline const juce::StringArray toneChoices     = utf8Choices ({ "Tono de marcado", "Llamando (timbre)", "Ocupado", "Marcando número",
                                                                "Beep de buzón de voz", "Colgaron", "Número fuera de servicio" });
inline const juce::StringArray toneRegionChoices = utf8Choices ({ "Perú / Latinoamérica", "EE.UU. (películas)" });

// Valores de todos los parametros en un instante. El motor de audio solo lee esto.
struct RadioParams
{
    float inGainDb = 0, hpFreq = 400, lpFreq = 3000;
    int   hpOrder = 4, lpOrder = 4;
    float midFreq = 1700, midGainDb = 7, midQ = 1.2f;
    float comp = 75, driveDb = 10;
    int   driveType = 0;
    float bits = 12, rateHz = 48000;
    float hissDb = -46, crackle = 8, dropouts = 0;
    bool  hissGate = true;
    float squelch = 60, squelchThrDb = -40;
    float mix = 100, outGainDb = 0;
    bool  mono = true;
    bool  quindar = false;
    float quindarDb = -12, glitch = 0, wow = 0, flutter = 0;
    struct Band { bool on = false, bypass = false; int type = 0; float freq = 1000, gainDb = 0, q = 1; };
    std::array<Band, numEqBands> bands;
    bool bandActive (int i) const { return bands[(size_t) i].on && ! bands[(size_t) i].bypass; }
    int   txMode = 0;      bool txKey = false;
    int   roger = 0;       float rogerDb = -14;   bool rogerStart = false;
    int   device = 0;      float deviceDrive = 30, deviceTune = 0;
    float distance = 0;
    int   room = 0;        float roomMix = 35;
    int   obstacle = 0;

    static RadioParams fromState (const juce::AudioProcessorValueTreeState& s)
    {
        auto v = [&] (const char* id) { return s.getRawParameterValue (id)->load(); };
        RadioParams p;
        p.inGainDb     = v (ParamID::inGain);
        p.hpFreq       = v (ParamID::hpFreq);
        p.hpOrder      = 2 * (1 + (int) v (ParamID::hpSlope));
        p.lpFreq       = v (ParamID::lpFreq);
        p.lpOrder      = 2 * (1 + (int) v (ParamID::lpSlope));
        p.midFreq      = v (ParamID::midFreq);
        p.midGainDb    = v (ParamID::midGain);
        p.midQ         = v (ParamID::midQ);
        p.comp         = v (ParamID::comp);
        p.driveDb      = v (ParamID::drive);
        p.driveType    = (int) v (ParamID::driveType);
        p.bits         = v (ParamID::bits);
        p.rateHz       = v (ParamID::rate);
        p.hissDb       = v (ParamID::hiss);
        p.crackle      = v (ParamID::crackle);
        p.dropouts     = v (ParamID::dropouts);
        p.hissGate     = v (ParamID::hissGate) > 0.5f;
        p.squelch      = v (ParamID::squelch);
        p.squelchThrDb = v (ParamID::squelchThr);
        p.mix          = v (ParamID::mix);
        p.outGainDb    = v (ParamID::outGain);
        p.mono         = v (ParamID::mono) > 0.5f;
        p.quindar      = v (ParamID::quindar) > 0.5f;
        p.quindarDb    = v (ParamID::quindarLvl);
        p.glitch       = v (ParamID::glitch);
        p.wow          = v (ParamID::wow);
        p.flutter      = v (ParamID::flutter);
        p.txMode       = (int) v (ParamID::txMode);
        p.txKey        = v (ParamID::txKey) > 0.5f;
        p.roger        = (int) v (ParamID::roger);
        p.rogerDb      = v (ParamID::rogerLvl);
        p.rogerStart   = v (ParamID::rogerStart) > 0.5f;
        p.device       = (int) v (ParamID::device);
        p.deviceDrive  = v (ParamID::deviceDrive);
        p.deviceTune   = v (ParamID::deviceTune);
        p.distance     = v (ParamID::distance);
        // Secciones apagadas: sus controles quedan en un valor que no cambia el sonido.
        if (v (ParamID::secDyn) < 0.5f)      p.comp = 0;
        if (v (ParamID::secDirt) < 0.5f)     { p.driveDb = 0; p.bits = 16; p.rateHz = 48000; p.glitch = 0; }
        if (v (ParamID::secNoise) < 0.5f)    { p.hissDb = -90; p.crackle = 0; p.dropouts = 0; }
        if (v (ParamID::secTape) < 0.5f)     { p.wow = 0; p.flutter = 0; }
        if (v (ParamID::secSquelch) < 0.5f)  p.squelch = 0;
        if (v (ParamID::secDevice) < 0.5f)   p.device = 0;
        if (v (ParamID::secDistance) < 0.5f) p.distance = 0;
        if (v (ParamID::secRoom) < 0.5f)     p.room = 0;
        if (v (ParamID::secObstacle) < 0.5f) p.obstacle = 0;

        for (int b = 0; b < numEqBands; ++b)
        {
            auto bv = [&] (const char* what) { return s.getRawParameterValue (bandParamID (b, what))->load(); };
            auto& band = p.bands[(size_t) b];
            band.on     = bv ("On") > 0.5f;
            band.bypass = bv ("Byp") > 0.5f;
            band.type   = (int) bv ("Type");
            band.freq   = bv ("Freq");
            band.gainDb = bv ("Gain");
            band.q      = bv ("Q");
        }
        p.room         = (int) v (ParamID::room);
        p.roomMix      = v (ParamID::roomMix);
        p.obstacle     = (int) v (ParamID::obstacle);
        return p;
    }
};

inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> ps;

    auto hz = [] (float lo, float hi, float centre)
    {
        NormalisableRange<float> r (lo, hi, 1.0f);
        r.setSkewForCentre (centre);
        return r;
    };
    // Texto que se ve en las perillas y en el editor (DaVinci/Premiere): "1.7 kHz", "+7.0 dB", "Off"...
    auto format = [] (const String& unit, const String& id)
    {
        return [unit, id] (float v, int) -> String
        {
            if (id == ParamID::rate && v >= 47900.0f) return "Off";
            if (id == ParamID::hiss && v <= -89.5f)   return "Off";
            if (id == ParamID::bits && v >= 15.95f)   return "Off";
            if (unit == "Hz") return v < 1000.0f ? String (roundToInt (v)) + " Hz" : String (v / 1000.0f, 2) + " kHz";
            if (unit == "dB") return (v > 0.05f ? "+" : "") + String (v, 1) + " dB";
            if (unit == "%")  return String (roundToInt (v)) + " %";
            if (unit == "+%") return (v > 0.5f ? "+" : "") + String (roundToInt (v)) + " %";
            if (unit == "bits") return String (v, 1) + " bits";
            return String (v, 2);
        };
    };
    auto fl = [&] (const char* id, const char* name, NormalisableRange<float> r, float def, const char* unit, int version = 1)
    {
        ps.push_back (std::make_unique<AudioParameterFloat> (ParameterID { id, version }, String::fromUTF8 (name), r, def,
                                                             AudioParameterFloatAttributes()
                                                                 .withStringFromValueFunction (format (unit, id))
                                                                 .withValueFromStringFunction ([r, idStr = String (id)] (const String& t)
                                                                 {
                                                                     const auto s = t.trim().toLowerCase();
                                                                     if (s.contains ("off"))
                                                                         return idStr == ParamID::hiss ? r.start : r.end;
                                                                     auto v = s.getFloatValue();
                                                                     return s.contains ("k") ? v * 1000.0f : v;
                                                                 })));
    };

    fl (ParamID::inGain,     "Entrada",          { -24.0f, 24.0f, 0.1f },  0.0f,  "dB");
    fl (ParamID::hpFreq,     "Corte graves",     hz (20.0f, 2000.0f, 300.0f), 400.0f, "Hz");
    ps.push_back (std::make_unique<AudioParameterChoice> (ParameterID { ParamID::hpSlope, 1 }, "Pendiente graves", slopeChoices, 1));
    fl (ParamID::lpFreq,     "Corte agudos",     hz (1000.0f, 20000.0f, 4000.0f), 3000.0f, "Hz");
    ps.push_back (std::make_unique<AudioParameterChoice> (ParameterID { ParamID::lpSlope, 1 }, "Pendiente agudos", slopeChoices, 1));
    fl (ParamID::midFreq,    "Medios frecuencia", hz (300.0f, 5000.0f, 1500.0f), 1700.0f, "Hz");
    fl (ParamID::midGain,    "Medios ganancia",  { -12.0f, 18.0f, 0.1f },  7.0f,  "dB");
    fl (ParamID::midQ,       "Medios ancho Q",   { 0.3f, 4.0f, 0.01f },    1.2f,  "");
    fl (ParamID::comp,       "Compresión",       { 0.0f, 100.0f, 1.0f },   75.0f, "%");
    fl (ParamID::drive,      "Drive",            { 0.0f, 36.0f, 0.1f },    10.0f, "dB");
    ps.push_back (std::make_unique<AudioParameterChoice> (ParameterID { ParamID::driveType, 1 }, "Tipo de drive", driveChoices, 0));
    fl (ParamID::bits,       "Bits",             { 3.0f, 16.0f, 0.1f },    12.0f, "bits");
    fl (ParamID::rate,       "Muestreo",         hz (1000.0f, 48000.0f, 8000.0f), 48000.0f, "Hz");
    fl (ParamID::hiss,       "Soplido",          { -90.0f, -20.0f, 0.1f }, -46.0f, "dB");
    fl (ParamID::crackle,    "Estática",         { 0.0f, 100.0f, 1.0f },   8.0f,  "%");
    fl (ParamID::dropouts,   "Cortes de señal",  { 0.0f, 100.0f, 1.0f },   0.0f,  "%");
    ps.push_back (std::make_unique<AudioParameterBool> (ParameterID { ParamID::hissGate, 1 }, "Ruido solo al hablar", true));
    fl (ParamID::squelch,    "Squelch (kshh)",   { 0.0f, 100.0f, 1.0f },   60.0f, "%");
    fl (ParamID::squelchThr, "Umbral de voz",    { -60.0f, -15.0f, 0.1f }, -40.0f, "dB");
    fl (ParamID::mix,        "Mezcla",           { 0.0f, 100.0f, 1.0f },   100.0f, "%");
    fl (ParamID::outGain,    "Salida",           { -24.0f, 12.0f, 0.1f },  0.0f,  "dB");
    ps.push_back (std::make_unique<AudioParameterBool> (ParameterID { ParamID::mono, 1 }, "Mono", true));

    // v2. Los valores por defecto no cambian el sonido, asi los proyectos viejos suenan igual.
    ps.push_back (std::make_unique<AudioParameterBool> (ParameterID { ParamID::quindar, 2 }, "Tonos Quindar", false));
    fl (ParamID::quindarLvl, "Nivel Quindar",    { -36.0f, 0.0f, 0.1f },   -12.0f, "dB", 2);
    fl (ParamID::glitch,     "Fallas digitales", { 0.0f, 100.0f, 1.0f },   0.0f,  "%",  2);
    fl (ParamID::wow,        "Wow",              { 0.0f, 100.0f, 1.0f },   0.0f,  "%",  2);
    fl (ParamID::flutter,    "Flutter",          { 0.0f, 100.0f, 1.0f },   0.0f,  "%",  2);

    // v3. Igual que en v2: los valores por defecto no cambian el sonido.
    auto choice = [&] (const char* id, const char* name, const StringArray& items, int def)
    {
        ps.push_back (std::make_unique<AudioParameterChoice> (ParameterID { id, 3 }, String::fromUTF8 (name), items, def));
    };
    auto toggle = [&] (const char* id, const char* name, bool def)
    {
        ps.push_back (std::make_unique<AudioParameterBool> (ParameterID { id, 3 }, String::fromUTF8 (name), def));
    };
    choice (ParamID::txMode,     "Modo de transmisión", txModeChoices, 0);
    toggle (ParamID::txKey,      "Transmitir (botón)", false);
    choice (ParamID::roger,      "Roger beep", rogerChoices, 0);
    fl     (ParamID::rogerLvl,   "Nivel roger beep",  { -36.0f, 0.0f, 0.1f },   -14.0f, "dB", 3);
    toggle (ParamID::rogerStart, "Bip al empezar", false);
    choice (ParamID::device,     "Aparato", deviceChoices, 0);
    fl     (ParamID::deviceDrive,"Saturación del parlante", { 0.0f, 100.0f, 1.0f }, 30.0f, "%", 3);
    fl     (ParamID::deviceTune, "Tono del aparato",  { -50.0f, 50.0f, 1.0f },  0.0f,  "+%", 3);
    fl     (ParamID::distance,   "Distancia",         { 0.0f, 100.0f, 1.0f },   0.0f,  "%",  3);
    choice (ParamID::room,       "Espacio", roomChoices, 0);
    fl     (ParamID::roomMix,    "Cantidad de espacio", { 0.0f, 100.0f, 1.0f }, 35.0f, "%", 3);
    choice (ParamID::obstacle,   "Obstáculo", obstacleChoices, 0);
    choice (ParamID::ambSource,  "(sin uso) Ambiente", ambChoices, 0);
    fl     (ParamID::ambLvl,     "(sin uso) Nivel ambiente", { -48.0f, 6.0f, 0.1f },  -6.0f,  "dB", 3);
    fl     (ParamID::ambDuck,    "(sin uso) Bajar ambiente", { 0.0f, 100.0f, 1.0f }, 50.0f, "%", 3);
    choice (ParamID::ambPlace,   "(sin uso) Lugar ambiente", ambPlaceChoices, 1);
    choice (ParamID::toneType,   "(sin uso) Tono teléfono", toneChoices, 0);
    choice (ParamID::toneRegion, "(sin uso) Norma telefónica", toneRegionChoices, 0);
    toggle (ParamID::toneOn,     "(sin uso) Tocar tono", false);
    fl     (ParamID::toneLvl,    "(sin uso) Nivel tono",    { -36.0f, 0.0f, 0.1f },   -10.0f, "dB", 3);

    // v4: bandas libres del ecualizador grafico. Apagadas por defecto: no cambian el sonido.
    const float bandDefaults[numEqBands] = { 200.0f, 500.0f, 1000.0f, 2500.0f, 6000.0f };
    for (int b = 0; b < numEqBands; ++b)
    {
        const auto n = "Banda " + String (b + 1) + " ";
        ps.push_back (std::make_unique<AudioParameterBool> (ParameterID { bandParamID (b, "On"), 4 }, n + "activa", false));
        ps.push_back (std::make_unique<AudioParameterBool> (ParameterID { bandParamID (b, "Byp"), 4 }, n + "bypass", false));
        ps.push_back (std::make_unique<AudioParameterChoice> (ParameterID { bandParamID (b, "Type"), 4 }, n + "forma", bandTypeChoices, 0));
        auto qRange = NormalisableRange<float> (0.1f, 10.0f, 0.01f);
        qRange.setSkewForCentre (1.0f);
        auto addF = [&] (const char* what, const String& name, NormalisableRange<float> r, float def, const char* unit)
        {
            const auto id = bandParamID (b, what);
            ps.push_back (std::make_unique<AudioParameterFloat> (ParameterID { id, 4 }, name, r, def,
                                                                 AudioParameterFloatAttributes()
                                                                     .withStringFromValueFunction (format (unit, id))
                                                                     .withValueFromStringFunction ([] (const String& t)
                                                                     {
                                                                         const auto s = t.trim().toLowerCase();
                                                                         auto v = s.getFloatValue();
                                                                         return s.contains ("k") ? v * 1000.0f : v;
                                                                     })));
        };
        addF ("Freq", n + "frecuencia", hz (20.0f, 20000.0f, 1000.0f), bandDefaults[b], "Hz");
        addF ("Gain", n + "ganancia", { -24.0f, 24.0f, 0.1f }, 0.0f, "dB");
        addF ("Q", n + "Q", qRange, 1.0f, "");
    }

    // v5: interruptores de seccion.
    for (auto [id, name] : std::initializer_list<std::pair<const char*, const char*>> {
             { ParamID::secDyn, "Dinámica activa" }, { ParamID::secDirt, "Suciedad activa" }, { ParamID::secNoise, "Ruido activo" },
             { ParamID::secTape, "Cinta activa" }, { ParamID::secSquelch, "Squelch activo" }, { ParamID::secDevice, "Aparato activo" },
             { ParamID::secDistance, "Distancia activa" }, { ParamID::secRoom, "Espacio activo" }, { ParamID::secObstacle, "Obstáculo activo" } })
        ps.push_back (std::make_unique<AudioParameterBool> (ParameterID { id, 5 }, String::fromUTF8 (name), true));

    return { ps.begin(), ps.end() };
}

// Presets de fabrica. Al aplicarlos, todo vuelve primero a su valor por defecto (los v3 son neutros),
// asi que cada preset solo necesita lo que cambia.
// Los nombres van en UTF-8 (se leen con String::fromUTF8).
struct FactoryPreset
{
    const char* name;
    std::vector<std::pair<const char*, float>> values;
};

inline const std::vector<FactoryPreset>& factoryPresets()
{
    using namespace ParamID;
    static const std::vector<FactoryPreset> presets {
        { "Radio policial", {
            { inGain, 0 }, { hpFreq, 400 }, { hpSlope, 1 }, { lpFreq, 3000 }, { lpSlope, 1 },
            { midFreq, 1700 }, { midGain, 7 }, { midQ, 1.2f }, { comp, 75 }, { drive, 10 }, { driveType, 0 },
            { bits, 12 }, { rate, 48000 }, { hiss, -46 }, { crackle, 8 }, { dropouts, 0 }, { hissGate, 1 },
            { squelch, 60 }, { squelchThr, -40 }, { mix, 100 }, { outGain, 0 }, { mono, 1 },
            { quindar, 0 }, { quindarLvl, -12 }, { glitch, 0 }, { wow, 0 }, { flutter, 0 } } },
        { "Walkie militar", {
            { inGain, 2 }, { hpFreq, 550 }, { hpSlope, 2 }, { lpFreq, 2600 }, { lpSlope, 2 },
            { midFreq, 1500 }, { midGain, 9 }, { midQ, 1.5f }, { comp, 90 }, { drive, 22 }, { driveType, 1 },
            { bits, 9 }, { rate, 16000 }, { hiss, -40 }, { crackle, 35 }, { dropouts, 18 }, { hissGate, 1 },
            { squelch, 75 }, { squelchThr, -40 }, { mix, 100 }, { outGain, -2 }, { mono, 1 },
            { quindar, 0 }, { quindarLvl, -12 }, { glitch, 0 }, { wow, 0 }, { flutter, 0 } } },
        { "NASA Apollo", {
            { inGain, 2 }, { hpFreq, 600 }, { hpSlope, 2 }, { lpFreq, 2500 }, { lpSlope, 2 },
            { midFreq, 1400 }, { midGain, 8 }, { midQ, 1.4f }, { comp, 85 }, { drive, 24 }, { driveType, 2 },
            { bits, 8 }, { rate, 11000 }, { hiss, -36 }, { crackle, 30 }, { dropouts, 6 }, { hissGate, 1 },
            { squelch, 20 }, { squelchThr, -40 }, { mix, 100 }, { outGain, -4.5f }, { mono, 1 },
            { quindar, 0 }, { quindarLvl, -12 }, { glitch, 0 }, { wow, 0 }, { flutter, 0 } } },
        { "NASA Apollo (con bips Quindar)", {
            { inGain, 2 }, { hpFreq, 600 }, { hpSlope, 2 }, { lpFreq, 2500 }, { lpSlope, 2 },
            { midFreq, 1400 }, { midGain, 8 }, { midQ, 1.4f }, { comp, 85 }, { drive, 24 }, { driveType, 2 },
            { bits, 8 }, { rate, 11000 }, { hiss, -36 }, { crackle, 30 }, { dropouts, 6 }, { hissGate, 1 },
            { squelch, 20 }, { squelchThr, -40 }, { mix, 100 }, { outGain, -4.5f }, { mono, 1 },
            { quindar, 1 }, { quindarLvl, -12 }, { glitch, 0 }, { wow, 0 }, { flutter, 0 } } },
        { "Teléfono fijo", {
            { inGain, 0 }, { hpFreq, 300 }, { hpSlope, 2 }, { lpFreq, 3400 }, { lpSlope, 2 },
            { midFreq, 1800 }, { midGain, 4 }, { midQ, 1.0f }, { comp, 60 }, { drive, 6 }, { driveType, 0 },
            { bits, 16 }, { rate, 48000 }, { hiss, -62 }, { crackle, 3 }, { dropouts, 0 }, { hissGate, 0 },
            { squelch, 0 }, { squelchThr, -40 }, { mix, 100 }, { outGain, -1.5f }, { mono, 1 },
            { quindar, 0 }, { quindarLvl, -12 }, { glitch, 0 }, { wow, 0 }, { flutter, 0 } } },
        { "Celular", {
            { inGain, 0 }, { hpFreq, 350 }, { hpSlope, 2 }, { lpFreq, 3400 }, { lpSlope, 3 },
            { midFreq, 1500 }, { midGain, 5 }, { midQ, 1.2f }, { comp, 80 }, { drive, 4 }, { driveType, 0 },
            { bits, 10 }, { rate, 8000 }, { hiss, -90 }, { crackle, 0 }, { dropouts, 0 }, { hissGate, 1 },
            { squelch, 0 }, { squelchThr, -40 }, { mix, 100 }, { outGain, 0 }, { mono, 1 },
            { quindar, 0 }, { quindarLvl, -12 }, { glitch, 30 }, { wow, 0 }, { flutter, 0 } } },
        { "VHS / Cassette", {
            { inGain, 0 }, { hpFreq, 60 }, { hpSlope, 0 }, { lpFreq, 6000 }, { lpSlope, 1 },
            { midFreq, 900 }, { midGain, 2 }, { midQ, 0.7f }, { comp, 40 }, { drive, 9 }, { driveType, 0 },
            { bits, 16 }, { rate, 48000 }, { hiss, -48 }, { crackle, 0 }, { dropouts, 15 }, { hissGate, 0 },
            { squelch, 0 }, { squelchThr, -40 }, { mix, 100 }, { outGain, -1.5f }, { mono, 1 },
            { quindar, 0 }, { quindarLvl, -12 }, { glitch, 0 }, { wow, 40 }, { flutter, 35 } } },
        { "Patrulla afuera (desde la casa)", {
            { inGain, 0 }, { hpFreq, 400 }, { hpSlope, 1 }, { lpFreq, 3000 }, { lpSlope, 1 },
            { midFreq, 1700 }, { midGain, 7 }, { midQ, 1.2f }, { comp, 75 }, { drive, 10 }, { driveType, 0 },
            { bits, 12 }, { rate, 48000 }, { hiss, -46 }, { crackle, 8 }, { dropouts, 0 }, { hissGate, 1 },
            { squelch, 60 }, { squelchThr, -40 }, { mix, 100 }, { outGain, 6 }, { mono, 1 },
            { device, 1 }, { deviceDrive, 40 }, { room, 7 }, { roomMix, 45 }, { distance, 45 }, { obstacle, 4 } } },
        { "Central en el cuarto de al lado", {
            { inGain, 0 }, { hpFreq, 400 }, { hpSlope, 1 }, { lpFreq, 3000 }, { lpSlope, 1 },
            { midFreq, 1700 }, { midGain, 7 }, { midQ, 1.2f }, { comp, 75 }, { drive, 10 }, { driveType, 0 },
            { bits, 12 }, { rate, 48000 }, { hiss, -46 }, { crackle, 8 }, { dropouts, 0 }, { hissGate, 1 },
            { squelch, 60 }, { squelchThr, -40 }, { mix, 100 }, { outGain, 14 }, { mono, 1 },
            { device, 2 }, { deviceDrive, 30 }, { room, 2 }, { roomMix, 30 }, { distance, 15 }, { obstacle, 2 } } },
        { "Central con roger beep", {
            { inGain, 0 }, { hpFreq, 400 }, { hpSlope, 1 }, { lpFreq, 3000 }, { lpSlope, 1 },
            { midFreq, 1700 }, { midGain, 7 }, { midQ, 1.2f }, { comp, 75 }, { drive, 10 }, { driveType, 0 },
            { bits, 12 }, { rate, 48000 }, { hiss, -46 }, { crackle, 8 }, { dropouts, 0 }, { hissGate, 1 },
            { squelch, 60 }, { squelchThr, -40 }, { mix, 100 }, { outGain, 0 }, { mono, 1 },
            { roger, 3 }, { rogerStart, 1 }, { rogerLvl, -14 } } },
        { "Megáfono policial", {
            { inGain, 0 }, { hpFreq, 300 }, { hpSlope, 1 }, { lpFreq, 6000 }, { lpSlope, 1 },
            { midFreq, 1500 }, { midGain, 3 }, { midQ, 1.0f }, { comp, 70 }, { drive, 6 }, { driveType, 0 },
            { bits, 16 }, { rate, 48000 }, { hiss, -90 }, { crackle, 0 }, { dropouts, 0 }, { hissGate, 1 },
            { squelch, 0 }, { mix, 100 }, { outGain, 2 }, { mono, 1 },
            { device, 7 }, { deviceDrive, 60 }, { room, 7 }, { roomMix, 55 }, { distance, 30 } } },
        { "Celular en altavoz sobre la mesa", {
            { inGain, 0 }, { hpFreq, 350 }, { hpSlope, 2 }, { lpFreq, 3400 }, { lpSlope, 3 },
            { midFreq, 1500 }, { midGain, 5 }, { midQ, 1.2f }, { comp, 80 }, { drive, 4 }, { driveType, 0 },
            { bits, 10 }, { rate, 8000 }, { hiss, -90 }, { crackle, 0 }, { dropouts, 0 }, { hissGate, 1 },
            { squelch, 0 }, { mix, 100 }, { outGain, 1 }, { mono, 1 }, { glitch, 20 },
            { device, 5 }, { deviceDrive, 35 }, { room, 1 }, { roomMix, 25 }, { distance, 20 } } },
        { "TV en otra habitación", {
            { hpFreq, 20 }, { hpSlope, 0 }, { lpFreq, 20000 }, { lpSlope, 0 }, { midGain, 0 }, { comp, 30 }, { drive, 0 },
            { bits, 16 }, { rate, 48000 }, { hiss, -90 }, { crackle, 0 }, { squelch, 0 }, { mono, 1 }, { outGain, 6 },
            { device, 3 }, { deviceDrive, 20 }, { room, 2 }, { roomMix, 35 }, { distance, 25 }, { obstacle, 1 } } },
        { "Limpio (sin efecto)", {
            { inGain, 0 }, { hpFreq, 20 }, { hpSlope, 0 }, { lpFreq, 20000 }, { lpSlope, 0 },
            { midFreq, 1500 }, { midGain, 0 }, { midQ, 1 }, { comp, 0 }, { drive, 0 }, { driveType, 0 },
            { bits, 16 }, { rate, 48000 }, { hiss, -90 }, { crackle, 0 }, { dropouts, 0 }, { hissGate, 1 },
            { squelch, 0 }, { squelchThr, -40 }, { mix, 100 }, { outGain, 0 }, { mono, 0 },
            { quindar, 0 }, { quindarLvl, -12 }, { glitch, 0 }, { wow, 0 }, { flutter, 0 } } },
    };
    return presets;
}
