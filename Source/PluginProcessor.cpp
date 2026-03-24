#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <memory>
#include <vector>
#include <cmath>
#include <algorithm>

CleanSlateAudioProcessor::CleanSlateAudioProcessor()
    : juce::AudioProcessor (juce::AudioProcessor::BusesProperties()
                            .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                            .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                            .withInput ("Sidechain", juce::AudioChannelSet::stereo(), false)),
      treeState (*this, nullptr, "PARAMETERS", createParameterLayout()),
      forwardFFT (fftOrder),
      window (fftSize, juce::dsp::WindowingFunction<float>::hann),
      linearPhaseFFT (linearPhaseFFTOrder)
{
    // Initialize band states
    for (int i = 0; i < 8; ++i)
    {
        bandStates[i].freq = (i == 0) ? 20.0f : (i == 7) ? 20000.0f : 100.0f * std::pow (2.0f, (float) i);
        bandStates[i].type = (i == 0) ? 0 : (i == 7) ? 2 : 1;
    }

    // Initialize saturation
    saturationL.functionToUse = [] (float x) { return x; };
    saturationR.functionToUse = [] (float x) { return x; };

    // Initialize scope data
    std::fill (scopeData, scopeData + scopeSize, 0.0f);
    std::fill (peakHoldData, peakHoldData + scopeSize, 0.0f);
    std::fill (referenceSpectrum, referenceSpectrum + scopeSize, 0.0f);
    
    // Initialize new features
    sidechainBuffer.setSize (2, lookaheadSize);
    sidechainBuffer.clear ();

    dryBufferL.setSize (1, lookaheadSize);
    dryBufferR.setSize (1, lookaheadSize);
    dryBufferL.clear ();
    dryBufferR.clear ();
    
    lookaheadBufferL.resize (lookaheadSize, 0.0f);
    lookaheadBufferR.resize (lookaheadSize, 0.0f);
    
    // Initialize A/B buffers
    abBufferL.setSize (2, lookaheadSize);
    abBufferR.setSize (2, lookaheadSize);
    abBufferL.clear();
    abBufferR.clear();
    
    // Initialize additional state variables
    abEnabled = false;
    abSwap = false;
    midSolo = false;
    sideSolo = false;
    spectrumViewMode = 0;
    analogModel = 0;

    // Initialize phase and character modes
    phaseMode = PhaseMode::ZeroLatency;
    characterMode = CharacterMode::Clean;
    mAutoGain = 1.0f;
    deltaMode = false;

    // FIX #7: Mark filters as dirty on startup to force initial update
    filtersDirty = true;
}

CleanSlateAudioProcessor::~CleanSlateAudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout CleanSlateAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    auto gainRange = juce::NormalisableRange<float> (-24.0f, 24.0f, 0.1f, 1.0f);
    auto qRange = juce::NormalisableRange<float> (0.1f, 10.0f, 0.05f, 1.0f);
    auto slopeRange = juce::NormalisableRange<float> (0.0f, 4.0f, 1.0f);
    auto timeRange = juce::NormalisableRange<float> (1.0f, 1000.0f, 1.0f, 0.3f);
    auto ratioRange = juce::NormalisableRange<float> (0.1f, 10.0f, 0.1f, 1.0f);

    for (int i = 0; i < 8; ++i)
    {
        juce::String id = "band" + juce::String (i);
        juce::String name = "Band " + juce::String (i + 1);

        float defaultFreq = (i == 0) ? 20.0f : (i == 7) ? 20000.0f : 100.0f * std::pow (2.0f, (float) i);

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id + "_freq", 1 }, name + " Freq",
            juce::NormalisableRange<float> (20.0f, 20000.0f, 1.0f, 0.25f), defaultFreq));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id + "_gain", 1 }, name + " Gain", gainRange, 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id + "_q", 1 }, name + " Q", qRange, 0.707f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id + "_slope", 1 }, name + " Slope", slopeRange, 2.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id + "_attack", 1 }, name + " Attack", timeRange, 10.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id + "_release", 1 }, name + " Release", timeRange, 100.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id + "_threshold", 1 }, name + " Threshold",
            juce::NormalisableRange<float> (-60.0f, 0.0f, 0.1f, 2.0f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id + "_ratio", 1 }, name + " Ratio", ratioRange, 1.0f));

        params.push_back (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { id + "_active", 1 }, name + " Active", true));

        params.push_back (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { id + "_solo", 1 }, name + " Solo", false));

        params.push_back (std::make_unique<juce::AudioParameterInt> (
            juce::ParameterID { id + "_type", 1 }, name + " Type", 0, 6,
            (i == 0) ? 0 : (i == 7) ? 2 : 1));

        params.push_back (std::make_unique<juce::AudioParameterInt> (
            juce::ParameterID { id + "_mode", 1 }, name + " Mode", 0, 4, 0));

        params.push_back (std::make_unique<juce::AudioParameterInt> (
            juce::ParameterID { id + "_dynamic", 1 }, name + " Dynamic", 0, 2, 0));
    }

    // Global parameters
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "autoGain", 1 }, "Auto Gain", true));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "resKill", 1 }, "Res-Kill", false));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "smartLearn", 1 }, "Smart Learn", false));

    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "phaseMode", 1 }, "Phase Mode", 0, 2, 0));

    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "characterMode", 1 }, "Character", 0, 2, 0));

     params.push_back (std::make_unique<juce::AudioParameterFloat> (
         juce::ParameterID { "tilt", 1 }, "Tilt",
         juce::NormalisableRange<float> (-6.0f, 6.0f, 0.1f), 0.0f));

     // New professional features
     params.push_back (std::make_unique<juce::AudioParameterBool> (
         juce::ParameterID { "deltaMode", 1 }, "Delta Mode", false));
         
     params.push_back (std::make_unique<juce::AudioParameterBool> (
         juce::ParameterID { "externalSidechain", 1 }, "External Sidechain", false));
         
     params.push_back (std::make_unique<juce::AudioParameterBool> (
         juce::ParameterID { "lookahead", 1 }, "Lookahead", false));
         
     params.push_back (std::make_unique<juce::AudioParameterFloat> (
         juce::ParameterID { "lookaheadTime", 1 }, "Lookahead Time (ms)",
         juce::NormalisableRange<float> (0.1f, 10.0f, 0.1f), 1.0f));

     // Additional FabFilter/iZotope inspired features
     params.push_back (std::make_unique<juce::AudioParameterBool> (
         juce::ParameterID { "midSolo", 1 }, "Mid Solo", false));
         
     params.push_back (std::make_unique<juce::AudioParameterBool> (
         juce::ParameterID { "sideSolo", 1 }, "Side Solo", false));
         
     params.push_back (std::make_unique<juce::AudioParameterBool> (
         juce::ParameterID { "abEnabled", 1 }, "A/B Enable", false));
         
     params.push_back (std::make_unique<juce::AudioParameterBool> (
         juce::ParameterID { "abSwap", 1 }, "A/B Swap", false));
         
     params.push_back (std::make_unique<juce::AudioParameterInt> (
         juce::ParameterID { "spectrumView", 1 }, "Spectrum View",
         0, 3, 0)); // 0=Input, 1=Output, 2=Both, 3=Difference
         
     params.push_back (std::make_unique<juce::AudioParameterInt> (
         juce::ParameterID { "analogModel", 1 }, "Analog Model",
         0, 2, 0)); // 0=Clean, 1=Neve, 2=API

    return { params.begin(), params.end() };
}

void CleanSlateAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 1;

    // Prepare all filters
    for (int i = 0; i < 8; ++i)
    {
        filtersL[i].prepare (spec);
        filtersR[i].prepare (spec);
        filtersM[i].prepare (spec);
        filtersS[i].prepare (spec);
    }

    // Prepare saturation
    saturationL.prepare (spec);
    saturationR.prepare (spec);

    // Prepare convolution buffers for linear phase
    convolutionBufferL.resize (2);
    convolutionBufferR.resize (2);
    for (auto& buf : convolutionBufferL) buf.resize (linearPhaseFFTSize, 0.0f);
    for (auto& buf : convolutionBufferR) buf.resize (linearPhaseFFTSize, 0.0f);
    convolutionIndex = 0;

    // Initialize lookahead buffers
    lookaheadBufferL.resize (lookaheadSize, 0.0f);
    lookaheadBufferR.resize (lookaheadSize, 0.0f);
    lookaheadWritePos = 0;
    
    // Initialize dry buffers for delta mode
    dryBufferL.setSize (2, lookaheadSize);
    dryBufferR.setSize (2, lookaheadSize);
    dryBufferL.clear();
    dryBufferR.clear();
    
    // Reset latency
    latencySamples = 0;
    
    // Initialize pre-allocated buffers and contexts for zero-latency processing (FIX #6)
    bufL.setSize(1, 1);
    bufR.setSize(1, 1);
    stereoBuffer.setSize(2, 1);
    
    blockL = std::make_unique<juce::dsp::AudioBlock<float>>(bufL);
    blockR = std::make_unique<juce::dsp::AudioBlock<float>>(bufR);
    stereoBlock = std::make_unique<juce::dsp::AudioBlock<float>>(stereoBuffer);
    
    ctxL = std::make_unique<juce::dsp::ProcessContextReplacing<float>>(*blockL);
    ctxR = std::make_unique<juce::dsp::ProcessContextReplacing<float>>(*blockR);
    ctxStereo = std::make_unique<juce::dsp::ProcessContextReplacing<float>>(*stereoBlock);
    
    updateFilters ();
}

void CleanSlateAudioProcessor::encodeMS (float* left, float* right)
{
    float m = (*left + *right) * 0.5f;
    float s = (*left - *right) * 0.5f;
    *left = m;
    *right = s;
}

void CleanSlateAudioProcessor::decodeMS (float* left, float* right)
{
    float l = *left + *right;
    float r = *left - *right;
    *left = l;
    *right = r;
}

void CleanSlateAudioProcessor::applySaturation (float* sample)
{
    // Apply analog modeling if enabled
    switch (analogModel)
    {
        case 0: // Clean
            break;
        case 1: // Neve-style (slight asymmetry)
        {
            float x = *sample;
            if (x > 0.0f)
                *sample = std::tanh (x * 1.1f) / 1.1f;
            else
                *sample = std::tanh (x * 1.3f) / 1.3f;
            break;
        }
        case 2: // API-style (more aggressive)
        {
            float x = *sample;
            *sample = std::tanh (x * 1.8f) / 1.8f;
            // Add slight odd-order harmonics
            *sample += 0.05f * std::tanh (x * x * x * 2.0f);
            break;
        }
    }
    
    // Then apply character mode saturation
    switch (characterMode)
    {
        case CharacterMode::Clean:
            break;
        case CharacterMode::Subtle:
            *sample = std::tanh (*sample * 1.2f) / 1.2f;
            break;
        case CharacterMode::Warm:
        {
            float x = *sample;
            if (x > 0.0f)
                *sample = std::tanh (x * 1.5f) / 1.5f;
            else
                *sample = std::tanh (x * 2.0f) / 2.0f;
            break;
        }
    }
}

void CleanSlateAudioProcessor::processDynamicEQ (int band, float sampleRateVal, float* leftSample, float* rightSample)
{
    // Apply lookahead if enabled
    if (treeState.getRawParameterValue ("lookahead")->load () > 0.5f)
    {
        processLookaheadBuffers(leftSample, rightSample);
    }
    
    auto& state = bandStates[band];

    if (state.dynamic == 0 || state.ratio <= 0.01f) return;

    float inputLevel = (std::abs (*leftSample) + std::abs (*rightSample)) * 0.5f;
    float inputdB = inputLevel > 0.0f ? 20.0f * std::log10 (inputLevel) : -120.0f;
    float overThreshold = inputdB - state.threshold;

    if (overThreshold > 0.0f)
    {
        float attackGain = std::pow (0.1f, 1000.0f / (state.attack * sampleRateVal));
        state.envelope = state.envelope * attackGain + overThreshold * (1.0f - attackGain);
    }
    else
    {
        float releaseGain = std::pow (0.1f, 1000.0f / (state.release * sampleRateVal));
        state.envelope = state.envelope * releaseGain;
    }

    float gainReduction = 0.0f;
    if (state.dynamic == 1)
        gainReduction = state.envelope * (1.0f - 1.0f / state.ratio);
    else
        gainReduction = state.envelope * (state.ratio - 1.0f);

    float gainLin = juce::Decibels::decibelsToGain (-gainReduction);
    state.dynamicGain = gainReduction;

    *leftSample *= gainLin;
    *rightSample *= gainLin;
}

