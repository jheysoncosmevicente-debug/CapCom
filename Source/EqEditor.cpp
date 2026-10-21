#include "EqEditor.h"

using namespace Style;

namespace
{
    constexpr float analyzerMaxDb = 0.0f, analyzerMinDb = -90.0f;  // escala del analizador
    constexpr double curveSampleRate = 48000.0;
    constexpr float hitRadius = 11.0f;

    const std::array<juce::Colour, numEqBands> bandColours {{
        juce::Colour (0xff4fd1ff), juce::Colour (0xff9b7bff), juce::Colour (0xff5ad17a),
        juce::Colour (0xffff8a3d), juce::Colour (0xffffe14f)
    }};
}

//==============================================================================
// Panel flotante con los controles del punto elegido (como el de Pro-Q).
class EqEditor::BandPanel : public juce::Component
{
public:
    BandPanel (EqEditor& o, const Node& n) : owner (o), node (n)
    {
        using APVTS = juce::AudioProcessorValueTreeState;
        auto& apvts = owner.proc.apvts;

        title.setText (owner.nodeTitle (node), juce::dontSendNotification);
        title.setFont (font (12.5f, true));
        title.setColour (juce::Label::textColourId, owner.nodeColour (node));
        title.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (title);

        // X: minimiza el panel (asincrono, porque minimizar destruye este panel y este boton).
        close.setTooltip ("Ocultar el panel");
        close.onClick = [this]
        {
            juce::MessageManager::callAsync ([safe = juce::Component::SafePointer<EqEditor> (&owner)]
            {
                if (safe != nullptr) safe->setPanelMinimized (true);
            });
        };
        addAndMakeVisible (close);

        for (auto* b : { &prev, &next })
        {
            b->onClick = [this, b] { owner.stepSelection (b == &prev ? -1 : 1); };
            addAndMakeVisible (*b);
        }

        auto setupKnob = [&] (juce::Slider& s, juce::RangedAudioParameter* p, std::unique_ptr<APVTS::SliderAttachment>& att)
        {
            if (p == nullptr) return;
            s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
            s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 76, 16);
            s.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
            s.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
            s.setColour (juce::Slider::textBoxTextColourId, Palette::text);
            s.setDoubleClickReturnValue (true, p->convertFrom0to1 (p->getDefaultValue()));
            addAndMakeVisible (s);
            att = std::make_unique<APVTS::SliderAttachment> (apvts, p->paramID, s);
        };
        // Solo: escuchar solo la zona de esta banda (como el boton de auriculares de Pro-Q).
        if (node.kind == Kind::mid || node.kind == Kind::band)
        {
            solo.setClickingTogglesState (true);
            solo.setTooltip ("Escuchar solo esta banda");
            solo.setColour (juce::TextButton::buttonOnColourId, owner.nodeColour (node));
            solo.onClick = [this] { owner.setSoloFor (node, solo.getToggleState()); };
            addAndMakeVisible (solo);
        }

        setupKnob (freq, owner.freqParam (node), freqAtt);
        setupKnob (gain, owner.gainParam (node), gainAtt);
        setupKnob (q, owner.qParam (node), qAtt);

        if (node.kind == Kind::band)
        {
            shape.addItemList (bandTypeChoices, 1);
            addAndMakeVisible (shape);
            shapeAtt = std::make_unique<APVTS::ComboBoxAttachment> (apvts, bandParamID (node.band, "Type"), shape);

            bypass.setButtonText ("Bypass");
            addAndMakeVisible (bypass);
            bypassAtt = std::make_unique<APVTS::ButtonAttachment> (apvts, bandParamID (node.band, "Byp"), bypass);

            del.setTooltip ("Borrar banda");
            // Asincrono: borrar la banda destruye este panel (y este boton).
            del.onClick = [this]
            {
                juce::MessageManager::callAsync ([safe = juce::Component::SafePointer<EqEditor> (&owner), n2 = node]
                {
                    if (safe != nullptr) safe->deleteNode (n2);
                });
            };
            addAndMakeVisible (del);
        }
        else if (auto* slope = owner.slopeParam (node))
        {
            shape.addItemList (slopeChoices, 1);
            addAndMakeVisible (shape);
            shapeAtt = std::make_unique<APVTS::ComboBoxAttachment> (apvts, slope->paramID, shape);
        }

        setSize (450, 122); // mismo tamano para todos los puntos: el panel nunca cambia de lugar ni de forma
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (0.5f);
        g.setColour (Palette::popup);
        g.fillRoundedRectangle (r, 12.0f);
        g.setColour (Palette::cardBorder);
        g.drawRoundedRectangle (r, 12.0f, 1.0f);
        g.setColour (owner.nodeColour (node));
        g.fillRoundedRectangle (r.withHeight (3.0f).reduced (14.0f, 0).translated (0, 1), 1.5f); // franja del color del punto

        g.setColour (Palette::dim);
        g.setFont (font (10.5f, true));
        auto label = [&] (const juce::Component& c, const char* text)
        {
            if (c.isVisible())
                g.drawText (text, c.getBounds().withHeight (14).translated (0, -14), juce::Justification::centred);
        };
        label (freq, "FREQ");
        label (gain, "GAIN");
        label (q, "Q");
        label (shape, node.kind == Kind::band ? "FORMA" : (node.kind == Kind::mid ? "" : "PENDIENTE"));
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (8, 6);
        auto top = r.removeFromTop (22);
        prev.setBounds (top.removeFromLeft (24));
        close.setBounds (top.removeFromRight (24)); top.removeFromRight (8);
        next.setBounds (top.removeFromRight (24));
        if (del.isVisible())  { top.removeFromRight (6); del.setBounds (top.removeFromRight (60)); }
        if (solo.isVisible()) { top.removeFromRight (4); solo.setBounds (top.removeFromRight (50)); }
        title.setBounds (top);
        r.removeFromTop (16);

        // Contenido centrado: el panel mide lo mismo para todos los puntos, aunque tengan menos controles.
        int used = 0;
        if (bypass.isVisible()) used += 82;
        if (shape.isVisible())  used += 124;
        for (auto* k : { &freq, &gain, &q }) if (k->isVisible()) used += 74;
        r.removeFromLeft (juce::jmax (0, (r.getWidth() - used) / 2));

        if (bypass.isVisible()) { bypass.setBounds (r.removeFromLeft (78).withSizeKeepingCentre (78, 24)); r.removeFromLeft (4); }
        if (shape.isVisible())  { shape.setBounds (r.removeFromLeft (118).withSizeKeepingCentre (118, 26)); r.removeFromLeft (6); }
        for (auto* k : { &freq, &gain, &q })
            if (k->isVisible())
                k->setBounds (r.removeFromLeft (74));
    }

    Node node;

