#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_data_structures/juce_data_structures.h>

// Colores de la interfaz. Cambian en vivo entre los dos temas oscuros (Theme::setNight),
// por eso son variables y no constantes: todo se pinta leyendo el valor actual.
//
// Lenguaje visual (inspirado en FabFilter): una superficie continua con paneles sutiles, lineas finas
// con resplandor, dos acentos con intencion (principal + secundario) y etiquetas pequenas en mayusculas.
namespace Palette
{
    inline juce::Colour bg, bgTop, card, cardBorder, panel, panelHi, line, gridMinor, track, text, dim, accent, accent2,
                        accentSoft, red, green, shadow, knobTop, knobBottom, knobEdge, pointer, plotBg, analyzer,
                        switchOff, highlight,
                        mars,      // naranja Marte: lo elegido y la transmision
                        popup,     // fondo opaco para menus, avisos y paneles flotantes
                        knobMid,   // tono medio de la esfera de las perillas
                        toggleTop, toggleBottom, // botones activos (A/B, pestanas)
                        nodeFill;  // relleno de los puntos del ecualizador
}

namespace Theme
{
    inline bool& nightFlag() { static bool n = false; return n; }
    inline std::atomic<int>& versionCounter() { static std::atomic<int> v { 0 }; return v; }

    // Los dos temas son oscuros (el claro se retiro): "Interplanetary Relay" y "Noche".
    inline bool isDark() { return true; }
    inline bool isNight() { return nightFlag(); }
    inline int version() { return versionCounter().load(); }

    inline void apply (bool night)
    {
        using C = juce::Colour;
        nightFlag() = night;
        if (! night) // "Interplanetary Relay": espacio profundo, vidrio, violeta, cian y naranja Marte
        {
            Palette::bg = C (0xff05060f);         Palette::bgTop = C (0xff0b0e2a);     Palette::card = C (0x0effffff);
            Palette::cardBorder = C (0x24aac3ff); Palette::panelHi = C (0x14ffffff);   Palette::line = C (0x1eaac3ff);
            Palette::gridMinor = C (0x0caac3ff);  Palette::track = C (0x38aac3ff);     Palette::text = C (0xffeef2ff);
            Palette::dim = C (0x99becdff);        Palette::accent = C (0xff8b7cff);    Palette::accent2 = C (0xff5be3f5);
            Palette::accentSoft = C (0xff2a2560); Palette::red = C (0xffff5d6c);       Palette::green = C (0xff5be3b0);
            Palette::shadow = C (0x99000000);     Palette::knobTop = C (0xff2e3470);   Palette::knobBottom = C (0xff0a0c22);
            Palette::knobEdge = C (0x40aac3ff);   Palette::pointer = C (0xffffffff);   Palette::plotBg = C (0x26050616);
            Palette::analyzer = C (0xff8b7cff);   Palette::switchOff = C (0x33aac3ff); Palette::highlight = C (0x14ffffff);
            Palette::mars = C (0xffff7a3d);       Palette::popup = C (0xf20d1030);     Palette::knobMid = C (0xff161a3d);
            Palette::toggleTop = C (0xff6a5bff);  Palette::toggleBottom = C (0xff3f3cb8); Palette::nodeFill = C (0xff0b0e26);
        }
        else // "Noche": negro neutro, grises y acentos apagados
        {
            Palette::bg = C (0xff050506);         Palette::bgTop = C (0xff0d0d10);     Palette::card = C (0x0bffffff);
            Palette::cardBorder = C (0x1cffffff); Palette::panelHi = C (0x12ffffff);   Palette::line = C (0x17ffffff);
            Palette::gridMinor = C (0x09ffffff);  Palette::track = C (0x2effffff);     Palette::text = C (0xffe9e9ec);
            Palette::dim = C (0x8cd2d2d8);        Palette::accent = C (0xff98a2b6);    Palette::accent2 = C (0xff9fd3db);
            Palette::accentSoft = C (0xff26282e); Palette::red = C (0xffe8606b);       Palette::green = C (0xff6cc29c);
            Palette::shadow = C (0x99000000);     Palette::knobTop = C (0xff2c2d33);   Palette::knobBottom = C (0xff09090b);
            Palette::knobEdge = C (0x30ffffff);   Palette::pointer = C (0xffffffff);   Palette::plotBg = C (0x40000000);
            Palette::analyzer = C (0xff98a2b6);   Palette::switchOff = C (0x2effffff); Palette::highlight = C (0x0fffffff);
            Palette::mars = C (0xffd5d8df);       Palette::popup = C (0xf5111114);     Palette::knobMid = C (0xff17181c);
            Palette::toggleTop = C (0xff4b505c);  Palette::toggleBottom = C (0xff2d3038); Palette::nodeFill = C (0xff0c0c0f);
        }
        Palette::panel = Palette::card;
        ++versionCounter();
    }