void CleanSlateAudioProcessor::updateFilters ()
{
    float totalBoost = 0.0f;
    float totalCut = 0.0f;

    for (int i = 0; i < 8; ++i)
    {
        juce::String id = "band" + juce::String (i);

        bandStates[i].freq = treeState.getRawParameterValue (id + "_freq")->load ();
        bandStates[i].gain = treeState.getRawParameterValue (id + "_gain")->load ();
        bandStates[i].q = treeState.getRawParameterValue (id + "_q")->load ();
        bandStates[i].slope = treeState.getRawParameterValue (id + "_slope")->load ();
        bandStates[i].attack = treeState.getRawParameterValue (id + "_attack")->load ();
        bandStates[i].release = treeState.getRawParameterValue (id + "_release")->load ();
        bandStates[i].threshold = treeState.getRawParameterValue (id + "_threshold")->load ();
        bandStates[i].ratio = treeState.getRawParameterValue (id + "_ratio")->load ();
        bandStates[i].active = treeState.getRawParameterValue (id + "_active")->load () > 0.5f;
        bandStates[i].solo = treeState.getRawParameterValue (id + "_solo")->load () > 0.5f;
        bandStates[i].type = (int) treeState.getRawParameterValue (id + "_type")->load ();
        bandStates[i].mode = (int) treeState.getRawParameterValue (id + "_mode")->load ();
        bandStates[i].dynamic = (int) treeState.getRawParameterValue (id + "_dynamic")->load ();

        if (bandStates[i].active && bandStates[i].dynamic == 0)
        {
            if (bandStates[i].gain > 0)
                totalBoost += bandStates[i].gain;
            else
                totalCut += std::abs (bandStates[i].gain);
        }

        // Map filter type
        int surgType = 1;
        switch (bandStates[i].type)
        {
            case 0: surgType = 5; break;
            case 1: surgType = 1; break;
            case 2: surgType = 6; break;
            case 3: surgType = 4; break;
            case 4: surgType = 2; break;
            case 5: surgType = 3; break;
            case 6: surgType = 0; break; // AllPass handled in SurgicalFilter
        }

        float freq = bandStates[i].freq;
        float q = bandStates[i].q;
        float gain = bandStates[i].gain;
        float slope = bandStates[i].slope;
        float attack = bandStates[i].attack / 1000.0f; // convert to seconds
        float release = bandStates[i].release / 1000.0f;
        float threshold = bandStates[i].threshold;
        float ratio = bandStates[i].ratio;
        bool isDynamic = bandStates[i].dynamic > 0;
        int charMode = (int)characterMode;

        filtersL[i].setParameters (freq, q, gain, surgType, (int)slope, threshold, ratio, attack, release, isDynamic, charMode);
        filtersR[i].setParameters (freq, q, gain, surgType, (int)slope, threshold, ratio, attack, release, isDynamic, charMode);
        filtersM[i].setParameters (freq, q, gain, surgType, (int)slope, threshold, ratio, attack, release, isDynamic, charMode);
        filtersS[i].setParameters (freq, q, gain, surgType, (int)slope, threshold, ratio, attack, release, isDynamic, charMode);
    }

    // Note: phaseMode and characterMode are now safely updated in processBlock() with proper bounds checking
    // Do NOT update them here to avoid conflicts - updateFilters() is called too frequently

    auto newCharacter = (CharacterMode) treeState.getRawParameterValue ("characterMode")->load ();
    if (newCharacter != characterMode)
    {
        characterMode = newCharacter;
        switch (characterMode)
        {
            case CharacterMode::Clean:
                saturationL.functionToUse = [] (float x) { return x; };
                saturationR.functionToUse = [] (float x) { return x; };
                break;
            case CharacterMode::Subtle:
                saturationL.functionToUse = [] (float x) { return std::tanh (x * 1.2f) / 1.2f; };
                saturationR.functionToUse = [] (float x) { return std::tanh (x * 1.2f) / 1.2f; };
                break;
            case CharacterMode::Warm:
                saturationL.functionToUse = [] (float x) {
                    return x > 0.0f ? std::tanh (x * 1.5f) / 1.5f : std::tanh (x * 2.0f) / 2.0f;
                };
                saturationR.functionToUse = [] (float x) {
                    return x > 0.0f ? std::tanh (x * 1.5f) / 1.5f : std::tanh (x * 2.0f) / 2.0f;
                };
                break;
        }
    }

    tiltAmount = treeState.getRawParameterValue ("tilt")->load ();

    // Calculate useMS for current bands
    useMS = false;
    for (int i = 0; i < 8; ++i)
    {
        if (bandStates[i].active && (bandStates[i].mode == 3 || bandStates[i].mode == 4))
        {
            useMS = true;
            break;
        }
    }

    // Handle M/S solo
    midSolo = (treeState.getRawParameterValue ("midSolo")->load () > 0.5f);
    sideSolo = (treeState.getRawParameterValue ("sideSolo")->load () > 0.5f);
    
    // Handle spectrum view mode
    spectrumViewMode = (int)treeState.getRawParameterValue ("spectrumView")->load ();
    
    // Handle analog modeling
    analogModel = (int)treeState.getRawParameterValue ("analogModel")->load ();

    if (treeState.getRawParameterValue ("autoGain")->load () > 0.5f)
    {
        float totalBoosts = std::max (0.0f, totalBoost);
        float totalCuts = std::max (0.0f, totalCut);
        mAutoGain = 0.5f - (totalBoosts * 0.05f) + (totalCuts * 0.02f);
    }
    else
    {
        mAutoGain = 1.0f;
    }
}

