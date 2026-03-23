#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "Presets.h"
#include "SurgicalFilter.h"

// Processing modes for each band
enum class BandMode
{
    Stereo = 0,
    Left,
    Right,
    Mid,
    Side
};

// Dynamic EQ modes
enum class DynamicMode
{
    Off = 0,
    Compressor,
    Expander
};

// Phase processing modes
enum class PhaseMode
{
    ZeroLatency = 0,
    NaturalPhase,
    LinearPhase
};

// Character/saturation modes
enum class CharacterMode
{
    Clean = 0,
    Subtle,     // Gentle transformer-style
    Warm        // Tube-style saturation
};

// Filter types (extended)
enum class FilterType
{
    HighPass = 0,
    Peak,
    LowPass,
    Notch,
    LowShelf,
    HighShelf,
    AllPass
};

// Band state structure
struct BandState
{
    float freq = 1000.0f;
    float gain = 0.0f;
    float q = 0.707f;
    float slope = 2.0f;           // 0-4 (6dB to 96dB)
    float attack = 10.0f;         // ms
    float release = 100.0f;       // ms
    float threshold = 0.0f;       // dB
    float ratio = 1.0f;         // 1:1 = off, >1 = comp, <1 = expand
    float dynamicGain = 0.0f;     // Current dynamic gain reduction
    float envelope = 0.0f;        // Envelope follower state
    int type = 1;                 // FilterType
    int mode = 0;                 // BandMode
    int dynamic = 0;            // DynamicMode
    bool active = true;
    bool solo = false;
};

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
    static constexpr int fftOrder = 12;        // 4096 samples for better resolution
    static constexpr int fftSize = 1 << fftOrder;
    static constexpr int scopeSize = 2048;   // Display resolution
    float fftData[fftSize * 2];
    float scopeData[scopeSize];
    float peakHoldData[scopeSize];
    bool nextFFTBlockReady = false;
    float tiltAmount = 0.0f;                   // dB/octave for display

    void pushNextSampleIntoFifo (float sample);
    void getFFTMagnitudes (float* result);
    void calculateScopeData();

    // AI Logic & Presets
    void findHarshResonances();
    void loadPreset (int index);

    // EQ Match
    void captureReference();
    void applyEqMatch();
    bool hasReferenceCapture() const { return referenceCaptured; }
    void clearReference() { referenceCaptured = false; }

    // EQ Sketch - create bands from response curve
    void createBandsFromSketch (const std::vector<std::pair<float, float>>& points);

    // Get current band states
    BandState& getBandState (int band) { return bandStates[band]; }
    const BandState& getBandState (int band) const { return bandStates[band]; }

    // Phase mode
    PhaseMode getPhaseMode() const { return phaseMode; }
    void setPhaseMode (PhaseMode mode);

    // Character mode
    CharacterMode getCharacterMode() const { return characterMode; }
    void setCharacterMode (CharacterMode mode);

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

    // For copy/paste
    juce::String getBandsAsXml() const;
    void setBandsFromXml (const juce::String& xmlString);

private:
    void updateFilters();
    void processWithZeroLatency (juce::AudioBuffer<float>& buffer);
    void processWithNaturalPhase (juce::AudioBuffer<float>& buffer);
    void processWithLinearPhase (juce::AudioBuffer<float>& buffer);
    void processDynamicEQ (int band, float sampleRate, float* leftSample, float* rightSample);
    void applySaturation (float* sample);

    // M/S encode/decode
    void encodeMS (float* left, float* right);
    void decodeMS (float* left, float* right);

    // Band states
    BandState bandStates[8];

    // Filters for each band and channel
    // For M/S we need separate processing: L/R and M/S
    SurgicalFilter filtersL[8], filtersR[8];      // Stereo processing
    SurgicalFilter filtersM[8], filtersS[8];      // M/S processing

    float mAutoGain { 1.0f };
    double currentSampleRate = 44100.0;

    // Phase mode
    PhaseMode phaseMode = PhaseMode::ZeroLatency;

    // Character mode
    CharacterMode characterMode = CharacterMode::Clean;
    juce::dsp::WaveShaper<float> saturationL, saturationR;

    // EQ Match
    float referenceSpectrum[scopeSize];
    bool referenceCaptured = false;

    // Linear Phase processing
    static constexpr int linearPhaseFFTOrder = 13;  // 8192 for convolution
    static constexpr int linearPhaseFFTSize = 1 << linearPhaseFFTOrder;
    juce::dsp::FFT linearPhaseFFT;
    std::vector<float> impulseResponse;
    std::vector<std::vector<float>> convolutionBufferL, convolutionBufferR;
    int convolutionIndex = 0;

    // FFT Components
    juce::dsp::FFT forwardFFT;
    juce::dsp::WindowingFunction<float> window;
    float fifo[fftSize];
    int fifoIndex = 0;

    // Sidechain
    bool hasSidechain = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CleanSlateAudioProcessor)
};
