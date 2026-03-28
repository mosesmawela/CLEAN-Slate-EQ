# ðŸš€ TIER 1 IMPLEMENTATION PLAN - CRITICAL FEATURES

## Overview
Implementing 5 critical features that make the plugin compete with Pro-Q 4.
**Total Time**: 16-18 hours | **Difficulty**: Medium

---

## FEATURE 1: Phase Invert Per Band âœ…
**Time**: 30 minutes | **Difficulty**: Trivial

### Changes Needed:
1. **PluginProcessor.h**: Add `bool phaseInvert = false;` to `BandState` struct
2. **PluginProcessor.cpp**: Add phase inversion in `processWithZeroLatency()` loop
3. **PluginEditor.cpp**: Add button in MiniHUD for phase invert toggle

### Code Locations:
- `PluginProcessor.h` line ~45: BandState struct
- `PluginProcessor.cpp` line ~600-700: processWithZeroLatency loop
- `PluginEditor.cpp` line ~500-600: MiniHudComponent setup

---

## FEATURE 2: More Filter Shapes
**Time**: 1-2 hours | **Difficulty**: Medium

### Add These Shapes:
- AllPass (already declared, need implementation)
- TiltShelf (3dB/oct tilt)
- FlatTilt (flat tilting over range)
- BandPass (narrow bell)

### Changes Needed:
1. **PluginProcessor.h**: Add to FilterType enum (lines 26-34)
2. **SurgicalFilter.h**: Add coefficient calculations for new shapes
3. **PluginEditor.cpp**: Update shape dropdown labels

### Code Locations:
- `PluginProcessor.h` line ~26: FilterType enum
- `SurgicalFilter.h` line ~100-200: setCoefficients() method
- `PluginEditor.cpp` line ~800-900: Shape combobox setup

---

## FEATURE 3: Undo/Redo System
**Time**: 3-4 hours | **Difficulty**: Medium (highest payoff)

### Changes Needed:
1. **PluginProcessor.h**: 
   - Add `juce::UndoManager undoManager;`
   - Create `ParameterChangeAction` class
2. **PluginProcessor.cpp**: 
   - Create undoable actions when parameters change
   - Manage transaction boundaries
3. **PluginEditor.cpp**:
   - Add Undo/Redo buttons or keyboard shortcuts (Ctrl+Z / Ctrl+Shift+Z)

### Key Implementation Points:
```cpp
// When parameter changes:
undoManager.beginNewTransaction();
undoManager.perform(new ParameterChangeAction(...));

// Keyboard handling:
if (e.getModifiers().isCommandDown() && e.getKeyCode() == 'z')
    undoManager.undo();
```

### Code Locations:
- `PluginProcessor.h` line ~200: Add UndoManager member
- `PluginProcessor.cpp` line ~800-900: Parameter change handling
- `PluginEditor.cpp` line ~100-200: Keyboard shortcuts

---

## FEATURE 4: Spectrum Grab
**Time**: 3-4 hours | **Difficulty**: Medium-Hard

### How It Works:
User clicks/drags on spectrum peak â†’ frequency snaps to that peak â†’ band gain set to +6dB

### Changes Needed:
1. **EqGraphComponent**: Enhance `mouseDown()` and `mouseDrag()` methods
2. **PluginProcessor.cpp**: Add `findNearestSpectrumPeak()` function
3. **PluginEditor.cpp**: Visual feedback (cursor change, highlight)

### Key Implementation Points:
```cpp
// In EqGraphComponent::mouseDown()
if (isSpectrumArea(mousePos))
{
    int peakBin = findNearestSpectrumPeak(mouseX);
    float freq = getFreqFromBin(peakBin);
    assignBandToFrequency(freq);
    processor.markFiltersForUpdate();
}
```

### Code Locations:
- `PluginEditor.h` line ~30-50: EqGraphComponent class
- `PluginEditor.cpp` line ~300-400: EqGraphComponent methods
- `PluginProcessor.cpp` line ~150: Add peak detection function

---

## FEATURE 5: MIDI Learn (Proper)
**Time**: 3-4 hours | **Difficulty**: Medium

### Implementation:
- Right-click any parameter â†’ "Learn MIDI CC"
- Assign CC#0-119 to frequency, gain, Q, slope
- Normalize CC value (0-127 â†’ 0.0-1.0) and map to parameter range

### Changes Needed:
1. **PluginProcessor.h**: 
   - Add `MidiLearnManager` class
   - Add `std::map<int, std::string> ccToParameterMap;`