private:
    EqEditor& owner;
    juce::Label title;
    juce::TextButton prev { "<" }, next { ">" }, del { "Borrar" }, solo { "Solo" }, close { juce::String::fromUTF8 ("✕") };
    juce::ToggleButton bypass;
    juce::ComboBox shape;
    juce::Slider freq, gain, q;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> freqAtt, gainAtt, qAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> shapeAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAtt;
};

//==============================================================================
EqEditor::EqEditor (RadioFXProcessor& p) : proc (p)
{
    fftData.assign ((size_t) fftSize * 2, 0.0f);
    preDb.assign ((size_t) fftSize / 2 + 1, analyzerMinDb);
    postDb.assign ((size_t) fftSize / 2 + 1, analyzerMinDb);
    setWantsKeyboardFocus (true);

    undoButton.setButtonText ("Deshacer");
    redoButton.setButtonText ("Rehacer");
    undoButton.setTooltip ("Deshacer");
    redoButton.setTooltip ("Rehacer");
    undoButton.onClick = [this] { proc.undoManager.undo(); };
    redoButton.onClick = [this] { proc.undoManager.redo(); };

    eqTabButton.getProperties().set ("segment", "left");
    specTabButton.getProperties().set ("segment", "right");
    for (auto* b : { &eqTabButton, &specTabButton })
    {
        b->setClickingTogglesState (true);
        b->setRadioGroupId (9001);
        b->onClick = [this, b] { if (b->getToggleState()) setSpectrumOnly (b == &specTabButton); };
    }
    menuButton.setButtonText (T ("•••"));
    menuButton.setTooltip ("Opciones del ecualizador");
    menuButton.onClick = [this] { showEqMenu(); };

    analyzerBox.addItemList ({ "Analizador: entrada + salida", "Analizador: solo salida", "Analizador: apagado" }, 1);
    analyzerBox.onChange = [this]
    {
        analyzerMode = analyzerBox.getSelectedItemIndex();
        proc.apvts.state.setProperty ("uiAnalyzer", analyzerMode, nullptr);
    };
    scaleBox.addItemList ({ T ("Escala ±6 dB"), T ("Escala ±12 dB"), T ("Escala ±24 dB"), T ("Escala ±30 dB") }, 1);
    scaleBox.onChange = [this]
    {
        const float ranges[] = { 6.0f, 12.0f, 24.0f, 30.0f };
        displayRange = ranges[juce::jlimit (0, 3, scaleBox.getSelectedItemIndex())];
        proc.apvts.state.setProperty ("uiScale", displayRange, nullptr);
        placePanel();
    };

    analyzerMode = (int) proc.apvts.state.getProperty ("uiAnalyzer", 0);
    displayRange = (float) proc.apvts.state.getProperty ("uiScale", 24.0f);
    analyzerBox.setSelectedItemIndex (analyzerMode, juce::dontSendNotification);
    scaleBox.setSelectedItemIndex (displayRange <= 6 ? 0 : displayRange <= 12 ? 1 : displayRange <= 24 ? 2 : 3, juce::dontSendNotification);

    for (auto* c : std::initializer_list<juce::Component*> { &eqTabButton, &specTabButton, &undoButton, &redoButton, &menuButton, &analyzerBox, &scaleBox })
        addAndMakeVisible (c);
    panelMinimized = (bool) proc.apvts.state.getProperty ("uiPanelMin", false);
    showPanelButton.setButtonText (T ("▲"));
    showPanelButton.setTooltip ("Mostrar el panel del punto");
    showPanelButton.onClick = [this] { setPanelMinimized (false); };
    addChildComponent (showPanelButton);

    setSpectrumOnly ((bool) proc.apvts.state.getProperty ("uiSpectrum", false));
    refreshToolbar();
}

void EqEditor::setSpectrumOnly (bool on)
{
    spectrumOnly = on;
    (on ? specTabButton : eqTabButton).setToggleState (true, juce::dontSendNotification);
    proc.apvts.state.setProperty ("uiSpectrum", on, nullptr);
    if (on) select (std::nullopt);
    repaint();
}

void EqEditor::setPanelMinimized (bool minimized)
{
    if (panelAnimating) return; // ignora clics repetidos mientras anima
    auto apply = [this, minimized]
    {
        panelMinimized = minimized;
        proc.apvts.state.setProperty ("uiPanelMin", minimized, nullptr);
        auto keep = selected;
        panel.reset();
        selected.reset();
        select (keep); // vuelve a armar (o no) el panel del punto elegido; select() anima la entrada
    };

    if (minimized && panel != nullptr) animatePanel (false, apply); // primero se va, despues se minimiza
    else
    {
        showPanelButton.setVisible (false);
        apply();
    }
}

void EqEditor::animatePanel (bool in, std::function<void()> onDone)
{
    if (panel == nullptr) { if (onDone) onDone(); return; }
    if (panelAnim.has_value()) animUpdater.removeAnimator (*panelAnim);

    // Al entrar: sube 18 px, crece de 94 % a 100 % y aparece. Al salir: lo contrario, acelerando.
    auto setFrame = [this] (float v)
    {
        if (panel == nullptr) return;
        const auto b = panel->getBounds().toFloat();
        const float scale = 0.94f + 0.06f * v;
        panel->setTransform (juce::AffineTransform::scale (scale, scale, b.getCentreX(), b.getBottom())
                                 .translated (0.0f, 18.0f * (1.0f - v)));
        panel->setAlpha (v);
    };
    setFrame (in ? 0.0f : 1.0f);
    panelAnimating = ! in;

    panelAnim = juce::ValueAnimatorBuilder {}
        .withDurationMs (in ? 280.0 : 200.0)
        .withEasing (in ? juce::Easings::createCubicBezier (0.16f, 1.0f, 0.3f, 1.0f)    // ease-out: arranca rapido, frena suave
                        : juce::Easings::createCubicBezier (0.7f, 0.0f, 0.84f, 0.0f))   // ease-in: arranca suave, se va rapido
        .withValueChangedCallback ([setFrame, in] (float v) { setFrame (in ? v : 1.0f - v); })
        .withOnCompleteCallback ([this, in, onDone]
        {
            if (panel != nullptr && in) { panel->setTransform ({}); panel->setAlpha (1.0f); }
            panelAnimating = false;
            // Fuera del animador: onDone puede destruir el panel y arrancar otra animacion.
            if (onDone)
                juce::MessageManager::callAsync ([safe = juce::Component::SafePointer<EqEditor> (this), onDone]
                {
                    if (safe != nullptr) onDone();
                });
        })
        .build();
    animUpdater.addAnimator (*panelAnim);
    panelAnim->start();
}

