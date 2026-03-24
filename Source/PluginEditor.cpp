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
    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xFF111111));
    setColour (juce::ComboBox::outlineColourId, juce::Colour (0xFF333333));
}

void PremiumLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool isButtonDown,
                                       int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox& box)
{
    auto cornerSize = 4.0f;
    auto bounds = juce::Rectangle<int> (0, 0, width, height).toFloat().reduced (0.5f);

    g.setColour (box.findColour (juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle (bounds, cornerSize);

    g.setColour (box.findColour (juce::ComboBox::outlineColourId));
    g.drawRoundedRectangle (bounds, cornerSize, 1.0f);

    auto arrowZone = juce::Rectangle<int> (width - 25, 0, 25, height).toFloat();
    juce::Path path;
    path.startNewSubPath (arrowZone.getCentreX() - 4, arrowZone.getCentreY() - 2);
    path.lineTo (arrowZone.getCentreX(), arrowZone.getCentreY() + 3);
    path.lineTo (arrowZone.getCentreX() + 4, arrowZone.getCentreY() - 2);
    
    g.setColour (juce::Colours::grey);
    g.strokePath (path, juce::PathStrokeType (1.5f));
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

// --- MiniHudComponent Implementation ---

MiniHudComponent::MiniHudComponent()
{
    shapeBox.addItemList ({"Peak", "Low Shelf", "High Shelf", "Notch", "High Pass", "Low Pass", "All Pass"}, 1);
    slopeBox.addItemList ({"6 dB", "12 dB", "24 dB", "48 dB", "96 dB", "Brickwall"}, 1);
    modeBox.addItemList ({"Stereo", "Left", "Right", "Mid", "Side"}, 1);
    dynamicBox.addItemList ({"Off", "Compress", "Expand"}, 1);

    for (auto* b : { &shapeBox, &slopeBox, &modeBox, &dynamicBox })
    {
        addAndMakeVisible (b);
        b->setJustificationType (juce::Justification::centred);
    }

    addAndMakeVisible (attackLabel);
    addAndMakeVisible (releaseLabel);
    attackLabel.setText ("ATTACK", juce::dontSendNotification);
    releaseLabel.setText ("RELEASE", juce::dontSendNotification);
    attackLabel.setFont (juce::Font (10.0f, juce::Font::bold));
    releaseLabel.setFont (juce::Font (10.0f, juce::Font::bold));
    attackLabel.setJustificationType (juce::Justification::centred);
    releaseLabel.setJustificationType (juce::Justification::centred);
    
    attackSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    releaseSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    attackSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    releaseSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    
    addAndMakeVisible (attackSlider);
    addAndMakeVisible (releaseSlider);
}

void MiniHudComponent::resized()
{
    auto r = getLocalBounds().reduced (10);
    auto top = r.removeFromTop (25);
    shapeBox.setBounds (top.removeFromLeft (top.getWidth() / 2).reduced (2, 0));
    slopeBox.setBounds (top.reduced (2, 0));
    
    auto mid = r.removeFromTop (25).withY (r.getY() + 5);
    modeBox.setBounds (mid.removeFromLeft (mid.getWidth() / 2).reduced (2, 0));
    dynamicBox.setBounds (mid.reduced (2, 0));
    
    auto bot = r.removeFromTop (45).withY (r.getY() + 10);
    auto left = bot.removeFromLeft (bot.getWidth() / 2).reduced (2, 0);
    attackLabel.setBounds (left.removeFromTop (15));
    attackSlider.setBounds (left);
    
    auto right = bot.reduced (2, 0);
    releaseLabel.setBounds (right.removeFromTop (15));
    releaseSlider.setBounds (right);
}

// --- EqGraphComponent Implementation ---

void EqGraphComponent::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Draw Frequency Grid (Logarithmic)
    g.setColour (juce::Colours::white.withAlpha (0.1f));
    for (float f : { 20.0f, 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f, 20000.0f })
    {
        float x = getXForFreq (f);
        g.drawVerticalLine (int (x), 0.0f, bounds.getHeight());
    }

    // Draw Gain Grid (Linear)
    for (float db : { -24.0f, -12.0f, 0.0f, 12.0f, 24.0f })
    {
        float y = getYForGain (db);
        g.drawHorizontalLine (int (y), 0.0f, bounds.getWidth());
    }

    // Draw Main EQ Curve
    juce::Path curve;
    bool started = false;

    for (int x = 0; x <= getWidth(); x += 2)
    {
        float freq = getFreqForX ((float)x);
        float totalGain = 0.0f;

        for (int i = 0; i < 8; ++i)
        {
            auto* freqParam = processor.treeState.getRawParameterValue ("band" + juce::String (i) + "_freq");
            auto* gainParam = processor.treeState.getRawParameterValue ("band" + juce::String (i) + "_gain");
            auto* qParam = processor.treeState.getRawParameterValue ("band" + juce::String (i) + "_q");
            auto* activeParam = processor.treeState.getRawParameterValue ("band" + juce::String (i) + "_active");

            if (activeParam->load() > 0.5f)
            {
                float f0 = freqParam->load();
                float g0 = gainParam->load();
                float q = qParam->load();
                
                // Peak filter approximation for curve rendering
                float dist = std::abs (std::log10 (freq / f0));
                totalGain += g0 * std::exp (-dist * dist * q * 5.0f);
            }
        }

        float y = getYForGain (totalGain);
        if (!started) { curve.startNewSubPath (x, y); started = true; }
        else curve.lineTo (x, y);
    }

    // Fill under curve (SVG Gradient look)
    juce::ColourGradient grad (juce::Colours::yellow.withAlpha (0.3f), 0, bounds.getCentreY(),
                               juce::Colours::yellow.withAlpha (0.0f), 0, bounds.getBottom(), false);
    g.setGradientFill (grad);
    juce::Path fillPath = curve;
    fillPath.lineTo (getWidth(), getHeight());
    fillPath.lineTo (0, getHeight());
    fillPath.closeSubPath();
    g.fillPath (fillPath);

    // Stroke curve
    g.setColour (juce::Colours::yellow);
    g.strokePath (curve, juce::PathStrokeType (2.5f));

    // Draw Nodes
    for (int i = 0; i < 8; ++i)
    {
        auto* activeParam = processor.treeState.getRawParameterValue ("band" + juce::String (i) + "_active");
        if (activeParam->load() > 0.5f)
        {
            float f = processor.treeState.getRawParameterValue ("band" + juce::String (i) + "_freq")->load();
            float gn = processor.treeState.getRawParameterValue ("band" + juce::String (i) + "_gain")->load();
            
            auto pos = juce::Point<float> (getXForFreq (f), getYForGain (gn));
            
            bool isSelected = false;
            if (auto* editor = dynamic_cast<CleanSlateAudioProcessorEditor*>(getParentComponent()))
                isSelected = (editor->selectedBand == i);

            g.setColour (isSelected ? juce::Colours::white : juce::Colours::white.withAlpha (0.7f));
            g.fillEllipse (pos.x - 5, pos.y - 5, 10, 10);
            
            if (i == hoveredBand || isSelected)
                g.drawEllipse (pos.x - 8, pos.y - 8, 16, 16, 1.5f);
        }
    }

    // Ghost Slope Preview or Sketch Path
    if (isSketching)
    {
        g.setColour (juce::Colours::cyan.withAlpha (0.6f));
        juce::Path sketchPath;
        if (sketchPoints.size() > 1)
        {
            sketchPath.startNewSubPath (sketchPoints[0]);
            for (int i = 1; i < sketchPoints.size(); ++i)
                sketchPath.lineTo (sketchPoints[i]);
            g.strokePath (sketchPath, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
    }
    else if (hoveredBand == -1 && ghostPos.x > 0)
    {
        g.setColour (juce::Colours::white.withAlpha (0.15f));
        g.drawEllipse (ghostPos.x - 4, ghostPos.y - 4, 8, 8, 1.0f);
    }
}

void EqGraphComponent::mouseDoubleClick (const juce::MouseEvent& e)
{
    for (int i = 0; i < 8; ++i)
    {
        auto* activeParam = processor.treeState.getRawParameterValue ("band" + juce::String (i) + "_active");
        if (activeParam->load() < 0.5f)
        {
            processor.treeState.getParameter ("band" + juce::String (i) + "_freq")->setValueNotifyingHost (processor.treeState.getParameterRange ("band" + juce::String (i) + "_freq").convertTo0to1 (getFreqForX (e.position.x)));
            processor.treeState.getParameter ("band" + juce::String (i) + "_gain")->setValueNotifyingHost (processor.treeState.getParameterRange ("band" + juce::String (i) + "_gain").convertTo0to1 (getGainForY (e.position.y)));
            processor.treeState.getParameter ("band" + juce::String (i) + "_active")->setValueNotifyingHost (1.0f);

            if (auto* editor = dynamic_cast<CleanSlateAudioProcessorEditor*>(getParentComponent()))
                editor->selectedBand = i;

            processor.markFiltersForUpdate();  // FIX #7: Tell processor to update filters
            repaint();
            return;
        }
    }
}

void EqGraphComponent::mouseDown (const juce::MouseEvent& e)
{
    draggingBand = -1;
    for (int i = 0; i < 8; ++i)
    {
        auto* activeParam = processor.treeState.getRawParameterValue ("band" + juce::String (i) + "_active");
        if (activeParam->load() > 0.5f)
        {
            float f = processor.treeState.getRawParameterValue ("band" + juce::String (i) + "_freq")->load();
            float gn = processor.treeState.getRawParameterValue ("band" + juce::String (i) + "_gain")->load();
            auto pos = juce::Point<float> (getXForFreq (f), getYForGain (gn));
            
            if (pos.getDistanceSquaredFrom (e.position) < 100)
            {
                draggingBand = i;
                if (auto* editor = dynamic_cast<CleanSlateAudioProcessorEditor*>(getParentComponent()))
                    editor->selectedBand = i;
                return;
            }
        }
    }
}

void EqGraphComponent::mouseDrag (const juce::MouseEvent& e)
{
    if (isSketching)
    {
        sketchPoints.push_back (e.position);
        repaint();
    }
    else if (draggingBand != -1)
    {
        processor.treeState.getParameter ("band" + juce::String (draggingBand) + "_freq")->setValueNotifyingHost (processor.treeState.getParameterRange ("band" + juce::String (draggingBand) + "_freq").convertTo0to1 (getFreqForX (e.position.x)));
        processor.treeState.getParameter ("band" + juce::String (draggingBand) + "_gain")->setValueNotifyingHost (processor.treeState.getParameterRange ("band" + juce::String (draggingBand) + "_gain").convertTo0to1 (getGainForY (e.position.y)));
        processor.markFiltersForUpdate();  // FIX #7: Tell processor to update filters
        repaint();
    }
}

void EqGraphComponent::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& d)
{
    if (hoveredBand != -1)
    {
        auto* qParam = processor.treeState.getParameter ("band" + juce::String (hoveredBand) + "_q");
        float currentQ = processor.treeState.getRawParameterValue ("band" + juce::String (hoveredBand) + "_q")->load();
        float newQ = juce::jlimit (0.1f, 10.0f, currentQ + d.deltaY * 0.5f);
        qParam->setValueNotifyingHost (processor.treeState.getParameterRange ("band" + juce::String (hoveredBand) + "_q").convertTo0to1 (newQ));
        processor.markFiltersForUpdate();  // FIX #7: Tell processor to update filters
        repaint();
    }
}

void EqGraphComponent::mouseMove (const juce::MouseEvent& e)
{
    hoveredBand = -1;
    ghostPos = e.position;
    for (int i = 0; i < 8; ++i)
    {
        auto* activeParam = processor.treeState.getRawParameterValue ("band" + juce::String (i) + "_active");
        if (activeParam->load() > 0.5f)
        {
            float f = processor.treeState.getRawParameterValue ("band" + juce::String (i) + "_freq")->load();
            float gn = processor.treeState.getRawParameterValue ("band" + juce::String (i) + "_gain")->load();
            auto pos = juce::Point<float> (getXForFreq (f), getYForGain (gn));
            
            if (pos.getDistanceSquaredFrom (e.position) < 100)
            {
                hoveredBand = i;
                break;
            }
        }
    }
    repaint();
}

float EqGraphComponent::getXForFreq (float freq) { return juce::jmap (std::log10 (freq), std::log10 (20.0f), std::log10 (20000.0f), 0.0f, (float)getWidth()); }
float EqGraphComponent::getFreqForX (float x) { return std::pow (10.0f, juce::jmap (x, 0.0f, (float)getWidth(), std::log10 (20.0f), std::log10 (20000.0f))); }
float EqGraphComponent::getYForGain (float gain) { return juce::jmap (gain, -24.0f, 24.0f, (float)getHeight(), 0.0f); }
float EqGraphComponent::getGainForY (float y) { return juce::jmap (y, (float)getHeight(), 0.0f, -24.0f, 24.0f); }

void EqGraphComponent::mouseUp (const juce::MouseEvent& e)
{
    if (isSketching)
    {
        finishSketch();
        isSketching = false;
        sketchPoints.clear();
        repaint();
    }
}

void EqGraphComponent::finishSketch()
{
    if (sketchPoints.size() < 10) return;

    std::vector<std::pair<float, float>> points;
    for (auto p : sketchPoints)
        points.push_back ({ getFreqForX (p.x), getGainForY (p.y) });

    processor.createBandsFromSketch (points);
    processor.markFiltersForUpdate();  // FIX #7: Tell processor to update filters
}

CleanSlateAudioProcessorEditor::CleanSlateAudioProcessorEditor (CleanSlateAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), graph (p)
{
    setLookAndFeel (&premiumLookAndFeel);

    addAndMakeVisible (graph);
    addAndMakeVisible (hud);
    hud.setAlpha (0.95f);
    hud.setVisible (false);

     // Dynamic Controls & New Buttons
     for (auto* b : { &copyButton, &pasteButton, &eqMatchButton, &referenceButton, &phaseModeButton, &characterButton, &sketchButton, &deltaModeButton, &sidechainButton, &lookaheadButton })
         addAndMakeVisible (b);

     sketchButton.setClickingTogglesState (true);
     sketchButton.onClick = [this] { graph.isSketching = sketchButton.getToggleState(); };

     copyButton.onClick = [this] { copyEqCurve(); };
     pasteButton.onClick = [this] { pasteEqCurve(); };

     phaseModeButton.onClick = [this] { phaseModeMenu(); };
     characterButton.onClick = [this] { characterModeMenu(); };
     
     deltaModeButton.onClick = [this] { 
         float currentState = audioProcessor.treeState.getRawParameterValue ("deltaMode")->load();
         bool newState = currentState <= 0.5f;
         audioProcessor.treeState.getParameter ("deltaMode")->setValueNotifyingHost (newState ? 1.0f : 0.0f);
         deltaModeButton.setButtonText (newState ? "DELTA ON" : "DELTA");
     };

     sidechainButton.onClick = [this] { 
         float currentState = audioProcessor.treeState.getRawParameterValue ("externalSidechain")->load();
         bool newState = currentState <= 0.5f;
         audioProcessor.treeState.getParameter ("externalSidechain")->setValueNotifyingHost (newState ? 1.0f : 0.0f);
         sidechainButton.setButtonText (newState ? "SIDECHAIN ON" : "SIDECHAIN");
     };
     
     lookaheadButton.onClick = [this] { 
         bool newState = (audioProcessor.treeState.getRawParameterValue ("lookahead")->load () <= 0.5f);
         audioProcessor.treeState.getParameter ("lookahead")->setValueNotifyingHost (newState ? 1.0f : 0.0f);
         lookaheadButton.setButtonText (newState ? "LOOKAHEAD ON" : "LOOKAHEAD");
     };

     // Resizing Logic
    setResizable (true, true);
    setResizeLimits (600, 400, 2000, 1200);
    // getConstrainer()->setFixedAspectRatio (1.53); // Optional: keep aspect ratio

    // Preset Selector
    addAndMakeVisible (presetSelector);
    addAndMakeVisible (uiScaleButton);
    auto presets = Presets::getFactoryPresets();
    for (int i = 0; i < presets.size(); ++i)
        presetSelector.addItem (presets[i].name, i + 1);
    
    presetSelector.setSelectedItemIndex (0, juce::dontSendNotification);
    presetSelector.onChange = [this] { audioProcessor.loadPreset (presetSelector.getSelectedItemIndex()); };

    for (int i = 0; i < 8; ++i)
    {
        bandButtons[i].setButtonText (juce::String (i + 1));
        bandButtons[i].setClickingTogglesState (true);
        bandButtons[i].setRadioGroupId (123);
        addAndMakeVisible (bandButtons[i]);
        
        bandButtons[i].onClick = [this, i] { 
            selectedBand = i; 
            updateAttachments(); 
            resized(); 
        };
    }
    
    bandButtons[0].setToggleState (true, juce::sendNotification);
    
    for (auto* b : { &resKillButton, &smartLearnButton, &monoSubButton, &phaseFlipButton })
        addAndMakeVisible (b);
    
    resKillButton.setClickingTogglesState (true);
    smartLearnButton.setClickingTogglesState (true);
    monoSubButton.setClickingTogglesState (true);
    phaseFlipButton.setClickingTogglesState (true);

    // OpenGL Setup
    openGLContext.setRenderer (this);
    openGLContext.setContinuousRepainting (true);
    openGLContext.attachTo (*this);

    setSize (1000, 650);
    startTimerHz (60);
    
    uiScaleButton.onClick = [this] { showUiScaleMenu(); };
}

CleanSlateAudioProcessorEditor::~CleanSlateAudioProcessorEditor()
{
    openGLContext.detach();
    setLookAndFeel (nullptr);
}

void CleanSlateAudioProcessorEditor::updateAttachments()
{
    if (selectedBand == -1) return;

    juce::String id = "band" + juce::String (selectedBand);
    
    // Refresh HUD Attachments
    hud.shapeBox.setSelectedItemIndex ((int)audioProcessor.treeState.getRawParameterValue (id + "_type")->load(), juce::dontSendNotification);
    hud.slopeBox.setSelectedItemIndex ((int)audioProcessor.treeState.getRawParameterValue (id + "_slope")->load(), juce::dontSendNotification);
    hud.modeBox.setSelectedItemIndex ((int)audioProcessor.treeState.getRawParameterValue (id + "_mode")->load(), juce::dontSendNotification);
    hud.dynamicBox.setSelectedItemIndex ((int)audioProcessor.treeState.getRawParameterValue (id + "_dynamic")->load(), juce::dontSendNotification);

    // Callbacks to update processor when HUD changes
    hud.shapeBox.onChange = [this, id] { audioProcessor.treeState.getParameter (id + "_type")->setValueNotifyingHost (audioProcessor.treeState.getParameterRange (id + "_type").convertTo0to1 (hud.shapeBox.getSelectedItemIndex())); };
    hud.slopeBox.onChange = [this, id] { audioProcessor.treeState.getParameter (id + "_slope")->setValueNotifyingHost (audioProcessor.treeState.getParameterRange (id + "_slope").convertTo0to1 (hud.slopeBox.getSelectedItemIndex())); };
    hud.modeBox.onChange = [this, id] { audioProcessor.treeState.getParameter (id + "_mode")->setValueNotifyingHost (audioProcessor.treeState.getParameterRange (id + "_mode").convertTo0to1 (hud.modeBox.getSelectedItemIndex())); };
    hud.dynamicBox.onChange = [this, id] { audioProcessor.treeState.getParameter (id + "_dynamic")->setValueNotifyingHost (audioProcessor.treeState.getParameterRange (id + "_dynamic").convertTo0to1 (hud.dynamicBox.getSelectedItemIndex())); };

    // Slider Attachments for Dynamic EQ
    thresholdAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.treeState, id + "_threshold", hud.attackSlider); // Placeholder for attack/release mapping
}

void CleanSlateAudioProcessorEditor::copyEqCurve()
{
    juce::SystemClipboard::copyTextToClipboard (audioProcessor.getBandsAsXml());
}

void CleanSlateAudioProcessorEditor::pasteEqCurve()
{
    audioProcessor.setBandsFromXml (juce::SystemClipboard::getTextFromClipboard());
    repaint();
}

void CleanSlateAudioProcessorEditor::phaseModeMenu()
{
    juce::PopupMenu m;
    float phaseValue = audioProcessor.treeState.getRawParameterValue ("phaseMode")->load();
    int phaseIdx = static_cast<int>(phaseValue * 2.0f);

    m.addItem (1, "Zero Latency", true, phaseIdx == 0);
    m.addItem (2, "Natural Phase", true, phaseIdx == 1);
    m.addItem (3, "Linear Phase", true, phaseIdx == 2);

    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (phaseModeButton), [this] (int result) {
        if (result > 0) audioProcessor.treeState.getParameter ("phaseMode")->setValueNotifyingHost ((result - 1) / 2.0f);
        phaseModeButton.setButtonText (result == 1 ? "Zero Latency" : result == 2 ? "Natural Phase" : "Linear Phase");
    });
}

void CleanSlateAudioProcessorEditor::characterModeMenu()
{
    juce::PopupMenu m;
    float charValue = audioProcessor.treeState.getRawParameterValue ("characterMode")->load();
    int charIdx = static_cast<int>(charValue * 2.0f);

    m.addItem (1, "Clean", true, charIdx == 0);
    m.addItem (2, "Subtle (Transformer)", true, charIdx == 1);
    m.addItem (3, "Warm (Tube)", true, charIdx == 2);

    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (characterButton), [this] (int result) {
        if (result > 0) audioProcessor.treeState.getParameter ("characterMode")->setValueNotifyingHost ((result - 1) / 2.0f);
        characterButton.setButtonText (result == 1 ? "Clean" : result == 2 ? "Subtle" : "Warm");
    });
}