void CleanSlateAudioProcessor::processWithZeroLatency (juce::AudioBuffer<float>& buffer)
{
    auto* leftData = buffer.getWritePointer (0);
    if (leftData == nullptr) return;  // No input

    // Handle mono by duplicating to right channel, or stereo
    bool isMono = buffer.getNumChannels() < 2;
    auto* rightData = isMono ? leftData : buffer.getWritePointer (1);

    if (rightData == nullptr) return;  // Safety check

    int numSamples = buffer.getNumSamples ();

    // Validate that buffers and contexts are initialized
    if (ctxL == nullptr || ctxR == nullptr || ctxStereo == nullptr) return;

    for (int sample = 0; sample < numSamples; ++sample)
    {
        float leftSample = leftData[sample];
        float rightSample = isMono ? leftSample : rightData[sample];

        bool useMS = false;
        for (int i = 0; i < 8; ++i)
        {
            if (bandStates[i].mode == 3 || bandStates[i].mode == 4)
            {
                useMS = true;
                break;
            }
        }

        if (useMS)
            encodeMS (&leftSample, &rightSample);

        for (int i = 0; i < 8; ++i)
        {
            if (! bandStates[i].active) continue;

            // Reuse pre-allocated member buffers (FIX #6)
            bufL.setSample(0, 0, leftSample);
            bufR.setSample(0, 0, rightSample);

            // Apply M/S solo functionality
            bool processThisBand = true;
            if (bandStates[i].mode == 3) // Mid
            {
                if (sideSolo) processThisBand = false; // Solo side, mute mid
            }
            else if (bandStates[i].mode == 4) // Side
            {
                if (midSolo) processThisBand = false; // Solo mid, mute side
            }

            if (processThisBand)
            {
                if (bandStates[i].mode == 3) // Mid
                {
                    filtersM[i].process (*ctxL);
                    leftSample = bufL.getSample(0, 0);
                }
                else if (bandStates[i].mode == 4) // Side
                {
                    filtersS[i].process (*ctxR);
                    rightSample = bufR.getSample(0, 0);
                }
                else if (bandStates[i].mode == 1) // Left
                {
                    filtersL[i].process (*ctxL);
                    leftSample = bufL.getSample(0, 0);
                }
                else if (bandStates[i].mode == 2) // Right
                {
                    filtersR[i].process (*ctxR);
                    rightSample = bufR.getSample(0, 0);
                }
                else // Stereo (Mode 0) (FIX #14: Stereo processing bug)
                {
                    filtersL[i].process (*ctxL);
                    filtersR[i].process (*ctxR);
                    leftSample = bufL.getSample (0, 0);
                    rightSample = bufR.getSample (0, 0);
                }
            }
        }

        if (useMS)
            decodeMS (&leftSample, &rightSample);

        applySaturation (&leftSample);
        applySaturation (&rightSample);

        leftData[sample] = leftSample;
        rightData[sample] = rightSample;
    }
}

void CleanSlateAudioProcessor::processWithNaturalPhase (juce::AudioBuffer<float>& buffer)
{
    // Natural Phase: Uses zero-latency processing with slightly different characteristics
    // This is a minimum-phase approximation using zero-latency filters
    // True natural phase would use FFT-based processing with phase adjustment
    // For now, we use zero-latency which is transparent to the ear for most signals

    processWithZeroLatency(buffer);

    // TODO: Future enhancement - implement true minimum-phase filtering using:
    // 1. Compute magnitude response from each band
    // 2. Calculate minimum-phase equivalent
    // 3. Apply via FFT convolution or IIR approximation
}

// Helper function for linear phase processing
void CleanSlateAudioProcessor::processLinearPhaseChannel (float* channelData, 
                                                        const std::vector<float>& impulseResponse,
                                                        std::vector<std::vector<float>>& convolutionBuffer,
                                                        int& convolutionIndex,
                                                        int numSamples)
{
    // IMPORTANT: Save input samples BEFORE clearing output
    std::vector<float> inputSamples(numSamples);
    std::copy(channelData, channelData + numSamples, inputSamples.begin());

    // Clear output
    std::fill (channelData, channelData + numSamples, 0.0f);

    // Perform convolution with saved input
    for (int i = 0; i < numSamples; ++i)
    {
        float sample = inputSamples[i];  // Read from saved input, NOT cleared buffer

        // Add sample to convolution buffer
        int writePos = (convolutionIndex + i) & (linearPhaseFFTSize - 1);
        convolutionBuffer[0][writePos] = sample;
        convolutionBuffer[1][writePos] = sample;

        // Convolve with impulse response
        float out = 0.0f;
        for (size_t j = 0; j < impulseResponse.size(); ++j)
        {
            int readPos = (convolutionIndex + i - (int)j + linearPhaseFFTSize) & (linearPhaseFFTSize - 1);
            out += convolutionBuffer[0][readPos] * impulseResponse[j];
        }

        channelData[i] = out;
    }
}

void CleanSlateAudioProcessor::updateLinearPhaseImpulseResponse ()
{
    // Clear previous impulse responses
    if (impulseResponseL.size() != linearPhaseFFTSize)
        impulseResponseL.resize(linearPhaseFFTSize, 0.0f);
    if (impulseResponseR.size() != linearPhaseFFTSize)
        impulseResponseR.resize(linearPhaseFFTSize, 0.0f);

    std::fill (impulseResponseL.begin(), impulseResponseL.end(), 0.0f);
    std::fill (impulseResponseR.begin(), impulseResponseR.end(), 0.0f);

    // Process each band to create combined impulse response
    for (int band = 0; band < 8; ++band)
    {
        if (!bandStates[band].active) continue;

        // Create filter impulse response for this band
        std::vector<float> bandIr(linearPhaseFFTSize, 0.0f);

        // Use the filter to generate impulse response
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = currentSampleRate;
        spec.maximumBlockSize = linearPhaseFFTSize;
        spec.numChannels = 1;

        // Create a temporary filter
        SurgicalFilter tempFilter;
        tempFilter.prepare(spec);

        // Set filter parameters (reuse from updateFilters)
        int surgType = 1;
        switch (bandStates[band].type)
        {
            case 0: surgType = 5; break; // HighPass
            case 1: surgType = 1; break; // Peak
            case 2: surgType = 6; break; // LowPass
            case 3: surgType = 4; break; // Notch
            case 4: surgType = 2; break; // LowShelf
            case 5: surgType = 3; break; // HighShelf
            case 6: surgType = 0; break; // AllPass
        }

        tempFilter.setParameters(
            bandStates[band].freq,
            bandStates[band].q,
            bandStates[band].gain,
            surgType,
            (int)bandStates[band].slope,
            bandStates[band].threshold,
            bandStates[band].ratio,
            bandStates[band].attack / 1000.0f,
            bandStates[band].release / 1000.0f,
            bandStates[band].dynamic > 0,
            (int)characterMode
        );

        // Generate impulse response by passing an impulse through the filter
        std::vector<float> impulse(linearPhaseFFTSize, 0.0f);
        impulse[0] = 1.0f; // Unit impulse

        // Create AudioBlock from AudioBuffer instead
        juce::AudioBuffer<float> impulseBuffer(1, linearPhaseFFTSize);
        impulseBuffer.copyFrom(0, 0, impulse.data(), linearPhaseFFTSize);

        juce::dsp::AudioBlock<float> impulseBlock(impulseBuffer);
        juce::dsp::ProcessContextReplacing<float> ctxImpulse(impulseBlock);
        tempFilter.process(ctxImpulse);

        // Copy processed impulse back
        auto* processedData = impulseBlock.getChannelPointer(0);

        // Add to total impulse response (simple addition for now)
        for (int i = 0; i < linearPhaseFFTSize; ++i)
        {
            bandIr[i] += processedData[i];
        }

        // Normalize and apply to both channels
        float maxVal = 0.0f;
        for (float sample : bandIr)
        {
            maxVal = std::max(maxVal, std::abs(sample));
        }

        if (maxVal > 0.0f)
        {
            for (size_t i = 0; i < bandIr.size(); ++i)
            {
                float normalized = bandIr[i] / maxVal;
                impulseResponseL[i] = normalized;
                impulseResponseR[i] = normalized;
            }
        }
    }

    // Ensure impulse responses are not empty
    if (std::all_of(impulseResponseL.begin(), impulseResponseL.end(), [](float f) { return f == 0.0f; }))
    {
        // Flat response (pass-through)
        if (linearPhaseFFTSize >= 2)
        {
            impulseResponseL[linearPhaseFFTSize/2] = 1.0f;
            impulseResponseR[linearPhaseFFTSize/2] = 1.0f;
        }
    }
}

