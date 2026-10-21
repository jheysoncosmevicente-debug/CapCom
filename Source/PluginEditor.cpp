#include "PluginEditor.h"
#include <BinaryData.h>

using namespace Style;

namespace
{
    juce::String formatTime (double s)
    {
        const int t = (int) s;
        return juce::String (t / 60) + ":" + juce::String (t % 60).paddedLeft ('0', 2);
    }

    // Contenido del globo de ayuda "?".
    class HelpBubble : public juce::Component
    {
    public:
        HelpBubble (const juce::String& title, const juce::String& body)
        {
            juce::AttributedString as;
            as.append (title + "\n", font (13.0f, true), Palette::text);
            as.append (body, font (13.0f), Palette::dim);
            as.setLineSpacing (3.0f);
            layout.createLayout (as, 300.0f);
            setSize (320, (int) layout.getHeight() + 22);
        }
        void paint (juce::Graphics& g) override { layout.draw (g, getLocalBounds().reduced (10).toFloat()); }

    private:
        juce::TextLayout layout;
    };
}

//==============================================================================
RadioLookAndFeel::RadioLookAndFeel()
{
    setDefaultSansSerifTypefaceName (appleFont()); // menus, avisos y cuadros de texto tambien en SF Pro
    Theme::loadSaved();
    applyTheme();
}

void RadioLookAndFeel::applyTheme()
{
    using namespace juce;
    setColour (ResizableWindow::backgroundColourId, Palette::bg);
    setColour (Slider::textBoxTextColourId, Palette::text);
    setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
    setColour (Slider::textBoxBackgroundColourId, Colours::transparentBlack);
    setColour (Slider::textBoxHighlightColourId, Palette::accent.withAlpha (0.3f));
    setColour (Label::textColourId, Palette::text);
    setColour (ComboBox::textColourId, Palette::text);
    setColour (ComboBox::arrowColourId, Palette::dim);
    setColour (PopupMenu::backgroundColourId, Palette::popup);
    setColour (PopupMenu::textColourId, Palette::text);
    setColour (PopupMenu::highlightedBackgroundColourId, Palette::accent);
    setColour (PopupMenu::highlightedTextColourId, Colours::white);
    setColour (PopupMenu::headerTextColourId, Palette::dim);
    setColour (TextButton::textColourOffId, Palette::text);
    setColour (TextButton::textColourOnId, Colours::white);
    setColour (ToggleButton::textColourId, Palette::text);
    setColour (AlertWindow::backgroundColourId, Palette::popup);
    setColour (AlertWindow::textColourId, Palette::text);
    setColour (AlertWindow::outlineColourId, Palette::cardBorder);
    setColour (TextEditor::backgroundColourId, Palette::popup);
    setColour (TextEditor::textColourId, Palette::text);
    setColour (TextEditor::outlineColourId, Palette::cardBorder);
    setColour (TextEditor::focusedOutlineColourId, Palette::accent);
    setColour (TooltipWindow::backgroundColourId, Palette::popup);
    setColour (TooltipWindow::textColourId, Palette::text);
    setColour (TooltipWindow::outlineColourId, Palette::cardBorder);
}

void RadioLookAndFeel::drawSpaceKnob (juce::Graphics& g, juce::Point<float> centre, float radius, float angle,
                                      float startAngle, float endAngle, float fromAngle)
{
    using namespace juce;
    auto at = [&] (float a, float r) { return Point<float> (centre.x + r * std::sin (a), centre.y - r * std::cos (a)); };
    const float orbitR = radius - 4.0f;

    // Orbita punteada
    Path orbit;
    orbit.addCentredArc (centre.x, centre.y, orbitR, orbitR, 0, startAngle, endAngle, true);
    Path dotted;
    const float dashes[] = { 1.5f, 3.5f };
    PathStrokeType (1.2f).createDashedStroke (dotted, orbit, dashes, 2);
    g.setColour (Palette::track);
    g.fillPath (dotted);

    // Arco recorrido: de violeta a cian, con resplandor
    if (std::abs (angle - fromAngle) > 0.01f)
    {
        Path arc;
        arc.addCentredArc (centre.x, centre.y, orbitR, orbitR, 0, jmin (fromAngle, angle), jmax (fromAngle, angle), true);
        const auto p0 = at (jmin (fromAngle, angle), orbitR), p1 = at (jmax (fromAngle, angle), orbitR);
        g.setGradientFill (ColourGradient (Palette::accent.withAlpha (0.22f), p0.x, p0.y, Palette::accent2.withAlpha (0.22f), p1.x, p1.y, false));
        g.strokePath (arc, PathStrokeType (7.0f, PathStrokeType::curved, PathStrokeType::rounded)); // halo
        g.setGradientFill (ColourGradient (Palette::accent, p0.x, p0.y, Palette::accent2, p1.x, p1.y, false));
        g.strokePath (arc, PathStrokeType (2.4f, PathStrokeType::curved, PathStrokeType::rounded));
    }

    // Cuerpo: esfera con luz desde arriba a la izquierda
    const float bodyR = orbitR - 8.0f;
    const auto body = Rectangle<float> (bodyR * 2, bodyR * 2).withCentre (centre);
    Path sphere;
    sphere.addEllipse (body);
    DropShadow (Colour (0xaa000000), (int) (bodyR * 0.5f), { 0, (int) (bodyR * 0.18f) + 1 }).drawForPath (g, sphere);
    ColourGradient grad (Palette::knobTop, centre.x - bodyR * 0.24f, centre.y - bodyR * 0.4f,
                         Palette::knobBottom, centre.x + bodyR, centre.y + bodyR, true);
    grad.addColour (0.55, Palette::knobMid);
    g.setGradientFill (grad);
    g.fillPath (sphere);
    g.setColour (Palette::knobEdge);
    g.strokePath (sphere, PathStrokeType (1.0f));

    // Indicador: punto blanco sobre la esfera
    const auto tip = at (angle, bodyR * 0.62f);
    g.setColour (Colours::white.withAlpha (0.92f));
    g.fillEllipse (Rectangle<float> (4.2f, 4.2f).withCentre (tip));

    // Satelite: recorre la orbita marcando el valor
    const auto sat = at (angle, orbitR);
    g.setGradientFill (ColourGradient (Palette::accent2.withAlpha (0.6f), sat.x, sat.y, Palette::accent2.withAlpha (0.0f), sat.x + 9.0f, sat.y, true));
    g.fillEllipse (Rectangle<float> (18.0f, 18.0f).withCentre (sat));
    g.setColour (Colours::white);
    g.fillEllipse (Rectangle<float> (5.5f, 5.5f).withCentre (sat));
}

void RadioLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                         float startAngle, float endAngle, juce::Slider& slider)
{
    using namespace juce;
    const auto bounds = Rectangle<int> (x, y, w, h).toFloat().reduced (2.0f);
    const float radius = jmin (bounds.getWidth(), bounds.getHeight()) / 2.0f;
    const auto centre = bounds.getCentre();
    const float angle = startAngle + pos * (endAngle - startAngle);
    if (Theme::isDark())
    {
        const bool bip = slider.getMinimum() < 0.0 && slider.getMaximum() > 0.0 && slider.getMaximum() <= 50.0;
        drawSpaceKnob (g, centre, radius, angle, startAngle, endAngle,
                       bip ? startAngle + (float) slider.valueToProportionOfLength (0.0) * (endAngle - startAngle) : startAngle);
        return;
    }
    auto at = [&] (float a, float r) { return Point<float> (centre.x + r * std::sin (a), centre.y - r * std::cos (a)); };

    // Puntos de escala alrededor (como en las perillas de FabFilter)
    const float dotR = radius - 1.5f;
    for (int i = 0; i <= 10; ++i)
    {
        const float a = startAngle + (endAngle - startAngle) * (float) i / 10.0f;
        const bool lit = a <= angle + 0.001f;
        g.setColour (lit ? Palette::accent.withAlpha (0.85f) : Palette::dim.withAlpha (0.35f));
        g.fillEllipse (Rectangle<float> (2.0f, 2.0f).withCentre (at (a, dotR)));
    }

    // Anillo: pista fina y arco de valor con resplandor
    const float ringR = radius - 6.5f;
    Path track;
    track.addCentredArc (centre.x, centre.y, ringR, ringR, 0, startAngle, endAngle, true);
    g.setColour (Palette::track);
    g.strokePath (track, PathStrokeType (2.0f, PathStrokeType::curved, PathStrokeType::rounded));

    const bool bipolar = slider.getMinimum() < 0.0 && slider.getMaximum() > 0.0 && slider.getMaximum() <= 50.0;
    const float fromAngle = bipolar ? startAngle + (float) slider.valueToProportionOfLength (0.0) * (endAngle - startAngle)
                                    : startAngle;
    if (std::abs (angle - fromAngle) > 0.01f)
    {
        Path value;
        value.addCentredArc (centre.x, centre.y, ringR, ringR, 0, jmin (fromAngle, angle), jmax (fromAngle, angle), true);
        Style::glowStroke (g, value, Palette::accent, 2.2f, 1.2f);
    }

    // Disco: degradado con luz desde arriba, borde fino y sombra interior
    const float bodyR = ringR - 6.0f;
    const auto body = Rectangle<float> (bodyR * 2, bodyR * 2).withCentre (centre);
    Path disc;
    disc.addEllipse (body);
    DropShadow (Palette::shadow, (int) (bodyR * 0.45f), { 0, (int) (bodyR * 0.15f) + 1 }).drawForPath (g, disc);
    g.setGradientFill (ColourGradient (Palette::knobTop, centre.x - bodyR * 0.4f, body.getY(),
                                       Palette::knobBottom, centre.x + bodyR * 0.3f, body.getBottom(), false));
    g.fillPath (disc);
    g.setColour (Palette::knobEdge);
    g.strokePath (disc, PathStrokeType (1.0f));
    g.setColour (Palette::highlight);
    Path rim;
    rim.addCentredArc (centre.x, centre.y, bodyR - 1.5f, bodyR - 1.5f, 0, -1.9f, 1.9f, true);
    g.strokePath (rim, PathStrokeType (1.0f));

    // Indicador corto que brilla en el borde del disco
    Path pointer;
    pointer.startNewSubPath (at (angle, bodyR * 0.48f));
    pointer.lineTo (at (angle, bodyR * 0.86f));
    Style::glowStroke (g, pointer, Palette::pointer, 2.0f, 0.8f);
}

void RadioLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool highlighted, bool)
{
    using namespace juce;
    const bool on = b.getToggleState();
    auto r = b.getLocalBounds().toFloat();

    // Boton de encendido de seccion (simbolo de power que se ilumina)
    if ((bool) b.getProperties()["power"])
    {
        auto c = r.withSizeKeepingCentre (22.0f, 22.0f);
        Path ring;
        ring.addEllipse (c);
        const auto pc = Theme::isDark() ? Palette::accent2 : Palette::accent;
        g.setColour (on ? pc.withAlpha (0.14f) : Palette::panelHi);
        g.fillPath (ring);
        g.setColour (on ? pc.withAlpha (0.6f) : Palette::cardBorder);
        g.strokePath (ring, PathStrokeType (1.0f));
        const auto ctr = c.getCentre();
        Path sym;
        sym.addCentredArc (ctr.x, ctr.y, 5.5f, 5.5f, 0, 0.6f, MathConstants<float>::twoPi - 0.6f, true);
        sym.startNewSubPath (ctr.x, ctr.y - 7.0f);
        sym.lineTo (ctr.x, ctr.y - 1.5f);
        if (on) Style::glowStroke (g, sym, pc, 1.6f, 1.0f);
        else
        {
            g.setColour (highlighted ? Palette::text : Palette::dim);
            g.strokePath (sym, PathStrokeType (1.6f, PathStrokeType::curved, PathStrokeType::rounded));
        }
        return;
    }

    // Interruptor fino
    auto drawSwitch = [&] (Rectangle<float> area)
    {
        auto pill = area.withSizeKeepingCentre (jmin (34.0f, area.getWidth()), 18.0f);
        if (on && Theme::isDark())
            g.setGradientFill (ColourGradient (Palette::accent2, pill.getX(), 0, Palette::accent, pill.getRight(), 0, false));
        else
            g.setColour (on ? Palette::accent : Palette::switchOff);
        g.fillRoundedRectangle (pill, 9.0f);
        g.setColour (Colours::black.withAlpha (0.12f));
        g.drawRoundedRectangle (pill.reduced (0.5f), 9.0f, 1.0f);
        if (highlighted) { g.setColour (Colours::white.withAlpha (0.07f)); g.fillRoundedRectangle (pill, 9.0f); }
        auto knob = Rectangle<float> (14.0f, 14.0f).withCentre ({ on ? pill.getRight() - 9.0f : pill.getX() + 9.0f, pill.getCentreY() });
        Path kp;
        kp.addEllipse (knob);
        DropShadow (Colour (0x50000000), 3, { 0, 1 }).drawForPath (g, kp);
        g.setColour (Colours::white);
        g.fillPath (kp);
    };

    if (b.getButtonText().isEmpty())
    {
        drawSwitch (r);
        return;
    }
    drawSwitch (r.removeFromLeft (38.0f));
    g.setColour (Palette::text);
    g.setFont (font (13.0f));
    g.drawText (b.getButtonText(), r.withTrimmedLeft (6), Justification::centredLeft);
}

void RadioLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    using namespace juce;
    auto r = Rectangle<int> (w, h).toFloat().reduced (0.5f);
    g.setGradientFill (ColourGradient (Palette::panelHi.brighter (0.04f), 0, 0, Palette::panelHi.darker (Theme::isDark() ? 0.1f : 0.02f), 0, (float) h, false));
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (box.hasKeyboardFocus (false) || box.isMouseOver (true) ? Palette::accent.withAlpha (0.7f) : Palette::cardBorder);
    g.drawRoundedRectangle (r, 6.0f, 1.0f);

    Path tri;
    const float ax = (float) w - 15.0f, ay = (float) h * 0.5f;
    tri.addTriangle (ax - 3.5f, ay - 1.5f, ax + 3.5f, ay - 1.5f, ax, ay + 2.5f);
    g.setColour (Palette::dim);
    g.fillPath (tri);
}

void RadioLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (8, 1, box.getWidth() - 30, box.getHeight() - 2);
    label.setFont (getComboBoxFont (box));
}

void RadioLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool highlighted, bool down)
{
    using namespace juce;
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    const auto seg = b.getProperties()["segment"].toString(); // "left" / "right" / "" (botones unidos)
    const float rad = 6.0f;

    Path p;
    if (seg == "left")       p.addRoundedRectangle (r.getX(), r.getY(), r.getWidth(), r.getHeight(), rad, rad, true, false, true, false);
    else if (seg == "right") p.addRoundedRectangle (r.getX(), r.getY(), r.getWidth(), r.getHeight(), rad, rad, false, true, false, true);
    else                     p.addRoundedRectangle (r, rad);

    if (b.getToggleState())
    {
        if (Theme::isDark())
            g.setGradientFill (ColourGradient (Palette::toggleTop, 0, r.getY(), Palette::toggleBottom, 0, r.getBottom(), false));
        else
            g.setGradientFill (ColourGradient (Palette::accent.brighter (0.12f), 0, r.getY(), Palette::accent.darker (0.08f), 0, r.getBottom(), false));
        g.fillPath (p);
        if (Theme::isDark()) { g.setColour (Colours::white.withAlpha (0.18f)); g.strokePath (p, PathStrokeType (1.0f)); }
        return;
    }
    auto c = Palette::panelHi;
    if (down) c = c.darker (0.08f);
    else if (highlighted) c = Theme::isDark() ? c.brighter (0.12f) : c.darker (0.025f);
    g.setGradientFill (ColourGradient (c.brighter (0.04f), 0, r.getY(), c.darker (Theme::isDark() ? 0.12f : 0.02f), 0, r.getBottom(), false));
    g.fillPath (p);
    g.setColour (highlighted ? Palette::accent.withAlpha (0.5f) : Palette::cardBorder);
    g.strokePath (p, PathStrokeType (1.0f));
}

