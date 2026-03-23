#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_opengl/juce_opengl.h>
#include "PluginProcessor.h"
#include <memory>

class PremiumLookAndFeel : public juce::LookAndFeel_V4
{
public:
    PremiumLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
};

class CleanSlateAudioProcessorEditor : public juce::AudioProcessorEditor, 
                                         public juce::Timer,
                                         private juce::OpenGLRenderer
{
public:
    CleanSlateAudioProcessorEditor (CleanSlateAudioProcessor&);
    ~CleanSlateAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

    // OpenGLRenderer
    void newOpenGLContextCreated() override;
    void renderOpenGL() override;
    void openGLContextClosing() override;

private:
    CleanSlateAudioProcessor& audioProcessor;
    PremiumLookAndFeel premiumLookAndFeel;
    
    int selectedBand = 1; // Default to first peak band (index 1)

    juce::Slider freqSlider, gainSlider, qSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> freqAttachment, gainAttachment, qAttachment;

    juce::TextButton bandButtons[8];
    juce::TextButton resKillButton { "RES-KILL (SOON)" }, smartLearnButton { "SMART LEARN (SOON)" };
    juce::TextButton monoSubButton { "MONO SUB" }, phaseFlipButton { "Ø" };

    juce::ComboBox presetSelector;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> presetAttachment;

    // OpenGL Spectrum
    juce::OpenGLContext openGLContext;
    std::unique_ptr<juce::OpenGLShaderProgram> shader;
    juce::String vertexShader, fragmentShader;
    GLuint fftTexture;
    float scopeData[1024];

    void updateAttachments();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CleanSlateAudioProcessorEditor)
};
