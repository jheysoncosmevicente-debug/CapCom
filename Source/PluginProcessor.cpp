#include "PluginProcessor.h"
#include "PluginEditor.h"

RadioFXProcessor::RadioFXProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    formatManager.registerBasicFormats();
}

RadioFXProcessor::~RadioFXProcessor()
{
    transport.setSource (nullptr);
}

void RadioFXProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    chain.prepare (sampleRate, samplesPerBlock, getTotalNumOutputChannels());
    transport.prepareToPlay (samplesPerBlock, sampleRate);
}

void RadioFXProcessor::releaseResources()
{
    transport.releaseResources();
}

bool RadioFXProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out;
}

void RadioFXProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); ++c)
        buffer.clear (c, 0, buffer.getNumSamples());

    // En la app independiente la entrada es el archivo cargado, nunca el microfono.
    if (isStandalone())
    {
        if (readerSource != nullptr)
        {
            juce::AudioSourceChannelInfo info (&buffer, 0, buffer.getNumSamples());
            transport.getNextAudioBlock (info);
        }
        else
        {
            buffer.clear();
        }
    }

    float inPeak = 0;
    for (int c = 0; c < buffer.getNumChannels(); ++c)
        inPeak = std::max (inPeak, buffer.getMagnitude (c, 0, buffer.getNumSamples()));
    inputPeak.store (std::max (inPeak, inputPeak.load() * 0.9f));
    preTap.push (buffer);

    chain.process (buffer, RadioParams::fromState (apvts));

    postTap.push (buffer);
    const int n = buffer.getNumSamples();
    const float l = buffer.getNumChannels() > 0 ? buffer.getMagnitude (0, 0, n) : 0.0f;
    const float r = buffer.getNumChannels() > 1 ? buffer.getMagnitude (1, 0, n) : l;
    outPeakL.store (std::max (l, outPeakL.load() * 0.9f));
    outPeakR.store (std::max (r, outPeakR.load() * 0.9f));
    // El medidor de compresion baja rapido y vuelve despacio, como en un compresor de hardware.
    const float gr = chain.getGainReductionDb();
    grDb.store (gr < grDb.load() ? gr : grDb.load() * 0.85f + gr * 0.15f);
    outputPeak.store (std::max (chain.getLastPeak(), outputPeak.load() * 0.9f));
    transmitting.store (chain.isTransmitting());
}

juce::AudioProcessorEditor* RadioFXProcessor::createEditor()
{
    return new RadioFXEditor (*this);
}

//==============================================================================
int RadioFXProcessor::getNumPrograms()
{
    return (int) factoryPresets().size();
}

void RadioFXProcessor::setCurrentProgram (int index)
{
    if (juce::isPositiveAndBelow (index, getNumPrograms()))
        applyFactoryPreset (index);
}

const juce::String RadioFXProcessor::getProgramName (int index)
{
    return juce::isPositiveAndBelow (index, getNumPrograms()) ? juce::String::fromUTF8 (factoryPresets()[(size_t) index].name) : juce::String();
}

void RadioFXProcessor::applyFactoryPreset (int index)
{
    currentProgram = index;
    // Primero todo a su valor por defecto, asi no queda nada del preset anterior.
    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            rp->setValueNotifyingHost (rp->getDefaultValue());
    for (const auto& [id, value] : factoryPresets()[(size_t) index].values)
        if (auto* param = apvts.getParameter (id))
            param->setValueNotifyingHost (param->convertTo0to1 (value));
}

void RadioFXProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void RadioFXProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
void RadioFXProcessor::switchAB (int slot)
{
    if (slot == currentAB || ! juce::isPositiveAndBelow (slot, 2)) return;
    const auto uiTab = apvts.state.getProperty ("uiTab"), uiCollapsed = apvts.state.getProperty ("uiCollapsed");
    abSlots[(size_t) currentAB] = apvts.copyState();
    if (abSlots[(size_t) slot].isValid())
        apvts.replaceState (abSlots[(size_t) slot].createCopy());
    // La vista (pestana, controles ocultos) no cambia al pasar de A a B.
    apvts.state.setProperty ("uiTab", uiTab, nullptr);
    apvts.state.setProperty ("uiCollapsed", uiCollapsed, nullptr);
    currentAB = slot;
}

void RadioFXProcessor::copyABToOther()
{
    abSlots[(size_t) (1 - currentAB)] = apvts.copyState();
}

//==============================================================================
juce::File RadioFXProcessor::userPresetFolder()
{
    // Mac: ~/Library/Application Support/CapCom/Presets · Windows: %APPDATA%\CapCom\Presets
    auto support = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory);
   #if JUCE_MAC
    support = support.getChildFile ("Application Support");
   #endif
    return support.getChildFile ("CapCom/Presets");
}