void RadioLookAndFeel::drawCallOutBoxBackground (juce::CallOutBox&, juce::Graphics& g, const juce::Path& path, juce::Image&)
{
    juce::DropShadow (Palette::shadow, 14, { 0, 4 }).drawForPath (g, path);
    g.setColour (Palette::popup);
    g.fillPath (path);
    g.setColour (Palette::cardBorder);
    g.strokePath (path, juce::PathStrokeType (1.0f));
}

juce::Label* RadioLookAndFeel::createSliderTextBox (juce::Slider& slider)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (slider);
    l->setFont (Theme::isDark() ? monoFont (12.5f) : font (12.5f));
    return l;
}

juce::Font RadioLookAndFeel::getComboBoxFont (juce::ComboBox&)                { return font (13.5f); }
juce::Font RadioLookAndFeel::getLabelFont (juce::Label& l)
{
    // Los valores de las perillas van en SF Mono en el tema oscuro (como en la maqueta)
    if (Theme::isDark() && dynamic_cast<juce::Slider*> (l.getParentComponent()) != nullptr)
        return monoFont (12.0f);
    return font (12.5f);
}
juce::Font RadioLookAndFeel::getTextButtonFont (juce::TextButton&, int h)     { return font (juce::jmin (14.0f, h * 0.48f)); }

//==============================================================================
void IconButton::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    using namespace juce;
    auto r = getLocalBounds().toFloat().reduced (0.5f);
    const bool on = getToggleState();

    Path bgPath;
    if (isRound) bgPath.addEllipse (r); else bgPath.addRoundedRectangle (r, 6.0f);
    if (icon != Icon::help)
    {
        if (isRound && ! Theme::isDark()) DropShadow (Palette::shadow, 6, { 0, 2 }).drawForPath (g, bgPath);
        auto c = on ? Palette::accent : Palette::panelHi;
        if (! on && down) c = c.darker (0.08f);
        else if (! on && highlighted) c = Theme::isDark() ? c.brighter (0.12f) : c.darker (0.025f);
        g.setGradientFill (ColourGradient (c.brighter (0.05f), 0, r.getY(), c.darker (Theme::isDark() ? 0.12f : 0.02f), 0, r.getBottom(), false));
        g.fillPath (bgPath);
        g.setColour (on ? Palette::accent : (highlighted ? Palette::accent.withAlpha (0.5f) : Palette::cardBorder));
        g.strokePath (bgPath, PathStrokeType (1.0f));
    }

    const auto ic = on ? Colours::white
                       : (icon == Icon::help ? Palette::dim : (isRound ? Palette::accent : Palette::text));
    g.setColour (ic);
    const auto c = r.getCentre();
    const float s = jmin (r.getWidth(), r.getHeight());
    PathStrokeType stroke (1.7f, PathStrokeType::curved, PathStrokeType::rounded);
    Path p;

    switch (icon)
    {
        case Icon::left:
        case Icon::right:
        {
            const float d = icon == Icon::left ? 1.0f : -1.0f, k = s * 0.13f;
            p.startNewSubPath (c.x + d * k * 0.6f, c.y - k);
            p.lineTo (c.x - d * k * 0.6f, c.y);
            p.lineTo (c.x + d * k * 0.6f, c.y + k);
            g.strokePath (p, stroke);
            break;
        }
        case Icon::dots:
            for (int i = -1; i <= 1; ++i)
                g.fillEllipse (Rectangle<float> (3.4f, 3.4f).withCentre ({ c.x + i * 6.0f, c.y }));
            break;
        case Icon::eqOnly: // onda de analizador
        {
            const float w = s * 0.5f, h = s * 0.2f;
            p.startNewSubPath (c.x - w / 2, c.y);
            p.lineTo (c.x - w / 4, c.y);
            p.lineTo (c.x - w / 8, c.y - h);
            p.lineTo (c.x + w / 16, c.y + h);
            p.lineTo (c.x + w / 5, c.y - h * 0.4f);
            p.lineTo (c.x + w / 3.2f, c.y);
            p.lineTo (c.x + w / 2, c.y);
            g.strokePath (p, stroke);
            break;
        }
        case Icon::moon:
        {
            const float rr = s * 0.2f;
            Path moon;
            moon.addEllipse (Rectangle<float> (rr * 2, rr * 2).withCentre (c));
            Path cut;
            cut.addEllipse (Rectangle<float> (rr * 2, rr * 2).withCentre ({ c.x + rr * 0.75f, c.y - rr * 0.6f }));
            moon.setUsingNonZeroWinding (false);
            moon.addPath (cut);
            Graphics::ScopedSaveState st (g);
            g.reduceClipRegion (moon);
            g.fillEllipse (Rectangle<float> (rr * 2, rr * 2).withCentre (c));
            break;
        }
        case Icon::planet:
        {
            // Planeta con anillo: vuelve al tema Interplanetary
            const float rr = s * 0.13f;
            g.fillEllipse (Rectangle<float> (rr * 2, rr * 2).withCentre (c));
            Path ring;
            ring.addCentredArc (c.x, c.y, rr * 2.1f, rr * 0.8f, -0.4f, 0, MathConstants<float>::twoPi, true);
            g.strokePath (ring, PathStrokeType (1.3f));
            break;
        }
        case Icon::sun:
        {
            const float rr = s * 0.11f;
            g.fillEllipse (Rectangle<float> (rr * 2, rr * 2).withCentre (c));
            for (int i = 0; i < 8; ++i)
            {
                const float a = MathConstants<float>::twoPi * (float) i / 8.0f;
                p.startNewSubPath (c.x + std::cos (a) * rr * 1.7f, c.y + std::sin (a) * rr * 1.7f);
                p.lineTo (c.x + std::cos (a) * rr * 2.5f, c.y + std::sin (a) * rr * 2.5f);
            }
            g.strokePath (p, stroke);
            break;
        }
        case Icon::help:
        {
            g.setColour (highlighted ? Palette::accent : Palette::cardBorder.darker (Theme::isDark() ? -0.4f : 0.15f));
            g.drawEllipse (r.reduced (1.0f), 1.2f);
            g.setColour (highlighted ? Palette::accent : Palette::dim);
            g.setFont (font (s * 0.6f, true));
            g.drawText ("?", r, Justification::centred);
            break;
        }
    }
}

//==============================================================================
Cell::Cell (RadioFXProcessor& p, const juce::String& paramId, const juce::String& t, Kind k)
    : kind (k), title (t)
{
    using APVTS = juce::AudioProcessorValueTreeState;
    if (kind == Kind::knob)
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 90, 20);
        slider.setPopupDisplayEnabled (false, false, nullptr);
        addAndMakeVisible (slider);
        sAtt = std::make_unique<APVTS::SliderAttachment> (p.apvts, paramId, slider);
        // Doble clic vuelve al valor por defecto del parametro.
        if (auto* param = p.apvts.getParameter (paramId))
            slider.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
    }
    else if (kind == Kind::choice)
    {
        if (auto* param = dynamic_cast<juce::AudioParameterChoice*> (p.apvts.getParameter (paramId)))
            combo.addItemList (param->choices, 1);
        addAndMakeVisible (combo);
        cAtt = std::make_unique<APVTS::ComboBoxAttachment> (p.apvts, paramId, combo);
    }
    else
    {
        addAndMakeVisible (toggle);
        bAtt = std::make_unique<APVTS::ButtonAttachment> (p.apvts, paramId, toggle);
    }
    refreshColours();
}

void Cell::refreshColours()
{
    slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    slider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    slider.setColour (juce::Slider::textBoxTextColourId, Palette::text);
    repaint();
}

void Cell::resized()
{
    auto r = getLocalBounds().withTrimmedTop (20);
    if (kind == Kind::knob)        slider.setBounds (r);
    else if (kind == Kind::choice) combo.setBounds (r.withSizeKeepingCentre (juce::jmin (r.getWidth() - 8, 230), 30));
    else                           toggle.setBounds (r.withSizeKeepingCentre (juce::jmin (r.getWidth() - 6, 60), 30));
}

void Cell::paint (juce::Graphics& g)
{
    g.setColour (Palette::dim);
    g.setFont (labelFont (10.0f, false));
    g.drawFittedText (upper (title), getLocalBounds().removeFromTop (20), juce::Justification::centred, 1, 0.8f);
}

