// Herramienta interna de CapCom (no abre ventanas):
//   CapComSnapshot <carpeta>                          dibuja la interfaz a PNG (plugin y app, las 2 pestanas)
//   CapComSnapshot <carpeta> <entrada.wav> <salida.wav>   prueba "Exportar WAV" con el estado por defecto
//   CapComSnapshot --preset "<nombre>" <entrada.wav> <salida.wav> [id=valor ...]
//                                                      exporta con un preset de fabrica y ajustes
#include <juce_audio_utils/juce_audio_utils.h>
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"

namespace
{
    void writePng (const juce::Image& image, const juce::File& file)
    {
        file.deleteFile();
        juce::FileOutputStream os (file);
        juce::PNGImageFormat().writeImageToStream (image, os);
    }

    void setParam (RadioFXProcessor& proc, const juce::String& id, float value)
    {
        if (auto* p = proc.apvts.getParameter (id))
            p->setValueNotifyingHost (p->convertTo0to1 (value));
        else
            std::printf ("parametro desconocido: %s\n", id.toRawUTF8());
    }

    bool exportWith (RadioFXProcessor& proc, const juce::String& in, const juce::File& out)
    {
        if (! proc.loadAudioFile (juce::File (in))) { std::printf ("no se pudo abrir %s\n", in.toRawUTF8()); return false; }
        juce::String error;
        const bool ok = proc.exportProcessed (out, error);
        std::printf ("%s %s %s\n", ok ? "OK   " : "FALLO", out.getFileName().toRawUTF8(), error.toRawUTF8());
        return ok;
    }

    int presetIndex (RadioFXProcessor& proc, const juce::String& name)
    {
        for (int i = 0; i < proc.getNumPrograms(); ++i)
            if (proc.getProgramName (i) == name) return i;
        return -1;
    }
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    juce::StringArray args;
    for (int i = 1; i < argc; ++i) args.add (juce::String::fromUTF8 (argv[i]));

    if (args[0] == "--preset" && args.size() >= 4)
    {
        juce::AudioProcessor::setTypeOfNextNewPlugin (juce::AudioProcessor::wrapperType_Standalone);
        RadioFXProcessor proc;
        const int idx = presetIndex (proc, args[1]);
        if (idx < 0) { std::printf ("preset no encontrado: %s\n", args[1].toRawUTF8()); return 1; }
        proc.applyFactoryPreset (idx);
        for (int i = 4; i < args.size(); ++i)
            setParam (proc, args[i].upToFirstOccurrenceOf ("=", false, false), args[i].fromFirstOccurrenceOf ("=", false, false).getFloatValue());
        return exportWith (proc, args[2], juce::File (args[3])) ? 0 : 1;
    }

    const juce::File outDir (args.isEmpty() ? juce::File::getCurrentWorkingDirectory().getFullPathName() : args[0]);

    if (args.size() >= 3)
    {
        juce::AudioProcessor::setTypeOfNextNewPlugin (juce::AudioProcessor::wrapperType_Standalone);
        RadioFXProcessor proc;
        return exportWith (proc, args[1], juce::File (args[2])) ? 0 : 1;
    }

    const char* tabNames[] = { "senal", "escena" };
    for (const bool standalone : { false, true })
        for (int tab = 0; tab < (standalone ? 2 : 2); ++tab)
        {
            // Plugin: tab 0 = vista normal, tab 1 = solo ecualizador (controles ocultos).
            const bool collapsed = ! standalone && tab == 1;
            juce::AudioProcessor::setTypeOfNextNewPlugin (standalone ? juce::AudioProcessor::wrapperType_Standalone
                                                                     : juce::AudioProcessor::wrapperType_Undefined);
            RadioFXProcessor proc;
            proc.apvts.state.setProperty ("uiTab", standalone ? tab : 0, nullptr);
            proc.apvts.state.setProperty ("uiCollapsed", collapsed, nullptr);
            proc.apvts.state.setProperty ("uiPanelMin", collapsed, nullptr); // en la vista "solo EQ": panel minimizado
            if (standalone && tab == 1) proc.applyFactoryPreset (presetIndex (proc, "Patrulla afuera (desde la casa)"));

            if (! standalone)
            {
                // Vista previa del ecualizador: dos bandas de ejemplo y voz real pasando, para ver el analizador.
                setParam (proc, bandParamID (1, "On"), 1);   setParam (proc, bandParamID (1, "Freq"), 2600);
                setParam (proc, bandParamID (1, "Gain"), -6); setParam (proc, bandParamID (1, "Q"), 2.0f);
                setParam (proc, bandParamID (3, "On"), 1);   setParam (proc, bandParamID (3, "Type"), 1);
                setParam (proc, bandParamID (3, "Freq"), 250); setParam (proc, bandParamID (3, "Gain"), 4);
                juce::AudioFormatManager fm;
                fm.registerBasicFormats();
                // Opcional: una voz en pruebas/voz.wav llena el analizador de la captura (si no existe, sale sin espectro)
                const auto voice = juce::File::getCurrentWorkingDirectory().getChildFile ("pruebas/voz.wav");
                if (std::unique_ptr<juce::AudioFormatReader> r { fm.createReaderFor (voice) })
                {
                    proc.prepareToPlay (r->sampleRate, 512);
                    juce::AudioBuffer<float> buf (2, 512);
                    juce::MidiBuffer midi;
                    for (juce::int64 pos = (juce::int64) (r->sampleRate * 1.0); pos < (juce::int64) (r->sampleRate * 1.6); pos += 512)
                    {
                        r->read (&buf, 0, 512, pos, true, true);
                        proc.processBlock (buf, midi);
                    }
                }
            }

            std::unique_ptr<juce::AudioProcessorEditor> editor (proc.createEditor());
            if (! standalone)
                if (auto* ed = dynamic_cast<RadioFXEditor*> (editor.get()))
                    ed->previewSelectMid();
            // Los dos temas, sin guardar la preferencia del usuario.
            for (const bool night : { false, true })
            {
                Theme::apply (night);
                if (auto* ed = dynamic_cast<RadioFXEditor*> (editor.get())) ed->refreshTheme();
                const auto image = editor->createComponentSnapshot (editor->getLocalBounds(), true, 2.0f);
                const juce::String base = standalone ? juce::String ("ui_app_") + tabNames[tab]
                                                     : juce::String (collapsed ? "ui_plugin_eq" : "ui_plugin");
                writePng (image, outDir.getChildFile (base + (night ? "_noche.png" : "_relay.png")));
            }
        }
    return 0;
}
