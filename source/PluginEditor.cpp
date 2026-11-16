#include "PluginEditor.h"

using namespace prisma;
using namespace prisma::ui;

namespace
{
    juce::Font font (float h, bool bold = false)
    {
        return juce::Font (juce::FontOptions (h, bold ? juce::Font::bold : juce::Font::plain));
    }
    constexpr int kW = 1120, kH = 690;
}

// =================================================================== LookAndFeel
LookAndFeel::LookAndFeel()
{
    setColour (juce::ComboBox::textColourId, col::cream);
    setColour (juce::PopupMenu::backgroundColourId, col::module);
    setColour (juce::PopupMenu::textColourId, col::cream);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, col::amber);
    setColour (juce::PopupMenu::highlightedTextColourId, col::ink);
    setColour (juce::PopupMenu::headerTextColourId, col::amber);
    setColour (juce::TextButton::buttonColourId, col::module);
    setColour (juce::TextButton::textColourOffId, col::cream);
    setColour (juce::ComboBox::outlineColourId, col::moduleEdge);
}

void LookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float start, float end, juce::Slider& s)
{
    const float size = (float) std::min (w, h) - 10.0f;
    const float r = size * 0.5f;
    const float cx = (float) x + (float) w * 0.5f, cy = (float) y + (float) h * 0.5f;
    const float ang = start + pos * (end - start);

    juce::Path track;
    track.addCentredArc (cx, cy, r + 3.5f, r + 3.5f, 0.0f, start, end, true);
    g.setColour (juce::Colour (0xff3d3832));
    g.strokePath (track, juce::PathStrokeType (2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const bool bipolar = s.getMinimum() < 0.0 && s.getMaximum() > 0.0;
    const float from = bipolar ? (start + end) * 0.5f : start;
    juce::Path val;
    val.addCentredArc (cx, cy, r + 3.5f, r + 3.5f, 0.0f, std::min (from, ang), std::max (from, ang), true);
    g.setColour (col::amber);
    g.strokePath (val, juce::PathStrokeType (2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // skirt
    g.setColour (juce::Colour (0xff0e0d0c));
    g.fillEllipse (cx - r, cy - r, size, size);
    // cap
    const float cr = r * 0.78f;
    juce::ColourGradient grad (juce::Colour (0xfff6eedb), cx - cr * 0.4f, cy - cr * 0.6f, juce::Colour (0xffb7a988), cx + cr, cy + cr, true);
    g.setGradientFill (grad);
    g.fillEllipse (cx - cr, cy - cr, cr * 2, cr * 2);
    g.setColour (juce::Colour (0x55000000));
    g.drawEllipse (cx - cr, cy - cr, cr * 2, cr * 2, 1.0f);

    // pointer
    juce::Path p;
    p.addRoundedRectangle (-1.6f, -r * 0.95f, 3.2f, r * 0.55f, 1.5f);
    g.setColour (col::ink);
    g.fillPath (p, juce::AffineTransform::rotation (ang).translated (cx, cy));
}

void LookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox&)
{
    auto b = juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);
    g.setColour (juce::Colour (0xff100e0c));
    g.fillRoundedRectangle (b, 5.0f);
    g.setColour (col::amber.withAlpha (0.7f));
    g.drawRoundedRectangle (b, 5.0f, 1.0f);
    juce::Path arrow;
    const float ax = (float) w - 18.0f, ay = (float) h * 0.5f;
    arrow.addTriangle (ax - 5, ay - 3, ax + 5, ay - 3, ax, ay + 4);
    g.setColour (col::amber);
    g.fillPath (arrow);
}

juce::Font LookAndFeel::getComboBoxFont (juce::ComboBox&) { return font (16.0f, true); }

void LookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (10, 1, box.getWidth() - 34, box.getHeight() - 2);
    label.setFont (getComboBoxFont (box));
    label.setJustificationType (juce::Justification::centred);
}

void LookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int w, int h)
{
    g.fillAll (col::module);
    g.setColour (col::moduleEdge);
    g.drawRect (0, 0, w, h);
}

// =================================================================== Knob
Knob::Knob (juce::AudioProcessorValueTreeState& s, const char* paramId, juce::String cap, bool isBig)
    : caption (std::move (cap)), big (isBig)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setRotaryParameters (juce::degreesToRadians (-135.0f), juce::degreesToRadians (135.0f), true);
    slider.setPopupDisplayEnabled (true, true, nullptr);
    slider.setDoubleClickReturnValue (true, 0.0);
    addAndMakeVisible (slider);
    att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (s, paramId, slider);
    if (auto* p = s.getParameter (paramId))
        slider.setDoubleClickReturnValue (true, p->convertFrom0to1 (p->getDefaultValue()));
}

void Knob::resized()
{
    auto b = getLocalBounds();
    b.removeFromBottom (16);
    const int d = std::min (b.getWidth(), b.getHeight());
    slider.setBounds (b.withSizeKeepingCentre (d, d));
}

