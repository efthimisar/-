#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout NektarProcessor::createLayout()
{
    using juce::NormalisableRange;
    using juce::ParameterID;
    juce::AudioProcessorValueTreeState::ParameterLayout l;

    l.add (std::make_unique<juce::AudioParameterFloat> (ParameterID { "level", 1 }, "Level",
                                                         NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.8f));
    l.add (std::make_unique<juce::AudioParameterFloat> (ParameterID { "pitch", 1 }, "Pitch",
                                                         NormalisableRange<float> (-12.0f, 12.0f, 0.01f), 0.0f,
                                                         juce::AudioParameterFloatAttributes().withLabel ("st")));
    l.add (std::make_unique<juce::AudioParameterFloat> (ParameterID { "tail", 1 }, "Tail",
                                                         NormalisableRange<float> (0.01f, 5.0f, 0.001f, 0.4f), 0.25f,
                                                         juce::AudioParameterFloatAttributes().withLabel ("s")));
    l.add (std::make_unique<juce::AudioParameterBool> (ParameterID { "loop", 1 }, "Loop", false));
    return l;
}

NektarProcessor::NektarProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    formats.registerBasicFormats();
}

void NektarProcessor::prepareToPlay (double sampleRate, int)
{
    player.prepare (sampleRate);
    keyboardState.reset();
}

bool NektarProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void NektarProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    {
        const juce::SpinLock::ScopedTryLockType lock (sampleLock);
        if (lock.isLocked() && samplePending)
        {
            player.setSample (pendingSample);
            samplePending = false;
        }
    }

    keyboardState.processNextMidiBuffer (midi, 0, buffer.getNumSamples(), true);

    SamplePlayer::Params p;
    p.level = apvts.getRawParameterValue ("level")->load();
    p.pitchSemis = apvts.getRawParameterValue ("pitch")->load();
    p.releaseSec = apvts.getRawParameterValue ("tail")->load();
    p.loop = apvts.getRawParameterValue ("loop")->load() > 0.5f;

    player.process (buffer, midi, p);
    midi.clear();
}

juce::String NektarProcessor::loadSample (const juce::File& file)
{
    std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (file));
    if (reader == nullptr)
        return "Could not read this file.";
    if (reader->lengthInSamples < 2)
        return "The file is empty.";
    if (reader->lengthInSamples > (juce::int64) (reader->sampleRate * 600.0))
        return "The file is longer than 10 minutes.";

    auto s = std::make_shared<Sample>();
    s->rate = reader->sampleRate;
    s->name = file.getFileNameWithoutExtension();
    s->path = file.getFullPathName();
    s->data.setSize ((int) juce::jmin (2u, reader->numChannels), (int) reader->lengthInSamples);
    reader->read (&s->data, 0, (int) reader->lengthInSamples, 0, true, s->data.getNumChannels() > 1);

    {
        const juce::SpinLock::ScopedLockType lock (sampleLock);
        currentSample = s;
        pendingSample = s;
        samplePending = true;
    }
    sendChangeMessage();
    return {};
}

std::shared_ptr<const Sample> NektarProcessor::getSample() const
{
    const juce::SpinLock::ScopedLockType lock (sampleLock);
    return currentSample;
}

void NektarProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    if (auto s = getSample())
        state.setProperty ("samplePath", s->path, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, dest);
}

void NektarProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
    {
        auto state = juce::ValueTree::fromXml (*xml);
        if (state.isValid())
        {
            apvts.replaceState (state);
            const auto path = state.getProperty ("samplePath").toString();
            if (path.isNotEmpty() && juce::File (path).existsAsFile())
                loadSample (juce::File (path));
        }
    }
}

juce::AudioProcessorEditor* NektarProcessor::createEditor()
{
    return new NektarEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NektarProcessor();
}
