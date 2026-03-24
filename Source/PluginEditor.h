#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_opengl/juce_opengl.h>
#include "PluginProcessor.h"
#include "Presets.h"
#include "SurgicalFilter.h"
#include <memory>

// --- Custom Look and Feel ---
class PremiumLookAndFeel : public juce::LookAndFeel_V4
{
public:
    PremiumLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&,
                               bool, bool) override;
    void drawComboBox (juce::Graphics& g, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox& box) override;
};

// --- Interactive Graph Component ---
struct EqGraphComponent : public juce::Component
{
    EqGraphComponent (CleanSlateAudioProcessor& p) : processor (p) {}

    void paint (juce::Graphics& g) override;
    void mouseDoubleClick (const juce::MouseEvent& e) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;
    void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& d) override;
    void mouseMove (const juce::MouseEvent& e) override;

    // Coordinate Mapping Helpers
    float getXForFreq (float freq);
    float getFreqForX (float x);
    float getYForGain (float gain);
    float getGainForY (float y);

    // EQ Sketch - generate bands from drawn curve
    void finishSketch();

    CleanSlateAudioProcessor& processor;
    int hoveredBand = -1;
    int draggingBand = -1;
    juce::Point<float> ghostPos;

    // EQ Sketch
    bool isSketching = false;
    std::vector<juce::Point<float>> sketchPoints;
    juce::Point<float> lastSketchPoint;
};

// --- Floating Mini-HUD Component ---
struct MiniHudComponent : public juce::Component
{
    MiniHudComponent();
    void resized() override;

    juce::ComboBox shapeBox, slopeBox, modeBox, dynamicBox;
    juce::Label attackLabel { "", "Attack" };
    juce::Label releaseLabel { "", "Release" };
    juce::Slider attackSlider, releaseSlider;
};

class CleanSlateAudioProcessorEditor : public juce::AudioProcessorEditor,
                                         public juce::Timer,
                                         public juce::OpenGLRenderer
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

    // Copy/Paste
    void copyEqCurve();
    void pasteEqCurve();

    int selectedBand = -1;
    float currentScale = 1.0f;

    // UI Scaling
    juce::PopupMenu uiScaleMenu;
    juce::TextButton uiScaleButton { "100%" };
    
    void phaseModeMenu();
    void characterModeMenu();
    void showUiScaleMenu();
    void uiScaleMenuAction();

private:
    CleanSlateAudioProcessor& audioProcessor;
    PremiumLookAndFeel premiumLookAndFeel;

    EqGraphComponent graph;
    MiniHudComponent hud;

    // Main parameter sliders
    juce::Slider freqSlider, gainSlider, qSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> freqAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> qAttachment;

    // Dynamic EQ controls
    juce::Slider thresholdSlider, ratioSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> thresholdAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> ratioAttachment;

    // Global controls
    juce::Slider tiltSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> tiltAttachment;

    // Buttons
    juce::TextButton bandButtons[8];
    juce::TextButton resKillButton { "RES-KILL" };
    juce::TextButton smartLearnButton { "SMART LEARN" };
    juce::TextButton monoSubButton { "MONO SUB" };
     juce::TextButton phaseFlipButton { "Ø" };
     juce::TextButton copyButton { "COPY" };
     juce::TextButton pasteButton { "PASTE" };
     juce::TextButton eqMatchButton { "EQ MATCH" };
     juce::TextButton referenceButton { "CAPTURE REF" };
     juce::TextButton phaseModeButton { "Zero Latency" };
     juce::TextButton characterButton { "Clean" };
     juce::TextButton sketchButton { "SKETCH" };
     juce::TextButton deltaModeButton { "DELTA" };
     juce::TextButton sidechainButton { "SIDECHAIN" };
     juce::TextButton lookaheadButton { "LOOKAHEAD" };

    juce::ComboBox presetSelector;

    // Spectrum analyzer
    juce::OpenGLContext openGLContext;
    std::unique_ptr<juce::OpenGLShaderProgram> shader;
    juce::String vertexShader, fragmentShader;
    GLuint fftTexture = 0;
    float scopeData[2048];
    float peakHoldData[2048];
    int peakHoldCounter[2048];

    void updateAttachments();
    void updateButtonStates();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CleanSlateAudioProcessorEditor)
};