void CleanSlateAudioProcessorEditor::showUiScaleMenu()
{
    juce::PopupMenu m;
    m.addItem (1, "100%", true, currentScale == 1.0f);
    m.addItem (2, "125%", true, currentScale == 1.25f);
    m.addItem (3, "150%", true, currentScale == 1.5f);

    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (uiScaleButton), [this] (int result) {
        if (result == 1) setScaleFactor (1.0f);
        else if (result == 2) setScaleFactor (1.25f);
        else if (result == 3) setScaleFactor (1.5f);
        
        if (result > 0) {
            currentScale = (result == 1 ? 1.0f : result == 2 ? 1.25f : 1.5f);
            uiScaleButton.setButtonText (juce::String (int (currentScale * 100)) + "%");
        }
    });
}
void CleanSlateAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Minimalist Luxury Background
    g.fillAll (juce::Colour (0xFF050505));

    // Logo / Branding
    g.setColour (juce::Colours::white);
    g.setFont (juce::Font ("Arial", 28.0f, juce::Font::bold));
    g.drawText ("CLEAN SLATE", 30, 20, 300, 40, juce::Justification::left);
    
    g.setColour (juce::Colour (0xFFFFD700)); // Gold
    g.setFont (juce::Font ("Arial", 12.0f, juce::Font::bold));
    g.drawText ("PRO-Q BUILD EXP v1.1", 32, 50, 200, 15, juce::Justification::left);
}

void CleanSlateAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    
    // Header layout
    auto topBar = area.removeFromTop (80);
    presetSelector.setBounds (topBar.removeFromRight (180).reduced (10, 25));
    uiScaleButton.setBounds (topBar.removeFromRight (80).reduced (10, 25));
    
    // Pro Button Row in Top Bar
    auto proBtnArea = topBar.removeFromRight (600).reduced (0, 25);
    int pW = proBtnArea.getWidth() / 8;
    sketchButton.setBounds (proBtnArea.removeFromLeft (pW).reduced (2, 0));
    copyButton.setBounds (proBtnArea.removeFromLeft (pW).reduced (2, 0));
    pasteButton.setBounds (proBtnArea.removeFromLeft (pW).reduced (2, 0));
    phaseModeButton.setBounds (proBtnArea.removeFromLeft (pW).reduced (2, 0));
    characterButton.setBounds (proBtnArea.removeFromLeft (pW).reduced (2, 0));
    deltaModeButton.setBounds (proBtnArea.removeFromLeft (pW).reduced (2, 0));
    sidechainButton.setBounds (proBtnArea.removeFromLeft (pW).reduced (2, 0));
    lookaheadButton.setBounds (proBtnArea.removeFromLeft (pW).reduced (2, 0));
    
    auto bottomBar = area.removeFromBottom (100);
    
    auto aiArea = bottomBar.removeFromRight (250);
    resKillButton.setBounds (aiArea.removeFromTop (45).reduced (5, 2));
    smartLearnButton.setBounds (aiArea.removeFromTop (45).reduced (5, 2));
    
    auto matchArea = bottomBar.removeFromLeft (180);
    eqMatchButton.setBounds (matchArea.removeFromTop (45).reduced (5, 2));
    referenceButton.setBounds (matchArea.removeFromTop (45).reduced (5, 2));
    
    auto bentoArea = bottomBar.removeFromLeft (120);
    monoSubButton.setBounds (bentoArea.removeFromTop (45).reduced (5, 2));
    phaseFlipButton.setBounds (bentoArea.removeFromTop (45).reduced (5, 2));
    
    auto bandArea = bottomBar.reduced (5, 10);
    int btnWidth = bandArea.getWidth() / 8;
    for (int i = 0; i < 8; ++i)
        bandButtons[i].setBounds (bandArea.removeFromLeft (btnWidth).reduced (2, 5));
    
    graph.setBounds (area.reduced (10));
    
    // Position Mini-HUD under selected node
    if (selectedBand != -1)
    {
        float f = audioProcessor.treeState.getRawParameterValue ("band" + juce::String (selectedBand) + "_freq")->load();
        float gn = audioProcessor.treeState.getRawParameterValue ("band" + juce::String (selectedBand) + "_gain")->load();
        
        auto pos = juce::Point<float> (graph.getXForFreq (f), graph.getYForGain (gn));
        hud.setBounds (int (pos.x + graph.getX() - 75), int (pos.y + graph.getY() + 30), 160, 120);
        hud.setVisible (true);
    }
    else
    {
        hud.setVisible (false);
    }
    
    updateButtonStates(); // Update button states after layout changes
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

