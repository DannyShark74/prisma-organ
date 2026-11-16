#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

namespace prisma::ui
{
namespace col
{
    const juce::Colour bg { 0xff1a120d }, cream { 0xffefe3c8 }, creamDim { 0xffb9ab90 }, amber { 0xfff0a830 },
        ink { 0xff141210 }, module { 0xff22201d }, moduleEdge { 0xff3a3530 }, wood1 { 0xff6a3d22 }, wood2 { 0xff3b2214 },
        tolex1 { 0xff8e2420 }, tolex2 { 0xff571512 }, chrome { 0xffd9d9d4 }, led { 0xffff5a3c };
}

class LookAndFeel : public juce::LookAndFeel_V4
{
public:
    LookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float start, float end, juce::Slider&) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int, int, int, int, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    void drawPopupMenuBackground (juce::Graphics&, int w, int h) override;
};

// Knob with its caption underneath
class Knob : public juce::Component
{
public:
    Knob (juce::AudioProcessorValueTreeState& s, const char* paramId, juce::String caption, bool big = false);
    void resized() override;
    void paint (juce::Graphics&) override;
    juce::Slider slider;

private:
    juce::String caption;
    bool big;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
};

// Drawbar: pulled DOWN for more, like the real thing
class Drawbar : public juce::Slider
{
public:
    Drawbar (juce::Colour cap, juce::String footage);
    void paint (juce::Graphics&) override;
    double proportionOfLengthToValue (double p) override { return Slider::proportionOfLengthToValue (1.0 - p); }
    double valueToProportionOfLength (double v) override { return 1.0 - Slider::valueToProportionOfLength (v); }

private:
    juce::Colour cap;
    juce::String footage;
};

// Rocker / tab switch bound to a bool parameter
class Tab : public juce::ToggleButton
{
public:
    enum Style { Rocker, Pill, Power, Head };
    Tab (juce::AudioProcessorValueTreeState& s, const char* paramId, juce::String text, Style st, juce::Colour c = col::cream);
    void paintButton (juce::Graphics&, bool over, bool down) override;

private:
    Style style;
    juce::Colour colour;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> att;
};

// Segmented selector for a choice parameter
class Segments : public juce::Component
{
public:
    Segments (juce::AudioProcessorValueTreeState& s, const char* paramId, juce::StringArray labels);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    int getIndex() const { return index; }
    void setHighlightOverride (int i) { if (overrideIndex != i) { overrideIndex = i; repaint(); } }

private:
    juce::StringArray labels;
    int index = 0, overrideIndex = -1;
    juce::RangedAudioParameter* param = nullptr;
    std::unique_ptr<juce::ParameterAttachment> att;
};

// The chicken-head vibrato / chorus selector
class VibratoSelector : public juce::Component
{
public:
    VibratoSelector (juce::AudioProcessorValueTreeState& s);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    juce::Point<float> centre() const;
    float angleFor (int i) const;
    int index = 0;
    std::unique_ptr<juce::ParameterAttachment> att;
};

// Spinning horn / drum indicator
class RotorView : public juce::Component
{
public:
    void paint (juce::Graphics&) override;
    void advance (float hornHz, float drumHz, float dt);

private:
    float horn = 0, drum = 0.3f, hornHz = 0, drumHz = 0;
};
}

class PrismaEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit PrismaEditor (PrismaProcessor&);
    ~PrismaEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void fillPresetMenu();
    void step (int delta);
    juce::Rectangle<int> twPanel, cbPanel, wahBox, driveBox, rotBox, echoBox, roomBox, header;

    PrismaProcessor& proc;
    prisma::ui::LookAndFeel lnf;

    juce::ComboBox presetBox;
    juce::TextButton prevBtn { "<" }, nextBtn { ">" };
    std::unique_ptr<prisma::ui::Segments> octave;
    std::unique_ptr<prisma::ui::Knob> master;

    std::vector<std::unique_ptr<prisma::ui::Drawbar>> drawbars;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> drawbarAtts;
    std::unique_ptr<prisma::ui::Tab> percOn, percThird, percFast, percSoft, deep;
    std::unique_ptr<prisma::ui::VibratoSelector> vib;
    std::unique_ptr<prisma::ui::Knob> click, leak, twLevel;

    std::vector<std::unique_ptr<prisma::ui::Tab>> comboTabs;
    std::unique_ptr<prisma::ui::Segments> cbVib;
    std::unique_ptr<prisma::ui::Tab> cbVibFast, cbRotary;
    std::unique_ptr<prisma::ui::Knob> cbTone, cbAge, cbLevel;

    std::unique_ptr<prisma::ui::Tab> wahOn, rotOn, echoOn;
    std::unique_ptr<prisma::ui::Segments> wahMode, rotSpeed, rotCab, echoMode;
    std::unique_ptr<prisma::ui::Knob> wahPos, wahRes, wahRate, drive, driveTone, rotBalance, rotWidth, rotInertia,
        echoTime, echoFb, echoTone, echoWear, echoSpread, echoMix, roomSize, roomMix;
    std::vector<std::unique_ptr<prisma::ui::Tab>> heads;
    prisma::ui::RotorView rotor;

    int shownProgram = -1;
    double lastTick = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PrismaEditor)
};