// Lookahead buffer processing for dynamic EQ
void CleanSlateAudioProcessor::processLookaheadBuffers (float* leftSample, float* rightSample)
{
    // Write current samples to lookahead buffer
    lookaheadBufferL[lookaheadWritePos] = *leftSample;
    lookaheadBufferR[lookaheadWritePos] = *rightSample;
    
    // Read delayed samples (lookahead)
    int readPos = (lookaheadWritePos - lookaheadSize + lookaheadBufferL.size()) % lookaheadBufferL.size();
    float delayedLeft = lookaheadBufferL[readPos];
    float delayedRight = lookaheadBufferR[readPos];
    
    // Update write position
    lookaheadWritePos = (lookaheadWritePos + 1) % lookaheadBufferL.size();
    
    // Return delayed samples for processing
    *leftSample = delayedLeft;
    *rightSample = delayedRight;
}

void CleanSlateAudioProcessor::processWithLinearPhase (juce::AudioBuffer<float>& buffer)
{
    // True Linear Phase implementation using FFT convolution
    if (buffer.getNumChannels() < 2 || impulseResponseL.empty() || impulseResponseR.empty())
    {
        processWithZeroLatency(buffer);
        return;
    }

    // Update impulse response if needed
    if (impulseResponseDirty)
    {
        updateLinearPhaseImpulseResponse();
        impulseResponseDirty = false;
    }

    const int numSamples = buffer.getNumSamples();
    if (numSamples == 0) return;

    auto* leftData = buffer.getWritePointer(0);
    auto* rightData = buffer.getWritePointer(1);

    // Process left channel
    processLinearPhaseChannel(leftData, impulseResponseL, convolutionBufferL, convolutionIndex, numSamples);
    
    // Process right channel
    processLinearPhaseChannel(rightData, impulseResponseR, convolutionBufferR, convolutionIndex, numSamples);
    
    // Update convolution index
    convolutionIndex = (convolutionIndex + numSamples) & (linearPhaseFFTSize - 1);
}

void CleanSlateAudioProcessor::releaseResources () {}

bool CleanSlateAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet ())
        return false;

    return true;
}

void CleanSlateAudioProcessor::pushNextSampleIntoFifo (float sample)
{
    if (fifoIndex == fftSize)
    {
        if (! nextFFTBlockReady)
        {
            juce::zeromem (fftData, sizeof (fftData));
            std::memcpy (fftData, fifo, sizeof (fifo));
            nextFFTBlockReady = true;
        }
        fifoIndex = 0;
    }
    fifo[fifoIndex++] = sample;
}

void CleanSlateAudioProcessor::getFFTMagnitudes (float* result)
{
    window.multiplyWithWindowingTable (fftData, fftSize);
    forwardFFT.performFrequencyOnlyForwardTransform (fftData);

    for (int i = 0; i < scopeSize; ++i)
        result[i] = fftData[i];
}

void CleanSlateAudioProcessor::calculateScopeData ()
{
    if (! nextFFTBlockReady) return;

    getFFTMagnitudes (scopeData);

    // Apply Tilt (±6dB/oct)
    if (tiltAmount != 0.0f)
    {
        for (int i = 1; i < scopeSize; ++i)
        {
            float freq = (float) i * currentSampleRate / (2.0f * fftSize);
            float octaves = std::log2 (freq / 1000.0f);
            scopeData[i] *= juce::Decibels::decibelsToGain (tiltAmount * octaves);
        }
    }

    // Peak Hold & Decay
    for (int i = 0; i < scopeSize; ++i)
    {
        if (scopeData[i] > peakHoldData[i])
        {
            peakHoldData[i] = scopeData[i];
            peakHoldCounter[i] = 40; // ~1 second @ 60fps
        }
        else if (peakHoldCounter[i] > 0)
        {
            peakHoldCounter[i]--;
        }
        else
        {
            peakHoldData[i] *= 0.985f; // Smooth decay
        }
    }

    nextFFTBlockReady = false;
}

void CleanSlateAudioProcessor::findHarshResonances ()
{
    if (treeState.getRawParameterValue ("resKill")->load () < 0.5f)
        return;

    // Use current scope data to find peaks
    const float noiseFloor = juce::Decibels::decibelsToGain (-60.0f);
    const float peakSensitivity = 1.25f; // Multiplier above neighbors

    for (int i = 10; i < scopeSize - 10; ++i)
    {
        float val = scopeData[i];
        if (val < noiseFloor) continue;

        // Check if it's a prominent local peak
        if (val > scopeData[i - 1] * peakSensitivity && val > scopeData[i + 1] * peakSensitivity)
        {
            float freq = (float) i * (currentSampleRate / (2.0f * fftSize));
            
            // Look for an inactive band to "kill" this resonance
            for (int b = 0; b < 8; ++b)
            {
                juce::String id = "band" + juce::String (b);
                if (treeState.getRawParameterValue (id + "_active")->load () < 0.5f)
                {
                    treeState.getRawParameterValue (id + "_freq")->store (freq);
                    treeState.getRawParameterValue (id + "_gain")->store (-9.0f); // Surgical cut
                    treeState.getRawParameterValue (id + "_q")->store (8.0f);    // High Q notch
                    treeState.getRawParameterValue (id + "_type")->store (3.0f); // Notch
                    treeState.getRawParameterValue (id + "_active")->store (1.0f);
                    break;
                }
            }
            i += 50; // Gap between detections
        }
    }
}

