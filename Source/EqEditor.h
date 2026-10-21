#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_animation/juce_animation.h>
#include <optional>
#include <deque>
#include "PluginProcessor.h"
#include "Style.h"

// Ecualizador grafico al estilo FabFilter Pro-Q: puntos que se arrastran sobre la curva,
// analizador de espectro detras y un panel flotante con los controles del punto elegido.
//
// Gestos (los mismos de Pro-Q):
//   doble clic en el fondo ........ crea una banda      arrastrar un punto ..... frecuencia y ganancia
//   rueda del mouse ............... Q (o pendiente)     Shift .................. ajuste fino
//   Option + clic ................. bypass de la banda  Cmd + Option + clic .... cambia la forma
//   Supr / Borrar ................. borra la banda elegida (tambien con la X del panel o el clic derecho)
//   clic derecho .................. menu del punto      clic en el fondo ....... deselecciona
class EqEditor : public juce::Component
{
public:
    explicit EqEditor (RadioFXProcessor&);
    ~EqEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    bool keyPressed (const juce::KeyPress&) override;

    // Lo llama el timer del editor (30 Hz): analizador y repintado.
    void tick();
    // Para la vista previa de la herramienta de capturas.
    void selectMidForPreview();
    void clearSelection() { select (std::nullopt); }
    void themeChanged();

    enum class Kind { lowCut, highCut, mid, band };
    struct Node
    {
        Kind kind = Kind::mid;
        int band = -1;
        bool operator== (const Node& o) const { return kind == o.kind && band == o.band; }
        bool operator!= (const Node& o) const { return ! (*this == o); }
    };

private:
    class BandPanel;

    // Geometria
    float freqToX (float f) const;
    float xToFreq (float x) const;
    float dbToY (float db) const;
    float yToDb (float y) const;
    float analyzerDbToY (float db) const;

    // Puntos
    std::vector<Node> existingNodes (const RadioParams&) const;
    juce::Point<float> nodePosition (const Node&, const RadioParams&) const;
    std::optional<Node> nodeAt (juce::Point<float>) const;
    juce::Colour nodeColour (const Node&) const;
    juce::String nodeTitle (const Node&) const;
    juce::String nodeReadout (const Node&, const RadioParams&) const;
    bool hasGain (const Node&, const RadioParams&) const;

    // Parametros de cada punto
    juce::RangedAudioParameter* param (const juce::String& id) const;
    juce::RangedAudioParameter* freqParam (const Node&) const;
    juce::RangedAudioParameter* gainParam (const Node&) const;
    juce::RangedAudioParameter* qParam (const Node&) const;
    juce::RangedAudioParameter* slopeParam (const Node&) const;
    float valueOf (juce::RangedAudioParameter*) const;
    void setNow (juce::RangedAudioParameter*, float value);

    // Acciones
    void select (std::optional<Node>);
    void placePanel();
    void setPanelMinimized (bool);
    void animatePanel (bool in, std::function<void()> onDone = {});
    void animateTabIn();
    void finishAnimations(); // salta al estado final (cambio de tema, capturas)
    void createBandAt (juce::Point<float>);
    void deleteNode (const Node&);
    void toggleBypass (const Node&);
    void cycleShape (const Node&);
    void showNodeMenu (const Node&);
    void stepSelection (int direction);

    // Barra superior y escuchar banda
    void refreshToolbar();
    void showEqMenu();
    void setSpectrumOnly (bool);
    void setSoloFor (const Node&, bool on);
    void updateSoloFromParams();

    // Analizador
    void updateSpectrum (const ScopeTap&, std::vector<float>& smoothed);
    juce::Path spectrumPath (const std::vector<float>& db, bool closed) const;

    RadioFXProcessor& proc;
    juce::Rectangle<float> plot;
    juce::Rectangle<int> toolbar;
    float displayRange = 24.0f;   // escala vertical: +-6, 12, 24 o 30 dB
    int analyzerMode = 0;         // 0 entrada + salida, 1 solo salida, 2 apagado
    std::optional<juce::Point<float>> cursor;

    juce::TextButton undoButton, redoButton, eqTabButton { "EQ" }, specTabButton { "Espectro" }, menuButton;
    bool spectrumOnly = false; // "Espectro": solo el analizador, sin puntos
    juce::ComboBox analyzerBox, scaleBox;

    std::optional<Node> selected, hovered, dragging;
    juce::Point<float> dragStartMouse;
    float dragStartFreqX = 0, dragStartGainY = 0;
    float wheelAccum = 0;

    std::unique_ptr<BandPanel> panel;
    bool panelMinimized = false;       // la X del panel lo minimiza; la pestana de abajo lo vuelve a abrir
    juce::TextButton showPanelButton;

    // Animaciones (modulo juce_animation, sincronizadas con el refresco de pantalla)
    juce::VBlankAnimatorUpdater animUpdater { this };
    std::optional<juce::Animator> panelAnim, tabAnim;
    bool panelAnimating = false;

    static constexpr int fftOrder = 12, fftSize = 1 << fftOrder;
    juce::dsp::FFT fft { fftOrder };
    juce::dsp::WindowingFunction<float> window { (size_t) fftSize, juce::dsp::WindowingFunction<float>::hann };
    std::vector<float> fftData, preDb, postDb;
    std::deque<std::vector<float>> postTrail; // cuadros anteriores: estelas tipo humo
    int trailTick = 0;

    std::vector<float> lastParamValues;
};
