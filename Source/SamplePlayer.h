#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <array>
#include <memory>

// A loaded sound (read-only once created, shared between the UI thread and the audio thread).
struct Sample
{
    juce::AudioBuffer<float> data;
    double rate = 44100.0;
    juce::String name;
    juce::String path;
};

// Polyphonic one-sample player: the key you press changes the pitch (root note = middle C, MIDI 60).
class SamplePlayer
{
public:
    struct Params
    {
        float level = 0.8f;      // 0..1
        float pitchSemis = 0.0f; // -12..+12
        float releaseSec = 0.25f;
        bool loop = false;
    };

    void prepare (double sampleRate);
    void setSample (std::shared_ptr<const Sample> s);
    void process (juce::AudioBuffer<float>& out, const juce::MidiBuffer& midi, const Params& p);

private:
    static constexpr int kVoices = 16;
    static constexpr float kAttackSec = 0.005f; // tiny fixed attack, avoids clicks

    struct Voice
    {
        bool active = false;
        int note = -1;
        double pos = 0.0;
        float velocity = 1.0f;
        juce::uint64 startedAt = 0;
        juce::ADSR adsr;
    };

    void noteOn (int note, float velocity, const Params& p);
    void noteOff (int note, const Params& p);
    void renderVoice (Voice& v, juce::AudioBuffer<float>& out, int start, int num, const Params& p);

    std::array<Voice, kVoices> voices;
    std::shared_ptr<const Sample> sample;
    double sr = 44100.0;
    juce::uint64 counter = 0;
};
