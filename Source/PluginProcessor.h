#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "SamplePlayer.h"

class NektarProcessor : public juce::AudioProcessor,
                        public juce::ChangeBroadcaster
{
public:
    NektarProcessor();
    ~NektarProcessor() override = default;

    void prepareToPlay (double sampleRate, int) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Nektar"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 5.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // Loads a sound into slot 1 (message thread). Returns an error text, or an empty string on success.
    juce::String loadSample (const juce::File& file);
    std::shared_ptr<const Sample> getSample() const;

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState; // computer-keyboard notes from the UI

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    SamplePlayer player;
    juce::AudioFormatManager formats;
    mutable juce::SpinLock sampleLock;
    std::shared_ptr<const Sample> currentSample;   // owned by the UI side
    std::shared_ptr<const Sample> pendingSample;   // handed to the audio thread
    bool samplePending = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NektarProcessor)
};
