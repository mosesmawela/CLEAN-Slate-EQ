#pragma once

#include <juce_dsp/juce_dsp.h>
#include <cmath>

/**
 * SurgicalFilter integrates state-of-the-art filtering techniques:
 * - Dynamic EQ: Threshold-based gain reduction with envelope following.
 * - Character: Transformer (tanh) and Tube (asymmetrical) saturation.
 * - Surgical Slopes: 6dB to 96dB via 8-stage cascading.
 */
class SurgicalFilter
{
public:
    enum FilterType { AllPass = 0, Peak = 1, LowShelf, HighShelf, Notch, HighPass, LowPass };

    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        sampleRate = spec.sampleRate;
        for (int i = 0; i < 8; ++i)
            cascade[i].prepare (spec);
    }

    void setParameters (float freq, float q, float gain, int type, int slopeIndex,
                        float threshold = 0.0f, float ratio = 1.0f, float attack = 0.1f, float release = 0.1f,
                        bool isDynamic = false, int character = 0)
    {
        bool paramsChanged = (currentFreq != freq || currentQ != q || currentGain != gain || currentType != type || this->isDynamic != isDynamic);
        
        this->currentFreq = freq;
        this->currentQ = q;
        this->currentGain = gain;
        this->currentType = type;
        this->isDynamic = isDynamic;
        this->charMode = character;
        
        // slopeIndex: 0=6dB, 1=12dB, 2=24dB, 3=48dB, 4=96dB
        int oldStages = stages;
        bool oldIsFirstOrder = isFirstOrder;
        
        isFirstOrder = (slopeIndex == 0);
        stages = 1;
        if (slopeIndex == 2) stages = 2;
        else if (slopeIndex == 3) stages = 4;
        else if (slopeIndex == 4) stages = 8;

        if (paramsChanged || stages != oldStages || isFirstOrder != oldIsFirstOrder)
        {
            lastGain = -999.0f; // Force update
            updateCoefficients (currentGain);
        }
    }

    template <typename ProcessContext>
    void process (const ProcessContext& context) noexcept
    {
        // Internal dynamic EQ logic removed as it's now handled in PluginProcessor 
        // per-sample for better accuracy.

        for (int i = 0; i < stages; ++i)
            cascade[i].process (context);

        // Saturation (Post-filter)
        if (charMode > 0)
        {
            auto outputBlock = context.getOutputBlock();
            for (size_t chan = 0; chan < outputBlock.getNumChannels(); ++chan)
            {
                auto* dst = outputBlock.getChannelPointer (chan);
                for (size_t i = 0; i < outputBlock.getNumSamples(); ++i)
                {
                    if (charMode == 1) // Subtle
                        dst[i] = std::tanh (dst[i] * 1.05f) / 1.05f;
                    else if (charMode == 2) // Warm
                        dst[i] = dst[i] > 0 ? (dst[i] - 0.15f * dst[i] * dst[i]) : dst[i];
                }
            }
        }
    }

    void reset()
    {
        for (int i = 0; i < 8; ++i)
            cascade[i].reset();
        lastGain = -999.0f;
    }

private:
    void updateCoefficients (float gain)
    {
        if (std::abs (gain - lastGain) < 0.001f) return;
        lastGain = gain;

        auto g = juce::Decibels::decibelsToGain (gain);
        juce::ReferenceCountedObjectPtr<juce::dsp::IIR::Coefficients<float>> coeffs;

        if (isFirstOrder)
        {
            // 6dB/oct slopes (1st order)
            switch (currentType)
            {
                case HighPass: coeffs = juce::dsp::IIR::Coefficients<float>::makeFirstOrderHighPass (sampleRate, currentFreq); break;
                case LowPass:  coeffs = juce::dsp::IIR::Coefficients<float>::makeFirstOrderLowPass (sampleRate, currentFreq); break;
                default:       coeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter (sampleRate, currentFreq, currentQ, g); break;
            }
        }
        else
        {
            // 12dB/oct base slopes (2nd order)
            switch (currentType)
            {
                case Peak:      coeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter (sampleRate, currentFreq, currentQ, g); break;
                case LowShelf:  coeffs = juce::dsp::IIR::Coefficients<float>::makeLowShelf (sampleRate, currentFreq, currentQ, g); break;
                case HighShelf: coeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf (sampleRate, currentFreq, currentQ, g); break;
                case Notch:     coeffs = juce::dsp::IIR::Coefficients<float>::makeNotch (sampleRate, currentFreq, currentQ); break;
                case HighPass:  coeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass (sampleRate, currentFreq, currentQ); break;
                case LowPass:   coeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass (sampleRate, currentFreq, currentQ); break;
                case AllPass:   coeffs = juce::dsp::IIR::Coefficients<float>::makeAllPass (sampleRate, currentFreq, currentQ); break;
                default:        coeffs = juce::dsp::IIR::Coefficients<float>::makeAllPass (sampleRate, currentFreq, currentQ); break;
            }
        }

        if (coeffs)
        {
            for (int i = 0; i < stages; ++i)
                *cascade[i].state = *coeffs;
        }
    }

    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> cascade[8];
    double sampleRate = 44100.0;
    int stages = 1;
    bool isFirstOrder = false;
    int currentType = Peak;
    float currentFreq = 1000.0f, currentQ = 0.707f, currentGain = 0.0f;
    float lastGain = -999.0f;
    
    // Dynamic Params
    bool isDynamic = false;
    int charMode = 0;
    
    // Placeholder for smoothing or just use LinearSmoothedValue
};
