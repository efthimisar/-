#include "PluginEditor.h"
#include "BinaryData.h"

using juce::Colour;
using juce::ColourGradient;
using juce::Graphics;

namespace
{
const Colour kLight (0xffc1c494), kMuted (0xffa5a38a), kAcc (0xfffbe6a0), kDark (0xff2a2826);
const Colour kWada[] = { Colour (0xfffdc57e), Colour (0xffb5b1d8), Colour (0xff78cdd0),
                         Colour (0xfffbe6a0), Colour (0xffc1c494) };
constexpr float kCx = 500.0f, kCy = 410.0f, kRingR = 255.0f, kScreenR = 106.0f, kSlotR = 135.0f;

juce::Font font (const juce::Typeface::Ptr& tf, float h)
{
    return juce::Font (juce::FontOptions (tf).withHeight (h));
}

void shadowCircle (Graphics& g, float cx, float cy, float r, float alpha, int radius, juce::Point<int> off)
{
    juce::Path p;
    p.addEllipse (cx - r, cy - r, 2 * r, 2 * r);
    juce::DropShadow (juce::Colours::black.withAlpha (alpha), radius, off).drawForPath (g, p);
}

void shadowRect (Graphics& g, juce::Rectangle<float> r, float corner, float alpha, int radius, juce::Point<int> off)
{
    juce::Path p;
    p.addRoundedRectangle (r, corner);
    juce::DropShadow (juce::Colours::black.withAlpha (alpha), radius, off).drawForPath (g, p);
}

// Flat "liquid" knob: the circle fills from the bottom.
void knob (Graphics& g, const juce::Typeface::Ptr& tf, float cx, float cy, float r, float p,
           const juce::String& label, bool dim = false, float fs = 12.0f)
{
    const float a = dim ? 0.35f : 1.0f;
    g.setColour (Colour (0xff1e1d1b));
    g.fillEllipse (cx - r, cy - r, 2 * r, 2 * r);
    {
        juce::Graphics::ScopedSaveState s (g);
        juce::Path c;
        c.addEllipse (cx - r, cy - r, 2 * r, 2 * r);
        g.reduceClipRegion (c);
        g.setColour (kAcc.withMultipliedAlpha (a));
        g.fillRect (cx - r, cy + r - 2 * r * p, 2 * r, 2 * r * p);
    }
    g.setColour (kAcc.withMultipliedAlpha (a));
    g.drawEllipse (cx - r, cy - r, 2 * r, 2 * r, 1.5f);
    g.setColour (kMuted.withMultipliedAlpha (dim ? 0.5f : 1.0f));
    g.setFont (font (tf, fs));
    g.drawText (label, juce::Rectangle<float> (cx - 45, cy + r + 3, 90, fs + 4), juce::Justification::centred);
}

void wave (Graphics& g, float cx, float cy, float w, float amp, int seed, float decay, int n = 40)
{
    juce::Random rnd (seed);
    g.setColour (kLight);
    for (int i = 0; i < n; ++i)
    {
        const float x = cx - w / 2 + w * (float) i / (float) (n - 1);
        const float h = juce::jmax (1.5f, amp * std::exp (-decay * (float) i / (float) n) * (0.35f + 0.65f * rnd.nextFloat()));
        g.drawLine (x, cy - h, x, cy + h, 2.0f);
    }
}

void insetBox (Graphics& g, juce::Rectangle<float> r, float corner)
{
    g.setColour (Colour (0xff1a1918));
    g.fillRoundedRectangle (r, corner);
    g.setColour (juce::Colours::white.withAlpha (0.05f));
    g.drawRoundedRectangle (r.translated (0, 0.5f), corner, 1.0f);
}

void sphere (Graphics& g, float x, float y, float s, Colour c)
{
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillEllipse (x + s * 0.2f, y + s * 0.45f, s * 2.0f, s * 2.0f);
    ColourGradient grad (c.brighter (0.28f), x - s * 0.3f, y - s * 0.35f, c.darker (0.5f), x + s * 0.6f, y + s * 0.7f, true);
    grad.addColour (0.45, c);
    g.setGradientFill (grad);
    g.fillEllipse (x - s, y - s, 2 * s, 2 * s);
}

void slotDisc (Graphics& g, const juce::Typeface::Ptr& reg, const juce::Typeface::Ptr& bold,
               float cx, float cy, int index, const juce::String& title, bool empty,
               const float* p, int seed)
{
    shadowCircle (g, cx, cy, kSlotR, 0.5f, 30, { 14, 22 });
    g.setGradientFill (ColourGradient (Colour (0xff3d3b39), cx - kSlotR * 0.6f, cy - kSlotR * 0.7f,
                                       Colour (0xff222120), cx + kSlotR * 0.7f, cy + kSlotR, false));
    g.fillEllipse (cx - kSlotR, cy - kSlotR, 2 * kSlotR, 2 * kSlotR);
    g.setColour (juce::Colours::white.withAlpha (0.07f));
    g.drawEllipse (cx - kSlotR + 0.75f, cy - kSlotR + 0.75f, 2 * kSlotR - 1.5f, 2 * kSlotR - 1.5f, 1.5f);

    // title
    g.setColour (kLight);
    g.drawEllipse (cx - 62 - 11, cy - 88 - 11, 22, 22, 1.2f);
    g.setFont (font (bold, 13));
    g.drawText (juce::String (index), juce::Rectangle<float> (cx - 73, cy - 99, 22, 22), juce::Justification::centred);
    g.setFont (font (reg, 16));
    g.drawText (title, juce::Rectangle<float> (cx - 44, cy - 99, 120, 22), juce::Justification::centredLeft);

    const juce::Rectangle<float> box (cx - 80, cy - 68, 160, 46);
    if (empty)
    {
        insetBox (g, box, 8);
        const float dash[] = { 4.0f, 4.0f };
        juce::Path outline;
        outline.addRoundedRectangle (box.reduced (1), 8.0f);
        juce::Path dashed;
        juce::PathStrokeType (1.2f).createDashedStroke (dashed, outline, dash, 2);
        g.setColour (kMuted.withAlpha (0.6f));
        g.fillPath (dashed);
        g.setColour (kLight);
        g.drawEllipse (box.getX() + 18, box.getCentreY() - 10, 20, 20, 1.5f);
        g.drawLine (box.getX() + 28, box.getCentreY() - 5, box.getX() + 28, box.getCentreY() + 5, 1.5f);
        g.drawLine (box.getX() + 23, box.getCentreY(), box.getX() + 33, box.getCentreY(), 1.5f);
        g.setFont (font (reg, 13));
        g.drawText ("from map", juce::Rectangle<float> (box.getX() + 44, box.getCentreY() - 9, 90, 18), juce::Justification::centredLeft);
    }
    else
    {
        insetBox (g, box, 8);
        wave (g, box.getCentreX(), box.getCentreY(), 140, 17.0f, seed, 1.4f);
    }

    const char* names[] = { "Level", "Pitch", "Noise", "Hum", "Low cut", "Tail" };
    for (int i = 0; i < 6; ++i)
        knob (g, reg, cx + ((float) (i % 3) - 1.0f) * 52.0f, cy + 21.0f + (float) (i / 3) * 53.0f, 15.5f,
              p[i], names[i], empty);
}
} // namespace