void Knob::paint (juce::Graphics& g)
{
    g.setColour (col::creamDim);
    g.setFont (font (big ? 12.5f : 11.5f, true));
    g.drawText (caption.toUpperCase(), getLocalBounds().removeFromBottom (16), juce::Justification::centred);
}

// =================================================================== Drawbar
Drawbar::Drawbar (juce::Colour c, juce::String f) : cap (c), footage (std::move (f))
{
    setSliderStyle (juce::Slider::LinearVertical);
    setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    setRange (0.0, 8.0, 1.0);
    setPopupDisplayEnabled (true, true, nullptr);
    setMouseDragSensitivity (220);
    setSliderSnapsToMousePosition (false);
}

void Drawbar::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    const float labelH = 18.0f, capH = 34.0f;
    g.setColour (col::cream);
    g.setFont (font (12.0f, true));
    g.drawText (footage, b.removeFromTop (labelH), juce::Justification::centred);

    const float cx = b.getCentreX();
    const float travel = b.getHeight() - capH;
    const float v = (float) getValue() / 8.0f;
    const float shaftLen = v * travel;

    // slot in the panel
    g.setColour (juce::Colour (0xff120a06));
    g.fillRoundedRectangle (cx - 8, b.getY() - 2, 16, 8, 3);

    // shaft with the numbers that appear as it is pulled out
    const bool light = cap.getBrightness() > 0.6f;
    const juce::Colour shaft = cap.interpolatedWith (juce::Colours::grey, 0.15f);
    juce::Rectangle<float> sh (cx - 7, b.getY(), 14, shaftLen + 4);
    g.setColour (shaft);
    g.fillRect (sh);
    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.fillRect (sh.removeFromRight (3));
    g.setFont (font (9.5f, true));
    for (int k = 1; k <= 8; ++k)
    {
        const float yk = b.getY() + travel * (float) k / 8.0f - travel / 16.0f;
        if (yk > b.getY() + shaftLen) break;
        g.setColour (light ? juce::Colour (0xff3a2a20) : col::creamDim);
        g.drawText (juce::String (k), juce::Rectangle<float> (cx - 7, yk - 6, 12, 12), juce::Justification::centred);
    }

    // cap
    juce::Rectangle<float> c (cx - 16, b.getY() + shaftLen, 32, capH);
    juce::ColourGradient grad (cap.brighter (0.35f), c.getX(), c.getY(), cap.darker (0.35f), c.getRight(), c.getBottom(), false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (c, 6.0f);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawRoundedRectangle (c, 6.0f, 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.25f));
    g.fillRoundedRectangle (c.reduced (5, 4).withHeight (5), 2.0f);
    g.setColour (light ? col::ink : col::cream);
    g.setFont (font (13.0f, true));
    g.drawText (juce::String ((int) getValue()), c.withTrimmedTop (10), juce::Justification::centred);
}

// =================================================================== Tab
Tab::Tab (juce::AudioProcessorValueTreeState& s, const char* paramId, juce::String label, Style st, juce::Colour c)
    : style (st), colour (c)
{
    setButtonText (label);
    att = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (s, paramId, *this);
}

void Tab::paintButton (juce::Graphics& g, bool over, bool)
{
    auto b = getLocalBounds().toFloat();
    const bool on = getToggleState();
    switch (style)
    {
        case Rocker:
        {
            auto textArea = b.removeFromBottom (30.0f);
            auto body = b.reduced (3.0f, 2.0f);
            const auto base = on ? colour : colour.darker (0.55f);
            juce::ColourGradient grad (on ? base.darker (0.25f) : base.brighter (0.3f), body.getX(), body.getY(),
                                       on ? base.brighter (0.25f) : base.darker (0.3f), body.getX(), body.getBottom(), false);
            g.setGradientFill (grad);
            g.fillRoundedRectangle (body, 4.0f);
            g.setColour (juce::Colours::black.withAlpha (0.6f));
            g.drawRoundedRectangle (body, 4.0f, 1.0f);
            // the pressed half of the rocker
            auto half = on ? body.withTrimmedTop (body.getHeight() * 0.5f) : body.withHeight (body.getHeight() * 0.5f);
            g.setColour (juce::Colours::white.withAlpha (on ? 0.18f : 0.08f));
            g.fillRoundedRectangle (half.reduced (2.0f), 3.0f);
            if (on)
            {
                g.setColour (col::led);
                g.fillEllipse (body.getCentreX() - 3, body.getY() + 6, 6, 6);
            }
            g.setColour (over ? col::cream : col::creamDim);
            g.setFont (font (9.5f, true));
            g.drawFittedText (getButtonText().replace (" ", "\n"), textArea.toNearestInt(), juce::Justification::centredTop, 2, 0.7f);
            break;
        }
        case Pill:
        {
            b = b.reduced (1.0f);
            g.setColour (on ? col::amber : juce::Colour (0xff100e0c));
            g.fillRoundedRectangle (b, b.getHeight() * 0.5f);
            g.setColour (on ? col::amber.darker (0.3f) : col::moduleEdge.brighter (0.2f));
            g.drawRoundedRectangle (b, b.getHeight() * 0.5f, 1.0f);
            g.setColour (on ? col::ink : (over ? col::cream : col::creamDim));
            g.setFont (font (11.5f, true));
            g.drawText (getButtonText(), b, juce::Justification::centred);
            break;
        }
        case Power:
        {
            auto led = b.removeFromLeft (b.getHeight()).reduced (3.0f);
            if (on)
            {
                g.setColour (col::led.withAlpha (0.35f));
                g.fillEllipse (led.expanded (2.5f));
            }
            g.setColour (on ? col::led : juce::Colour (0xff4a2a24));
            g.fillEllipse (led);
            g.setColour (on ? col::cream : col::creamDim);
            g.setFont (font (11.0f, true));
            g.drawText (getButtonText(), b.withTrimmedLeft (3), juce::Justification::centredLeft);
            break;
        }
        case Head:
        {
            b = b.reduced (1.5f);
            g.setColour (on ? col::amber : juce::Colour (0xff100e0c));
            g.fillRoundedRectangle (b, 4.0f);
            g.setColour (col::moduleEdge.brighter (0.3f));
            g.drawRoundedRectangle (b, 4.0f, 1.0f);
            g.setColour (on ? col::ink : col::creamDim);
            g.setFont (font (13.0f, true));
            g.drawText (getButtonText(), b, juce::Justification::centred);
            break;
        }
    }
}

