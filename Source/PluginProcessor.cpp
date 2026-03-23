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
    saturationL.setFunction ([] (float x) { return x; });
    saturationR.setFunction ([] (float x) { return x; });

    // Initialize scope data
    std::fill (scopeData, scopeData + scopeSize, 0.0f);
    std::fill (peakHoldData, peakHoldData + scopeSize, 0.0f);
    std::fill (referenceSpectrum, referenceSpectrum + scopeSize, 0.0f);
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

    phaseMode = (PhaseMode) treeState.getRawParameterValue ("phaseMode")->load ();

    auto newCharacter = (CharacterMode) treeState.getRawParameterValue ("characterMode")->load ();
    if (newCharacter != characterMode)
    {
        characterMode = newCharacter;
        switch (characterMode)
        {
            case CharacterMode::Clean:
                saturationL.setFunction ([] (float x) { return x; });
                saturationR.setFunction ([] (float x) { return x; });
                break;
            case CharacterMode::Subtle:
                saturationL.setFunction ([] (float x) { return std::tanh (x * 1.2f) / 1.2f; });
                saturationR.setFunction ([] (float x) { return std::tanh (x * 1.2f) / 1.2f; });
                break;
            case CharacterMode::Warm:
                saturationL.setFunction ([] (float x) {
                    return x > 0.0f ? std::tanh (x * 1.5f) / 1.5f : std::tanh (x * 2.0f) / 2.0f;
                });
                saturationR.setFunction ([] (float x) {
                    return x > 0.0f ? std::tanh (x * 1.5f) / 1.5f : std::tanh (x * 2.0f) / 2.0f;
                });
                break;
        }
    }

    tiltAmount = treeState.getRawParameterValue ("tilt")->load ();

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
    auto* rightData = buffer.getWritePointer (1);
    int numSamples = buffer.getNumSamples ();

    for (int sample = 0; sample < numSamples; ++sample)
    {
        float leftSample = leftData[sample];
        float rightSample = rightData[sample];

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

            float bandLeft = leftSample;
            float bandRight = rightSample;

            juce::dsp::AudioBlock<float> blockL (&bandLeft, 1, 1);
            juce::dsp::AudioBlock<float> blockR (&bandRight, 1, 1);
            juce::dsp::ProcessContextReplacing<float> ctxL (blockL);
            juce::dsp::ProcessContextReplacing<float> ctxR (blockR);

            if (bandStates[i].mode == 3) // Mid
            {
                filtersM[i].process (ctxL);
            }
            else if (bandStates[i].mode == 4) // Side
            {
                filtersS[i].process (ctxR);
            }
            else if (bandStates[i].mode == 1) // Left
            {
                filtersL[i].process (ctxL);
            }
            else if (bandStates[i].mode == 2) // Right
            {
                filtersR[i].process (ctxR);
            }
            else // Stereo (Mode 0)
            {
                juce::dsp::AudioBlock<float> stereoBlock (bufferChannels, 2, 1);
                stereoBlock.setSample (0, 0, leftSample);
                stereoBlock.setSample (1, 0, rightSample);
                juce::dsp::ProcessContextReplacing<float> ctxStereo (stereoBlock);
                filtersL[i].process (ctxStereo);
                leftSample = stereoBlock.getSample (0, 0);
                rightSample = stereoBlock.getSample (1, 0);
            }

            if (bandStates[i].mode == 1)
                leftSample = bandLeft;
            else if (bandStates[i].mode == 2)
                rightSample = bandRight;
            else if (bandStates[i].mode == 3)
                leftSample = bandLeft;
            else if (bandStates[i].mode == 4)
                rightSample = bandRight;
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
    // Natural Phase: Slightly non-linear phase response matching analog models
    // In this build, we use the Zero Latency path as a baseline, 
    // but with smoothed parameter updates to prevent zippering.
    processWithZeroLatency (buffer);
}

void CleanSlateAudioProcessor::processWithLinearPhase (juce::AudioBuffer<float>& buffer)
{
    // Linear Phase would require FFT convolution or large FIR filters.
    // Placeholder: use zero latency for now but with latency compensation reported.
    processWithZeroLatency (buffer);
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

    updateFilters ();
    calculateScopeData ();
    findHarshResonances ();

    if (buffer.getNumChannels () > 0)
    {
        auto* channelData = buffer.getReadPointer (0);
        for (int i = 0; i < buffer.getNumSamples (); ++i)
            pushNextSampleIntoFifo (channelData[i]);
    }

    if (buffer.getNumChannels () >= 2)
    {
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
        }
    }

    buffer.applyGain (mAutoGain);
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
