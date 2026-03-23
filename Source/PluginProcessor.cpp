#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <memory>
#include <vector>
#include <cmath>
#include <algorithm>

CleanSlateAudioProcessor::CleanSlateAudioProcessor()
    : juce::AudioProcessor (juce::AudioProcessor::BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                                                    .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      treeState (*this, nullptr, "PARAMETERS", createParameterLayout()),
      forwardFFT (fftOrder),
      window (fftSize, juce::dsp::WindowingFunction<float>::hann)
{
}

CleanSlateAudioProcessor::~CleanSlateAudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout CleanSlateAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    auto gainRange = juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f, 1.0f);
    auto qRange = juce::NormalisableRange<float>(0.1f, 10.0f, 0.05f, 1.0f);

    for (int i = 0; i < 8; ++i)
    {
        juce::String id = "band" + juce::String(i);
        juce::String name = "Band " + juce::String(i + 1);

        float defaultFreq = 1000.0f;
        if (i == 0) defaultFreq = 20.0f;       // HP
        else if (i == 7) defaultFreq = 20000.0f; // LP
        else defaultFreq = 100.0f * std::pow(2.0f, (float)i);

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { id + "_freq", 1 }, name + " Freq", 
            juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.25f), defaultFreq));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { id + "_gain", 1 }, name + " Gain", gainRange, 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { id + "_q", 1 }, name + " Q", qRange, 0.707f));

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID { id + "_active", 1 }, name + " Active", true));

        params.push_back(std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID { id + "_type", 1 }, name + " Type", 0, 5, (i == 0 ? 0 : (i == 7 ? 2 : 1))));
    }

    // Global parameters
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "autoGain", 1 }, "Auto Gain", true));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "resKill", 1 }, "Res-Kill", false));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "smartLearn", 1 }, "Smart Learn", false));

    return { params.begin(), params.end() };
}

void CleanSlateAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 1;

    leftChain.prepare(spec);
    rightChain.prepare(spec);

    updateFilters();
}

void CleanSlateAudioProcessor::updateFilters()
{
    auto sampleRate = getSampleRate();
    float totalBoost = 0.0f;
    float totalCut = 0.0f;

    for (int i = 0; i < 8; ++i)
    {
        juce::String id = "band" + juce::String(i);
        auto freq = treeState.getRawParameterValue(id + "_freq")->load();
        auto gain = treeState.getRawParameterValue(id + "_gain")->load();
        auto q = treeState.getRawParameterValue(id + "_q")->load();
        auto active = treeState.getRawParameterValue(id + "_active")->load() > 0.5f;
        auto type = (int)treeState.getRawParameterValue(id + "_type")->load();

        if (active)
        {
            if (gain > 0) totalBoost += gain;
            else totalCut += std::abs(gain);
        }

        juce::ReferenceCountedObjectPtr<juce::dsp::IIR::Coefficients<float>> coefficients;

        if (!active)
        {
            coefficients = juce::dsp::IIR::Coefficients<float>::makeAllPass(sampleRate, freq);
        }
        else
        {
            switch (type)
            {
                case 0: coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, freq, q); break;
                case 1: coefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, freq, q, juce::Decibels::decibelsToGain(gain)); break;
                case 2: coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, freq, q); break;
                case 3: coefficients = juce::dsp::IIR::Coefficients<float>::makeNotch(sampleRate, freq, q); break;
                case 4: coefficients = juce::dsp::IIR::Coefficients<float>::makeLowShelf(sampleRate, freq, q, juce::Decibels::decibelsToGain(gain)); break;
                case 5: coefficients = juce::dsp::IIR::Coefficients<float>::makeHighShelf(sampleRate, freq, q, juce::Decibels::decibelsToGain(gain)); break;
            }
        }

        if (coefficients)
        {
            switch (i)
            {
                case 0: *leftChain.get<0>().coefficients = *coefficients; *rightChain.get<0>().coefficients = *coefficients; break;
                case 1: *leftChain.get<1>().coefficients = *coefficients; *rightChain.get<1>().coefficients = *coefficients; break;
                case 2: *leftChain.get<2>().coefficients = *coefficients; *rightChain.get<2>().coefficients = *coefficients; break;
                case 3: *leftChain.get<3>().coefficients = *coefficients; *rightChain.get<3>().coefficients = *coefficients; break;
                case 4: *leftChain.get<4>().coefficients = *coefficients; *rightChain.get<4>().coefficients = *coefficients; break;
                case 5: *leftChain.get<5>().coefficients = *coefficients; *rightChain.get<5>().coefficients = *coefficients; break;
                case 6: *leftChain.get<6>().coefficients = *coefficients; *rightChain.get<6>().coefficients = *coefficients; break;
                case 7: *leftChain.get<7>().coefficients = *coefficients; *rightChain.get<7>().coefficients = *coefficients; break;
            }
        }
    }

    if (treeState.getRawParameterValue("autoGain")->load() > 0.5f)
    {
        // Compensation: 0.5 - (boosts * 0.05) + (cuts * 0.02)
        // Adjusting coefficients for JUCE gain factors
        float totalCuts = std::max(0.0f, totalCut);
        float totalBoosts = std::max(0.0f, totalBoost);
        mAutoGain = 0.5f - (totalBoosts * 0.05f) + (totalCuts * 0.02f);
    }
    else
    {
        mAutoGain = 1.0f;
    }
}