// =================================================================== Segments
Segments::Segments (juce::AudioProcessorValueTreeState& s, const char* paramId, juce::StringArray l) : labels (std::move (l))
{
    param = s.getParameter (paramId);
    jassert (param != nullptr);
    att = std::make_unique<juce::ParameterAttachment> (*param, [this] (float v)
    {
        index = juce::roundToInt (param->convertTo0to1 (v) * (float) (labels.size() - 1));
        repaint();
    });
    att->sendInitialUpdate();
}

void Segments::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (juce::Colour (0xff100e0c));
    g.fillRoundedRectangle (b, 5.0f);
    const float w = b.getWidth() / (float) labels.size();
    for (int i = 0; i < labels.size(); ++i)
    {
        auto seg = juce::Rectangle<float> (b.getX() + w * (float) i, b.getY(), w, b.getHeight());
        const bool sel = i == index;
        if (sel)
        {
            g.setColour (col::amber);
            g.fillRoundedRectangle (seg.reduced (2.0f), 4.0f);
        }
        if (i == overrideIndex && ! sel)
        {
            g.setColour (col::amber.withAlpha (0.8f));
            g.drawRoundedRectangle (seg.reduced (2.5f), 4.0f, 1.5f);
        }
        g.setColour (sel ? col::ink : col::creamDim);
        g.setFont (font (std::min (12.5f, b.getHeight() * 0.48f), true));
        g.drawText (labels[i], seg, juce::Justification::centred);
        if (i > 0)
        {
            g.setColour (col::moduleEdge);
            g.drawVerticalLine ((int) seg.getX(), b.getY() + 4, b.getBottom() - 4);
        }
    }
    g.setColour (col::moduleEdge.brighter (0.2f));
    g.drawRoundedRectangle (b, 5.0f, 1.0f);
}

void Segments::mouseDown (const juce::MouseEvent& e)
{
    const int i = juce::jlimit (0, labels.size() - 1, (int) ((float) e.x / ((float) getWidth() / (float) labels.size())));
    att->setValueAsCompleteGesture (param->convertFrom0to1 ((float) i / (float) (labels.size() - 1)));
}

// =================================================================== Vibrato selector
VibratoSelector::VibratoSelector (juce::AudioProcessorValueTreeState& s)
{
    auto* p = s.getParameter (pid::vibMode);
    att = std::make_unique<juce::ParameterAttachment> (*p, [this] (float v) { index = juce::roundToInt (v); repaint(); });
    att->sendInitialUpdate();
}

juce::Point<float> VibratoSelector::centre() const { return { (float) getWidth() * 0.5f, (float) getHeight() * 0.56f }; }
float VibratoSelector::angleFor (int i) const { return juce::degreesToRadians (-120.0f + 40.0f * (float) i); }