//==============================================================================
RadioFXEditor::RadioFXEditor (RadioFXProcessor& p)
    : AudioProcessorEditor (p), proc (p), eq (p), standalone (p.isStandalone())
{
    setLookAndFeel (&lnf);
    using K = Cell::Kind;

    setupSection (secInput, "ENTRADA", { addCell (ParamID::inGain, "Ganancia") }, tabSignal, {},
                  T ("Ganancia antes de todo el efecto. Súbela si la voz llega baja y el squelch no se activa; bájala si satura."));
    setupSection (secDyn, T ("DINÁMICA"), { addCell (ParamID::comp, T ("Compresión")) }, tabSignal, ParamID::secDyn,
                  T ("Aplasta la voz como un transmisor, para que todo suene al mismo nivel. El medidor GR muestra cuántos dB está bajando."));
    setupSection (secDirt, "SUCIEDAD", { addCell (ParamID::drive, "Drive"), addCell (ParamID::driveType, "Tipo", K::choice),
                                         addCell (ParamID::bits, "Bits"), addCell (ParamID::rate, "Muestreo"), addCell (ParamID::glitch, "Fallas") },
                  tabSignal, ParamID::secDirt,
                  T ("Drive satura la señal (suave, duro o asimétrico). Bits y Muestreo la degradan como audio digital barato. Fallas imita los cortes del códec de un celular."));
    setupSection (secNoise, "RUIDO", { addCell (ParamID::hiss, "Soplido"), addCell (ParamID::crackle, T ("Estática")),
                                       addCell (ParamID::dropouts, "Cortes"), addCell (ParamID::hissGate, "Solo al hablar", K::toggle) },
                  tabSignal, ParamID::secNoise,
                  T ("Soplido de fondo, estática (crujidos) y cortes de señal. \"Solo al hablar\" hace que el ruido aparezca solo mientras se transmite."));
    setupSection (secTape, "CINTA", { addCell (ParamID::wow, "Wow"), addCell (ParamID::flutter, "Flutter") }, tabSignal, ParamID::secTape,
                  T ("Wow es el vaivén lento de afinación de una cinta gastada; Flutter, el temblor rápido. Úsalos para VHS o cassette."));
    setupSection (secSquelch, "SQUELCH", { addCell (ParamID::squelch, "Kshh"), addCell (ParamID::squelchThr, "Umbral voz") }, tabSignal,
                  ParamID::secSquelch,
                  T ("El \"kshh\" al apretar y soltar el botón de la radio. Umbral voz: desde qué nivel se considera que alguien está hablando."));
    setupSection (secOut, "SALIDA", { addCell (ParamID::mix, "Mezcla"), addCell (ParamID::outGain, "Volumen"), addCell (ParamID::mono, "Mono", K::toggle) },
                  tabSignal, {}, T ("Mezcla entre la voz original y el efecto, volumen final (después de la escena) y salida en mono."));

    setupSection (secDevice, "APARATO", { addCell (ParamID::device, T ("Qué lo reproduce"), K::choice), addCell (ParamID::deviceDrive, T ("Saturación")),
                                          addCell (ParamID::deviceTune, "Tono") }, tabScene, ParamID::secDevice,
                  T ("Qué reproduce la voz: radio de patrulla, TV, celular, megáfono… Saturación: cuánto se rompe el parlante. Tono: aparato más grande o más chico."));
    setupSection (secDistance, "DISTANCIA", { addCell (ParamID::distance, "Lejos") }, tabScene, ParamID::secDistance,
                  T ("Qué tan lejos está el sonido: baja el volumen, quita agudos y, si hay espacio, aumenta el eco."));
    setupSection (secRoom, "ESPACIO", { addCell (ParamID::room, T ("Dónde suena"), K::choice), addCell (ParamID::roomMix, "Cantidad") },
                  tabScene, ParamID::secRoom, T ("Dónde suena: cuarto, baño, pasillo, garaje, calle, iglesia… Cantidad: cuánto se oye el lugar."));
    setupSection (secObstacle, T ("OBSTÁCULO"), { addCell (ParamID::obstacle, "Entre el sonido y quien escucha", K::choice) }, tabScene,
                  ParamID::secObstacle,
                  T ("Lo que hay entre el sonido y quien escucha: puerta, pared o ventana; o el aparato metido en una caja, maletera, manta o bolsillo."));
    setupSection (secTransmit, T ("TRANSMISIÓN"), { addCell (ParamID::txMode, "Modo", K::choice), addCell (ParamID::txKey, "Transmitir", K::toggle) },
                  tabScene, {}, T ("Automático: el transmisor se abre cuando detecta voz. Manual: solo con el botón Transmitir (puedes automatizarlo en la línea de tiempo)."));
    setupSection (secRoger, "ROGER BEEP", { addCell (ParamID::roger, "Tipo", K::choice), addCell (ParamID::rogerLvl, "Nivel"),
                                            addCell (ParamID::rogerStart, "Bip al empezar", K::toggle) },
                  tabScene, {}, T ("El bip al soltar el botón de la radio. \"Bip al empezar\" agrega otro al apretarlo."));
    setupSection (secQuindar, "QUINDAR", { addCell (ParamID::quindarLvl, "Nivel") }, tabScene, ParamID::quindar,
                  T ("Los bips de la NASA: 2525 Hz al empezar a hablar y 2475 Hz al terminar."));

    // Pestanas como control segmentado
    const char* tabNames[] = { "Señal", "Escena y transmisión" };
    for (int t = 0; t < numTabs; ++t)
    {
        auto& b = tabButtons[(size_t) t];
        b.setButtonText (T (tabNames[t]));
        b.setClickingTogglesState (true);
        b.setRadioGroupId (4711);
        b.getProperties().set ("segment", t == 0 ? "left" : "right");
        b.onClick = [this, t] { if (tabButtons[(size_t) t].getToggleState()) showTab (t); };
        addAndMakeVisible (b);
    }

    addAndMakeVisible (eq);

    // Cabecera
    presetBox.setTextWhenNothingSelected ("Elegir preset");
    presetBox.setJustificationType (juce::Justification::centred);
    presetBox.onChange = [this] { presetChosen(); };
    rebuildPresetList();
    prevPreset.onClick = [this] { stepPreset (-1); };
    nextPreset.onClick = [this] { stepPreset (1); };
    prevPreset.setTooltip ("Preset anterior");
    nextPreset.setTooltip ("Preset siguiente");

    aButton.getProperties().set ("segment", "left");
    bButton.getProperties().set ("segment", "right");
    for (auto* b : { &aButton, &bButton })
    {
        b->setClickingTogglesState (true);
        b->setRadioGroupId (8123);
        b->onClick = [this, b] { if (b->getToggleState()) switchAB (b == &aButton ? 0 : 1); };
    }
    aButton.setToggleState (proc.getCurrentAB() == 0, juce::dontSendNotification);
    bButton.setToggleState (proc.getCurrentAB() == 1, juce::dontSendNotification);

    moreButton.onClick = [this] { showMoreMenu(); };
    moreButton.setTooltip (T ("Más opciones"));
    saveButton.setButtonText (T ("Guardar…"));
    saveButton.setTooltip ("Guardar preset");
    saveButton.onClick = [this] { savePresetDialog(); };
    eqOnlyButton.setClickingTogglesState (true);
    eqOnlyButton.setTooltip ("Solo ecualizador");
    eqOnlyButton.onClick = [this] { setControlsHidden (eqOnlyButton.getToggleState()); };
    themeButton.onClick = [this] { Theme::setNight (! Theme::isNight()); refreshTheme(); };

    for (auto* c : std::initializer_list<juce::Component*> { &prevPreset, &presetBox, &nextPreset, &aButton, &bButton,
                                                             &moreButton, &saveButton, &eqOnlyButton, &themeButton })
        addAndMakeVisible (c);

    if (standalone)
    {
        openButton.onClick = [this] { openFile(); };
        playButton.onClick = [this] { proc.setPlaying (! proc.isPlaying()); };
        exportButton.onClick = [this] { exportFile(); };
        loopButton.onClick = [this] { proc.setLooping (loopButton.getToggleState()); };
        fileLabel.setFont (font (13.0f));
        timeLabel.setFont (font (13.0f));
        timeLabel.setJustificationType (juce::Justification::centredRight);
        for (auto* c : std::initializer_list<juce::Component*> { &openButton, &playButton, &exportButton, &loopButton, &fileLabel, &timeLabel })
            addAndMakeVisible (c);
        const auto f = proc.getLoadedFile();
        fileLabel.setText (f.existsAsFile() ? f.getFileName() : T ("Arrastra un audio aquí o pulsa \"Abrir audio\""), juce::dontSendNotification);
    }

    addMouseListener (this, true); // ver mouseDown: transacciones de Deshacer
    logoImage = juce::ImageCache::getFromMemory (BinaryData::capcomlogo_png, BinaryData::capcomlogo_pngSize);

    setSize (1240, standalone ? 832 : 770);
    showTab ((int) proc.apvts.state.getProperty ("uiTab", 0));
    setControlsHidden ((bool) proc.apvts.state.getProperty ("uiCollapsed", false));
    refreshTheme();
    startTimerHz (30);
}