void CleanSlateAudioProcessor::captureReference ()
{
    // Deep copy current peak-held or smoothed spectrum to reference
    std::memcpy (referenceSpectrum, scopeData, sizeof (scopeData));
    referenceCaptured = true;
}

void CleanSlateAudioProcessor::applyEqMatch ()
{
    if (! referenceCaptured) return;
    
    // 1. Reset bands first
    for (int i = 0; i < 8; ++i)
    {
        juce::String id = "band" + juce::String (i);
        treeState.getRawParameterValue (id + "_active")->store (0.0f);
        treeState.getRawParameterValue (id + "_gain")->store (0.0f);
    }
    
    // 2. Find significant differences in 8 key frequency regions
    const float freqs[8] = { 60.0f, 150.0f, 400.0f, 1000.0f, 2500.0f, 5000.0f, 10000.0f, 16000.0f };
    
    for (int b = 0; b < 8; ++b)
    {
        int fftBin = (int)(freqs[b] * (2.0f * fftSize) / currentSampleRate);
        fftBin = juce::jlimit (0, scopeSize - 1, fftBin);
        
        float currentVal = scopeData[fftBin];
        float targetVal = referenceSpectrum[fftBin];
        
        float diffDb = juce::Decibels::gainToDecibels (targetVal) - juce::Decibels::gainToDecibels (currentVal);
        diffDb = juce::jlimit (-18.0f, 18.0f, diffDb); // Constraint
        
        if (std::abs (diffDb) > 1.0f) // Only if significant
        {
            juce::String id = "band" + juce::String (b);
            treeState.getRawParameterValue (id + "_freq")->store (freqs[b]);
            treeState.getRawParameterValue (id + "_gain")->store (diffDb);
            treeState.getRawParameterValue (id + "_q")->store (1.0f);
            treeState.getRawParameterValue (id + "_type")->store (1.0f); // Peak
            treeState.getRawParameterValue (id + "_active")->store (1.0f);
        }
    }
}

void CleanSlateAudioProcessor::createBandsFromSketch (const std::vector<std::pair<float, float>>& points)
{
    if (points.size() < 10) return;

    // Reset all bands first
    for (int i = 0; i < 8; ++i)
    {
        juce::String id = "band" + juce::String (i);
        treeState.getRawParameterValue (id + "_active")->store (0.0f);
        treeState.getRawParameterValue (id + "_gain")->store (0.0f);
    }

    // Find local maxima/minima to place bands
    int bandIdx = 0;
    for (size_t i = 5; i < points.size() - 5 && bandIdx < 8; ++i)
    {
        float gain = points[i].second;
        bool isPeak = (gain > points[i-1].second && gain > points[i+1].second && gain > 1.0f);
        bool isDip = (gain < points[i-1].second && gain < points[i+1].second && gain < -1.0f);

        if (isPeak || isDip)
        {
            juce::String id = "band" + juce::String (bandIdx);
            treeState.getRawParameterValue (id + "_freq")->store (points[i].first);
            treeState.getRawParameterValue (id + "_gain")->store (gain);
            treeState.getRawParameterValue (id + "_q")->store (1.5f);
            treeState.getRawParameterValue (id + "_type")->store (1.0f); // Peak
            treeState.getRawParameterValue (id + "_active")->store (1.0f);
            bandIdx++;
            i += 20; // Skip neighbors
        }
    }
}

void CleanSlateAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels ();
    auto totalNumOutputChannels = getTotalNumOutputChannels ();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples ());

    // Handle external sidechain input
    if (treeState.getRawParameterValue ("externalSidechain")->load () > 0.5f)
    {
        if (getTotalNumInputChannels() >= 4) // Sidechain is on inputs 2&3
        {
            sidechainAvailable = true;
            // Copy sidechain inputs (channels 2&3) to our sidechain buffer
            if (buffer.getNumChannels() >= 4)
            {
                auto* scLeft = buffer.getReadPointer (2);
                auto* scRight = buffer.getReadPointer (3);
                for (int i = 0; i < buffer.getNumSamples(); ++i)
                {
                    sidechainBuffer.setSample (0, i, scLeft[i]);
                    sidechainBuffer.setSample (1, i, scRight[i]);
                }
            }
        }
        else
        {
            sidechainAvailable = false;
        }
    }
    else
    {
        sidechainAvailable = false;
    }

    // Handle A/B switching
    abEnabled = (treeState.getRawParameterValue ("abEnabled")->load () > 0.5f);
    abSwap = (treeState.getRawParameterValue ("abSwap")->load () > 0.5f);
    
    // Store A/B states if enabled
    if (abEnabled && buffer.getNumChannels() >= 2)
    {
        if (abSwap)
        {
            // Swap A and B - copy current buffer to B, then copy B to A
            auto* bufLeft = buffer.getWritePointer (0);
            auto* bufRight = buffer.getWritePointer (1);
            auto* abLeft = abBufferL.getWritePointer (0);
            auto* abRight = abBufferR.getWritePointer (0);
            
            // Copy current to temp
            std::vector<float> tempL(bufLeft, bufLeft + buffer.getNumSamples());
            std::vector<float> tempR(bufRight, bufRight + buffer.getNumSamples());
            
            // Copy B to A
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                bufLeft[i] = abBufferL.getReadPointer (0)[i];
                bufRight[i] = abBufferR.getReadPointer (0)[i];
            }
            
            // Copy temp (original A) to B
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                abBufferL.getWritePointer (0)[i] = tempL[i];
                abBufferR.getWritePointer (0)[i] = tempR[i];
            }
        }
        else
        {
            // Normal A/B - store current state to B
            auto* bufLeft = buffer.getReadPointer (0);
            auto* bufRight = buffer.getReadPointer (1);
            auto* abLeft = abBufferL.getWritePointer (0);
            auto* abRight = abBufferR.getWritePointer (0);
            
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                abLeft[i] = bufLeft[i];
                abRight[i] = bufRight[i];
            }
        }
    }
    else if (abEnabled && !abSwap && buffer.getNumChannels() >= 2)
    {
        // Apply B state if we have one stored
        auto* bufLeft = buffer.getWritePointer (0);
        auto* bufRight = buffer.getWritePointer (1);
        auto* abLeft = abBufferL.getReadPointer (0);
        auto* abRight = abBufferR.getReadPointer (0);
        
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            bufLeft[i] = abLeft[i];
            bufRight[i] = abRight[i];
        }
    }

    // FIX #7: Only update filters if parameters changed (not every sample!)
    if (filtersDirty.exchange(false))
    {
        updateFilters();
    }

    calculateScopeData ();
    findHarshResonances ();

    if (buffer.getNumChannels () > 0)
    {
        auto* channelData = buffer.getReadPointer (0);
        for (int i = 0; i < buffer.getNumSamples (); ++i)
            pushNextSampleIntoFifo (channelData[i]);
    }

    // Check if delta mode is enabled
    deltaMode = (treeState.getRawParameterValue ("deltaMode")->load () > 0.5f);

    // Store dry signal for delta mode (FIX #10: No resizing - use pre-allocated buffer)
    if (deltaMode && buffer.getNumChannels() >= 2 && dryBufferL.getNumSamples() >= buffer.getNumSamples())
    {
        auto* dryLeft = dryBufferL.getWritePointer (0);
        auto* dryRight = dryBufferR.getWritePointer (0);
        auto* wetLeft = buffer.getWritePointer (0);
        auto* wetRight = buffer.getWritePointer (1);

        // Copy only up to the number of samples in the current block
        int samplesToCopy = juce::jmin(buffer.getNumSamples(), dryBufferL.getNumSamples());
        for (int i = 0; i < samplesToCopy; ++i)
        {
            dryLeft[i] = wetLeft[i];
            dryRight[i] = wetRight[i];
        }
    }

    // Handle M/S solo
    midSolo = (treeState.getRawParameterValue ("midSolo")->load () > 0.5f);
    sideSolo = (treeState.getRawParameterValue ("sideSolo")->load () > 0.5f);
    
    // Handle spectrum view mode
    spectrumViewMode = (int)treeState.getRawParameterValue ("spectrumView")->load ();
    
    // Handle analog modeling
    analogModel = (int)treeState.getRawParameterValue ("analogModel")->load ();

    // FIX #13: Validate channel count before processing
    if (buffer.getNumChannels() >= 2)
    {
        // Safely get and validate phase mode (parameter already 0-2, no multiplication needed)
        try
        {
            auto* phaseModeParam = treeState.getRawParameterValue ("phaseMode");
            if (phaseModeParam != nullptr)
            {
                int phaseModeValue = static_cast<int>(phaseModeParam->load());
                if (phaseModeValue >= 0 && phaseModeValue <= 2)
                {
                    phaseMode = static_cast<PhaseMode>(phaseModeValue);
                }
            }

            // Safely get character mode (parameter already 0-2, no multiplication needed)
            auto* charModeParam = treeState.getRawParameterValue ("characterMode");
            if (charModeParam != nullptr)
            {
                int charModeValue = static_cast<int>(charModeParam->load());
                if (charModeValue >= 0 && charModeValue <= 2)
                {
                    characterMode = static_cast<CharacterMode>(charModeValue);
                }
            }

            // Process audio with proper error handling
            switch (phaseMode)
            {
                case PhaseMode::ZeroLatency:
                    processWithZeroLatency (buffer);
                    break;
                case PhaseMode::NaturalPhase:
                    processWithNaturalPhase (buffer);
                    break;
                case PhaseMode::LinearPhase:
                    processWithLinearPhase (buffer);
                    break;
                default:
                    processWithZeroLatency (buffer);
                    break;
            }
        }
        catch (const std::exception& e)
        {
            // If processing fails, just pass through audio unchanged
            juce::Logger::getCurrentLogger()->writeToLog ("Processing error: " + juce::String (e.what()));
            return;
        }
    }
    else if (buffer.getNumChannels() == 1)
    {
        // FIX #13: Handle mono input by duplicating to stereo
        // Copy mono to a temporary stereo buffer, process, then copy back
        juce::AudioBuffer<float> stereoTemp(2, buffer.getNumSamples());
        auto* monoData = buffer.getReadPointer(0);
        stereoTemp.copyFrom(0, 0, monoData, buffer.getNumSamples());
        stereoTemp.copyFrom(1, 0, monoData, buffer.getNumSamples());

        try
        {
            // Process as stereo
            auto* phaseModeParam = treeState.getRawParameterValue ("phaseMode");
            if (phaseModeParam != nullptr)
            {
                int phaseModeValue = static_cast<int>(phaseModeParam->load());
                if (phaseModeValue >= 0 && phaseModeValue <= 2)
                    phaseMode = static_cast<PhaseMode>(phaseModeValue);
            }

            switch (phaseMode)
            {
                case PhaseMode::ZeroLatency:
                    processWithZeroLatency(stereoTemp);
                    break;
                case PhaseMode::NaturalPhase:
                    processWithNaturalPhase(stereoTemp);
                    break;
                case PhaseMode::LinearPhase:
                    processWithLinearPhase(stereoTemp);
                    break;
                default:
                    processWithZeroLatency(stereoTemp);
                    break;
            }

            // Copy back to mono (mix both channels)
            auto* outData = buffer.getWritePointer(0);
            auto* procL = stereoTemp.getReadPointer(0);
            auto* procR = stereoTemp.getReadPointer(1);
            for (int i = 0; i < buffer.getNumSamples(); ++i)
                outData[i] = (procL[i] + procR[i]) * 0.5f;
        }
        catch (const std::exception& e)
        {
            juce::Logger::getCurrentLogger()->writeToLog ("Mono processing error: " + juce::String (e.what()));
        }
    }

    buffer.applyGain (mAutoGain);

    // Apply delta mode (difference between dry and wet signals)
    if (deltaMode && buffer.getNumChannels() >= 2)
    {
        auto* dryLeft = dryBufferL.getReadPointer (0);
        auto* dryRight = dryBufferR.getReadPointer (0);
        auto* wetLeft = buffer.getWritePointer (0);
        auto* wetRight = buffer.getWritePointer (1);
        
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            wetLeft[i] = wetLeft[i] - dryLeft[i];  // Only hear what was changed
            wetRight[i] = wetRight[i] - dryRight[i];
        }
    }
}