void VibratoSelector::paint (juce::Graphics& g)
{
    static const char* names[7] = { "OFF", "V1", "V2", "V3", "C1", "C2", "C3" };
    const auto c = centre();
    const float r = std::min ((float) getWidth(), (float) getHeight()) * 0.27f;
    g.setFont (font (10.5f, true));
    for (int i = 0; i < 7; ++i)
    {
        const float a = angleFor (i);
        const auto pt = c.getPointOnCircumference (r + 17.0f, a);
        g.setColour (i == index ? col::amber : col::creamDim);
        g.drawText (names[i], juce::Rectangle<float> (26, 13).withCentre (pt), juce::Justification::centred);
        g.setColour (col::creamDim.withAlpha (0.5f));
        g.drawLine (juce::Line<float> (c.getPointOnCircumference (r + 2.0f, a), c.getPointOnCircumference (r + 6.0f, a)), 1.2f);
    }
    // chicken-head knob
    g.setColour (juce::Colour (0xff0e0d0c));
    g.fillEllipse (juce::Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (c));
    juce::Path head;
    head.startNewSubPath (0.0f, -r * 1.05f);
    head.quadraticTo (r * 0.42f, -r * 0.2f, r * 0.28f, r * 0.55f);
    head.lineTo (-r * 0.28f, r * 0.55f);
    head.quadraticTo (-r * 0.42f, -r * 0.2f, 0.0f, -r * 1.05f);
    head.closeSubPath();
    const auto tf = juce::AffineTransform::rotation (angleFor (index)).translated (c.x, c.y);
    g.setColour (juce::Colour (0xff1c1a18));
    g.fillPath (head, tf);
    g.setColour (col::cream);
    g.strokePath (head, juce::PathStrokeType (1.0f), tf);
    juce::Path tick;
    tick.addRectangle (-1.0f, -r * 0.95f, 2.0f, r * 0.5f);
    g.fillPath (tick, tf);
    g.setColour (col::creamDim);
    g.setFont (font (11.0f, true));
    g.drawText ("VIBRATO / CHORUS", getLocalBounds().removeFromBottom (14), juce::Justification::centred);
}

void VibratoSelector::mouseDown (const juce::MouseEvent& e)
{
    const auto c = centre();
    const float a = std::atan2 ((float) e.x - c.x, c.y - (float) e.y);
    int best = 0;
    float bestD = 10.0f;
    for (int i = 0; i < 7; ++i)
    {
        const float d = std::abs (a - angleFor (i));
        if (d < bestD) { bestD = d; best = i; }
    }
    att->setValueAsCompleteGesture ((float) best);
}

void VibratoSelector::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w)
{
    const int i = juce::jlimit (0, 6, index + (w.deltaY > 0 ? 1 : -1));
    att->setValueAsCompleteGesture ((float) i);
}

// =================================================================== Rotor view
void RotorView::advance (float h, float d, float dt)
{
    hornHz = h; drumHz = d;
    horn += h * dt; horn -= std::floor (horn);
    drum += d * dt; drum -= std::floor (drum);
    repaint();
}

void RotorView::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    auto top = b.removeFromTop (b.getHeight() * 0.5f);
    auto drawRotor = [&g] (juce::Rectangle<float> area, float turn, bool isHorn, float hz)
    {
        const auto c = area.getCentre();
        const float r = std::min (area.getWidth(), area.getHeight()) * 0.38f;
        g.setColour (juce::Colour (0xff100e0c));
        g.fillEllipse (juce::Rectangle<float> (r * 2.2f, r * 2.2f).withCentre (c));
        g.setColour (col::moduleEdge.brighter (0.2f));
        g.drawEllipse (juce::Rectangle<float> (r * 2.2f, r * 2.2f).withCentre (c), 1.0f);
        const float a = turn * juce::MathConstants<float>::twoPi;
        // perspective: the rotor seen from above, squashed
        const auto tf = juce::AffineTransform::rotation (a).scaled (1.0f, 1.0f).translated (c.x, c.y);
        juce::Path p;
        if (isHorn)
        {
            p.startNewSubPath (-r, -2.0f); p.lineTo (-r * 0.15f, -1.5f); p.lineTo (r * 0.15f, -1.5f);
            p.lineTo (r, -r * 0.35f); p.lineTo (r, r * 0.35f); p.lineTo (r * 0.15f, 1.5f); p.lineTo (-r * 0.15f, 1.5f);
            p.lineTo (-r, 2.0f); p.closeSubPath();
        }
        else
            p.addPieSegment (-r, -r, r * 2, r * 2, 0.0f, juce::MathConstants<float>::pi, 0.0f);
        g.setColour (col::amber.withAlpha (0.85f));
        g.fillPath (p, tf);
        g.setColour (col::creamDim);
        g.setFont (font (10.0f, true));
        g.drawText (juce::String (isHorn ? "HORN " : "DRUM ") + juce::String (juce::roundToInt (hz * 60.0f)) + " rpm",
                    area.removeFromBottom (12).translated (0, 2), juce::Justification::centred);
    };
    drawRotor (top, horn, true, hornHz);
    drawRotor (b, drum, false, drumHz);
}

