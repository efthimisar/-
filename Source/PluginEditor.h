#pragma once
#include <set>
#include "PluginProcessor.h"

namespace nektar
{
// Flat "liquid" knob (the circle fills from the bottom). Drawn in design units (1000x1000 space).
void drawKnob (juce::Graphics& g, const juce::Typeface::Ptr& tf, float cx, float cy, float r, float p,
               const juce::String& label, bool dim = false, float fontSize = 12.0f);
}

// One knob on screen. Linked to a parameter it can be dragged (double-click = reset);
// without a parameter it only shows a fixed value (not wired up yet).
class KnobComponent : public juce::Component
{
public:
    KnobComponent (juce::Typeface::Ptr tf, float radius, juce::String label, bool dim, float fixedValue,
                   juce::RangedAudioParameter* param = nullptr, bool isToggle = false);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

    static constexpr float kWidth = 90.0f; // design units
    float height() const { return 2.0f * radius + 20.0f; }

private:
    juce::Typeface::Ptr typeface;
    float radius;
    juce::String text;
    bool dimmed;
    float fixed;
    juce::RangedAudioParameter* parameter;
    bool toggle;
    float dragStart = 0.0f;
    std::unique_ptr<juce::ParameterAttachment> attachment;
};

class NektarEditor : public juce::AudioProcessorEditor,
                     public juce::FileDragAndDropTarget,
                     private juce::ChangeListener
{
public:
    explicit NektarEditor (NektarProcessor&);
    ~NektarEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    bool keyPressed (const juce::KeyPress&) override;
    bool keyStateChanged (bool isKeyDown) override;

    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int, int) override;

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void renderDesign (juce::Graphics&);
    void addGrain (juce::Image&);
    void chooseFile();
    void loadFile (const juce::File&);
    void releaseAllKeys();
    void addKnob (float cx, float cy, float r, const juce::String& label, bool dim, float fixed,
                  const char* paramId = nullptr, bool toggle = false);

    NektarProcessor& proc;
    juce::Typeface::Ptr regular, bold;
    juce::Image cache;
    juce::ComponentBoundsConstrainer constrainer;
    juce::OwnedArray<KnobComponent> knobs;
    juce::Array<juce::Point<float>> knobPos; // centre of each knob in design units
    std::unique_ptr<juce::FileChooser> chooser;
    juce::String message;
    std::set<int> heldNotes;
    int octaveShift = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NektarEditor)
};