void EqEditor::animateTabIn()
{
    if (tabAnim.has_value()) animUpdater.removeAnimator (*tabAnim);
    showPanelButton.setAlpha (0.0f);
    tabAnim = juce::ValueAnimatorBuilder {}
        .withDurationMs (240.0)
        .withEasing (juce::Easings::createCubicBezier (0.16f, 1.0f, 0.3f, 1.0f))
        .withValueChangedCallback ([this] (float v)
        {
            showPanelButton.setAlpha (v);
            showPanelButton.setTransform (juce::AffineTransform::translation (0.0f, 10.0f * (1.0f - v)));
        })
        .withOnCompleteCallback ([this] { showPanelButton.setTransform ({}); showPanelButton.setAlpha (1.0f); })
        .build();
    animUpdater.addAnimator (*tabAnim);
    tabAnim->start();
}

void EqEditor::themeChanged()
{
    // Rearma el panel con los colores nuevos, sin repetir la animacion de entrada.
    if (selected.has_value()) { auto keep = selected; panel.reset(); selected.reset(); select (keep); finishAnimations(); }
    repaint();
}

void EqEditor::showEqMenu()
{
    juce::PopupMenu m;
    m.addItem (1, "Quitar todas las bandas libres");
    m.addItem (2, T ("Ecualizador plano (sin cortes ni realce)"));
    m.addSeparator();
    m.addItem (3, "Ver los gestos del ecualizador");
    juce::Component::SafePointer<EqEditor> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&menuButton), [safe] (int r)
    {
        if (safe == nullptr || r == 0) return;
        safe->proc.undoManager.beginNewTransaction();
        if (r == 1 || r == 2)
        {
            safe->select (std::nullopt);
            for (int b = 0; b < numEqBands; ++b) safe->setNow (safe->param (bandParamID (b, "On")), 0);
        }
        if (r == 2)
        {
            safe->setNow (safe->param (ParamID::hpFreq), 20);
            safe->setNow (safe->param (ParamID::lpFreq), 20000);
            safe->setNow (safe->param (ParamID::midGain), 0);
        }
        if (r == 3)
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon, "Gestos del ecualizador",
                T ("Doble clic en el fondo: nueva banda\nArrastrar un punto: frecuencia y ganancia\nRueda del mouse: Q (en los cortes, la pendiente)\n"
                   "Shift: ajuste fino\nOption + arrastrar: un solo eje\nOption + clic: bypass de la banda\nCmd + Option + clic: cambiar la forma\n"
                   "Clic derecho: menú del punto\nSupr / Borrar: borrar la banda elegida"));
    });
}

void EqEditor::refreshToolbar()
{
    undoButton.setEnabled (proc.undoManager.canUndo());
    redoButton.setEnabled (proc.undoManager.canRedo());
}

void EqEditor::setSoloFor (const Node& n, bool on)
{
    if (! on) { proc.setSolo (false); return; }
    selected = n;
    proc.setSolo (true);
    updateSoloFromParams();
}

void EqEditor::updateSoloFromParams()
{
    if (! proc.isSoloOn() || ! selected.has_value()) return;
    const auto p = RadioParams::fromState (proc.apvts);
    if (selected->kind == Kind::mid)
        proc.setSolo (true, p.midFreq, juce::jmax (0.7f, p.midQ), 0);
    else if (selected->kind == Kind::band)
    {
        const auto& b = p.bands[(size_t) selected->band];
        proc.setSolo (true, b.freq, juce::jmax (0.7f, b.q), b.type == 1 ? 1 : (b.type == 2 ? 2 : 0));
    }
}

EqEditor::~EqEditor() = default;

void EqEditor::resized()
{
    auto r = getLocalBounds();
    toolbar = r.removeFromTop (48).reduced (14, 8);
    auto t = toolbar;
    eqTabButton.setBounds (t.removeFromLeft (70));
    specTabButton.setBounds (t.removeFromLeft (92)); t.removeFromLeft (12);
    undoButton.setBounds (t.removeFromLeft (92)); t.removeFromLeft (4);
    redoButton.setBounds (t.removeFromLeft (84));
    menuButton.setBounds (t.removeFromRight (40)); t.removeFromRight (8);
    scaleBox.setBounds (t.removeFromRight (150)); t.removeFromRight (8);
    analyzerBox.setBounds (t.removeFromRight (230));
    plot = r.toFloat().withTrimmedLeft (44).withTrimmedRight (42).withTrimmedTop (6).withTrimmedBottom (26);
    placePanel();
}

//------------------------------------------------------------------ geometria
float EqEditor::freqToX (float f) const
{
    return plot.getX() + (float) (std::log (juce::jmax (1.0f, f) / 20.0) / std::log (1000.0)) * plot.getWidth();
}

float EqEditor::xToFreq (float x) const
{
    return 20.0f * (float) std::pow (1000.0, (x - plot.getX()) / plot.getWidth());
}

float EqEditor::dbToY (float db) const   { return juce::jmap (db, displayRange, -displayRange, plot.getY(), plot.getBottom()); }
float EqEditor::yToDb (float y) const    { return juce::jmap (y, plot.getY(), plot.getBottom(), displayRange, -displayRange); }
float EqEditor::analyzerDbToY (float db) const
{
    return juce::jmap (juce::jlimit (analyzerMinDb, analyzerMaxDb, db), analyzerMaxDb, analyzerMinDb, plot.getY(), plot.getBottom());
}

//------------------------------------------------------------------ parametros
juce::RangedAudioParameter* EqEditor::param (const juce::String& id) const { return proc.apvts.getParameter (id); }

juce::RangedAudioParameter* EqEditor::freqParam (const Node& n) const
{
    switch (n.kind)
    {
        case Kind::lowCut:  return param (ParamID::hpFreq);
        case Kind::highCut: return param (ParamID::lpFreq);
        case Kind::mid:     return param (ParamID::midFreq);
        default:            return param (bandParamID (n.band, "Freq"));
    }
}

juce::RangedAudioParameter* EqEditor::gainParam (const Node& n) const
{
    if (n.kind == Kind::mid)  return param (ParamID::midGain);
    if (n.kind == Kind::band) return param (bandParamID (n.band, "Gain"));
    return nullptr;
}

juce::RangedAudioParameter* EqEditor::qParam (const Node& n) const
{
    if (n.kind == Kind::mid)  return param (ParamID::midQ);
    if (n.kind == Kind::band) return param (bandParamID (n.band, "Q"));
    return nullptr;
}

juce::RangedAudioParameter* EqEditor::slopeParam (const Node& n) const
{
    if (n.kind == Kind::lowCut)  return param (ParamID::hpSlope);
    if (n.kind == Kind::highCut) return param (ParamID::lpSlope);
    return nullptr;
}