// =================================================================== Editor
PrismaEditor::PrismaEditor (PrismaProcessor& p) : AudioProcessorEditor (p), proc (p)
{
    setLookAndFeel (&lnf);
    auto& s = proc.apvts;

    addAndMakeVisible (presetBox);
    presetBox.setJustificationType (juce::Justification::centred);
    fillPresetMenu();
    presetBox.onChange = [this]
    {
        const int i = presetBox.getSelectedId() - 1;
        if (i >= 0 && i != proc.getCurrentProgram()) { proc.setCurrentProgram (i); shownProgram = i; }
    };
    for (auto* b : { &prevBtn, &nextBtn }) addAndMakeVisible (*b);
    prevBtn.onClick = [this] { step (-1); };
    nextBtn.onClick = [this] { step (1); };
    octave = std::make_unique<Segments> (s, pid::octave, juce::StringArray { "-2", "-1", "0", "+1" });
    master = std::make_unique<Knob> (s, pid::master, "Volume");
    addAndMakeVisible (*octave); addAndMakeVisible (*master);

    // tonewheel
    const juce::Colour brown (0xff7a3f1c), white (0xffece6d6), black (0xff1d1b1a);
    const juce::Colour caps[9] = { brown, brown, white, white, black, white, black, black, white };
    const char* foot[9] = { "16'", "5 1/3'", "8'", "4'", "2 2/3'", "2'", "1 3/5'", "1 1/3'", "1'" };
    for (int i = 0; i < 9; ++i)
    {
        drawbars.push_back (std::make_unique<Drawbar> (caps[i], foot[i]));
        addAndMakeVisible (*drawbars.back());
        drawbarAtts.push_back (std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (s, pid::drawbar (i), *drawbars.back()));
    }
    percOn = std::make_unique<Tab> (s, pid::percOn, "PERC ON", Tab::Rocker, col::cream);
    percThird = std::make_unique<Tab> (s, pid::percThird, "3RD", Tab::Rocker, col::cream);
    percFast = std::make_unique<Tab> (s, pid::percFast, "FAST", Tab::Rocker, col::cream);
    percSoft = std::make_unique<Tab> (s, pid::percSoft, "SOFT", Tab::Rocker, col::cream);
    deep = std::make_unique<Tab> (s, pid::deep, "DEEP 16'", Tab::Pill);
    vib = std::make_unique<VibratoSelector> (s);
    click = std::make_unique<Knob> (s, pid::click, "Click");
    leak = std::make_unique<Knob> (s, pid::leak, "Leakage");
    twLevel = std::make_unique<Knob> (s, pid::twLevel, "Level", true);
    for (juce::Component* c : std::initializer_list<juce::Component*> { percOn.get(), percThird.get(), percFast.get(), percSoft.get(),
                                                                       deep.get(), vib.get(), click.get(), leak.get(), twLevel.get() })
        addAndMakeVisible (c);

    // combo
    const juce::Colour tabCol[10] = { juce::Colour (0xff8c8c86), juce::Colour (0xff232120), col::cream, juce::Colour (0xffe0702a),
                                      juce::Colour (0xffe0702a), juce::Colour (0xff232120), col::cream, juce::Colour (0xff232120),
                                      col::cream, col::cream };
    static const char* tabText[10] = { "BASS 16'", "STRINGS 16'", "FLUTE 8'", "OBOE 8'", "TRUMPET 8'",
                                       "STRINGS 8'", "FLUTE 4'", "STRINGS 4'", "2 2/3'", "2'" };
    for (int i = 0; i < pid::kComboTabs; ++i)
    {
        comboTabs.push_back (std::make_unique<Tab> (s, pid::comboTab (i), tabText[i], Tab::Rocker, tabCol[i]));
        addAndMakeVisible (*comboTabs.back());
    }
    cbVib = std::make_unique<Segments> (s, pid::cbVib, juce::StringArray { "OFF", "LIGHT", "HEAVY" });
    cbVibFast = std::make_unique<Tab> (s, pid::cbVibFast, "FAST", Tab::Pill);
    cbRotary = std::make_unique<Tab> (s, pid::cbRotary, "INTO ROTARY", Tab::Pill);
    cbTone = std::make_unique<Knob> (s, pid::cbTone, "Tone");
    cbAge = std::make_unique<Knob> (s, pid::cbAge, "Age");
    cbLevel = std::make_unique<Knob> (s, pid::cbLevel, "Level", true);
    for (juce::Component* c : std::initializer_list<juce::Component*> { cbVib.get(), cbVibFast.get(), cbRotary.get(), cbTone.get(),
                                                                       cbAge.get(), cbLevel.get() })
        addAndMakeVisible (c);

    // effects
    wahOn = std::make_unique<Tab> (s, pid::wahOn, "ON", Tab::Power);
    wahMode = std::make_unique<Segments> (s, pid::wahMode, juce::StringArray { "PEDAL", "LFO", "ENV" });
    wahPos = std::make_unique<Knob> (s, pid::wahPos, "Pedal", true);
    wahRes = std::make_unique<Knob> (s, pid::wahRes, "Reso");
    wahRate = std::make_unique<Knob> (s, pid::wahRate, "Rate");
    drive = std::make_unique<Knob> (s, pid::drive, "Drive", true);
    driveTone = std::make_unique<Knob> (s, pid::driveTone, "Tone");
    rotOn = std::make_unique<Tab> (s, pid::rotOn, "ON", Tab::Power);
    rotSpeed = std::make_unique<Segments> (s, pid::rotSpeed, juce::StringArray { "STOP", "SLOW", "FAST" });
    rotCab = std::make_unique<Segments> (s, pid::rotCab, juce::StringArray { "LARGE", "SMALL" });
    rotBalance = std::make_unique<Knob> (s, pid::rotBalance, "Drum/Horn");
    rotWidth = std::make_unique<Knob> (s, pid::rotWidth, "Width");
    rotInertia = std::make_unique<Knob> (s, pid::rotInertia, "Inertia");
    echoOn = std::make_unique<Tab> (s, pid::echoOn, "ON", Tab::Power);
    echoMode = std::make_unique<Segments> (s, pid::echoMode, juce::StringArray { "ECHO", "REPEAT", "SWELL" });
    for (int i = 0; i < 4; ++i)
    {
        heads.push_back (std::make_unique<Tab> (s, pid::echoHead (i), juce::String (i + 1), Tab::Head));
        addAndMakeVisible (*heads.back());
    }
    echoTime = std::make_unique<Knob> (s, pid::echoTime, "Time", true);
    echoFb = std::make_unique<Knob> (s, pid::echoFb, "Swell", true);
    echoMix = std::make_unique<Knob> (s, pid::echoMix, "Mix", true);
    echoTone = std::make_unique<Knob> (s, pid::echoTone, "Tone");
    echoWear = std::make_unique<Knob> (s, pid::echoWear, "Wear");
    echoSpread = std::make_unique<Knob> (s, pid::echoSpread, "Spread");
    roomSize = std::make_unique<Knob> (s, pid::roomSize, "Size", true);
    roomMix = std::make_unique<Knob> (s, pid::roomMix, "Mix", true);
    for (juce::Component* c : std::initializer_list<juce::Component*> {
             wahOn.get(), wahMode.get(), wahPos.get(), wahRes.get(), wahRate.get(), drive.get(), driveTone.get(), rotOn.get(),
             rotSpeed.get(), rotCab.get(), rotBalance.get(), rotWidth.get(), rotInertia.get(), echoOn.get(), echoMode.get(),
             echoTime.get(), echoFb.get(), echoMix.get(), echoTone.get(), echoWear.get(), echoSpread.get(), roomSize.get(),
             roomMix.get(), &rotor })
        addAndMakeVisible (c);

    setSize (kW, kH);
    lastTick = juce::Time::getMillisecondCounterHiRes();
    startTimerHz (30);
}