juce::StringArray RadioFXProcessor::userPresetNames() const
{
    juce::StringArray names;
    for (const auto& f : userPresetFolder().findChildFiles (juce::File::findFiles, false, "*.xml"))
        names.add (f.getFileNameWithoutExtension());
    names.sortNatural();
    return names;
}

bool RadioFXProcessor::saveUserPreset (const juce::String& name)
{
    const auto clean = juce::File::createLegalFileName (name.trim());
    if (clean.isEmpty()) return false;
    auto folder = userPresetFolder();
    folder.createDirectory();
    if (auto xml = apvts.copyState().createXml())
        return xml->writeTo (folder.getChildFile (clean + ".xml"));
    return false;
}

bool RadioFXProcessor::loadUserPreset (const juce::String& name)
{
    if (auto xml = juce::XmlDocument::parse (userPresetFolder().getChildFile (name + ".xml")))
    {
        const auto tree = juce::ValueTree::fromXml (*xml);
        for (auto* p : getParameters())
            if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            {
                const auto child = tree.getChildWithProperty ("id", rp->paramID);
                // Los presets guardados con versiones anteriores no traen los parametros nuevos: van por defecto.
                rp->setValueNotifyingHost (child.isValid() ? rp->convertTo0to1 ((float) child.getProperty ("value"))
                                                           : rp->getDefaultValue());
            }
        return true;
    }
    return false;
}

//==============================================================================
bool RadioFXProcessor::loadAudioFile (const juce::File& file)
{
    auto* reader = formatManager.createReaderFor (file);
    if (reader == nullptr)
        return false;

    const double fileRate = reader->sampleRate;
    auto newSource = std::make_unique<juce::AudioFormatReaderSource> (reader, true);
    newSource->setLooping (looping);

    transport.stop();
    transport.setSource (newSource.get(), 0, nullptr, fileRate, 2);
    readerSource = std::move (newSource);
    loadedFile = file;
    return true;
}

void RadioFXProcessor::setPlaying (bool shouldPlay)
{
    if (readerSource == nullptr) return;
    if (shouldPlay)
    {
        if (transport.hasStreamFinished()) transport.setPosition (0);
        transport.start();
    }
    else
    {
        transport.stop();
    }
}

void RadioFXProcessor::setLooping (bool shouldLoop)
{
    looping = shouldLoop;
    if (readerSource != nullptr) readerSource->setLooping (shouldLoop);
}

bool RadioFXProcessor::exportProcessed (const juce::File& destination, juce::String& error)
{
    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (loadedFile));
    if (reader == nullptr) { error = "No se pudo leer el archivo de origen."; return false; }

    const auto params = RadioParams::fromState (apvts);
    const double rate = reader->sampleRate;
    const int srcChannels = (int) reader->numChannels;
    // El espacio es estereo; si no hay espacio, se respeta el original (o mono).
    const bool stereoScene = params.room > 0;
    const int outChannels = stereoScene ? 2 : (params.mono ? 1 : juce::jlimit (1, 2, srcChannels));
    constexpr int block = 4096;

    destination.deleteFile();
    std::unique_ptr<juce::OutputStream> stream = std::make_unique<juce::FileOutputStream> (destination);
    if (static_cast<juce::FileOutputStream*> (stream.get())->failedToOpen())
    {
        error = "No se pudo crear el archivo de destino.";
        return false;
    }

    juce::WavAudioFormat wav;
    auto writer = wav.createWriterFor (stream, juce::AudioFormatWriterOptions{}
                                                   .withSampleRate (rate)
                                                   .withNumChannels (outChannels)
                                                   .withBitsPerSample (24));
    if (writer == nullptr) { error = "No se pudo crear el WAV."; return false; }

    RadioChain offline;
    offline.prepare (rate, block, 2);

    juce::AudioBuffer<float> buffer (2, block);
    const auto length = reader->lengthInSamples;
    const double tail = params.room > 0 ? 3.5 : 1.0; // cola del squelch y del espacio
    const auto total = length + (juce::int64) (tail * rate);

    for (juce::int64 pos = 0; pos < total; pos += block)
    {
        const int num = (int) std::min<juce::int64> (block, total - pos);
        buffer.clear();
        if (pos < length)
            reader->read (&buffer, 0, (int) std::min<juce::int64> (num, length - pos), pos, true, true);

        juce::AudioBuffer<float> view (buffer.getArrayOfWritePointers(), 2, num);
        offline.process (view, params);
        if (! writer->writeFromAudioSampleBuffer (view, 0, num))
        {
            error = "Error al escribir el WAV.";
            return false;
        }
    }
    return true;
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RadioFXProcessor();
}