float EqEditor::valueOf (juce::RangedAudioParameter* p) const
{
    return p != nullptr ? p->convertFrom0to1 (p->getValue()) : 0.0f;
}

void EqEditor::setNow (juce::RangedAudioParameter* p, float value)
{
    if (p == nullptr) return;
    p->beginChangeGesture();
    p->setValueNotifyingHost (p->convertTo0to1 (value));
    p->endChangeGesture();
}

//------------------------------------------------------------------ puntos
std::vector<EqEditor::Node> EqEditor::existingNodes (const RadioParams& p) const
{
    std::vector<Node> v { { Kind::lowCut, -1 }, { Kind::mid, -1 } };
    for (int b = 0; b < numEqBands; ++b)
        if (p.bands[(size_t) b].on) v.push_back ({ Kind::band, b });
    v.push_back ({ Kind::highCut, -1 });
    return v;
}

bool EqEditor::hasGain (const Node& n, const RadioParams& p) const
{
    if (n.kind == Kind::mid) return true;
    return n.kind == Kind::band && p.bands[(size_t) n.band].type != 3; // la muesca no tiene ganancia
}

juce::Point<float> EqEditor::nodePosition (const Node& n, const RadioParams& p) const
{
    switch (n.kind)
    {
        case Kind::lowCut:  return { freqToX (p.hpFreq), dbToY (0) };
        case Kind::highCut: return { freqToX (p.lpFreq), dbToY (0) };
        case Kind::mid:     return { freqToX (p.midFreq), dbToY (p.midGainDb) };
        default:
        {
            const auto& b = p.bands[(size_t) n.band];
            return { freqToX (b.freq), dbToY (hasGain (n, p) ? b.gainDb : 0.0f) };
        }
    }
}

std::optional<EqEditor::Node> EqEditor::nodeAt (juce::Point<float> pos) const
{
    const auto p = RadioParams::fromState (proc.apvts);
    std::optional<Node> best;
    float bestDist = hitRadius;
    for (const auto& n : existingNodes (p))
    {
        const float d = nodePosition (n, p).getDistanceFrom (pos);
        if (d <= bestDist) { bestDist = d; best = n; }
    }
    return best;
}

juce::Colour EqEditor::nodeColour (const Node& n) const
{
    switch (n.kind)
    {
        case Kind::mid:  return Palette::accent;
        case Kind::band: return bandColours[(size_t) n.band];
        default:         return Palette::accent;
    }
}

juce::String EqEditor::nodeTitle (const Node& n) const
{
    switch (n.kind)
    {
        case Kind::lowCut:  return "Corte graves";
        case Kind::highCut: return "Corte agudos";
        case Kind::mid:     return "Medios (realce)";
        default:            return "Banda " + juce::String (n.band + 1);
    }
}

juce::String EqEditor::nodeReadout (const Node& n, const RadioParams& p) const
{
    auto gainText = [] (float db) { return (db > 0.05f ? "+" : "") + juce::String (db, 1) + " dB"; };
    switch (n.kind)
    {
        case Kind::lowCut:  return nodeTitle (n) + T (" · ") + slopeChoices[(p.hpOrder / 2) - 1] + "\n" + formatHz (p.hpFreq);
        case Kind::highCut: return nodeTitle (n) + T (" · ") + slopeChoices[(p.lpOrder / 2) - 1] + "\n" + formatHz (p.lpFreq);
        case Kind::mid:     return nodeTitle (n) + "\n" + formatHz (p.midFreq) + "   " + gainText (p.midGainDb) + "   Q " + juce::String (p.midQ, 2);
        default:
        {
            const auto& b = p.bands[(size_t) n.band];
            auto t = nodeTitle (n) + T (" · ") + bandTypeChoices[b.type] + (b.bypass ? " (bypass)" : "") + "\n" + formatHz (b.freq);
            if (hasGain (n, p)) t += "   " + gainText (b.gainDb);
            return t + "   Q " + juce::String (b.q, 2);
        }
    }
}

//------------------------------------------------------------------ acciones
void EqEditor::select (std::optional<Node> n)
{
    const bool hadPanel = panel != nullptr;
    if (n != selected) proc.setSolo (false); // el solo es de la banda elegida
    selected = n;
    panel.reset();
    if (n.has_value())
    {
        if (! panelMinimized)
        {
            panel = std::make_unique<BandPanel> (*this, *n);
            addAndMakeVisible (*panel);
        }
    }
    const bool showTab = n.has_value() && panelMinimized && ! spectrumOnly;
    if (showTab && ! showPanelButton.isVisible()) { showPanelButton.setVisible (true); placePanel(); animateTabIn(); }
    else if (! showTab) showPanelButton.setVisible (false);
    placePanel();
    if (panel != nullptr && ! hadPanel) animatePanel (true); // aparece con animacion la primera vez
    tick();
    repaint();
}

void EqEditor::selectMidForPreview()
{
    select (Node { Kind::mid, -1 });
    finishAnimations(); // en la captura no corre el refresco de pantalla
}

void EqEditor::finishAnimations()
{
    for (auto* a : { &panelAnim, &tabAnim })
        if (a->has_value()) { animUpdater.removeAnimator (**a); a->reset(); }
    panelAnimating = false;
    if (panel != nullptr) { panel->setTransform ({}); panel->setAlpha (1.0f); }
    showPanelButton.setTransform ({});
    showPanelButton.setAlpha (1.0f);
}

void EqEditor::createBandAt (juce::Point<float> pos)
{
    const auto p = RadioParams::fromState (proc.apvts);
    for (int b = 0; b < numEqBands; ++b)
    {
        if (p.bands[(size_t) b].on) continue;
        setNow (param (bandParamID (b, "Type")), 0);
        setNow (param (bandParamID (b, "Byp")), 0);
        setNow (param (bandParamID (b, "Freq")), juce::jlimit (20.0f, 20000.0f, xToFreq (pos.x)));
        setNow (param (bandParamID (b, "Gain")), juce::jlimit (-24.0f, 24.0f, yToDb (pos.y)));
        setNow (param (bandParamID (b, "Q")), 1.0f);
        setNow (param (bandParamID (b, "On")), 1);
        select (Node { Kind::band, b });
        return;
    }
}

void EqEditor::deleteNode (const Node& n)
{
    if (n.kind == Kind::band)
    {
        if (selected == n) select (std::nullopt);
        setNow (param (bandParamID (n.band, "On")), 0);
    }
    else if (n.kind == Kind::mid)    setNow (param (ParamID::midGain), 0);
    else if (n.kind == Kind::lowCut) setNow (param (ParamID::hpFreq), 20);
    else                             setNow (param (ParamID::lpFreq), 20000);
    repaint();
}

