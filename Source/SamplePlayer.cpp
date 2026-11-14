#include "SamplePlayer.h"
#include <cmath>

void SamplePlayer::prepare (double sampleRate)
{
    sr = sampleRate;
    for (auto& v : voices)
    {
        v.active = false;
        v.adsr.setSampleRate (sr);
    }
}

void SamplePlayer::setSample (std::shared_ptr<const Sample> s)
{
    sample = std::move (s);
    for (auto& v : voices)
        v.active = false;
}

void SamplePlayer::noteOn (int note, float velocity, const Params& p)
{
    if (sample == nullptr || sample->data.getNumSamples() < 2)
        return;

    Voice* target = nullptr;
    for (auto& v : voices)
        if (! v.active) { target = &v; break; }
    if (target == nullptr) // steal the oldest voice
    {
        target = &voices[0];
        for (auto& v : voices)
            if (v.startedAt < target->startedAt) target = &v;
    }

    target->active = true;
    target->note = note;
    target->pos = 0.0;
    target->velocity = velocity;
    target->startedAt = ++counter;
    target->adsr.setSampleRate (sr);
    target->adsr.setParameters ({ kAttackSec, 0.0f, 1.0f, p.releaseSec });
    target->adsr.noteOn();
}

void SamplePlayer::noteOff (int note, const Params& p)
{
    for (auto& v : voices)
        if (v.active && v.note == note && v.adsr.isActive())
        {
            v.adsr.setParameters ({ kAttackSec, 0.0f, 1.0f, p.releaseSec });
            v.adsr.noteOff();
        }
}

void SamplePlayer::renderVoice (Voice& v, juce::AudioBuffer<float>& out, int start, int num, const Params& p)
{
    if (sample == nullptr)
    {
        v.active = false;
        return;
    }

    const auto& d = sample->data;
    const int n = d.getNumSamples();
    const int srcCh = d.getNumChannels();
    const double ratio = (sample->rate / sr) * std::pow (2.0, ((double) (v.note - 60) + (double) p.pitchSemis) / 12.0);
    const float gain = p.level * p.level * v.velocity;

    float* outL = out.getWritePointer (0);
    float* outR = out.getNumChannels() > 1 ? out.getWritePointer (1) : nullptr;
    const float* srcL = d.getReadPointer (0);
    const float* srcR = srcCh > 1 ? d.getReadPointer (1) : srcL;

    for (int i = 0; i < num; ++i)
    {
        if (v.pos >= (double) n)
        {
            if (p.loop)
                v.pos = std::fmod (v.pos, (double) n);
            else
            {
                v.active = false;
                return;
            }
        }

        const int i0 = (int) v.pos;
        int i1 = i0 + 1;
        if (i1 >= n)
            i1 = p.loop ? 0 : n - 1;
        const float f = (float) (v.pos - (double) i0);

        const float l = srcL[i0] + (srcL[i1] - srcL[i0]) * f;
        const float r = srcR[i0] + (srcR[i1] - srcR[i0]) * f;
        const float env = v.adsr.getNextSample() * gain;

        outL[start + i] += l * env;
        if (outR != nullptr)
            outR[start + i] += r * env;

        v.pos += ratio;

        if (! v.adsr.isActive())
        {
            v.active = false;
            return;
        }
    }
}

void SamplePlayer::process (juce::AudioBuffer<float>& out, const juce::MidiBuffer& midi, const Params& p)
{
    out.clear();
    const int total = out.getNumSamples();
    int done = 0;

    auto renderTo = [&] (int upTo)
    {
        const int num = upTo - done;
        if (num <= 0)
            return;
        for (auto& v : voices)
            if (v.active)
                renderVoice (v, out, done, num, p);
        done = upTo;
    };

    for (const auto meta : midi)
    {
        renderTo (juce::jlimit (0, total, meta.samplePosition));
        const auto m = meta.getMessage();
        if (m.isNoteOn())
            noteOn (m.getNoteNumber(), m.getFloatVelocity(), p);
        else if (m.isNoteOff())
            noteOff (m.getNoteNumber(), p);
        else if (m.isAllNotesOff() || m.isAllSoundOff())
            for (auto& v : voices)
                if (v.active) v.adsr.noteOff();
    }
    renderTo (total);
}