PrismaEditor::~PrismaEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void PrismaEditor::fillPresetMenu()
{
    presetBox.clear (juce::dontSendNotification);
    std::string lastCat;
    const auto& list = factoryPresets();
    for (size_t i = 0; i < list.size(); ++i)
    {
        if (list[i].category != lastCat)
        {
            presetBox.addSectionHeading (list[i].category);
            lastCat = list[i].category;
        }
        presetBox.addItem (list[i].name, (int) i + 1);
    }
    presetBox.setSelectedId (proc.getCurrentProgram() + 1, juce::dontSendNotification);
    shownProgram = proc.getCurrentProgram();
}

void PrismaEditor::step (int delta)
{
    const int n = proc.getNumPrograms();
    const int i = ((proc.getCurrentProgram() + delta) % n + n) % n;
    proc.setCurrentProgram (i);
    presetBox.setSelectedId (i + 1, juce::dontSendNotification);
    shownProgram = i;
}

void PrismaEditor::timerCallback()
{
    const double now = juce::Time::getMillisecondCounterHiRes();
    const float dt = (float) std::min (0.1, (now - lastTick) * 0.001);
    lastTick = now;
    rotor.advance (proc.getHornSpeed(), proc.getDrumSpeed(), dt);
    rotSpeed->setHighlightOverride (proc.getRotaryOverride());
    if (proc.getCurrentProgram() != shownProgram)
    {
        shownProgram = proc.getCurrentProgram();
        presetBox.setSelectedId (shownProgram + 1, juce::dontSendNotification);
    }
}