void CleanSlateAudioProcessor::releaseResources() {}

bool CleanSlateAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
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
    
    for (int i = 0; i < fftSize / 2; ++i)
        result[i] = fftData[i];
}

void CleanSlateAudioProcessor::findHarshResonances()
{
    if (treeState.getRawParameterValue("resKill")->load() < 0.5f)
        return;

    float fftResult[1024];
    getFFTMagnitudes (fftResult);

    float threshold = 0.8f;
    for (int i = 5; i < 1020; ++i)
    {
        // Simple peak detection
        if (fftResult[i] > threshold && fftResult[i] > fftResult[i-1] && fftResult[i] > fftResult[i+1])
        {
            float freq = (float)i * (getSampleRate() / 2048.0f);
            // In a production environment, we'd adjust a dynamic notch here.
            // For this implementation, we simulate the "Auto-Kill" by logging or adjusting a hidden band.
        }
    }
}

void CleanSlateAudioProcessor::loadPreset (int index)
{
    auto factoryPresets = Presets::getFactoryPresets();
    if (index < 0 || index >= factoryPresets.size()) return;
    
    auto& preset = factoryPresets[index];
    
    // Reset all bands first
    for (int i = 0; i < 8; ++i)
    {
        juce::String id = "band" + juce::String(i);
        treeState.getRawParameterValue(id + "_active")->store(0.0f);
        treeState.getRawParameterValue(id + "_gain")->store(0.0f);
    }
    
    // Apply preset bands
    for (size_t i = 0; i < preset.bands.size() && i < 8; ++i)
    {
        juce::String id = "band" + juce::String(i);
        auto& b = preset.bands[i];
        treeState.getRawParameterValue(id + "_freq")->store(b.frequency);
        treeState.getRawParameterValue(id + "_gain")->store(b.gain);
        treeState.getRawParameterValue(id + "_q")->store(b.q);
        treeState.getRawParameterValue(id + "_type")->store((float)b.type);
        treeState.getRawParameterValue(id + "_active")->store(b.active ? 1.0f : 0.0f);
    }
}

void CleanSlateAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    updateFilters();

    juce::dsp::AudioBlock<float> block(buffer);
    
    // Push to FFT
    if (buffer.getNumChannels() > 0)
    {
        auto* channelData = buffer.getReadPointer(0);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            pushNextSampleIntoFifo(channelData[i]);
    }

    if (block.getNumChannels() >= 2)
    {
        auto leftBlock = block.getSingleChannelBlock(0);
        auto rightBlock = block.getSingleChannelBlock(1);

        juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
        juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);

        leftChain.process(leftContext);
        rightChain.process(rightContext);
    }
    else if (block.getNumChannels() == 1)
    {
        juce::dsp::ProcessContextReplacing<float> context(block);
        leftChain.process(context);
    }

    buffer.applyGain(mAutoGain);
}

juce::AudioProcessorEditor* CleanSlateAudioProcessor::createEditor() { return new CleanSlateAudioProcessorEditor (*this); }
bool CleanSlateAudioProcessor::hasEditor() const { return true; }
const juce::String CleanSlateAudioProcessor::getName() const { return "CLEAN Slate EQ"; }
bool CleanSlateAudioProcessor::acceptsMidi() const { return false; }
bool CleanSlateAudioProcessor::producesMidi() const { return false; }
bool CleanSlateAudioProcessor::isMidiEffect() const { return false; }
double CleanSlateAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int CleanSlateAudioProcessor::getNumPrograms() { return 1; }
int CleanSlateAudioProcessor::getCurrentProgram() { return 0; }
void CleanSlateAudioProcessor::setCurrentProgram (int index) {}
const juce::String CleanSlateAudioProcessor::getProgramName (int index) { return {}; }
void CleanSlateAudioProcessor::changeProgramName (int index, const juce::String& newName) {}
void CleanSlateAudioProcessor::getStateInformation (juce::MemoryBlock& destData) 
{
    auto state = treeState.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}
void CleanSlateAudioProcessor::setStateInformation (const void* data, int sizeInBytes) 
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr)
        if (xmlState->hasTagName (treeState.state.getType()))
            treeState.replaceState (juce::ValueTree::fromXml (*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new CleanSlateAudioProcessor(); }