2. **PluginProcessor.cpp**:
   - Implement MIDI handling in `processBlock()`
3. **PluginEditor.cpp**:
   - Right-click context menus on sliders/controls
   - Visual indicator when MIDI learning is active

### Key Implementation Points:
```cpp
class MidiLearnManager
{
    std::map<int, std::string> ccToParameterMap;
    
    void onMidiCC(int cc, int value)
    {
        if (auto param = ccToParameterMap[cc])
        {
            float normalized = value / 127.0f;
            processor.setParameterByID(param, normalized);
        }
    }
};
```

### Code Locations:
- `PluginProcessor.h` line ~250: Add MidiLearnManager
- `PluginProcessor.cpp` line ~500: processBlock MIDI handling
- `PluginEditor.cpp` line ~400-500: Right-click menus

---

## Implementation Order

### Day 1 (3-4 hours):
1. âœ… Phase Invert (30 min - quick win)
2. âœ… More Filter Shapes (1-2 hours - medium effort)
3. âœ… Test & validate build

### Day 2 (3-4 hours):
1. âœ… Undo/Redo System (3-4 hours - highest priority)
2. âœ… Test & validate in FL Studio

### Day 3 (4-5 hours):
1. âœ… Spectrum Grab (3-4 hours)
2. âœ… Test & polish

### Day 4 (4-5 hours):
1. âœ… MIDI Learn (3-4 hours)
2. âœ… Full integration testing
3. âœ… Bug fixes & optimization

---

## Testing Checklist

### Phase Invert:
- [ ] Invert works on all 8 bands
- [ ] Sound inverts correctly
- [ ] UI reflects state
- [ ] Survives DAW reload

### Filter Shapes:
- [ ] All 8+ shapes available
- [ ] Each shape sounds correct
- [ ] No distortion or artifacts
- [ ] Dropdown labels clear

### Undo/Redo:
- [ ] Undo works for parameter changes
- [ ] Redo works correctly
- [ ] Transaction boundaries correct
- [ ] Keyboard shortcuts work (Ctrl+Z)
- [ ] Multiple undos in sequence work
- [ ] Switching bands + undo works

### Spectrum Grab:
- [ ] Click on peak â†’ snaps frequency
- [ ] Drag on peak â†’ updates smoothly
- [ ] Visual feedback visible
- [ ] Works with all zoom levels
- [ ] Doesn't interfere with other interactions

### MIDI Learn:
- [ ] Right-click shows "Learn MIDI" option
- [ ] Learning enters active state
- [ ] CC assignment successful
- [ ] Adjusting CC updates parameter
- [ ] Visual indicator shows active CC
- [ ] Multiple parameters can have CCs

---

## File Priority Map

### Must Modify:
1. **PluginProcessor.h** - Core class members
2. **PluginProcessor.cpp** - DSP logic
3. **PluginEditor.h** - UI declarations
4. **PluginEditor.cpp** - UI implementation

### Reference:
- **SurgicalFilter.h** - For filter shape implementation
- **FABFILTER_FEATURE_ANALYSIS.md** - Implementation details

---

## Risk Mitigation

### Potential Issues:
1. **Undo/Redo complexity**: Start simple (parameter-only), expand later
2. **Spectrum Grab false positives**: Require click near peak + tolerance
3. **MIDI Learn conflicts**: Use CCToParameterMap with careful locking
4. **Filter shape bugs**: Test each shape in isolation first

### Testing Strategy:
- Build after each feature
- Test in FL Studio after UI changes
- Use dry signal A/B to verify audio changes
- Check CPU usage doesn't increase

---

## Success Criteria

After Tier 1:
- âœ… Plugin has 12+ filter shapes
- âœ… Phase invert per band works
- âœ… Undo/Redo fully functional
- âœ… Spectrum Grab interactive and smooth
- âœ… MIDI Learn operational
- âœ… Zero crashes in 1-hour session
- âœ… <12% CPU usage
- âœ… Ready for Tier 2 (Workflow features)

---

## Next Steps

1. Start with **Phase Invert** (easiest, builds momentum)
2. Add **Filter Shapes** (medium effort, big feature)
3. Implement **Undo/Redo** (highest priority, most complex)
4. Add **Spectrum Grab** (game-changer UX)
5. Implement **MIDI Learn** (professional workflow)

**Ready to start? I'll begin with Phase Invert now.** âœ¨