NektarEditor::NektarEditor (NektarProcessor& p) : juce::AudioProcessorEditor (&p)
{
    regular = juce::Typeface::createSystemTypefaceFor (BinaryData::CarlitoRegular_ttf, BinaryData::CarlitoRegular_ttfSize);
    bold    = juce::Typeface::createSystemTypefaceFor (BinaryData::CarlitoBold_ttf, BinaryData::CarlitoBold_ttfSize);

    constrainer.setFixedAspectRatio (1.0);
    constrainer.setMinimumSize (500, 500);
    constrainer.setMaximumSize (2400, 2400);
    setConstrainer (&constrainer);
    setResizable (true, true);
    setSize (860, 860);
}

void NektarEditor::resized() {}

void NektarEditor::paint (Graphics& g)
{
    const float ps = g.getInternalContext().getPhysicalPixelScaleFactor();
    const int pw = juce::roundToInt ((float) getWidth() * ps);
    const int ph = juce::roundToInt ((float) getHeight() * ps);

    if (cache.getWidth() != pw || cache.getHeight() != ph)
    {
        cache = juce::Image (juce::Image::ARGB, pw, ph, true);
        Graphics ig (cache);
        ig.addTransform (juce::AffineTransform::scale ((float) pw / 1000.0f, (float) ph / 1000.0f));
        renderDesign (ig);
        addGrain (cache);
    }
    g.drawImage (cache, getLocalBounds().toFloat());
}

