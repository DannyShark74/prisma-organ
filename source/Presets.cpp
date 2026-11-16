#include "ParamTable.h"

namespace prisma
{
namespace
{
    using V = std::map<std::string, float>;

    // drawbars as a registration string "88 8000 000" (spaces ignored)
    void bars (V& v, const char* reg)
    {
        int k = 0;
        for (const char* c = reg; *c && k < 9; ++c)
            if (*c >= '0' && *c <= '8') v["db" + std::to_string (++k)] = (float) (*c - '0');
    }
    void heads (V& v, const char* h)   // e.g. "1010"
    {
        for (int k = 0; k < 4 && h[k]; ++k) v["echo_h" + std::to_string (k + 1)] = h[k] == '1' ? 1.0f : 0.0f;
    }
    // combo tabs in order: Bass16 Str16 Fl8 Ob8 Tr8 Str8 Fl4 Str4 2-2/3 2
    void tabs (V& v, const char* t)
    {
        static const char* ids[10] = { "cb_bass16", "cb_str16", "cb_flute8", "cb_oboe8", "cb_trump8",
                                       "cb_str8", "cb_flute4", "cb_str4", "cb_q223", "cb_2" };
        for (int k = 0; k < 10 && t[k]; ++k) v[ids[k]] = t[k] == '1' ? 1.0f : 0.0f;
    }
    V tone (const char* reg, V extra = {})
    {
        V v { { "tw_level", 0.8f }, { "cb_level", 0.0f } };
        bars (v, reg);
        for (auto& kv : extra) v[kv.first] = kv.second;
        return v;
    }
    V combo (const char* t, V extra = {})
    {
        V v { { "tw_level", 0.0f }, { "cb_level", 0.85f }, { "rot_on", 0.0f } };
        tabs (v, t);
        for (auto& kv : extra) v[kv.first] = kv.second;
        return v;
    }
    V withHeads (V v, const char* h) { heads (v, h); return v; }
}

const std::vector<Preset>& factoryPresets()
{
    static const std::vector<Preset> list = []
    {
        std::vector<Preset> p;
        auto add = [&p] (const char* cat, const char* name, V v) { p.push_back ({ name, cat, std::move (v) }); };

        // ---------------- Floyd 70s (tonewheel + rotary)
        add ("Floyd 70s", "Classic 888 Slow", tone ("888 000 000", { { "vib_mode", 6 }, { "rot_speed", 1 } }));
        add ("Floyd 70s", "Pompeii Spinet", tone ("888 600 000", { { "vib_mode", 6 }, { "rot_cab", 1 }, { "drive", 0.28f },
                                                                     { "room_mix", 0.25f }, { "click", 0.6f } }));
        add ("Floyd 70s", "Echoes Organ", withHeads (tone ("008 800 000", { { "vib_mode", 6 }, { "rot_speed", 1 }, { "echo_on", 1 },
                                                                            { "echo_mode", 1 }, { "echo_time", 75 }, { "echo_fb", 0.35f },
                                                                            { "echo_mix", 0.3f }, { "room_mix", 0.35f }, { "room_size", 0.6f } }), "1010"));
        add ("Floyd 70s", "Dark Side Pad", tone ("668 604 000", { { "vib_mode", 6 }, { "drive", 0.1f }, { "room_mix", 0.35f },
                                                                  { "room_size", 0.6f }, { "click", 0.25f } }));
        add ("Floyd 70s", "Wish Organ", withHeads (tone ("888 400 000", { { "vib_mode", 6 }, { "drive", 0.22f }, { "room_mix", 0.3f },
                                                                          { "echo_on", 1 }, { "echo_mode", 0 }, { "echo_mix", 0.15f },
                                                                          { "echo_time", 90 } }), "1000"));
        add ("Floyd 70s", "Shine Swell", withHeads (tone ("868 000 000", { { "vib_mode", 6 }, { "click", 0.15f }, { "echo_on", 1 },
                                                                           { "echo_mode", 2 }, { "echo_fb", 0.38f }, { "echo_mix", 0.22f },
                                                                           { "echo_tone", 0.35f }, { "room_mix", 0.5f }, { "room_size", 0.85f } }), "1111"));
        add ("Floyd 70s", "Animals Grind", tone ("888 800 000", { { "vib_mode", 0 }, { "drive", 0.55f }, { "rot_speed", 1 },
                                                                  { "click", 0.7f }, { "room_mix", 0.18f } }));
        add ("Floyd 70s", "Layered Floyd", withHeads (tone ("008 800 000", { { "tw_level", 0.7f }, { "cb_level", 0.45f }, { "cb_flute8", 1 },
                                                                             { "cb_str8", 1 }, { "cb_vib", 1 }, { "echo_on", 1 }, { "echo_mode", 1 },
                                                                             { "echo_fb", 0.4f }, { "echo_mix", 0.3f }, { "room_mix", 0.35f } }), "1010"));

        // ---------------- Psych 60s (combo + drum echo)
        add ("Psych 60s", "Interstellar Combo", withHeads (combo ("0010011000", { { "cb_vib", 1 }, { "echo_on", 1 }, { "echo_mode", 1 },
                                                                                  { "echo_time", 80 }, { "echo_fb", 0.45f }, { "echo_mix", 0.4f },
                                                                                  { "room_mix", 0.2f } }), "1010"));
        add ("Psych 60s", "Astronomy Swell", withHeads (combo ("0000110100", { { "cb_vib", 2 }, { "cb_vibfast", 1 }, { "echo_on", 1 },
                                                                               { "echo_mode", 2 }, { "echo_fb", 0.55f }, { "echo_mix", 0.45f },
                                                                               { "echo_wear", 0.5f }, { "room_mix", 0.25f } }), "1111"));
        add ("Psych 60s", "Heart Of The Sun", withHeads (combo ("0011000000", { { "cb_vib", 1 }, { "wah_on", 1 }, { "wah_mode", 1 },
                                                                                { "wah_rate", 0.35f }, { "wah_res", 0.6f }, { "echo_on", 1 },
                                                                                { "echo_mode", 1 }, { "echo_time", 90 }, { "echo_fb", 0.5f },
                                                                                { "echo_mix", 0.4f }, { "room_mix", 0.3f } }), "0101"));
        add ("Psych 60s", "Careful Axe", withHeads (combo ("1010000000", { { "cb_vib", 1 }, { "cb_tone", 0.35f }, { "echo_on", 1 },
                                                                           { "echo_mode", 2 }, { "echo_fb", 0.62f }, { "echo_tone", 0.35f },
                                                                           { "echo_mix", 0.5f }, { "room_mix", 0.35f }, { "room_size", 0.7f } }), "1111"));
        add ("Psych 60s", "Saucer Drone", withHeads (combo ("0100010010", { { "cb_age", 0.7f }, { "cb_vib", 0 }, { "echo_on", 1 },
                                                                            { "echo_mode", 2 }, { "echo_fb", 0.7f }, { "echo_mix", 0.5f },
                                                                            { "echo_wear", 0.6f }, { "room_mix", 0.5f }, { "room_size", 0.9f } }), "1111"));
        add ("Psych 60s", "Bright Combo Dry", combo ("0000111101", { { "cb_vib", 1 }, { "cb_tone", 0.6f }, { "room_mix", 0.1f } }));
        add ("Psych 60s", "Combo Spinning", combo ("0010010000", { { "cb_rotary", 1 }, { "rot_on", 1 }, { "rot_speed", 2 },
                                                                     { "rot_cab", 1 }, { "cb_vib", 0 }, { "room_mix", 0.2f } }));

        // ---------------- R&B / Soul
        add ("R&B / Soul", "R&B Warm Pad", tone ("808 000 000", { { "vib_mode", 6 }, { "drive", 0.1f }, { "click", 0.2f }, { "room_mix", 0.25f } }));
        add ("R&B / Soul", "Neo Soul Perc", tone ("888 000 000", { { "perc_on", 1 }, { "perc_third", 1 }, { "perc_soft", 1 },
                                                                   { "vib_mode", 5 }, { "drive", 0.15f }, { "room_mix", 0.2f } }));
        add ("R&B / Soul", "Dark R&B Low", withHeads (tone ("806 000 000", { { "tw_deep", 1 }, { "vib_mode", 6 }, { "drive", 0.2f },
                                                                             { "echo_on", 1 }, { "echo_mode", 1 }, { "echo_time", 120 },
                                                                             { "echo_fb", 0.3f }, { "echo_mix", 0.2f }, { "echo_tone", 0.35f },
                                                                             { "room_mix", 0.3f }, { "click", 0.2f } }), "0010"));
        add ("R&B / Soul", "Slow Jam Chorale", tone ("688 600 000", { { "vib_mode", 5 }, { "click", 0.2f }, { "room_mix", 0.3f } }));
        add ("R&B / Soul", "Trap Soul Organ", withHeads (tone ("800 800 000", { { "tw_deep", 1 }, { "vib_mode", 6 }, { "rot_speed", 0 },
                                                                                { "echo_on", 1 }, { "echo_mode", 1 }, { "echo_time", 110 },
                                                                                { "echo_fb", 0.3f }, { "echo_mix", 0.25f }, { "room_mix", 0.4f },
                                                                                { "click", 0.15f } }), "0101"));
        add ("R&B / Soul", "Guitar Bed", tone ("608 400 000", { { "vib_mode", 6 }, { "rot_width", 1.0f }, { "drive", 0.12f },
                                                                { "click", 0.15f }, { "room_mix", 0.3f } }));

        // ---------------- Gospel & Jazz
        add ("Gospel & Jazz", "Gospel Full", tone ("888 888 888", { { "rot_speed", 2 }, { "drive", 0.35f }, { "vib_mode", 6 }, { "click", 0.7f } }));
        add ("Gospel & Jazz", "Jazz Trio Perc", tone ("888 000 000", { { "perc_on", 1 }, { "perc_third", 1 }, { "vib_mode", 0 },
                                                                       { "click", 0.75f }, { "leak", 0.5f }, { "room_mix", 0.15f } }));
        add ("Gospel & Jazz", "Soul Jazz Second", tone ("888 000 000", { { "perc_on", 1 }, { "perc_third", 0 }, { "vib_mode", 0 },
                                                                         { "click", 0.7f }, { "drive", 0.25f } }));
        add ("Gospel & Jazz", "Sunday Ballad", tone ("408 400 002", { { "vib_mode", 6 }, { "click", 0.3f }, { "room_mix", 0.3f } }));

        // ---------------- Rock
        add ("Rock", "Rock Growl", tone ("888 800 000", { { "drive", 0.62f }, { "vib_mode", 0 }, { "click", 0.8f } }));
        add ("Rock", "Overdrive Fast", tone ("888 000 000", { { "drive", 0.72f }, { "rot_speed", 2 }, { "vib_mode", 0 }, { "click", 0.8f } }));
        add ("Rock", "Wah Organ", tone ("888 400 000", { { "wah_on", 1 }, { "wah_mode", 1 }, { "wah_rate", 0.8f }, { "wah_pos", 0.35f }, { "wah_res", 0.65f }, { "drive", 0.3f } }));

        // ---------------- Dark & Low
        add ("Dark & Low", "Sub Organ", tone ("800 000 000", { { "tw_level", 1.0f }, { "master", -3.0f }, { "tw_deep", 1 }, { "octave", -1 }, { "vib_mode", 6 }, { "drive", 0.15f },
                                                               { "click", 0.1f }, { "room_mix", 0.3f } }));
        add ("Dark & Low", "Dark Chapel", tone ("868 000 000", { { "tw_deep", 1 }, { "rot_speed", 0 }, { "vib_mode", 6 }, { "click", 0.1f },
                                                                 { "room_mix", 0.7f }, { "room_size", 0.95f } }));
        add ("Dark & Low", "Haunted Combo", withHeads (combo ("1100000000", { { "cb_vib", 2 }, { "cb_age", 0.8f }, { "cb_tone", 0.3f },
                                                                              { "echo_on", 1 }, { "echo_mode", 2 }, { "echo_fb", 0.5f },
                                                                              { "echo_mix", 0.4f }, { "room_mix", 0.6f }, { "room_size", 0.9f } }), "1111"));

        // ---------------- Ambient
        add ("Ambient", "Endless Swell", withHeads (tone ("006 000 006", { { "tw_level", 1.0f }, { "master", -2.0f }, { "vib_mode", 6 }, { "click", 0.05f }, { "echo_on", 1 },
                                                                           { "echo_mode", 2 }, { "echo_fb", 0.5f }, { "echo_mix", 0.4f },
                                                                           { "echo_tone", 0.4f }, { "room_mix", 0.6f }, { "room_size", 0.9f } }), "1111"));
        add ("Ambient", "Glass Combo", withHeads (combo ("0000001011", { { "cb_vib", 1 }, { "cb_vibfast", 1 }, { "echo_on", 1 },
                                                                         { "echo_mode", 1 }, { "echo_time", 140 }, { "echo_fb", 0.5f },
                                                                         { "echo_mix", 0.35f }, { "echo_spread", 1.0f }, { "room_mix", 0.5f } }), "0101"));
        return p;
    }();
    return list;
}
}
