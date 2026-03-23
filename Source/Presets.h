#pragma once
#include <juce_core/juce_core.h>
#include <vector>

namespace Presets {

struct PresetBand {
    float frequency;
    float gain;
    float q;
    int type; // 0: HP, 1: Peak, 2: LP, 3: Notch, 4: LowShelf, 5: HighShelf
    bool active;
};

struct Preset {
    juce::String name;
    juce::String category;
    std::vector<PresetBand> bands;
    float outputGain;
};

static inline std::vector<Preset> getFactoryPresets() {
    return {
        { "Default (Total Flat)", "FACTORY", {}, 0.5f },
        { "Sub-Clear 30Hz", "UTILITY", {{ 30.0f, 0.0f, 0.707f, 0, true }}, 0.5f },
        { "Bass Focus 60Hz", "UTILITY", {{ 60.0f, 0.0f, 0.707f, 0, true }}, 0.5f },
        { "Rumble Killer 80Hz", "UTILITY", {{ 80.0f, 0.0f, 0.707f, 0, true }}, 0.5f },
        { "Harshness Kill 4kHz", "UTILITY", {{ 4000.0f, -12.0f, 8.0f, 3, true }}, 0.5f },
        { "50Hz AC Hum Remover", "UTILITY", {{ 50.0f, -24.0f, 10.0f, 3, true }}, 0.5f },
        { "Air Seal 18kHz", "UTILITY", {{ 18000.0f, 0.0f, 0.707f, 2, true }}, 0.5f },
        { "Hiss Cut 15kHz", "UTILITY", {{ 15000.0f, 0.0f, 0.707f, 2, true }}, 0.5f },
        { "Vintage Roll-off", "UTILITY", {{ 10000.0f, 0.0f, 0.5f, 2, true }}, 0.52f },
        { "Underwater Effect", "UTILITY", {{ 300.0f, 0.0f, 1.0f, 2, true }}, 0.6f },
        { "Telephone Vocal", "FX", {
            { 400.0f, 0.0f, 0.707f, 0, true },
            { 3000.0f, 0.0f, 0.707f, 2, true },
            { 1500.0f, 6.0f, 1.5f, 1, true }
        }, 0.45f },
        { "Walkie Talkie", "FX", {
            { 1000.0f, 0.0f, 0.707f, 0, true },
            { 4000.0f, 0.0f, 0.707f, 2, true },
            { 2500.0f, 8.0f, 2.0f, 1, true }
        }, 0.42f },
        { "Log Thump Stabilizer", "AMAPIANO", {{ 50.0f, -2.0f, 4.0f, 1, true }}, 0.5f },
        { "Shaker Air", "AMAPIANO", {{ 12000.0f, 3.0f, 0.7f, 5, true }}, 0.48f },
        { "Vocal Clarity Focus", "AMAPIANO", {{ 300.0f, -3.0f, 2.0f, 1, true }}, 0.52f },
        { "KICK | Sub Weight", "DRUMS", {{ 40.0f, 4.0f, 1.5f, 1, true }}, 0.45f },
        { "LOG | Wood Knock", "LOG & BASS", {{ 1200.0f, 5.0f, 3.5f, 1, true }}, 0.45f },
        { "MIX BUS | Amapiano Master", "MASTERING", {
            { 30.0f, 0.0f, 1.0f, 0, true },
            { 120.0f, 1.2f, 0.7f, 4, true },
            { 10000.0f, 1.5f, 0.7f, 5, true }
        }, 0.49f },
        { "[AI] | Bad Vocal Rescue", "AI SURGICAL", {
            { 350.0f, -4.5f, 1.2f, 1, true },
            { 8000.0f, 3.2f, 0.7f, 5, true }
        }, 0.5f }
    };
}

}