void PrismaEditor::paint (juce::Graphics& g)
{
    // background
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff24170f), 0, 0, juce::Colour (0xff0f0a07), 0, (float) kH, false));
    g.fillAll();

    // logo
    auto logo = header.withWidth (300).toFloat().withTrimmedLeft (18);
    juce::ColourGradient lg (juce::Colour (0xfff6c453), logo.getX(), 0, juce::Colour (0xffe0452c), logo.getX() + 170, 0, false);
    lg.addColour (0.5, juce::Colour (0xffef8a2f));
    g.setGradientFill (lg);
    g.setFont (font (34.0f, true).withExtraKerningFactor (0.18f));
    g.drawText ("PRISMA", logo.withHeight (44).translated (0, 6), juce::Justification::centredLeft);
    g.setColour (col::creamDim);
    g.setFont (font (11.5f, true).withExtraKerningFactor (0.25f));
    g.drawText ("PSYCHEDELIC ORGAN", logo.withY (48).withHeight (14), juce::Justification::centredLeft);
    g.setFont (font (11.0f, true));
    g.drawText ("OCTAVE", octave->getBounds().withY (10).withHeight (12), juce::Justification::centred);

    // tonewheel panel: wood
    {
        auto r = twPanel.toFloat();
        g.setGradientFill (juce::ColourGradient (col::wood1, r.getX(), r.getY(), col::wood2, r.getRight(), r.getBottom(), false));
        g.fillRoundedRectangle (r, 8.0f);
        juce::Random rnd (17);
        for (int i = 0; i < 70; ++i)
        {
            const float y = r.getY() + rnd.nextFloat() * r.getHeight();
            g.setColour (juce::Colours::black.withAlpha (0.04f + 0.05f * rnd.nextFloat()));
            g.drawLine (r.getX() + 6, y, r.getRight() - 6, y + rnd.nextFloat() * 6 - 3, 0.6f + rnd.nextFloat());
        }
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.drawRoundedRectangle (r, 8.0f, 1.5f);
        // drawbar bed
        auto bed = juce::Rectangle<float> (r.getX() + 12, r.getY() + 52, 390, 4);
        g.setColour (juce::Colour (0xff1a0e07));
        g.fillRoundedRectangle (bed.expanded (0, 3), 3);
        g.setColour (juce::Colour (0xffe9c46a));
        g.setFont (font (14.0f, true).withExtraKerningFactor (0.2f));
        g.drawText ("TONEWHEEL", r.reduced (16, 9).withHeight (20), juce::Justification::topLeft);
        g.setColour (col::cream.withAlpha (0.8f));
        g.setFont (font (11.5f, true));
        g.drawText ("PERCUSSION", juce::Rectangle<float> (r.getX() + 420, r.getY() + 12, 210, 14), juce::Justification::centred);
    }
    // combo panel: red tolex with a chrome strip
    {
        auto r = cbPanel.toFloat();
        g.setGradientFill (juce::ColourGradient (col::tolex1, r.getX(), r.getY(), col::tolex2, r.getX(), r.getBottom(), false));
        g.fillRoundedRectangle (r, 8.0f);
        juce::Random rnd (5);
        for (int i = 0; i < 900; ++i)
        {
            g.setColour (juce::Colours::black.withAlpha (0.06f * rnd.nextFloat()));
            g.fillRect (r.getX() + rnd.nextFloat() * r.getWidth(), r.getY() + rnd.nextFloat() * r.getHeight(), 2.0f, 2.0f);
        }
        auto strip = juce::Rectangle<float> (r.getX() + 8, r.getY() + 32, r.getWidth() - 16, 3);
        g.setGradientFill (juce::ColourGradient (col::chrome, strip.getX(), strip.getY(), juce::Colour (0xff8a8a85), strip.getX(), strip.getBottom(), false));
        g.fillRect (strip);
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.drawRoundedRectangle (r, 8.0f, 1.5f);
        g.setColour (col::cream);
        g.setFont (font (14.0f, true).withExtraKerningFactor (0.2f));
        g.drawText ("COMBO", r.reduced (16, 9).withHeight (20), juce::Justification::topLeft);
        g.setColour (col::cream.withAlpha (0.8f));
        g.setFont (font (11.0f, true));
        g.drawText ("VIBRATO", juce::Rectangle<float> (r.getX() + 16, r.getY() + 186, 160, 14), juce::Justification::centredLeft);
    }
    // effect modules
    auto moduleBg = [&g] (juce::Rectangle<int> rr, const char* title)
    {
        auto r = rr.toFloat();
        g.setColour (col::module);
        g.fillRoundedRectangle (r, 8.0f);
        g.setColour (col::moduleEdge);
        g.drawRoundedRectangle (r, 8.0f, 1.0f);
        g.setColour (col::amber);
        g.setFont (font (13.0f, true).withExtraKerningFactor (0.2f));
        g.drawText (title, r.reduced (12, 8).withHeight (18), juce::Justification::topLeft);
    };
    moduleBg (wahBox, "WAH");
    moduleBg (driveBox, "DRIVE");
    moduleBg (rotBox, "ROTARY");
    moduleBg (echoBox, "DRUM ECHO");
    moduleBg (roomBox, "ROOM");
    g.setColour (col::creamDim);
    g.setFont (font (10.5f, true));
    g.drawText ("HEADS", juce::Rectangle<int> (echoBox.getX() + 214, echoBox.getY() + 66, 128, 12), juce::Justification::centred);
    g.drawText ("MOD WHEEL = FAST", juce::Rectangle<int> (rotBox.getX() + 12, rotBox.getY() + 108, 160, 12), juce::Justification::centredLeft);
}