    // Preferencia compartida por todas las ventanas de CapCom (Mac: ~/Library/Application Support/CapCom, Windows: %APPDATA%\CapCom).
    inline juce::PropertiesFile::Options settingsOptions (const juce::String& name)
    {
        juce::PropertiesFile::Options o;
        o.applicationName = name;
        o.folderName = name;
        o.filenameSuffix = "settings";
        o.osxLibrarySubFolder = "Application Support";
        return o;
    }

    inline juce::PropertiesFile& settings()
    {
        static juce::PropertiesFile file (settingsOptions ("CapCom"));
        return file;
    }

    inline void loadSaved()
    {
        static bool loaded = false;
        if (loaded) return;
        loaded = true;
        apply (settings().getBoolValue ("nightTheme", false)); // por defecto: Interplanetary Relay
    }

    inline void setNight (bool night)
    {
        apply (night);
        settings().setValue ("nightTheme", night);
        settings().saveIfNeeded();
    }
}

namespace Style
{
    // Textos con tildes: el codigo fuente es UTF-8.
    inline juce::String T (const char* utf8) { return juce::String::fromUTF8 (utf8); }

    // Tipografia: SF Pro Display en Mac (si no esta instalada, la del sistema) y Segoe UI en Windows.
    inline const juce::String& appleFont()
    {
       #if JUCE_WINDOWS
        static const juce::String name ("Segoe UI");
       #else
        static const juce::String name ("SF Pro Display");
       #endif
        return name;
    }

    inline juce::Font font (float size, bool bold = false)
    {
        return juce::Font (juce::FontOptions (appleFont(), bold ? "Bold" : "Regular", size));
    }

    // Fuente monoespaciada para valores, escalas y telemetria: SF Mono, Menlo o Consolas (Windows).
    inline const juce::String& monoFontName()
    {
        static const juce::String name = []
        {
            const auto installed = juce::Font::findAllTypefaceNames();
            for (auto* candidate : { "SF Mono", "Menlo", "Consolas" })
                if (installed.contains (candidate)) return juce::String (candidate);
            return juce::Font::getDefaultMonospacedFontName();
        }();
        return name;
    }

    inline juce::Font monoFont (float size)
    {
        return juce::Font (juce::FontOptions (monoFontName(), "Regular", size));
    }

    inline juce::Font mediumFont (float size)
    {
       #if JUCE_WINDOWS
        return juce::Font (juce::FontOptions (appleFont(), "Semibold", size)); // Segoe UI no tiene "Medium"
       #else
        return juce::Font (juce::FontOptions (appleFont(), "Medium", size));
       #endif
    }

    // Etiqueta pequena en mayusculas con letras separadas (el sello de las interfaces de audio pro).
    inline juce::Font labelFont (float size = 10.5f, bool bold = true)
    {
        return (bold ? mediumFont (size) : font (size)).withExtraKerningFactor (0.07f);
    }

    // Mayusculas que respetan las tildes del espanol (toUpperCase de JUCE no siempre las convierte).
    inline juce::String upper (const juce::String& s)
    {
        auto u = s.toUpperCase();
        const char* pairs[][2] = { { "á", "Á" }, { "é", "É" }, { "í", "Í" }, { "ó", "Ó" }, { "ú", "Ú" }, { "ñ", "Ñ" }, { "ü", "Ü" } };
        for (auto& p : pairs) u = u.replace (juce::String::fromUTF8 (p[0]), juce::String::fromUTF8 (p[1]));
        return u;
    }

