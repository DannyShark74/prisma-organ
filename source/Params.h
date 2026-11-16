#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

namespace prisma::pid
{
    // global
    inline constexpr const char* master = "master";
    inline constexpr const char* octave = "octave";
    inline constexpr const char* twLevel = "tw_level";
    inline constexpr const char* cbLevel = "cb_level";

    // tonewheel
    inline const char* drawbar (int i)
    {
        static const char* ids[9] = { "db1", "db2", "db3", "db4", "db5", "db6", "db7", "db8", "db9" };
        return ids[i];
    }
    inline constexpr const char* percOn = "perc_on";
    inline constexpr const char* percThird = "perc_third";
    inline constexpr const char* percFast = "perc_fast";
    inline constexpr const char* percSoft = "perc_soft";
    inline constexpr const char* vibMode = "vib_mode";
    inline constexpr const char* click = "click";
    inline constexpr const char* leak = "leak";
    inline constexpr const char* deep = "tw_deep";

    // combo
    inline constexpr int kComboTabs = 10;
    inline const char* comboTab (int i)
    {
        static const char* ids[kComboTabs] = { "cb_bass16", "cb_str16", "cb_flute8", "cb_oboe8", "cb_trump8",
                                               "cb_str8", "cb_flute4", "cb_str4", "cb_q223", "cb_2" };
        return ids[i];
    }
    inline const char* comboTabName (int i)
    {
        static const char* n[kComboTabs] = { "Bass 16'", "Strings 16'", "Flute 8'", "Oboe 8'", "Trumpet 8'",
                                             "Strings 8'", "Flute 4'", "Strings 4'", "2 2/3'", "2'" };
        return n[i];
    }
    inline constexpr const char* cbVib = "cb_vib";
    inline constexpr const char* cbVibFast = "cb_vibfast";
    inline constexpr const char* cbTone = "cb_tone";
    inline constexpr const char* cbAge = "cb_age";
    inline constexpr const char* cbRotary = "cb_rotary";

    // wah
    inline constexpr const char* wahOn = "wah_on";
    inline constexpr const char* wahMode = "wah_mode";
    inline constexpr const char* wahPos = "wah_pos";
    inline constexpr const char* wahRes = "wah_res";
    inline constexpr const char* wahRate = "wah_rate";

    // drive
    inline constexpr const char* drive = "drive";
    inline constexpr const char* driveTone = "drive_tone";

    // rotary
    inline constexpr const char* rotOn = "rot_on";
    inline constexpr const char* rotSpeed = "rot_speed";
    inline constexpr const char* rotCab = "rot_cab";
    inline constexpr const char* rotBalance = "rot_balance";
    inline constexpr const char* rotWidth = "rot_width";
    inline constexpr const char* rotInertia = "rot_inertia";

    // echo
    inline constexpr const char* echoOn = "echo_on";
    inline constexpr const char* echoMode = "echo_mode";
    inline const char* echoHead (int i)
    {
        static const char* ids[4] = { "echo_h1", "echo_h2", "echo_h3", "echo_h4" };
        return ids[i];
    }
    inline constexpr const char* echoTime = "echo_time";
    inline constexpr const char* echoFb = "echo_fb";
    inline constexpr const char* echoTone = "echo_tone";
    inline constexpr const char* echoWear = "echo_wear";
    inline constexpr const char* echoSpread = "echo_spread";
    inline constexpr const char* echoMix = "echo_mix";

    // room
    inline constexpr const char* roomSize = "room_size";
    inline constexpr const char* roomMix = "room_mix";
}

namespace prisma
{
    juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
}
