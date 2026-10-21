#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "Style.h"
#include "EqEditor.h"

class RadioLookAndFeel : public juce::LookAndFeel_V4
{
public:
    RadioLookAndFeel();
    void applyTheme(); // vuelve a leer la paleta (Interplanetary o Noche)

    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool highlighted, bool down) override;
    void drawCallOutBoxBackground (juce::CallOutBox&, juce::Graphics&, const juce::Path&, juce::Image&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getLabelFont (juce::Label&) override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;
    juce::Label* createSliderTextBox (juce::Slider&) override;

private:
    void drawSpaceKnob (juce::Graphics&, juce::Point<float> centre, float radius, float angle, float startAngle, float endAngle, float fromAngle);
};

// Boton con icono dibujado (flechas, menu, ecualizador, luna/sol, deshacer...).
class IconButton : public juce::Button
{
public:
    enum class Icon { left, right, dots, eqOnly, moon, sun, planet, help };
    IconButton (Icon i, bool round = false) : juce::Button ({}), icon (i), isRound (round) {}
    void setIcon (Icon i) { icon = i; repaint(); }
    void paintButton (juce::Graphics&, bool highlighted, bool down) override;

private:
    Icon icon;
    bool isRound;
};

// Una celda de la grilla: perilla, lista desplegable o interruptor con su nombre.
class Cell : public juce::Component
{
public:
    enum class Kind { knob, choice, toggle };
    Cell (RadioFXProcessor&, const juce::String& paramId, const juce::String& title, Kind);
    void resized() override;
    void paint (juce::Graphics&) override;
    void refreshColours();
    float widthWeight() const { return kind == Kind::choice ? 1.5f : 1.0f; } // las listas necesitan mas ancho

private:
    Kind kind;
    juce::String title;
    juce::Slider slider;
    juce::ComboBox combo;
    juce::ToggleButton toggle;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> cAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bAtt;
};

class RadioFXEditor : public juce::AudioProcessorEditor,
                      public juce::FileDragAndDropTarget,
                      private juce::Timer
{
public:
    explicit RadioFXEditor (RadioFXProcessor&);
    ~RadioFXEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void previewSelectMid() { eq.selectMidForPreview(); }
    void refreshTheme();
    // Cada clic en cualquier parte de la ventana abre una accion nueva para Deshacer.
    void mouseDown (const juce::MouseEvent&) override { proc.undoManager.beginNewTransaction(); }

    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int x, int y) override;

private:
    struct Section
    {
        juce::String title;
        std::vector<Cell*> cells;
        juce::Rectangle<int> bounds;
        int tab = 0;
        juce::String toggleId, help;   // interruptor de la seccion (opcional) y texto del "?"
        std::unique_ptr<juce::ToggleButton> enable;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> enableAtt;
        std::unique_ptr<IconButton> helpButton;
    };

    enum Tab { tabSignal = 0, tabScene, numTabs };

    void timerCallback() override;
    void setupSection (Section&, const juce::String& title, std::vector<Cell*> cells, int tab,
                       const juce::String& toggleId, const juce::String& help);
    void rebuildPresetList();
    void presetChosen();
    void stepPreset (int direction);
    void savePresetDialog();
    void showMoreMenu();
    void showHelp (Section&);
    void openFile();
    void exportFile();
    void loadFile (const juce::File&);
    Cell* addCell (const juce::String& id, const juce::String& title, Cell::Kind kind = Cell::Kind::knob);
    void layoutRow (juce::Rectangle<int> area, std::vector<Section*> row);
    void showTab (int tab);
    void setControlsHidden (bool hidden);
    void switchAB (int slot);
    void paintMeters (juce::Graphics&);
    void paintRoute (juce::Graphics&);
    void buildBackground();

    RadioFXProcessor& proc;
    RadioLookAndFeel lnf;

    juce::OwnedArray<Cell> cells;
    Section secInput, secDyn, secDirt, secNoise, secTape, secSquelch, secQuindar, secOut;
    Section secDevice, secDistance, secRoom, secObstacle;
    Section secTransmit, secRoger;
    std::vector<Section*> allSections() { return { &secInput, &secDyn, &secDirt,
                                                   &secNoise, &secTape, &secSquelch, &secOut,
                                                   &secDevice, &secDistance, &secRoom, &secObstacle,
                                                   &secTransmit, &secRoger, &secQuindar }; }

    // Pestanas (control segmentado)
    std::array<juce::TextButton, numTabs> tabButtons;
    juce::Rectangle<int> tabArea;
    int currentTab = tabSignal;
    bool controlsHidden = false;

    // Cabecera
    juce::Rectangle<int> headerArea, logoArea, meterArea, fileBarArea;
    IconButton prevPreset { IconButton::Icon::left }, nextPreset { IconButton::Icon::right };
    juce::ComboBox presetBox;
    juce::TextButton aButton { "A" }, bButton { "B" };
    IconButton moreButton { IconButton::Icon::dots }, eqOnlyButton { IconButton::Icon::eqOnly },
               themeButton { IconButton::Icon::moon, true };
    juce::TextButton saveButton;

    EqEditor eq;
    float outL = 0, outR = 0, grLevel = 0, holdL = 0, holdR = 0; // hold = marca de pico
    bool txOn = false;
    int seenThemeVersion = -1;

    // Fondo espacial (nebulosas, estrellas, Marte) dibujado una vez y guardado: el ecualizador repinta a 30 fps.
    juce::Image bgCache, logoImage;
    int bgVersion = -1;
    juce::Rectangle<int> routeArea;   // ruta de la senal Tierra -> rele -> Marte en la cabecera
    float routePhase = 0;             // pulso que viaja por la ruta

    // Barra de archivo (solo app independiente)
    bool standalone;
    juce::TextButton openButton { "Abrir audio" }, playButton { "Reproducir" }, exportButton { "Exportar WAV" };
    juce::ToggleButton loopButton { "Repetir" };
    juce::Label fileLabel, timeLabel;
    std::unique_ptr<juce::FileChooser> chooser;
    juce::Component::SafePointer<juce::CallOutBox> helpBox;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RadioFXEditor)
};