    inline juce::String formatHz (float f)
    {
        return f < 1000.0f ? juce::String (juce::roundToInt (f)) + " Hz" : juce::String (f / 1000.0f, 2) + " kHz";
    }

    // Linea con resplandor: varias pasadas anchas y transparentes debajo del trazo fino.
    inline void glowStroke (juce::Graphics& g, const juce::Path& p, juce::Colour c, float width, float glow = 1.0f)
    {
        const float a = Theme::isDark() ? 1.0f : 0.55f;
        g.setColour (c.withMultipliedAlpha (0.06f * glow * a)); g.strokePath (p, juce::PathStrokeType (width * 7.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (c.withMultipliedAlpha (0.12f * glow * a)); g.strokePath (p, juce::PathStrokeType (width * 3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (c);                                        g.strokePath (p, juce::PathStrokeType (width, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // Panel: en oscuro, vidrio translucido sobre el espacio; en claro, panel sutil con sombra suave.
    inline void drawPanel (juce::Graphics& g, juce::Rectangle<float> r, float radius = 8.0f)
    {
        juce::Path p;
        p.addRoundedRectangle (r, Theme::isDark() ? radius + 4.0f : radius);
        if (Theme::isDark())
            g.setGradientFill (juce::ColourGradient (juce::Colour (0x10ffffff), 0, r.getY(), juce::Colour (0x06ffffff), 0, r.getBottom(), false));
        else
        {
            juce::DropShadow (Palette::shadow, 10, { 0, 2 }).drawForPath (g, p);
            g.setGradientFill (juce::ColourGradient (Palette::card.brighter (0.02f), 0, r.getY(), Palette::card.darker (0.01f), 0, r.getBottom(), false));
        }
        g.fillPath (p);
        g.setColour (Palette::cardBorder);
        g.strokePath (p, juce::PathStrokeType (1.0f));
        g.setColour (Palette::highlight);
        g.drawHorizontalLine ((int) r.getY() + 1, r.getX() + radius, r.getRight() - radius);
    }

    // Planeta con luz desde arriba a la izquierda (Tierra, Marte, icono del logotipo).
    inline void drawPlanet (juce::Graphics& g, juce::Point<float> c, float r, juce::Colour light, juce::Colour mid, juce::Colour dark, bool glow = true)
    {
        if (glow)
        {
            g.setGradientFill (juce::ColourGradient (mid.withAlpha (0.35f), c.x, c.y, mid.withAlpha (0.0f), c.x + r * 2.6f, c.y, true));
            g.fillEllipse (juce::Rectangle<float> (r * 5.2f, r * 5.2f).withCentre (c));
        }
        juce::ColourGradient grad (light, c.x - r * 0.32f, c.y - r * 0.4f, dark, c.x + r * 0.9f, c.y + r * 0.9f, true);
        grad.addColour (0.45, mid);
        g.setGradientFill (grad);
        g.fillEllipse (juce::Rectangle<float> (r * 2, r * 2).withCentre (c));
    }

    // En el modo noche los planetas son grises claros (como lunas), sin color que distraiga.
    inline void drawEarth (juce::Graphics& g, juce::Point<float> c, float r)
    {
        if (Theme::isNight()) drawPlanet (g, c, r, juce::Colour (0xfff4f5f8), juce::Colour (0xff9da2ad), juce::Colour (0xff2a2c32), false);
        else                  drawPlanet (g, c, r, juce::Colour (0xffc4f6ff), juce::Colour (0xff3d8ff2), juce::Colour (0xff0c245e));
    }
    inline void drawMars (juce::Graphics& g, juce::Point<float> c, float r)
    {
        if (Theme::isNight()) drawPlanet (g, c, r, juce::Colour (0xffe6e8ed), juce::Colour (0xff868b96), juce::Colour (0xff232529), false);
        else                  drawPlanet (g, c, r, juce::Colour (0xffffd1aa), juce::Colour (0xffff7a3d), juce::Colour (0xff6e1e0c));
    }

    // Compatibilidad con el codigo anterior.
    inline void drawCard (juce::Graphics& g, juce::Rectangle<float> r, float radius = 8.0f) { drawPanel (g, r, radius); }
}