void EqEditor::toggleBypass (const Node& n)
{
    if (n.kind != Kind::band) return;
    auto* p = param (bandParamID (n.band, "Byp"));
    setNow (p, valueOf (p) > 0.5f ? 0.0f : 1.0f);
}

void EqEditor::cycleShape (const Node& n)
{
    if (n.kind == Kind::band)
    {
        auto* p = param (bandParamID (n.band, "Type"));
        setNow (p, (float) (((int) valueOf (p) + 1) % bandTypeChoices.size()));
    }
    else if (auto* s = slopeParam (n))
    {
        setNow (s, (float) (((int) valueOf (s) + 1) % slopeChoices.size()));
    }
}

void EqEditor::showNodeMenu (const Node& n)
{
    juce::PopupMenu m;
    m.addSectionHeader (nodeTitle (n));
    if (n.kind == Kind::band)
    {
        const int type = (int) valueOf (param (bandParamID (n.band, "Type")));
        for (int i = 0; i < bandTypeChoices.size(); ++i)
            m.addItem (100 + i, bandTypeChoices[i], true, i == type);
        m.addSeparator();
        m.addItem (1, "Bypass", true, valueOf (param (bandParamID (n.band, "Byp"))) > 0.5f);
        m.addItem (2, "Borrar banda");
    }
    else if (auto* s = slopeParam (n))
    {
        const int cur = (int) valueOf (s);
        for (int i = 0; i < slopeChoices.size(); ++i)
            m.addItem (200 + i, slopeChoices[i], true, i == cur);
        m.addSeparator();
        m.addItem (2, n.kind == Kind::lowCut ? "Apagar (20 Hz)" : "Apagar (20 kHz)");
    }
    else
    {
        m.addItem (2, "Realce a 0 dB");
    }

    juce::Component::SafePointer<EqEditor> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [safe, n] (int r)
    {
        if (safe == nullptr || r == 0) return;
        if (r == 1) safe->toggleBypass (n);
        else if (r == 2) safe->deleteNode (n);
        else if (r >= 200) safe->setNow (safe->slopeParam (n), (float) (r - 200));
        else if (r >= 100) safe->setNow (safe->param (bandParamID (n.band, "Type")), (float) (r - 100));
    });
}

void EqEditor::stepSelection (int direction)
{
    const auto nodes = existingNodes (RadioParams::fromState (proc.apvts));
    if (nodes.empty()) return;
    int idx = 0;
    for (int i = 0; i < (int) nodes.size(); ++i)
        if (selected.has_value() && nodes[(size_t) i] == *selected) idx = i;
    idx = (idx + direction + (int) nodes.size()) % (int) nodes.size();
    juce::MessageManager::callAsync ([safe = juce::Component::SafePointer<EqEditor> (this), n = nodes[(size_t) idx]]
    {
        if (safe != nullptr) safe->select (n); // asincrono: el boton que llama pertenece al panel que se reemplaza
    });
}

//------------------------------------------------------------------ mouse
void EqEditor::mouseMove (const juce::MouseEvent& e)
{
    if (spectrumOnly) { cursor.reset(); hovered.reset(); return; }
    cursor = plot.contains (e.position) ? std::optional<juce::Point<float>> (e.position) : std::nullopt;
    const auto h = nodeAt (e.position);
    if (h != hovered) { hovered = h; repaint(); }
    setMouseCursor (h.has_value() ? juce::MouseCursor::DraggingHandCursor : juce::MouseCursor::NormalCursor);
}

void EqEditor::mouseExit (const juce::MouseEvent&)
{
    cursor.reset();
    if (hovered.has_value()) { hovered.reset(); repaint(); }
}

void EqEditor::mouseDown (const juce::MouseEvent& e)
{
    if (spectrumOnly) return; // en "Espectro" no se editan puntos
    grabKeyboardFocus();
    const auto n = nodeAt (e.position);

    if (e.mods.isPopupMenu())
    {
        if (n.has_value()) { select (n); showNodeMenu (*n); }
        return;
    }

    if (! n.has_value())
    {
        select (std::nullopt); // un clic en el fondo deselecciona; doble clic crea
        return;
    }

    if (e.mods.isAltDown() && e.mods.isCommandDown()) { cycleShape (*n); select (n); return; }

    if (selected != n) select (n);
    dragging = n;
    dragStartMouse = e.position;
    const auto p = RadioParams::fromState (proc.apvts);
    const auto pos = nodePosition (*n, p);
    dragStartFreqX = pos.x;
    dragStartGainY = pos.y;
    if (auto* f = freqParam (*n)) f->beginChangeGesture();
    if (auto* g = gainParam (*n)) g->beginChangeGesture();
}

void EqEditor::mouseDrag (const juce::MouseEvent& e)
{
    if (spectrumOnly) return; // en "Espectro" no se editan puntos
    if (! dragging.has_value()) return;
    const float fine = e.mods.isShiftDown() ? 0.15f : 1.0f;
    auto delta = (e.position - dragStartMouse) * fine;

    // Option + arrastrar: solo un eje (el que mas se movio), como en Pro-Q.
    if (e.mods.isAltDown())
    {
        if (std::abs (delta.x) > std::abs (delta.y)) delta.y = 0; else delta.x = 0;
    }

    if (auto* f = freqParam (*dragging))
        f->setValueNotifyingHost (f->convertTo0to1 (juce::jlimit (10.0f, 22000.0f, xToFreq (dragStartFreqX + delta.x))));

    const auto p = RadioParams::fromState (proc.apvts);
    if (hasGain (*dragging, p))
        if (auto* g = gainParam (*dragging))
            g->setValueNotifyingHost (g->convertTo0to1 (yToDb (dragStartGainY + delta.y)));
}

void EqEditor::mouseUp (const juce::MouseEvent& e)
{
    if (! dragging.has_value()) return;
    if (auto* f = freqParam (*dragging)) f->endChangeGesture();
    if (auto* g = gainParam (*dragging)) g->endChangeGesture();

    // Option + clic sin arrastrar: bypass de la banda.
    if (e.mods.isAltDown() && ! e.mods.isCommandDown() && e.getDistanceFromDragStart() < 3)
        toggleBypass (*dragging);
    dragging.reset();
    placePanel();
}

void EqEditor::mouseDoubleClick (const juce::MouseEvent& e)
{
    if (spectrumOnly) return; // en "Espectro" no se editan puntos
    // Como en Pro-Q, el doble clic solo crea bandas: sobre un punto no hace nada (asi no se borra sin querer).
    if (! nodeAt (e.position).has_value() && plot.contains (e.position))
        createBandAt (e.position);
}

