#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
#include <algorithm>

PremiumLookAndFeel::PremiumLookAndFeel()
{
    setColour (juce::Slider::thumbColourId, juce::Colour (0xFFFFD700)); // Gold
    setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xFF1A1A1A));
    setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xFFFFD700).withAlpha (0.6f));
    setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF111111));
    setColour (juce::TextButton::textColourOffId, juce::Colours::grey);
}

void PremiumLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                           float sliderPosProportional, float rotaryStartAngle,
                                           float rotaryEndAngle, juce::Slider& slider)
{
    auto outline = slider.findColour (juce::Slider::rotarySliderOutlineColourId);
    auto fill    = slider.findColour (juce::Slider::rotarySliderFillColourId);

    auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (10);
    auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) / 2.0f;
    auto toAngle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
    auto lineW = 6.0f;
    auto arcRadius = radius - lineW * 0.5f;

    juce::Path backgroundArc;
    backgroundArc.addCentredArc (bounds.getCentreX(), bounds.getCentreY(), arcRadius, arcRadius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (outline);
    g.strokePath (backgroundArc, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    if (slider.isEnabled())
    {
        juce::Path valueArc;
        valueArc.addCentredArc (bounds.getCentreX(), bounds.getCentreY(), arcRadius, arcRadius, 0.0f, rotaryStartAngle, toAngle, true);
        
        // Gold Glow
        g.setColour (fill.withAlpha (0.3f));
        g.strokePath (valueArc, juce::PathStrokeType (lineW + 4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        
        g.setColour (fill);
        g.strokePath (valueArc, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    auto thumbWidth = lineW * 2.0f;
    juce::Point<float> thumbPoint (bounds.getCentreX() + arcRadius * std::cos (toAngle - juce::MathConstants<float>::halfPi),
                                   bounds.getCentreY() + arcRadius * std::sin (toAngle - juce::MathConstants<float>::halfPi));

    g.setColour (slider.findColour (juce::Slider::thumbColourId));
    g.fillEllipse (juce::Rectangle<float> (thumbWidth, thumbWidth).withCentre (thumbPoint));
}

void PremiumLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour,
                                              bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    auto cornerSize = 6.0f;
    auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);

    auto baseColour = backgroundColour.withMultipliedSaturation (button.hasKeyboardFocus (true) ? 1.3f : 0.9f)
                                      .withMultipliedAlpha (button.isEnabled() ? 1.0f : 0.5f);

    if (shouldDrawButtonAsDown || shouldDrawButtonAsHighlighted)
        baseColour = baseColour.brighter (0.1f);

    g.setColour (baseColour);
    g.fillRoundedRectangle (bounds, cornerSize);

    g.setColour (button.findColour (juce::ComboBox::outlineColourId));
    g.drawRoundedRectangle (bounds, cornerSize, 1.0f);
}

CleanSlateAudioProcessorEditor::CleanSlateAudioProcessorEditor (CleanSlateAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel (&premiumLookAndFeel);

    // Preset Selector
    addAndMakeVisible (presetSelector);
    auto presets = Presets::getFactoryPresets();
    for (int i = 0; i < presets.size(); ++i)
        presetSelector.addItem (presets[i].name, i + 1);
    
    presetSelector.setSelectedItemIndex (0, juce::dontSendNotification);
    presetSelector.onChange = [this] { audioProcessor.loadPreset (presetSelector.getSelectedItemIndex()); };

    presetAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.treeState, "preset", presetSelector);

    freqSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    freqSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);
    addAndMakeVisible (freqSlider);

    gainSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    gainSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);
    addAndMakeVisible (gainSlider);

    qSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    qSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);
    addAndMakeVisible (qSlider);

    for (int i = 0; i < 8; ++i)
    {
        bandButtons[i].setButtonText (juce::String (i + 1));
        bandButtons[i].setClickingTogglesState (true);
        bandButtons[i].setRadioGroupId (123);
        addAndMakeVisible (bandButtons[i]);
        
        bandButtons[i].onClick = [this, i] { 
            selectedBand = i; 
            updateAttachments(); 
        };
    }
    
    bandButtons[1].setToggleState (true, juce::sendNotification);
    
    addAndMakeVisible (resKillButton);
    addAndMakeVisible (smartLearnButton);
    addAndMakeVisible (monoSubButton);
    addAndMakeVisible (phaseFlipButton);
    
    resKillButton.setClickingTogglesState (true);
    smartLearnButton.setClickingTogglesState (true);
    monoSubButton.setClickingTogglesState (true);
    phaseFlipButton.setClickingTogglesState (true);

    updateAttachments();

    // OpenGL Setup
    openGLContext.setRenderer (this);
    openGLContext.setContinuousRepainting (true);
    openGLContext.attachTo (*this);

    setSize (1000, 650);
    startTimerHz (60);
}

