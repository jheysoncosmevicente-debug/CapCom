#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "RadioChain.h"

// Ultimas muestras (mono) para el analizador de espectro. Un solo escritor (audio) y un lector (interfaz):
// si el lector pilla una escritura a medias solo se ve un cuadro imperfecto, nunca se bloquea el audio.
struct ScopeTap
{
    static constexpr int size = 1 << 14;
    std::array<float, size> buf {};
    std::atomic<int> pos { 0 };

    void push (const juce::AudioBuffer<float>& b)
    {
        const int n = b.getNumSamples(), chans = juce::jmin (2, b.getNumChannels());
        int w = pos.load (std::memory_order_relaxed);
        for (int i = 0; i < n; ++i)
        {
            float s = 0;
            for (int c = 0; c < chans; ++c) s += b.getReadPointer (c)[i];
            buf[(size_t) w] = chans > 0 ? s / (float) chans : 0.0f;
            w = (w + 1) & (size - 1);
        }
        pos.store (w, std::memory_order_release);
    }

    void copyLatest (float* dest, int n) const
    {
        int r = (pos.load (std::memory_order_acquire) - n + size) & (size - 1);
        for (int i = 0; i < n; ++i) { dest[i] = buf[(size_t) r]; r = (r + 1) & (size - 1); }
    }
};

class RadioFXProcessor : public juce::AudioProcessor
{
public:
    RadioFXProcessor();
    ~RadioFXProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "CapCom"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; } // cola del squelch + reverb del espacio mas largo

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Deshacer/Rehacer de todos los parametros (va antes de apvts: se construye primero).
    juce::UndoManager undoManager;
    // "RadioFX" es la etiqueta interna del estado guardado en los proyectos: no cambiarla aunque el plugin se llame CapCom.
    juce::AudioProcessorValueTreeState apvts { *this, &undoManager, "RadioFX", createParameterLayout() };

    // --- Comparacion A/B (como en Pro-Q): dos estados completos del plugin ---
    int getCurrentAB() const noexcept { return currentAB; }
    void switchAB (int slot);
    void copyABToOther();

    // --- Escuchar una banda (Solo): solo suena esa zona de frecuencias. No es un parametro. ---
    // type: 0 = paso-banda (campana, muesca, medios), 1 = paso-bajos (estante grave), 2 = paso-altos (estante agudo)
    void setSolo (bool on, float freq = 1000, float q = 1, int type = 0) { chain.setSolo (on, freq, q, type); }
    bool isSoloOn() const noexcept { return chain.isSoloOn(); }

    // --- Presets del usuario (Mac: ~/Library/Application Support/CapCom/Presets, Windows: %APPDATA%\CapCom\Presets) ---
    static juce::File userPresetFolder();
    juce::StringArray userPresetNames() const;
    bool saveUserPreset (const juce::String& name);
    bool loadUserPreset (const juce::String& name);
    void applyFactoryPreset (int index);

    // --- Medidores para la interfaz ---
    float getOutputPeak() const noexcept { return outputPeak.load(); }
    bool isTransmitting() const noexcept { return transmitting.load(); }
    float getInputPeak() const noexcept { return inputPeak.load(); }
    float getOutputPeakL() const noexcept { return outPeakL.load(); }
    float getOutputPeakR() const noexcept { return outPeakR.load(); }
    float getGainReductionDb() const noexcept { return grDb.load(); }
    const ScopeTap& getPreTap() const noexcept  { return preTap; }
    const ScopeTap& getPostTap() const noexcept { return postTap; }

    // --- Reproductor de archivos (solo en la app independiente) ---
    bool isStandalone() const noexcept { return wrapperType == wrapperType_Standalone; }
    bool loadAudioFile (const juce::File& file);
    juce::File getLoadedFile() const { return loadedFile; }
    void setPlaying (bool shouldPlay);
    bool isPlaying() const { return transport.isPlaying(); }
    void setLooping (bool shouldLoop);
    double getPositionSeconds() const { return transport.getCurrentPosition(); }
    double getLengthSeconds() const { return transport.getLengthInSeconds(); }
    void setPositionSeconds (double s) { transport.setPosition (s); }
    bool exportProcessed (const juce::File& destination, juce::String& error);

private:
    RadioChain chain;
    int currentProgram = 0;
    int currentAB = 0;
    std::array<juce::ValueTree, 2> abSlots;
    std::atomic<float> outputPeak { 0.0f };
    std::atomic<bool> transmitting { false };
    std::atomic<float> inputPeak { 0.0f }, grDb { 0.0f }, outPeakL { 0.0f }, outPeakR { 0.0f };
    ScopeTap preTap, postTap;

    juce::AudioFormatManager formatManager;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    juce::AudioTransportSource transport;
    juce::File loadedFile;
    bool looping = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RadioFXProcessor)
};