RadioFXEditor::~RadioFXEditor()
{
    stopTimer();
    if (helpBox != nullptr) { helpBox->setLookAndFeel (nullptr); helpBox->dismiss(); }
    setLookAndFeel (nullptr);
}

void RadioFXEditor::setupSection (Section& s, const juce::String& title, std::vector<Cell*> sectionCells, int tab,
                                  const juce::String& toggleId, const juce::String& help)
{
    s.title = title;
    s.cells = std::move (sectionCells);
    s.tab = tab;
    s.toggleId = toggleId;
    s.help = help;
    if (toggleId.isNotEmpty())
    {
        s.enable = std::make_unique<juce::ToggleButton>();
        s.enable->getProperties().set ("power", true);
        s.enable->setTooltip ("Activar o apagar " + title.toLowerCase());
        s.enableAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, toggleId, *s.enable);
        addAndMakeVisible (*s.enable);
    }
    s.helpButton = std::make_unique<IconButton> (IconButton::Icon::help);
    s.helpButton->onClick = [this, &s] { showHelp (s); };
    addAndMakeVisible (*s.helpButton);
}

Cell* RadioFXEditor::addCell (const juce::String& id, const juce::String& title, Cell::Kind kind)
{
    auto* c = cells.add (new Cell (proc, id, title, kind));
    addAndMakeVisible (c);
    return c;
}

void RadioFXEditor::refreshTheme()
{
    seenThemeVersion = Theme::version();
    lnf.applyTheme();
    for (auto* c : cells) c->refreshColours();
    fileLabel.setColour (juce::Label::textColourId, Palette::dim);
    timeLabel.setColour (juce::Label::textColourId, Palette::text);
    themeButton.setIcon (Theme::isNight() ? IconButton::Icon::planet : IconButton::Icon::moon);
    themeButton.setTooltip (Theme::isNight() ? "Tema Interplanetary" : "Modo noche");
    eq.themeChanged();
    sendLookAndFeelChange();
    repaint();
}

//==============================================================================
void RadioFXEditor::paint (juce::Graphics& g)
{
    using namespace juce;
    // Fondo (espacio o claro), dibujado una vez y guardado
    if (bgCache.isNull() || bgVersion != Theme::version() || bgCache.getWidth() != getWidth() * 2 || bgCache.getHeight() != getHeight() * 2)
        buildBackground();
    g.drawImage (bgCache, getLocalBounds().toFloat());

    // Logotipo (Assets/capcom-logo.png)
    if (logoImage.isValid())
    {
        g.setImageResamplingQuality (Graphics::highResamplingQuality);
        g.drawImage (logoImage, logoArea.toFloat(), RectanglePlacement::xLeft | RectanglePlacement::yMid);
    }

    paintRoute (g);

    if (standalone)
        drawCard (g, fileBarArea.toFloat(), 12.0f);
    drawCard (g, eq.getBounds().toFloat()); // el ecualizador pinta su grafico encima

    // Tarjetas de seccion
    for (auto* s : allSections())
    {
        if (s->tab != currentTab || controlsHidden) continue;
        drawCard (g, s->bounds.toFloat());
        auto head = s->bounds.reduced (14, 0).withHeight (40);
        if (s->enable != nullptr) head.removeFromLeft (34);
        head.removeFromRight (28); // zona del "?"
        const bool on = s->enable == nullptr || s->enable->getToggleState();
        g.setColour (on ? Palette::text : Palette::dim);
        g.setFont (labelFont (11.5f));
        g.drawFittedText (s->title, head, Justification::centredLeft, 1, 0.85f);
    }

    paintMeters (g);
}

void RadioFXEditor::buildBackground()
{
    using namespace juce;
    const int w = getWidth(), h = getHeight();
    bgCache = Image (Image::ARGB, w * 2, h * 2, true);
    Graphics g (bgCache);
    g.addTransform (AffineTransform::scale (2.0f));
    bgVersion = Theme::version();

    if (! Theme::isDark())
    {
        g.setGradientFill (ColourGradient (Palette::bgTop, 0, 0, Palette::bg, 0, (float) h * 0.6f, false));
        g.fillAll();
        const float bandBottom = (float) headerArea.getBottom() + 7.0f;
        g.setColour (Palette::cardBorder);
        g.drawHorizontalLine ((int) bandBottom, 0.0f, (float) w);
        return;
    }

    if (Theme::isNight())
    {
        // Noche: negro neutro con pocas estrellas tenues, sin nebulosas ni Marte
        g.setGradientFill (ColourGradient (Palette::bgTop, 0, 0, Palette::bg, 0, (float) h * 0.7f, false));
        g.fillAll();
        Random rng (1969);
        for (int i = 0; i < 120; ++i)
        {
            const float x = rng.nextFloat() * (float) w, y = rng.nextFloat() * (float) h;
            const float r = 0.35f + std::pow (rng.nextFloat(), 3.0f) * 0.9f;
            g.setColour (Colours::white.withAlpha (0.12f + rng.nextFloat() * 0.3f));
            g.fillEllipse (Rectangle<float> (r * 2, r * 2).withCentre ({ x, y }));
        }
        return;
    }

    // Espacio profundo
    g.setGradientFill (ColourGradient (Colour (0xff070818), 0, 0, Palette::bg, 0, (float) h, false));
    g.fillAll();
    // Nebulosas violeta y azul
    auto nebula = [&] (Colour c, float alpha, float cx, float cy, float r)
    {
        g.setGradientFill (ColourGradient (c.withAlpha (alpha), cx, cy, c.withAlpha (0.0f), cx + r, cy, true));
        g.fillEllipse (Rectangle<float> (r * 2, r * 2).withCentre ({ cx, cy }));
    };
    nebula (Colour (0xff6a4bff), 0.22f, w * 0.38f, h * 0.08f, w * 0.48f);
    nebula (Colour (0xff24174f), 0.55f, w * 0.75f, h * 0.30f, w * 0.40f);
    nebula (Colour (0xff1fa6c9), 0.13f, w * 0.10f, h * 0.88f, w * 0.42f);
    nebula (Colour (0xff0b3550), 0.40f, w * 0.30f, h * 0.70f, w * 0.35f);

    // Estrellas (siempre las mismas)
    Random rng (1969);
    for (int i = 0; i < 230; ++i)
    {
        const float x = rng.nextFloat() * (float) w, y = rng.nextFloat() * (float) h;
        const float r = 0.35f + std::pow (rng.nextFloat(), 3.0f) * 1.3f;
        const float a = 0.25f + rng.nextFloat() * 0.6f;
        const auto c = rng.nextFloat() < 0.15f ? Colour (0xffcfe8ff) : (rng.nextFloat() < 0.1f ? Colour (0xffffe9d6) : Colours::white);
        if (r > 1.2f)
        {
            g.setGradientFill (ColourGradient (c.withAlpha (a * 0.4f), x, y, c.withAlpha (0.0f), x + r * 4, y, true));
            g.fillEllipse (Rectangle<float> (r * 8, r * 8).withCentre ({ x, y }));
        }
        g.setColour (c.withAlpha (a));
        g.fillEllipse (Rectangle<float> (r * 2, r * 2).withCentre ({ x, y }));
    }

    // Borde de Marte asomando abajo a la derecha, con su atmosfera
    const Point<float> mc ((float) w + 150.0f, (float) h + 330.0f);
    const float mr = 470.0f;
    g.setGradientFill (ColourGradient (Colour (0xffff7a3d).withAlpha (0.18f), mc.x, mc.y, Colour (0xffff7a3d).withAlpha (0.0f), mc.x + mr + 60, mc.y, true));
    g.fillEllipse (Rectangle<float> ((mr + 60) * 2, (mr + 60) * 2).withCentre (mc));
    ColourGradient limb (Colour (0xffc2522a), mc.x - mr * 0.35f, mc.y - mr * 0.75f, Colour (0xff14070a), mc.x, mc.y, true);
    limb.addColour (0.55, Colour (0xff5a1e14));
    g.setGradientFill (limb);
    g.fillEllipse (Rectangle<float> (mr * 2, mr * 2).withCentre (mc));
}