void EqEditor::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    if (spectrumOnly) return; // en "Espectro" no se editan puntos
    auto target = nodeAt (e.position);
    if (! target.has_value()) target = selected;
    if (! target.has_value()) return;

    const float fine = e.mods.isShiftDown() ? 0.25f : 1.0f;
    if (auto* q = qParam (*target))
    {
        setNow (q, juce::jlimit (0.1f, 10.0f, valueOf (q) * std::exp (w.deltaY * 3.0f * fine)));
    }
    else if (auto* s = slopeParam (*target))
    {
        wheelAccum += w.deltaY;
        if (std::abs (wheelAccum) > 0.12f)
        {
            setNow (s, juce::jlimit (0.0f, (float) slopeChoices.size() - 1, valueOf (s) + (wheelAccum > 0 ? 1.0f : -1.0f)));
            wheelAccum = 0;
        }
    }
}

bool EqEditor::keyPressed (const juce::KeyPress& k)
{
    if ((k == juce::KeyPress::deleteKey || k == juce::KeyPress::backspaceKey) && selected.has_value() && selected->kind == Kind::band)
    {
        deleteNode (*selected);
        return true;
    }
    return false;
}

//------------------------------------------------------------------ analizador
void EqEditor::updateSpectrum (const ScopeTap& tap, std::vector<float>& smoothed)
{
    std::fill (fftData.begin(), fftData.end(), 0.0f);
    tap.copyLatest (fftData.data(), fftSize);
    window.multiplyWithWindowingTable (fftData.data(), (size_t) fftSize);
    fft.performFrequencyOnlyForwardTransform (fftData.data());

    for (size_t i = 0; i < smoothed.size(); ++i)
    {
        const float db = juce::Decibels::gainToDecibels (fftData[i] / ((float) fftSize * 0.25f), analyzerMinDb - 10.0f);
        smoothed[i] = std::max (db, smoothed[i] - 1.2f); // sube al instante, cae ~36 dB/s
    }
}

juce::Path EqEditor::spectrumPath (const std::vector<float>& db, bool closed) const
{
    const double sr = proc.getSampleRate() > 0 ? proc.getSampleRate() : 48000.0;
    juce::Path path;
    bool started = false;
    for (float x = plot.getX(); x <= plot.getRight(); x += 1.5f)
    {
        const float f0 = xToFreq (x), f1 = xToFreq (x + 1.5f);
        const float b0 = f0 / (float) sr * fftSize, b1 = f1 / (float) sr * fftSize;
        float v = analyzerMinDb - 10.0f;
        if (b1 - b0 < 1.0f)
        {
            // Graves: menos de una barra por pixel; se interpola para que la linea no quede escalonada.
            const int i = juce::jlimit (1, (int) db.size() - 2, (int) b0);
            const float frac = juce::jlimit (0.0f, 1.0f, b0 - (float) i);
            v = db[(size_t) i] + (db[(size_t) i + 1] - db[(size_t) i]) * frac;
        }
        else
        {
            const int i0 = juce::jlimit (1, (int) db.size() - 1, (int) b0);
            const int i1 = juce::jlimit (i0, (int) db.size() - 1, (int) b1);
            for (int i = i0; i <= i1; ++i) v = std::max (v, db[(size_t) i]);
        }
        if (v > analyzerMinDb - 5.0f) // el silencio queda abajo; la inclinacion solo se aplica a senal real
            v += 3.0f * std::log2 (juce::jmax (20.0f, f0) / 1000.0f); // inclinacion de 3 dB/oct: la voz se ve pareja
        const float y = analyzerDbToY (v);
        if (! started) { path.startNewSubPath (x, closed ? plot.getBottom() : y); if (closed) path.lineTo (x, y); started = true; }
        else path.lineTo (x, y);
    }
    if (closed && started)
    {
        path.lineTo (plot.getRight(), plot.getBottom());
        path.closeSubPath();
    }
    return path;
}

void EqEditor::tick()
{
    if (analyzerMode == 0) updateSpectrum (proc.getPreTap(), preDb);
    if (analyzerMode <= 1 || spectrumOnly)
    {
        updateSpectrum (proc.getPostTap(), postDb);
        if (++trailTick % 3 == 0) // cada ~100 ms se guarda un cuadro para la estela
        {
            postTrail.push_front (postDb);
            if (postTrail.size() > 6) postTrail.pop_back();
        }
    }
    refreshToolbar();
    updateSoloFromParams();

    // Si la banda elegida se borro (desde el host o con deshacer), se cierra su panel.
    if (panel != nullptr && selected.has_value() && selected->kind == Kind::band
        && ! RadioParams::fromState (proc.apvts).bands[(size_t) selected->band].on)
    {
        select (std::nullopt);
        return;
    }
    repaint();
}

void EqEditor::placePanel()
{
    // Panel fijo: siempre abajo al centro del grafico, sea cual sea el punto elegido.
    showPanelButton.setBounds (juce::Rectangle<int> (46, 22).withCentre ({ (int) plot.getCentreX(), (int) plot.getBottom() - 13 }));
    if (panel == nullptr) return;
    auto b = panel->getBounds();
    b.setPosition ((int) plot.getCentreX() - b.getWidth() / 2, (int) plot.getBottom() - b.getHeight() - 6);
    panel->setBounds (b);
}