void CleanSlateAudioProcessorEditor::updateButtonStates()
{
    auto phaseMode = audioProcessor.getPhaseMode();
    switch (phaseMode)
    {
        case CleanSlateAudioProcessor::PhaseMode::ZeroLatency: phaseModeButton.setButtonText ("Zero Latency"); break;
        case CleanSlateAudioProcessor::PhaseMode::NaturalPhase: phaseModeButton.setButtonText ("Natural Phase"); break;
        case CleanSlateAudioProcessor::PhaseMode::LinearPhase: phaseModeButton.setButtonText ("Linear Phase"); break;
    }

    auto charMode = audioProcessor.getCharacterMode();
    switch (charMode)
    {
        case CleanSlateAudioProcessor::CharacterMode::Clean: characterButton.setButtonText ("Clean"); break;
        case CleanSlateAudioProcessor::CharacterMode::Subtle: characterButton.setButtonText ("Subtle"); break;
        case CleanSlateAudioProcessor::CharacterMode::Warm: characterButton.setButtonText ("Warm"); break;
    }

    // Update new feature buttons
    float deltaValue = audioProcessor.treeState.getRawParameterValue ("deltaMode")->load();
    deltaModeButton.setButtonText (deltaValue > 0.5f ? "DELTA ON" : "DELTA");

    float sidechainValue = audioProcessor.treeState.getRawParameterValue ("externalSidechain")->load();
    sidechainButton.setButtonText (sidechainValue > 0.5f ? "SIDECHAIN ON" : "SIDECHAIN");

    float lookaheadValue = audioProcessor.treeState.getRawParameterValue ("lookahead")->load();
    lookaheadButton.setButtonText (lookaheadValue > 0.5f ? "LOOKAHEAD ON" : "LOOKAHEAD");
}