void RadioFXEditor::paintRoute (juce::Graphics& g)
{
    // Ruta de la senal: Tierra -> rele MRO -> Marte, con un pulso que viaja (y se acelera al transmitir)
    using namespace juce;
    if (routeArea.getWidth() < 160) return;
    const auto r = routeArea.toFloat();
    const Point<float> earth (r.getX() + 16.0f, r.getCentreY() + 4.0f), mars (r.getRight() - 16.0f, r.getCentreY() + 4.0f);
    const Point<float> relay (r.getCentreX() + r.getWidth() * 0.18f, r.getY() + 6.0f);
    const Point<float> ctrl (r.getCentreX(), r.getY() - 8.0f);

    Path route;
    route.startNewSubPath (earth);
    route.quadraticTo (ctrl, mars);
    Path dashed;
    const float dashes[] = { 3.0f, 4.0f };
    PathStrokeType (1.2f).createDashedStroke (dashed, route, dashes, 2);
    g.setGradientFill (ColourGradient (Palette::accent2.withAlpha (0.8f), earth.x, 0, Palette::mars.withAlpha (0.8f), mars.x, 0, false));
    g.fillPath (dashed);

    // Pulso de senal
    const float len = route.getLength();
    const auto pulse = route.getPointAlongPath (len * routePhase);
    g.setGradientFill (ColourGradient (Colours::white.withAlpha (0.9f), pulse.x, pulse.y, Palette::accent2.withAlpha (0.0f), pulse.x + 8, pulse.y, true));
    g.fillEllipse (Rectangle<float> (16, 16).withCentre (pulse));

    // Rele (satelite) y planetas
    g.setColour (Palette::dim);
    g.fillRect (Rectangle<float> (7, 3).withCentre (relay));
    g.setColour (Palette::accent2);
    g.fillRect (Rectangle<float> (3, 3).withCentre (relay));
    drawEarth (g, earth, 9.0f);
    drawMars (g, mars, 8.0f);

    g.setFont (monoFont (9.5f));
    g.setColour (Palette::dim);
    g.drawText (T ("RETARDO 12 min 41 s · 228 M km"), Rectangle<float> (r.getWidth(), 12).withCentre ({ r.getCentreX(), r.getCentreY() + 1.0f }), Justification::centred);
    g.setFont (labelFont (8.5f, false));
    g.drawText ("TIERRA", Rectangle<float> (60, 11).withCentre ({ earth.x, r.getBottom() - 4.0f }), Justification::centred);
    g.setColour (Palette::mars);
    g.drawText ("MARTE", Rectangle<float> (60, 11).withCentre ({ mars.x, r.getBottom() - 4.0f }), Justification::centred);
    g.setColour (Palette::dim);
    g.drawText (T ("RELÉ MRO"), Rectangle<float> (70, 11).withCentre ({ relay.x, relay.y - 8.0f }), Justification::centred);
}

void RadioFXEditor::paintMeters (juce::Graphics& g)
{
    using namespace juce;
    drawPanel (g, meterArea.toFloat());
    auto m = meterArea.reduced (14, 12);

    // Titulo + indicador TX
    auto head = m.removeFromTop (22);
    g.setColour (Palette::text);
    g.setFont (labelFont (11.0f));
    g.drawText ("SALIDA", head, Justification::centredLeft);
    auto led = head.removeFromRight (40).toFloat();
    auto dot = Rectangle<float> (7, 7).withCentre ({ led.getX() + 6, led.getCentreY() });
    const auto txColour = Theme::isDark() ? Palette::mars : Palette::red;
    if (txOn) { g.setColour (txColour.withAlpha (0.3f)); g.fillEllipse (dot.expanded (4)); }
    g.setColour (txOn ? txColour : Palette::switchOff);
    g.fillEllipse (dot);
    g.setColour (txOn ? txColour : Palette::dim);
    g.setFont (labelFont (10.0f));
    g.drawText ("TX", led.withTrimmedLeft (14), Justification::centredLeft);
    m.removeFromTop (12);

    auto grArea = m.removeFromBottom (36);
    auto values = m.removeFromBottom (34);

    // Escala y barras L/R
    auto scale = m.removeFromLeft (30);
    auto bars = m.reduced (4, 0);
    const float bw = 8.0f;
    auto barL = Rectangle<float> (bw, (float) bars.getHeight()).withCentre ({ (float) bars.getX() + bars.getWidth() * 0.36f, (float) bars.getCentreY() });
    auto barR = Rectangle<float> (bw, (float) bars.getHeight()).withCentre ({ (float) bars.getX() + bars.getWidth() * 0.64f, (float) bars.getCentreY() });
    auto yFor = [&] (float db) { return jmap (jlimit (-60.0f, 0.0f, db), 0.0f, -60.0f, barL.getY(), barL.getBottom()); };

    g.setFont (font (9.5f));
    for (float db : { 0.0f, -6.0f, -12.0f, -24.0f, -36.0f, -48.0f, -60.0f })
    {
        const float y = yFor (db);
        g.setColour (Palette::dim.withAlpha (0.85f));
        g.drawText (String ((int) db), scale.withY ((int) y - 6).withHeight (12), Justification::centredRight);
        g.setColour (Palette::line);
        g.drawHorizontalLine ((int) y, (float) scale.getRight() + 3, (float) bars.getRight());
    }

    auto drawBar = [&] (Rectangle<float> bar, float gain, float hold)
    {
        g.setColour (Theme::isDark() ? Palette::bg.withAlpha (0.8f) : Palette::switchOff.withAlpha (0.55f));
        g.fillRoundedRectangle (bar.expanded (1.5f), 3.0f);
        const float db = Decibels::gainToDecibels (gain, -60.0f);
        auto lvl = bar.withTop (yFor (db));
        // verde -> acento -> rojo arriba, en segmentos de 2 px
        if (Theme::isDark())
        {
            ColourGradient cg (Palette::mars, 0, bar.getY(), Palette::accent2, 0, bar.getBottom(), false);
            cg.addColour (0.45, Palette::accent);
            g.setGradientFill (cg);
        }
        else
            g.setGradientFill (ColourGradient (Palette::red, 0, bar.getY(), Palette::green, 0, bar.getBottom(), false));
        for (float y = lvl.getBottom() - 2.0f; y >= lvl.getY(); y -= 3.0f)
            g.fillRect (bar.getX(), y, bar.getWidth(), 2.0f);
        // marca de pico
        const float hdb = Decibels::gainToDecibels (hold, -60.0f);
        if (hdb > -59.5f)
        {
            g.setColour (hdb > -0.5f ? Palette::red : Palette::text.withAlpha (0.8f));
            g.fillRect (bar.getX(), yFor (hdb) - 1.0f, bar.getWidth(), 1.5f);
        }
        return db;
    };
    const float dbL = drawBar (barL, outL, holdL), dbR = drawBar (barR, outR, holdR);

    auto num = [] (float db) { return db <= -59.9f ? String ("-inf") : String (db, 1); };
    auto labelRow = values.removeFromTop (14);
    g.setColour (Palette::dim);
    g.setFont (labelFont (9.5f));
    g.drawText ("L", Rectangle<float> (30, 14).withCentre ({ barL.getCentreX(), (float) labelRow.getCentreY() }), Justification::centred);
    g.drawText ("R", Rectangle<float> (30, 14).withCentre ({ barR.getCentreX(), (float) labelRow.getCentreY() }), Justification::centred);
    g.setColour (Palette::text);
    g.setFont (Theme::isDark() ? monoFont (10.0f) : font (11.5f));
    g.drawText (num (dbL), Rectangle<float> (46, 18).withCentre ({ barL.getCentreX(), (float) values.getCentreY() }), Justification::centred);
    g.drawText (num (dbR), Rectangle<float> (46, 18).withCentre ({ barR.getCentreX(), (float) values.getCentreY() }), Justification::centred);

    // Barra de compresion: fina, con resplandor cuando trabaja
    g.setColour (Palette::dim);
    g.setFont (labelFont (9.5f));
    auto grHead = grArea.removeFromTop (16);
    g.drawText (T ("COMPRESIÓN"), grHead, Justification::centredLeft);
    g.setColour (grLevel < -0.5f ? Palette::accent : Palette::dim);
    g.setFont (font (11.0f));
    g.drawText (String (grLevel, 1) + " dB", grHead, Justification::centredRight);
    auto grBar = grArea.withSizeKeepingCentre (grArea.getWidth(), 4).toFloat();
    g.setColour (Theme::isDark() ? Palette::bg.withAlpha (0.8f) : Palette::switchOff.withAlpha (0.55f));
    g.fillRoundedRectangle (grBar.expanded (1.0f), 2.5f);
    const float frac = jlimit (0.0f, 1.0f, -grLevel / 24.0f);
    if (frac > 0.0f)
    {
        Path gp;
        gp.startNewSubPath (grBar.getX(), grBar.getCentreY());
        gp.lineTo (grBar.getX() + grBar.getWidth() * frac, grBar.getCentreY());
        glowStroke (g, gp, Palette::accent, 4.0f, 0.8f);
    }
}