//------------------------------------------------------------------ dibujo
void EqEditor::paint (juce::Graphics& g)
{
    using namespace juce;
    // La tarjeta (fondo y sombra) la dibuja la ventana principal; aqui va el area del grafico.
    g.setColour (Palette::plotBg);
    g.fillRoundedRectangle (plot.expanded (6, 4), 8.0f);

    // Cuadricula
    // Lineas secundarias (cada decada 20, 30... 90) muy tenues; principales un poco mas marcadas
    for (float decade : { 10.0f, 100.0f, 1000.0f, 10000.0f })
        for (int k = 2; k <= 9; ++k)
        {
            const float f = decade * (float) k;
            if (f < 20.0f || f > 20000.0f) continue;
            g.setColour (k == 2 || k == 5 ? Palette::line : Palette::gridMinor);
            g.drawVerticalLine ((int) freqToX (f), plot.getY(), plot.getBottom());
        }
    g.setFont (Theme::isDark() ? monoFont (10.0f) : font (10.0f));
    for (double f : { 20.0, 50.0, 100.0, 200.0, 500.0, 1000.0, 2000.0, 5000.0, 10000.0, 20000.0 })
    {
        const float x = freqToX ((float) f);
        if (f == 100.0 || f == 1000.0 || f == 10000.0)
        {
            g.setColour (Palette::line.brighter (Theme::isDark() ? 0.15f : -0.05f));
            g.drawVerticalLine ((int) x, plot.getY(), plot.getBottom());
        }
        g.setColour (Palette::dim);
        g.drawText (f >= 1000 ? String ((int) (f / 1000)) + "k" : String ((int) f), (int) x - 15, (int) plot.getBottom() + 5, 30, 12, Justification::centred);
    }
    for (float db : { displayRange, displayRange / 2, 0.0f, -displayRange / 2, -displayRange })
    {
        const float y = dbToY (db);
        g.setColour (db == 0.0f ? Palette::dim.withAlpha (0.4f) : Palette::line);
        g.drawHorizontalLine ((int) y, plot.getX(), plot.getRight());
        g.setColour (Palette::dim);
        g.drawText ((db > 0 ? "+" : "") + String ((int) db), (int) plot.getX() - 42, (int) y - 6, 34, 12, Justification::centredRight);
    }
    g.setColour (Palette::dim.withAlpha (0.6f));
    for (float db : { -30.0f, -60.0f })
        g.drawText (String ((int) db), (int) plot.getRight() + 2, (int) analyzerDbToY (db) - 6, 32, 12, Justification::centredLeft);

    // Analizador: entrada (linea gris) y salida (relleno)
    if (analyzerMode <= 1 || spectrumOnly)
    {
        const auto tone = spectrumOnly ? Palette::accent : Palette::analyzer;
        // Estelas: cuadros anteriores cada vez mas transparentes (el "humo" de FabFilter)
        for (size_t i = 0; i < postTrail.size(); ++i)
        {
            const float a = (Theme::isDark() ? 0.16f : 0.12f) * (1.0f - (float) i / (float) (postTrail.size() + 1));
            g.setColour (tone.withAlpha (a));
            g.strokePath (spectrumPath (postTrail[i], false), PathStrokeType (1.6f + (float) i * 0.8f));
        }
        g.setGradientFill (ColourGradient (tone.withAlpha (spectrumOnly ? 0.28f : (Theme::isDark() ? 0.22f : 0.30f)), 0, plot.getY() + plot.getHeight() * 0.3f,
                                           tone.withAlpha (0.0f), 0, plot.getBottom(), false));
        g.fillPath (spectrumPath (postDb, true));
        g.setColour (tone.withAlpha (spectrumOnly ? 0.95f : 0.65f));
        g.strokePath (spectrumPath (postDb, false), PathStrokeType (1.0f));
    }
    if (analyzerMode == 0 || spectrumOnly)
    {
        g.setColour (Theme::isDark() ? Palette::accent2.withAlpha (0.5f) : Palette::dim.withAlpha (0.45f));
        g.strokePath (spectrumPath (preDb, false), PathStrokeType (1.0f));
    }
    if (spectrumOnly)
    {
        g.setColour (Palette::dim);
        g.setFont (font (11.5f));
        g.drawText (T ("Espectro en vivo   ·   relleno azul: salida   ·   línea gris: entrada"),
                    plot.toNearestInt().removeFromTop (16).withTrimmedLeft (8), Justification::centredLeft);
        return;
    }

    const auto p = RadioParams::fromState (proc.apvts);
    const auto nodes = existingNodes (p);
    const float y0 = dbToY (0);

    // Relleno de cada banda con ganancia (como en Pro-Q)
    for (const auto& n : nodes)
    {
        if (! hasGain (n, p) || (n.kind == Kind::band && p.bands[(size_t) n.band].bypass)) continue;
        auto coefs = n.kind == Kind::mid
            ? juce::dsp::IIR::Coefficients<float>::makePeakFilter (curveSampleRate, p.midFreq, p.midQ, Decibels::decibelsToGain (p.midGainDb))
            : RadioEngine::bandCoefs (p.bands[(size_t) n.band], curveSampleRate);
        Path band;
        band.startNewSubPath (plot.getX(), y0);
        for (float x = plot.getX(); x <= plot.getRight(); x += 2.0f)
        {
            const float db = (float) Decibels::gainToDecibels (coefs->getMagnitudeForFrequency (xToFreq (x), curveSampleRate), -60.0);
            band.lineTo (x, dbToY (jlimit (-displayRange, displayRange, db)));
        }
        band.lineTo (plot.getRight(), y0);
        band.closeSubPath();
        const bool focus = selected == n || hovered == n;
        const auto bc = nodeColour (n);
        g.setGradientFill (ColourGradient (bc.withAlpha (focus ? 0.28f : 0.13f), 0, plot.getY(), bc.withAlpha (focus ? 0.12f : 0.04f), 0, y0, false));
        g.fillPath (band);
    }

    // Curva total de la senal de radio (y la escena, punteada)
    const bool withScene = SceneEngine::isActive (p);
    Path total, scene;
    for (float x = plot.getX(); x <= plot.getRight(); x += 2.0f)
    {
        const float f = xToFreq (x);
        const float db = RadioEngine::responseDb (p, f, curveSampleRate);
        const float y = dbToY (jlimit (-displayRange - 6.0f, displayRange, db));
        if (x == plot.getX()) total.startNewSubPath (x, y); else total.lineTo (x, y);
        if (withScene)
        {
            const float ys = dbToY (jlimit (-displayRange - 6.0f, displayRange, db + SceneEngine::responseDb (p, f, curveSampleRate)));
            if (x == plot.getX()) scene.startNewSubPath (x, ys); else scene.lineTo (x, ys);
        }
    }
    {
        Graphics::ScopedSaveState clip (g);
        g.reduceClipRegion (plot.toNearestInt());
        if (withScene)
        {
            Path dashed;
            const float dashes[] = { 3.0f, 4.0f };
            PathStrokeType (1.2f).createDashedStroke (dashed, scene, dashes, 2);
            g.setColour (Palette::accent2.withAlpha (0.8f));
            g.fillPath (dashed);
        }
        Path area (total);
        area.lineTo (plot.getRight(), y0);
        area.lineTo (plot.getX(), y0);
        area.closeSubPath();
        g.setGradientFill (ColourGradient (Palette::accent.withAlpha (0.10f), 0, plot.getY(), Palette::accent.withAlpha (0.02f), 0, plot.getBottom(), false));
        g.fillPath (area);
        Style::glowStroke (g, total, Palette::accent, 1.7f, 1.3f);
    }

    // Puntos
    for (const auto& n : nodes)
    {
        auto pos = nodePosition (n, p);
        pos.y = jlimit (plot.getY(), plot.getBottom(), pos.y); // con escala chica, el punto queda en el borde
        const bool sel = selected == n, hov = hovered == n || dragging == n;
        const bool byp = n.kind == Kind::band && p.bands[(size_t) n.band].bypass;
        const float rad = sel || hov ? 6.5f : 5.5f;
        auto c = byp ? Palette::dim : nodeColour (n);

        if (Theme::isDark())
        {
            if (sel)
            {
                // Guia punteada hasta el eje, orbita inclinada y el punto como Marte
                Path guide;
                guide.startNewSubPath (pos.x, pos.y + rad + 4);
                guide.lineTo (pos.x, plot.getBottom());
                Path dashedGuide;
                const float dg[] = { 2.0f, 3.0f };
                PathStrokeType (1.0f).createDashedStroke (dashedGuide, guide, dg, 2);
                g.setColour (Palette::mars.withAlpha (0.6f));
                g.fillPath (dashedGuide);
                Path orbitRing;
                orbitRing.addCentredArc (pos.x, pos.y, rad * 2.6f, rad * 0.95f, -0.3f, 0, MathConstants<float>::twoPi, true);
                g.setColour (Palette::mars.withAlpha (0.75f));
                g.strokePath (orbitRing, PathStrokeType (1.0f));
                drawMars (g, pos, rad + 1.0f);
                g.setColour (Palette::mars);
                g.setFont (mediumFont (11.5f));
                g.drawText ((n.kind == Kind::band ? String (n.band + 1) + T (" · ") : String()) + nodeTitle (n).upToFirstOccurrenceOf (" (", false, false),
                            Rectangle<float> (160, 14).withPosition (pos.x + rad + 8, pos.y - rad - 18), Justification::centredLeft);
            }
            else
            {
                const auto ring = byp ? Palette::dim : Palette::accent2;
                g.setGradientFill (ColourGradient (ring.withAlpha (hov ? 0.4f : 0.22f), pos.x, pos.y, ring.withAlpha (0.0f), pos.x + rad * 3.2f, pos.y, true));
                g.fillEllipse (Rectangle<float> (rad * 6.4f, rad * 6.4f).withCentre (pos));
                g.setColour (Palette::nodeFill);
                g.fillEllipse (Rectangle<float> (rad * 2, rad * 2).withCentre (pos));
                g.setColour (ring);
                g.drawEllipse (Rectangle<float> (rad * 2, rad * 2).withCentre (pos), 2.0f);
                if (n.kind == Kind::band)
                {
                    g.setColour (Palette::text.withAlpha (0.85f));
                    g.setFont (monoFont (10.0f));
                    g.drawText (String (n.band + 1), Rectangle<float> (14, 12).withPosition (pos.x + rad + 2, pos.y - rad - 12), Justification::centredLeft);
                }
            }
            continue;
        }

        // Tema claro: punto de color con halo de luz (mas intenso si esta elegido o bajo el mouse)
        const float haloR = rad * (sel ? 4.2f : (hov ? 3.6f : 3.0f));
        g.setGradientFill (ColourGradient (c.withAlpha (byp ? 0.0f : (sel ? 0.45f : 0.25f)), pos.x, pos.y,
                                           c.withAlpha (0.0f), pos.x + haloR, pos.y, true));
        g.fillEllipse (Rectangle<float> (haloR * 2, haloR * 2).withCentre (pos));

        Path dotPath;
        dotPath.addEllipse (Rectangle<float> (rad * 2, rad * 2).withCentre (pos));
        g.setColour (byp ? Palette::plotBg : c);
        g.fillPath (dotPath);
        g.setColour (byp ? Palette::dim : c.brighter (0.5f));
        g.strokePath (dotPath, PathStrokeType (1.0f));
        if (sel)
        {
            g.setColour (Palette::text.withAlpha (0.85f));
            g.drawEllipse (Rectangle<float> (rad * 2 + 7, rad * 2 + 7).withCentre (pos), 1.0f);
        }
        if (n.kind == Kind::band)
        {
            g.setColour (byp ? Palette::dim : c);
            g.setFont (font (10.0f, true));
            g.drawText (String (n.band + 1), Rectangle<float> (14, 12).withPosition (pos.x + rad + 2, pos.y - rad - 12), Justification::centredLeft);
        }
    }

    // Lectura del punto bajo el mouse (o el que se arrastra)
    const auto info = dragging.has_value() ? dragging : hovered;
    if (info.has_value())
    {
        const auto pos = nodePosition (*info, p);
        const auto text = nodeReadout (*info, p);
        AttributedString as;
        as.append (text.upToFirstOccurrenceOf ("\n", false, false) + "\n", font (11.5f, true), nodeColour (*info));
        as.append (text.fromFirstOccurrenceOf ("\n", false, false), font (12.0f), Palette::text);
        TextLayout tl;
        tl.createLayout (as, 260.0f);
        auto box = Rectangle<float> (tl.getWidth() + 16, tl.getHeight() + 10);
        box.setCentre (pos.x, pos.y - 34 - box.getHeight() / 2);
        box = box.constrainedWithin (getLocalBounds().toFloat().reduced (4));
        g.setColour (Palette::popup);
        g.fillRoundedRectangle (box, 8.0f);
        g.setColour (Palette::cardBorder);
        g.drawRoundedRectangle (box, 8.0f, 1.0f);
        tl.draw (g, box.reduced (8, 5));
    }

    // Titulo y ayuda
    g.setColour (Palette::dim);
    if (withScene)
    {
        g.setFont (font (10.5f));
        g.drawText (T ("punteada: con la escena"), plot.toNearestInt().removeFromTop (14).withTrimmedLeft (6), Justification::centredLeft);
    }
    if (proc.isSoloOn())
    {
        g.setColour (Palette::accent);
        g.setFont (font (11.0f, true));
        g.drawText (T ("SOLO: escuchando solo la banda elegida"), plot.toNearestInt().removeFromTop (14).withTrimmedRight (8), Justification::centredRight);
    }
    if (cursor.has_value() && ! info.has_value())
    {
        g.setColour (Palette::text.withAlpha (0.8f));
        g.setFont (font (11.0f));
        g.drawText (formatHz (xToFreq (cursor->x)) + T ("   ·   ") + String (yToDb (cursor->y), 1) + " dB",
                    plot.toNearestInt().removeFromBottom (16).withTrimmedLeft (8), Justification::centredLeft);
    }
    if (! selected.has_value())
    {
        g.setFont (font (11.0f));
        g.setColour (Palette::dim.withAlpha (0.8f));
        g.drawText (T ("Doble clic: nueva banda   ·   Arrastra los puntos   ·   Rueda: Q / pendiente   ·   Shift: fino   ·   Option+clic: bypass"),
                    plot.toNearestInt().removeFromBottom (16).withTrimmedRight (8), Justification::centredRight);
    }
}