void PrismaEditor::resized()
{
    header = { 0, 0, kW, 64 };
    presetBox.setBounds (380, 16, 320, 34);
    prevBtn.setBounds (344, 18, 30, 30);
    nextBtn.setBounds (706, 18, 30, 30);
    octave->setBounds (presetBox.getRight() + 62, 26, 150, 24);
    master->setBounds (kW - 86, 4, 70, 60);

    twPanel = { 12, 70, 640, 304 };
    cbPanel = { 664, 70, 444, 304 };

    // drawbars
    for (int i = 0; i < 9; ++i)
        drawbars[(size_t) i]->setBounds (twPanel.getX() + 16 + i * 43, twPanel.getY() + 34, 40, 264);
    const int px = twPanel.getX() + 420;
    percOn->setBounds (px, twPanel.getY() + 30, 52, 76);
    percThird->setBounds (px + 53, twPanel.getY() + 30, 52, 76);
    percFast->setBounds (px + 106, twPanel.getY() + 30, 52, 76);
    percSoft->setBounds (px + 159, twPanel.getY() + 30, 52, 76);
    vib->setBounds (px - 6, twPanel.getY() + 112, 128, 120);
    twLevel->setBounds (px + 128, twPanel.getY() + 124, 84, 96);
    click->setBounds (px, twPanel.getY() + 236, 58, 62);
    leak->setBounds (px + 62, twPanel.getY() + 236, 58, 62);
    deep->setBounds (px + 130, twPanel.getY() + 254, 78, 24);

    for (int i = 0; i < pid::kComboTabs; ++i)
        comboTabs[(size_t) i]->setBounds (cbPanel.getX() + 12 + i * 42, cbPanel.getY() + 46, 42, 126);
    cbVib->setBounds (cbPanel.getX() + 16, cbPanel.getY() + 204, 170, 26);
    cbVibFast->setBounds (cbPanel.getX() + 192, cbPanel.getY() + 204, 62, 26);
    // tone / age sit under the vibrato row, level on the right
    cbRotary->setBounds (cbPanel.getX() + 16, cbPanel.getY() + 240, 110, 26);
    cbTone->setBounds (cbPanel.getX() + 136, cbPanel.getY() + 234, 56, 66);
    cbAge->setBounds (cbPanel.getX() + 196, cbPanel.getY() + 234, 56, 66);
    cbLevel->setBounds (cbPanel.getX() + 344, cbPanel.getY() + 196, 86, 100);

    const int my = 386, mh = 292;
    wahBox = { 12, my, 180, mh };
    driveBox = { wahBox.getRight() + 10, my, 120, mh };
    rotBox = { driveBox.getRight() + 10, my, 290, mh };
    echoBox = { rotBox.getRight() + 10, my, 344, mh };
    roomBox = { echoBox.getRight() + 10, my, kW - 12 - echoBox.getRight() - 10, mh };

    auto power = [] (juce::Rectangle<int> box) { return juce::Rectangle<int> (box.getRight() - 56, box.getY() + 8, 46, 18); };
    wahOn->setBounds (power (wahBox));
    rotOn->setBounds (power (rotBox));
    echoOn->setBounds (power (echoBox));

    wahMode->setBounds (wahBox.getX() + 12, wahBox.getY() + 38, 156, 26);
    wahPos->setBounds (wahBox.getX() + 40, wahBox.getY() + 76, 100, 112);
    wahRes->setBounds (wahBox.getX() + 14, wahBox.getY() + 198, 74, 82);
    wahRate->setBounds (wahBox.getX() + 92, wahBox.getY() + 198, 74, 82);

    drive->setBounds (driveBox.getX() + 10, driveBox.getY() + 46, 100, 112);
    driveTone->setBounds (driveBox.getX() + 22, driveBox.getY() + 190, 76, 84);

    rotSpeed->setBounds (rotBox.getX() + 12, rotBox.getY() + 38, 266, 32);
    rotCab->setBounds (rotBox.getX() + 12, rotBox.getY() + 78, 160, 24);
    rotor.setBounds (rotBox.getX() + 182, rotBox.getY() + 76, 98, 116);
    rotBalance->setBounds (rotBox.getX() + 12, rotBox.getY() + 196, 86, 86);
    rotWidth->setBounds (rotBox.getX() + 102, rotBox.getY() + 196, 86, 86);
    rotInertia->setBounds (rotBox.getX() + 192, rotBox.getY() + 196, 86, 86);

    echoMode->setBounds (echoBox.getX() + 12, echoBox.getY() + 38, 196, 26);
    for (int i = 0; i < 4; ++i)
        heads[(size_t) i]->setBounds (echoBox.getX() + 218 + i * 30, echoBox.getY() + 36, 30, 30);
    echoTime->setBounds (echoBox.getX() + 8, echoBox.getY() + 82, 108, 108);
    echoFb->setBounds (echoBox.getX() + 118, echoBox.getY() + 82, 108, 108);
    echoMix->setBounds (echoBox.getX() + 228, echoBox.getY() + 82, 108, 108);
    echoTone->setBounds (echoBox.getX() + 24, echoBox.getY() + 200, 80, 82);
    echoWear->setBounds (echoBox.getX() + 132, echoBox.getY() + 200, 80, 82);
    echoSpread->setBounds (echoBox.getX() + 240, echoBox.getY() + 200, 80, 82);

    roomSize->setBounds (roomBox.getX() + 8, roomBox.getY() + 40, roomBox.getWidth() - 16, 112);
    roomMix->setBounds (roomBox.getX() + 8, roomBox.getY() + 164, roomBox.getWidth() - 16, 112);
}