void RadioFXEditor::layoutRow (juce::Rectangle<int> area, std::vector<Section*> row)
{
    // Cada tarjeta necesita lugar para interruptor + titulo + "?", aunque tenga una sola perilla.
    auto weightOf = [] (const Section* s)
    {
        float w = 0;
        for (auto* c : s->cells) w += c->widthWeight();
        return std::max (w, s->enable != nullptr ? 1.8f : 1.2f);
    };
    float totalWeight = 0;
    for (auto* s : row) totalWeight += weightOf (s);
    const int gap = 12, pad = 14;
    const float unitW = (float) (area.getWidth() - gap * ((int) row.size() - 1) - pad * 2 * (int) row.size()) / totalWeight;

    for (auto* s : row)
    {
        const float sectionW = unitW * weightOf (s);
        s->bounds = area.removeFromLeft (juce::roundToInt (sectionW) + pad * 2);
        area.removeFromLeft (gap);

        auto head = s->bounds.reduced (pad, 0).withHeight (40);
        if (s->enable != nullptr) s->enable->setBounds (head.removeFromLeft (26).withSizeKeepingCentre (26, 26));
        s->helpButton->setBounds (head.removeFromRight (22).withSizeKeepingCentre (20, 20));

        auto inner = s->bounds.reduced (pad, 8).withTrimmedTop (34);
        float cellsW = 0;
        for (auto* c : s->cells) cellsW += unitW * c->widthWeight();
        inner = inner.withSizeKeepingCentre (juce::roundToInt (cellsW), inner.getHeight()); // celdas centradas
        for (auto* c : s->cells)
            c->setBounds (inner.removeFromLeft (juce::roundToInt (unitW * c->widthWeight())));
    }
}

void RadioFXEditor::resized()
{
    auto r = getLocalBounds().reduced (18, 14);

    // Cabecera: titulo | < preset > | A B | ... | Guardar | solo EQ | tema
    headerArea = r.removeFromTop (58);
    logoArea = headerArea.withSizeKeepingCentre (headerArea.getWidth(), 44).withX (headerArea.getX());
    logoArea = logoArea.withWidth (juce::roundToInt (44.0f * (float) logoImage.getWidth() / (float) juce::jmax (1, logoImage.getHeight())));
    auto h = headerArea;
    themeButton.setBounds (h.removeFromRight (42).withSizeKeepingCentre (42, 42));
    h.removeFromRight (10);
    eqOnlyButton.setBounds (h.removeFromRight (40).withSizeKeepingCentre (40, 36));
    h.removeFromRight (16);
    saveButton.setBounds (h.removeFromRight (124).withSizeKeepingCentre (124, 36));
    h.removeFromRight (16);
    moreButton.setBounds (h.removeFromRight (40).withSizeKeepingCentre (40, 36));
    h.removeFromRight (8);
    bButton.setBounds (h.removeFromRight (42).withSizeKeepingCentre (42, 36));
    aButton.setBounds (h.removeFromRight (42).withSizeKeepingCentre (42, 36));
    h.removeFromRight (16);
    nextPreset.setBounds (h.removeFromRight (38).withSizeKeepingCentre (38, 36));
    presetBox.setBounds (h.removeFromRight (210).withSizeKeepingCentre (210, 36));
    prevPreset.setBounds (h.removeFromRight (38).withSizeKeepingCentre (38, 36));
    h.removeFromRight (18);
    h.removeFromLeft (logoArea.getWidth() + 12); // logotipo
    routeArea = h.withSizeKeepingCentre (juce::jmin (h.getWidth(), 300), 46);
    r.removeFromTop (12);

    if (standalone)
    {
        fileBarArea = r.removeFromTop (52);
        auto f = fileBarArea.reduced (12, 9);
        openButton.setBounds (f.removeFromLeft (120)); f.removeFromLeft (8);
        playButton.setBounds (f.removeFromLeft (110)); f.removeFromLeft (12);
        loopButton.setBounds (f.removeFromLeft (110)); f.removeFromLeft (8);
        exportButton.setBounds (f.removeFromRight (130)); f.removeFromRight (8);
        timeLabel.setBounds (f.removeFromRight (110));
        fileLabel.setBounds (f);
        r.removeFromTop (12);
    }

    auto top = controlsHidden ? r : r.removeFromTop (312);
    meterArea = top.removeFromRight (150);
    top.removeFromRight (12);
    eq.setBounds (top);
    if (controlsHidden) return;
    r.removeFromTop (12);

    tabArea = r.removeFromTop (34);
    auto tabs = tabArea;
    tabButtons[0].setBounds (tabs.removeFromLeft (150));
    tabButtons[1].setBounds (tabs.removeFromLeft (210));
    r.removeFromTop (12);

    const int rowH = (r.getHeight() - 12) / 2;
    auto row1 = r.removeFromTop (rowH);
    r.removeFromTop (12);
    auto row2 = r.removeFromTop (rowH);

    layoutRow (row1, { &secInput, &secDyn, &secDirt });
    layoutRow (row2, { &secNoise, &secTape, &secSquelch, &secOut });
    layoutRow (row1, { &secDevice, &secDistance, &secRoom, &secObstacle });
    layoutRow (row2, { &secTransmit, &secRoger, &secQuindar });
}

void RadioFXEditor::showTab (int tab)
{
    currentTab = juce::jlimit (0, (int) numTabs - 1, tab);
    tabButtons[(size_t) currentTab].setToggleState (true, juce::dontSendNotification);
    for (auto* s : allSections())
    {
        const bool show = s->tab == currentTab && ! controlsHidden;
        for (auto* c : s->cells) c->setVisible (show);
        if (s->enable != nullptr) s->enable->setVisible (show);
        s->helpButton->setVisible (show);
    }
    proc.apvts.state.setProperty ("uiTab", currentTab, nullptr);
    repaint();
}

void RadioFXEditor::setControlsHidden (bool hidden)
{
    controlsHidden = hidden;
    eqOnlyButton.setToggleState (hidden, juce::dontSendNotification);
    for (auto& b : tabButtons) b.setVisible (! hidden);
    proc.apvts.state.setProperty ("uiCollapsed", hidden, nullptr);
    showTab (currentTab);
    resized();
}

