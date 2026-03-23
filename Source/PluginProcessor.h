#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "Presets.h"

class CleanSlateAudioProcessor : public juce::AudioProcessor
{
public:
    CleanSlateAudioProcessor();
    ~CleanSlateAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const juce::AudioProcessor::BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    // FFT & Spectrum Data
    static constexpr int fftOrder = 11;
    static constexpr int fftSize = 1 << fftOrder;
    float fftData[fftSize * 2];
    bool nextFFTBlockReady = false;
    
    void pushNextSampleIntoFifo (float sample);
    void getFFTMagnitudes (float* result);

    // AI Logic & Presets
    void findHarshResonances();
    void loadPreset (int index);

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState treeState;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    void updateFilters();
    
    // 8 Bands: HighPass, 6x Peak/Notch/Bell, LowPass
    using Filter = juce::dsp::IIR::Filter<float>;
    using FilterChain = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter, Filter, Filter, Filter, Filter>;
    
    FilterChain leftChain, rightChain;
    float mAutoGain { 1.0f };

    // FFT Components
    juce::dsp::FFT forwardFFT;
    juce::dsp::WindowingFunction<float> window;
    float fifo[fftSize];
    int fifoIndex = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CleanSlateAudioProcessor)
};