CleanSlateAudioProcessorEditor::~CleanSlateAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void CleanSlateAudioProcessorEditor::updateAttachments()
{
    juce::String id = "band" + juce::String (selectedBand);
    
    freqAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.treeState, id + "_freq", freqSlider);
    gainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.treeState, id + "_gain", gainSlider);
    qAttachment    = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.treeState, id + "_q", qSlider);
}

void CleanSlateAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Dark luxury background
    juce::ColourGradient grad (juce::Colour (0xFF050505), 0, 0, juce::Colour (0xFF0F0F0F), 0, getHeight(), false);
    g.setGradientFill (grad);
    g.fillAll();

    // Subtle Grid
    g.setColour (juce::Colour (0xFF222222).withAlpha (0.3f));
    for (int i = 0; i < getWidth(); i += 40) g.drawVerticalLine (i, 0.0f, (float)getHeight());
    for (int i = 0; i < getHeight(); i += 40) g.drawHorizontalLine (i, 0.0f, (float)getWidth());

    // Header
    g.setColour (juce::Colours::white);
    g.setFont (juce::Font ("Arial", 32.0f, juce::Font::bold | juce::Font::italic));
    g.drawText ("CLEAN SLATE", 40, 30, 300, 40, juce::Justification::left);
    
    g.setColour (juce::Colour (0xFFFFD700)); // Gold
    g.setFont (juce::Font ("Arial", 10.0f, juce::Font::bold));
    g.drawText ("SURGICAL AI EQ", 42, 65, 200, 15, juce::Justification::left);

    // Labels for focused knobs
    g.setColour (juce::Colours::grey);
    g.setFont (14.0f);
    g.drawText ("FREQUENCY", 100, 380, 200, 20, juce::Justification::centred);
    g.drawText ("GAIN", 300, 380, 200, 20, juce::Justification::centred);
    g.drawText ("Q", 500, 380, 200, 20, juce::Justification::centred);
    
    g.setColour (juce::Colour (0xFFFFD700).withAlpha (0.5f));
    g.drawText ("SELECTED BAND: " + juce::String (selectedBand + 1), 300, 420, 200, 20, juce::Justification::centred);
}

void CleanSlateAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    
    // Header layout
    auto topBar = area.removeFromTop (100);
    presetSelector.setBounds (topBar.removeFromRight (200).reduced (20, 30));
    
    monoSubButton.setBounds (400, 40, 80, 30);
    phaseFlipButton.setBounds (490, 40, 40, 30);
    resKillButton.setBounds (550, 40, 100, 30);
    smartLearnButton.setBounds (660, 40, 100, 30);

    // Main Knobs
    auto knobArea = area.removeFromTop (300).reduced (40, 0);
    freqSlider.setBounds (knobArea.removeFromLeft (area.getWidth() / 3).reduced (20));
    gainSlider.setBounds (knobArea.removeFromLeft (area.getWidth() / 3).reduced (20));
    qSlider.setBounds (knobArea.reduced (20));

    // Band Selector Bar
    auto bottomBar = area.removeFromBottom (60).reduced (100, 10);
    auto buttonWidth = bottomBar.getWidth() / 8;
    for (int i = 0; i < 8; ++i)
        bandButtons[i].setBounds (bottomBar.removeFromLeft (buttonWidth).reduced (4));
}

void CleanSlateAudioProcessorEditor::timerCallback()
{
    if (audioProcessor.nextFFTBlockReady)
    {
        audioProcessor.getFFTMagnitudes (scopeData);
        audioProcessor.nextFFTBlockReady = false;
        openGLContext.triggerRepaint();
    }
}

void CleanSlateAudioProcessorEditor::newOpenGLContextCreated()
{
    vertexShader = 
        "attribute vec4 position;\n"
        "attribute vec2 texCoords;\n"
        "varying vec2 vUv;\n"
        "void main() {\n"
        "    gl_Position = position;\n"
        "    vUv = texCoords;\n"
        "}";

    fragmentShader = 
        "uniform float uTime;\n"
        "uniform sampler2D uFFT;\n"
        "varying vec2 vUv;\n"
        "void main() {\n"
        "    vec2 uv = vUv;\n"
        "    float fft = texture2D(uFFT, vec2(uv.x, 0.5)).r;\n"
        "    vec3 color = vec3(0.01);\n"
        "    if (uv.y < fft) {\n"
        "        color = mix(vec3(0.05), vec3(1.0, 0.84, 0.0), uv.y / fft);\n" // Gold mix
        "    }\n"
        "    gl_FragColor = vec4(color, 1.0);\n"
        "}";

    shader.reset (new juce::OpenGLShaderProgram (openGLContext));
    if (shader->addVertexShader (vertexShader) && shader->addFragmentShader (fragmentShader))
    {
        shader->link();
    }
}

void CleanSlateAudioProcessorEditor::renderOpenGL()
{
    juce::OpenGLHelpers::clear (juce::Colours::black);
    
    if (shader == nullptr) return;
    
    shader->use();
    // In a real implementation, we would update the texture with scopeData here
}

void CleanSlateAudioProcessorEditor::openGLContextClosing()
{
    shader.reset();
}