void RadioFXEditor::switchAB (int slot)
{
    proc.switchAB (slot);
    eq.clearSelection();
    aButton.setToggleState (proc.getCurrentAB() == 0, juce::dontSendNotification);
    bButton.setToggleState (proc.getCurrentAB() == 1, juce::dontSendNotification);
}

void RadioFXEditor::timerCallback()
{
    if (seenThemeVersion != Theme::version()) refreshTheme(); // otra ventana de CapCom cambio el tema
    routePhase += txOn ? 0.03f : 0.008f; // el pulso corre mas rapido mientras se transmite
    if (routePhase > 1.0f) routePhase -= 1.0f;
    repaint (routeArea.expanded (10, 8));
    eq.tick();

    const float l = proc.getOutputPeakL(), rr = proc.getOutputPeakR(), gr = proc.getGainReductionDb();
    const bool tx = proc.isTransmitting();
    if (std::abs (l - outL) > 1.0e-4f || std::abs (rr - outR) > 1.0e-4f || std::abs (gr - grLevel) > 0.05f || tx != txOn)
    {
        outL = l; outR = rr; grLevel = gr; txOn = tx;
        holdL = std::max (l, holdL * 0.985f); holdR = std::max (rr, holdR * 0.985f);
        repaint (meterArea);
    }

    // Secciones apagadas: sus controles se ven atenuados.
    for (auto* s : allSections())
        if (s->enable != nullptr)
        {
            const float alpha = s->enable->getToggleState() ? 1.0f : 0.4f;
            for (auto* c : s->cells)
                if (c->getAlpha() != alpha) c->setAlpha (alpha);
        }

    if (standalone)
    {
        playButton.setButtonText (proc.isPlaying() ? "Detener" : "Reproducir");
        timeLabel.setText (formatTime (proc.getPositionSeconds()) + " / " + formatTime (proc.getLengthSeconds()),
                           juce::dontSendNotification);
    }
}

//==============================================================================
void RadioFXEditor::rebuildPresetList()
{
    presetBox.clear (juce::dontSendNotification);
    presetBox.addSectionHeading (T ("Presets de fábrica"));
    for (int i = 0; i < proc.getNumPrograms(); ++i)
        presetBox.addItem (proc.getProgramName (i), i + 1);

    const auto user = proc.userPresetNames();
    if (! user.isEmpty())
    {
        presetBox.addSeparator();
        presetBox.addSectionHeading ("Mis presets");
        for (int i = 0; i < user.size(); ++i)
            presetBox.addItem (user[i], 1000 + i);
    }
}

void RadioFXEditor::presetChosen()
{
    const int id = presetBox.getSelectedId();
    if (id <= 0) return;
    if (id < 1000) proc.applyFactoryPreset (id - 1);
    else           proc.loadUserPreset (presetBox.getText());
}

void RadioFXEditor::stepPreset (int direction)
{
    const int n = presetBox.getNumItems();
    if (n == 0) return;
    int idx = presetBox.getSelectedItemIndex();
    idx = idx < 0 ? (direction > 0 ? 0 : n - 1) : (idx + direction + n) % n;
    presetBox.setSelectedItemIndex (idx, juce::sendNotificationSync);
}

void RadioFXEditor::showMoreMenu()
{
    juce::PopupMenu m;
    const bool a = proc.getCurrentAB() == 0;
    m.addItem (1, a ? T ("Copiar A → B") : T ("Copiar B → A"));
    m.addSeparator();
    m.addItem (2, "Deshacer", proc.undoManager.canUndo());
    m.addItem (3, "Rehacer", proc.undoManager.canRedo());
    m.addSeparator();
    m.addItem (4, Theme::isNight() ? "Tema Interplanetary" : "Modo noche");
    m.addItem (5, controlsHidden ? "Mostrar controles" : "Solo ecualizador");

    juce::Component::SafePointer<RadioFXEditor> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&moreButton), [safe] (int r)
    {
        if (safe == nullptr) return;
        switch (r)
        {
            case 1: safe->proc.copyABToOther(); break;
            case 2: safe->proc.undoManager.undo(); break;
            case 3: safe->proc.undoManager.redo(); break;
            case 4: Theme::setNight (! Theme::isNight()); safe->refreshTheme(); break;
            case 5: safe->setControlsHidden (! safe->controlsHidden); break;
            default: break;
        }
    });
}

void RadioFXEditor::showHelp (Section& s)
{
    auto& box = juce::CallOutBox::launchAsynchronously (std::make_unique<HelpBubble> (s.title, s.help),
                                                         s.helpButton->getScreenBounds(), nullptr);
    box.setLookAndFeel (&lnf);
    helpBox = &box;
}

void RadioFXEditor::savePresetDialog()
{
    auto* w = new juce::AlertWindow ("Guardar preset", "Nombre del preset:", juce::MessageBoxIconType::NoIcon);
    w->addTextEditor ("name", presetBox.getSelectedId() > 0 ? presetBox.getText() : "");
    w->addButton ("Guardar", 1, juce::KeyPress (juce::KeyPress::returnKey));
    w->addButton ("Cancelar", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    juce::Component::SafePointer<RadioFXEditor> safe (this);
    w->enterModalState (true, juce::ModalCallbackFunction::create ([safe, w] (int result)
    {
        if (result != 1 || safe == nullptr) return;
        const auto name = w->getTextEditorContents ("name").trim();
        if (safe->proc.saveUserPreset (name))
        {
            safe->rebuildPresetList();
            for (int i = 0; i < safe->presetBox.getNumItems(); ++i)
                if (safe->presetBox.getItemText (i) == juce::File::createLegalFileName (name))
                    safe->presetBox.setSelectedItemIndex (i, juce::dontSendNotification);
        }
    }), true);
}

//==============================================================================
void RadioFXEditor::openFile()
{
    chooser = std::make_unique<juce::FileChooser> ("Elige un audio", juce::File(), "*.wav;*.aif;*.aiff;*.mp3;*.m4a;*.flac;*.caf");
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [this] (const juce::FileChooser& fc)
                          {
                              if (fc.getResult().existsAsFile()) loadFile (fc.getResult());
                          });
}

void RadioFXEditor::loadFile (const juce::File& file)
{
    if (proc.loadAudioFile (file))
        fileLabel.setText (file.getFileName(), juce::dontSendNotification);
    else
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "CapCom",
                                                T ("No se pudo abrir ese archivo de audio."));
}

void RadioFXEditor::exportFile()
{
    const auto src = proc.getLoadedFile();
    if (! src.existsAsFile())
    {
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon, "CapCom", "Primero abre un audio.");
        return;
    }

    const auto suggested = src.getSiblingFile (src.getFileNameWithoutExtension() + "_radio.wav");
    chooser = std::make_unique<juce::FileChooser> ("Guardar WAV procesado", suggested, "*.wav");
    chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting,
                          [this] (const juce::FileChooser& fc)
                          {
                              auto dest = fc.getResult();
                              if (dest == juce::File()) return;
                              dest = dest.withFileExtension ("wav");
                              juce::String error;
                              juce::MouseCursor::showWaitCursor();
                              const bool ok = proc.exportProcessed (dest, error);
                              juce::MouseCursor::hideWaitCursor();
                              juce::AlertWindow::showMessageBoxAsync (ok ? juce::MessageBoxIconType::InfoIcon : juce::MessageBoxIconType::WarningIcon,
                                                                      "CapCom", ok ? "Exportado:\n" + dest.getFullPathName() : error);
                          });
}

bool RadioFXEditor::isInterestedInFileDrag (const juce::StringArray& files)
{
    if (! standalone) return false;
    for (const auto& f : files)
        if (juce::File (f).hasFileExtension ("wav;aif;aiff;mp3;m4a;flac;caf"))
            return true;
    return false;
}

void RadioFXEditor::filesDropped (const juce::StringArray& files, int, int)
{
    for (const auto& f : files)
        if (juce::File (f).hasFileExtension ("wav;aif;aiff;mp3;m4a;flac;caf"))
        {
            loadFile (juce::File (f));
            return;
        }
}
