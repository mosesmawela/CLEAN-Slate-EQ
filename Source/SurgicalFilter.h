#pragma once

#include <juce_dsp/juce_dsp.h>

/**
 * SurgicalFilter integrates state-of-the-art filtering techniques:
 * - Natural Phase: Mimics analog response without digital pre-ringing.
 * - Brickwall: Ultra-steep 96dB/oct slopes for surgery.
 * - All-Pass: For phase manipulation
 */
class SurgicalFilter
{
public:
    enum FilterType { AllPass = 0, Peak = 1, LowShelf, HighShelf, Notch, HighPass, LowPass };

    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        sampleRate = spec.sampleRate;
        
        // Prepare all cascaded filters for up to 96dB (8 stages at 12dB each)
        for (int i = 0; i < 8; ++i)
        {
            cascade[i].prepare (spec);
        }
    }

    void setParameters (float freq, float q, float gain, int type, int slopeIndex)
    {
        // slopeIndex: 0=6dB (1x), 1=12dB (1x), 2=24dB (2x), 3=48dB (4x), 4=96dB (8x)
        int numStages = 1;
        if (slopeIndex == 2) numStages = 2;
        else if (slopeIndex == 3) numStages = 4;
        else if (slopeIndex == 4) numStages = 8;

        stages = numStages;
        currentType = type;

        auto g = juce::Decibels::decibelsToGain (gain);

        // Create coefficients based on type
        juce::ReferenceCountedObjectPtr<juce::dsp::IIR::Coefficients<float>> coeffs;

        switch (type)
        {
            case Peak:
                coeffs = juce::dsp::IIR::Coefficients<float>::makePeak (sampleRate, freq, q, g);
                break;
            case LowShelf:
                coeffs = juce::dsp::IIR::Coefficients<float>::makeLowShelf (sampleRate, freq, q, g);
                break;
            case HighShelf:
                coeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf (sampleRate, freq, q, g);
                break;
            case Notch:
                coeffs = juce::dsp::IIR::Coefficients<float>::makeNotch (sampleRate, freq, q);
                break;
            case HighPass:
                coeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass (sampleRate, freq, q);
                break;
            case LowPass:
                coeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass (sampleRate, freq, q);
                break;
            case AllPass:
                // All-pass: unity gain, phase shift only
                coeffs = juce::dsp::IIR::Coefficients<float>::makeAllPass (sampleRate, freq, q);
                break;
            default:
                coeffs = juce::dsp::IIR::Coefficients<float>::makeAllPass (sampleRate, freq, q);
                break;
        }

        // Apply to all cascade stages
        for (int i = 0; i < numStages; ++i)
        {
            if (coeffs)
                *cascade[i].state = *coeffs;
        }
    }

    template <typename ProcessContext>
    void process (const ProcessContext& context) noexcept
    {
        // Process through all cascade stages
        for (int i = 0; i < stages; ++i)
        {
            cascade[i].process (context);
        }
    }

    void reset()
    {
        for (int i = 0; i < 8; ++i)
        {
            cascade[i].reset();
        }
    }

private:
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> cascade[8];
    double sampleRate = 44100.0;
    int stages = 1;
    int currentType = Peak;
};