void NektarEditor::addGrain (juce::Image& img)
{
    juce::Image::BitmapData bd (img, juce::Image::BitmapData::readWrite);
    juce::Random rnd (7);
    for (int y = 0; y < bd.height; ++y)
        for (int x = 0; x < bd.width; ++x)
        {
            const int n = rnd.nextInt (11) - 5;
            const auto c = bd.getPixelColour (x, y);
            bd.setPixelColour (x, y, Colour ((juce::uint8) juce::jlimit (0, 255, (int) c.getRed() + n),
                                             (juce::uint8) juce::jlimit (0, 255, (int) c.getGreen() + n),
                                             (juce::uint8) juce::jlimit (0, 255, (int) c.getBlue() + n)));
        }
}

void NektarEditor::renderDesign (Graphics& g)
{
    const auto& reg = regular;
    const auto& bld = bold;

    // ---- background: taupe, light from the top-left
    {
        ColourGradient bg (Colour (0xff9a8a73), 280, 140, Colour (0xff4e4237), 1100, 1100, true);
        bg.addColour (0.45, Colour (0xff7b6b5b));
        g.setGradientFill (bg);
        g.fillAll();
        g.setGradientFill (ColourGradient (Colour (0xfffff4dc).withAlpha (0.14f), 260, 150,
                                           Colour (0xfffff4dc).withAlpha (0.0f), 260, 480, true));
        g.fillEllipse (-160, -150, 840, 600);
    }

    // ---- top bar
    shadowRect (g, { 0, 0, 1000, 60 }, 0, 0.5f, 14, { 0, 6 });
    g.setGradientFill (ColourGradient (Colour (0xff353331), 0, 0, Colour (0xff272523), 0, 60, false));
    g.fillRect (0.0f, 0.0f, 1000.0f, 60.0f);

    g.setColour (kAcc);
    g.setFont (font (bld, 26));
    g.drawText (juce::String (juce::CharPointer_UTF8 ("\xcf\x80/-")), juce::Rectangle<float> (22, 12, 70, 36), juce::Justification::centredLeft);
    g.setColour (kLight);
    g.setFont (font (reg, 19));
    g.drawText ("00:02.4", juce::Rectangle<float> (100, 14, 90, 32), juce::Justification::centredLeft);
    insetBox (g, { 196, 26, 110, 9 }, 4.5f);
    g.setColour (kLight);
    g.fillRoundedRectangle (196, 26, 52, 9, 4.5f);

    const float bx[] = { 398, 447, 497 };
    for (int i = 0; i < 3; ++i)
    {
        g.setColour (Colour (0xff1e1d1b));
        g.fillEllipse (bx[i] - 21, 9, 42, 42);
        g.setColour (kAcc);
        g.drawEllipse (bx[i] - 21, 9, 42, 42, 2.0f);
    }
    g.fillEllipse (bx[0] - 9, 21, 18, 18); // record
    for (int k = -2; k <= 2; ++k)           // room tone
    {
        const float h = 4.0f + (float) (3 - std::abs (k)) * 3.0f;
        g.drawLine (bx[1] + (float) k * 4.5f, 30 - h, bx[1] + (float) k * 4.5f, 30 + h, 2.0f);
    }
    {                                       // resample
        juce::Path arc;
        arc.addCentredArc (bx[2], 30, 9, 9, 0, 0.6f, 5.6f, true);
        g.strokePath (arc, juce::PathStrokeType (2.0f));
        juce::Path head;
        head.addTriangle (bx[2] + 5, 20, bx[2] + 13, 22, bx[2] + 8, 29);
        g.fillPath (head);
    }

    g.setColour (kAcc);
    g.fillRoundedRectangle (570, 11, 142, 38, 19);
    g.setColour (kDark);
    g.setFont (font (bld, 15));
    g.drawText ("Save to library", juce::Rectangle<float> (570, 11, 142, 38), juce::Justification::centred);

    knob (g, reg, 740, 30, 13, 0.6f, "");
    g.setColour (kMuted);
    g.setFont (font (reg, 13));
    g.drawText ("Out  -6.0 dB", juce::Rectangle<float> (760, 18, 90, 24), juce::Justification::centredLeft);
    g.setColour (kLight);
    g.setFont (font (reg, 32));
    g.drawText (juce::String (juce::CharPointer_UTF8 ("\xce\xbd\xce\xad\xce\xba\xcf\x84\xce\xb1\xcf\x81")),
                juce::Rectangle<float> (850, 8, 130, 44), juce::Justification::centredRight);

    // ---- style chips
    {
        const char* chips[] = { "All", "Glass", "Metal", "Wood", "Paper", "Water", "Voice" };
        const Colour cc[] = { kLight, kWada[0], kWada[1], kWada[2], kWada[3], kWada[2], kWada[0] };
        float x = 253;
        for (int i = 0; i < 7; ++i)
        {
            const juce::Rectangle<float> r (x, 77, 62, 26);
            if (i == 1)
            {
                g.setColour (kWada[0]);
                g.fillRoundedRectangle (r, 13);
                g.setColour (kDark);
            }
            else
            {
                g.setColour (Colour (0xff2f2d2b));
                g.fillRoundedRectangle (r, 13);
                g.setColour (cc[i].withAlpha (0.8f));
                g.drawRoundedRectangle (r, 13, 1.3f);
                g.setColour (kLight);
            }
            g.setFont (font (reg, 14));
            g.drawText (chips[i], r, juce::Justification::centred);
            x += 72;
        }
    }

    // ---- ring (library map): recessed bowl
    g.setColour (juce::Colours::white.withAlpha (0.16f));
    g.fillEllipse (kCx - kRingR + 4, kCy - kRingR + 5, 2 * kRingR, 2 * kRingR);
    g.setGradientFill (ColourGradient (Colour (0xff141312), kCx - 120, kCy - 150, Colour (0xff2a2826), kCx + 200, kCy + 230, true));
    g.fillEllipse (kCx - kRingR, kCy - kRingR, 2 * kRingR, 2 * kRingR);

    {
        juce::Random rnd (42);
        for (int i = 0; i < 70; ++i)
        {
            const float a = rnd.nextFloat() * juce::MathConstants<float>::twoPi;
            const float rr = std::sqrt (rnd.nextFloat()) * (kRingR - 18.0f);
            sphere (g, kCx + std::cos (a) * rr, kCy + std::sin (a) * rr, 3.8f + rnd.nextFloat() * 4.2f, kWada[i % 5]);
        }
        g.setColour (kAcc);
        g.drawEllipse (640 - 17, 290 - 17, 34, 34, 1.5f);
        g.setColour (kAcc.withAlpha (0.18f));
        g.fillEllipse (640 - 26, 290 - 26, 52, 52);
        g.setColour (kAcc);
        g.fillEllipse (640 - 6.5f, 290 - 6.5f, 13, 13);
    }

    // ---- centre screen
    g.setGradientFill (ColourGradient (kAcc.withAlpha (0.10f), kCx, kCy, kAcc.withAlpha (0.0f), kCx + 150, kCy, true));
    g.fillEllipse (kCx - 150, kCy - 150, 300, 300);
    shadowCircle (g, kCx, kCy, kScreenR, 0.6f, 26, { 9, 14 });
    g.setGradientFill (ColourGradient (Colour (0xff3a3836), kCx - 70, kCy - 80, Colour (0xff222120), kCx + 80, kCy + 100, false));
    g.fillEllipse (kCx - kScreenR, kCy - kScreenR, 2 * kScreenR, 2 * kScreenR);
    g.setGradientFill (ColourGradient (Colour (0xff181716), kCx - 40, kCy - 60, Colour (0xff201f1d), kCx + 60, kCy + 70, false));
    g.fillEllipse (kCx - 94, kCy - 94, 188, 188);

    g.setColour (kMuted);
    g.setFont (font (reg, 13));
    g.drawText (juce::String (juce::CharPointer_UTF8 ("Check \xc2\xb7 slot 1")), juce::Rectangle<float> (kCx - 80, kCy - 88, 160, 18), juce::Justification::centred);
    wave (g, kCx, kCy - 52, 114, 10.0f, 11, 1.2f, 34);
    const char* rows[] = { "Noise floor", "Hum 50 Hz", "Clipping" };
    const char* vals[] = { "-58 dB", "+3 harm.", "none" };
    for (int i = 0; i < 3; ++i)
    {
        const float y = kCy - 12 + (float) i * 22.0f;
        g.setColour (kAcc);
        if (i < 2) g.fillEllipse (kCx - 80, y - 5, 10, 10);
        else       g.drawEllipse (kCx - 79.5f, y - 4.5f, 9, 9, 1.3f);
        g.setColour (kLight);
        g.setFont (font (reg, 14));
        g.drawText (rows[i], juce::Rectangle<float> (kCx - 64, y - 9, 90, 18), juce::Justification::centredLeft);
        g.setColour (kMuted);
        g.setFont (font (reg, 13));
        g.drawText (vals[i], juce::Rectangle<float> (kCx + 10, y - 9, 70, 18), juce::Justification::centredRight);
    }

    // ---- four slots
    const float p1[] = { 0.82f, 0.62f, 0.50f, 0.95f, 0.18f, 0.88f };
    const float p2[] = { 0.40f, 0.70f, 0.55f, 0.92f, 0.45f, 0.80f };
    const float p4[] = { 0.55f, 0.38f, 0.30f, 0.10f, 0.22f, 0.72f };
    const float p3[] = { 0.5f, 0.5f, 0.5f, 0.5f, 0.2f, 0.7f };
    slotDisc (g, reg, bld, 205, 262, 1, "Glass tap", false, p1, 3);
    slotDisc (g, reg, bld, 795, 262, 2, "Paper rub", false, p2, 5);
    slotDisc (g, reg, bld, 205, 590, 3, "Empty", true, p3, 0);
    slotDisc (g, reg, bld, 795, 590, 4, "Bowl hum", false, p4, 9);

    // ---- module panel
    const float PX = 130, PY = 772, PW = 740, PH = 192;
    shadowRect (g, { PX, PY, PW, PH }, 14, 0.55f, 26, { 10, 14 });
    g.setGradientFill (ColourGradient (Colour (0xff393735), PX, PY, Colour (0xff242220), PX + PW, PY + PH, false));
    g.fillRoundedRectangle (PX, PY, PW, PH, 14);
    g.setColour (juce::Colours::white.withAlpha (0.06f));
    g.drawRoundedRectangle (PX + 0.5f, PY + 0.5f, PW - 1, PH - 1, 14, 1.2f);

    struct Mod { const char* name; juce::String value; const char* k[3]; float p[3]; };
    const Mod mods[] = {
        { "Time",      juce::CharPointer_UTF8 ("\xc3\x97" "1.8 \xc2\xb7 +0 st"), { "Stretch", "Pitch", "Loop" },  { 0.45f, 0.72f, 0.90f } },
        { "Grain",     juce::CharPointer_UTF8 ("90 ms \xc2\xb7 soft"),             { "Size", "Density", "Scatter" }, { 0.55f, 0.68f, 0.30f } },
        { "Spectral",  juce::CharPointer_UTF8 ("blur \xc2\xb7 warm"),              { "Morph", "Blur", "Freeze" },    { 0.50f, 0.78f, 0.04f } },
        { "Resonator", "follows key",                                              { "Tune", "Decay", "Material" },  { 0.62f, 0.85f, 0.42f } },
    };
    for (int m = 0; m < 4; ++m)
    {
        const float mx = PX + (float) m * (PW / 4.0f);
        if (m > 0)
        {
            g.setColour (juce::Colours::black.withAlpha (0.3f));
            g.drawLine (mx, PY + 14, mx, PY + PH - 14, 1.0f);
        }
        insetBox (g, { mx + 10, PY + 12, PW / 4.0f - 20, 28 }, 7);
        g.setColour (kLight);
        g.setFont (font (bld, 15));
        g.drawText (mods[m].name, juce::Rectangle<float> (mx + 18, PY + 12, 80, 28), juce::Justification::centredLeft);
        g.setColour (kMuted);
        g.setFont (font (reg, 12));
        g.drawText (mods[m].value, juce::Rectangle<float> (mx + 70, PY + 12, PW / 4.0f - 88, 28), juce::Justification::centredRight);
        for (int k = 0; k < 3; ++k)
            knob (g, reg, mx + 34 + (float) k * 58.0f, PY + 100, 22, mods[m].p[k], mods[m].k[k]);
    }
}
