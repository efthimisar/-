#pragma once
#include "PluginProcessor.h"

// Step 1: static design (Flow look). Everything is drawn in a 1000x1000 space and scaled.
class NektarEditor : public juce::AudioProcessorEditor
{
public:
    explicit NektarEditor (NektarProcessor&);
    ~NektarEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void renderDesign (juce::Graphics&);
    void addGrain (juce::Image&);

    juce::Typeface::Ptr regular, bold;
    juce::Image cache;
    juce::ComponentBoundsConstrainer constrainer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NektarEditor)
};