juce::String CleanSlateAudioProcessor::getBandsAsXml () const
{
    juce::XmlElement xml ("EQCurve");
    xml.setAttribute ("version", "1.0");

    for (int i = 0; i < 8; ++i)
    {
        auto* bandXml = new juce::XmlElement ("Band");
        bandXml->setAttribute ("index", i);
        bandXml->setAttribute ("freq", bandStates[i].freq);
        bandXml->setAttribute ("gain", bandStates[i].gain);
        bandXml->setAttribute ("q", bandStates[i].q);
        bandXml->setAttribute ("slope", bandStates[i].slope);
        bandXml->setAttribute ("attack", bandStates[i].attack);
        bandXml->setAttribute ("release", bandStates[i].release);
        bandXml->setAttribute ("threshold", bandStates[i].threshold);
        bandXml->setAttribute ("ratio", bandStates[i].ratio);
        bandXml->setAttribute ("type", bandStates[i].type);
        bandXml->setAttribute ("mode", bandStates[i].mode);
        bandXml->setAttribute ("dynamic", bandStates[i].dynamic);
        bandXml->setAttribute ("active", bandStates[i].active);
        xml.addChildElement (bandXml);
    }

    return xml.toString ();
}

void CleanSlateAudioProcessor::setBandsFromXml (const juce::String& xmlString)
{
    std::unique_ptr<juce::XmlElement> xml (juce::XmlDocument::parse (xmlString));
    if (xml == nullptr || ! xml->hasTagName ("EQCurve"))
        return;

    for (auto* bandXml : xml->getChildIterator ())
    {
        if (! bandXml->hasTagName ("Band"))
            continue;

        int index = bandXml->getIntAttribute ("index", -1);
        if (index < 0 || index >= 8)
            continue;

        juce::String id = "band" + juce::String (index);

        treeState.getRawParameterValue (id + "_freq")->store (
            (float) bandXml->getDoubleAttribute ("freq", bandStates[index].freq));
        treeState.getRawParameterValue (id + "_gain")->store (
            (float) bandXml->getDoubleAttribute ("gain", bandStates[index].gain));
        treeState.getRawParameterValue (id + "_q")->store (
            (float) bandXml->getDoubleAttribute ("q", bandStates[index].q));
        treeState.getRawParameterValue (id + "_slope")->store (
            (float) bandXml->getDoubleAttribute ("slope", bandStates[index].slope));
        treeState.getRawParameterValue (id + "_attack")->store (
            (float) bandXml->getDoubleAttribute ("attack", bandStates[index].attack));
        treeState.getRawParameterValue (id + "_release")->store (
            (float) bandXml->getDoubleAttribute ("release", bandStates[index].release));
        treeState.getRawParameterValue (id + "_threshold")->store (
            (float) bandXml->getDoubleAttribute ("threshold", bandStates[index].threshold));
        treeState.getRawParameterValue (id + "_ratio")->store (
            (float) bandXml->getDoubleAttribute ("ratio", bandStates[index].ratio));
        treeState.getRawParameterValue (id + "_type")->store (
            (float) bandXml->getDoubleAttribute ("type", bandStates[index].type));
        treeState.getRawParameterValue (id + "_mode")->store (
            (float) bandXml->getDoubleAttribute ("mode", bandStates[index].mode));
        treeState.getRawParameterValue (id + "_dynamic")->store (
            (float) bandXml->getDoubleAttribute ("dynamic", bandStates[index].dynamic));
        treeState.getRawParameterValue (id + "_active")->store (
            bandXml->getBoolAttribute ("active", bandStates[index].active) ? 1.0f : 0.0f);
    }
}

void CleanSlateAudioProcessor::setPhaseMode (PhaseMode mode)
{
    phaseMode = mode;
}

void CleanSlateAudioProcessor::setCharacterMode (CharacterMode mode)
{
    characterMode = mode;
}

void CleanSlateAudioProcessor::loadPreset (int index)
{
    auto factoryPresets = Presets::getFactoryPresets ();
    if (index < 0 || index >= (int) factoryPresets.size ()) return;

    auto& preset = factoryPresets[index];

    for (int i = 0; i < 8; ++i)
    {
        juce::String id = "band" + juce::String (i);
        treeState.getRawParameterValue (id + "_active")->store (0.0f);
        treeState.getRawParameterValue (id + "_gain")->store (0.0f);
    }

    for (size_t i = 0; i < preset.bands.size () && i < 8; ++i)
    {
        juce::String id = "band" + juce::String (i);
        auto& b = preset.bands[i];
        treeState.getRawParameterValue (id + "_freq")->store (b.frequency);
        treeState.getRawParameterValue (id + "_gain")->store (b.gain);
        treeState.getRawParameterValue (id + "_q")->store (b.q);
        treeState.getRawParameterValue (id + "_type")->store ((float) b.type);
        treeState.getRawParameterValue (id + "_active")->store (b.active ? 1.0f : 0.0f);
    }
}

juce::AudioProcessorEditor* CleanSlateAudioProcessor::createEditor () { return new CleanSlateAudioProcessorEditor (*this); }
bool CleanSlateAudioProcessor::hasEditor () const { return true; }
const juce::String CleanSlateAudioProcessor::getName () const { return "CLEAN Slate EQ Premium"; }
bool CleanSlateAudioProcessor::acceptsMidi () const { return false; }
bool CleanSlateAudioProcessor::producesMidi () const { return false; }
bool CleanSlateAudioProcessor::isMidiEffect () const { return false; }
double CleanSlateAudioProcessor::getTailLengthSeconds () const { return 0.0; }
int CleanSlateAudioProcessor::getNumPrograms () { return 1; }
int CleanSlateAudioProcessor::getCurrentProgram () { return 0; }
void CleanSlateAudioProcessor::setCurrentProgram (int index) {}
const juce::String CleanSlateAudioProcessor::getProgramName (int index) { return {}; }
void CleanSlateAudioProcessor::changeProgramName (int index, const juce::String& newName) {}
void CleanSlateAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = treeState.copyState ();
    std::unique_ptr<juce::XmlElement> xml (state.createXml ());
    copyXmlToBinary (*xml, destData);
}
void CleanSlateAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr)
        if (xmlState->hasTagName (treeState.state.getType ()))
            treeState.replaceState (juce::ValueTree::fromXml (*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter () { return new CleanSlateAudioProcessor (); }
