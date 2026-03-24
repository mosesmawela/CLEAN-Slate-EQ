# 🔴 COMPREHENSIVE BUG AUDIT - CLEAN SLATE EQ FINAL REPORT

## CRITICAL BUGS FOUND (Will Crash or Prevent Audio)

### Bug #1: OFF-BY-ONE PHASE MODE CALCULATION 🔴 CRITICAL
**Location:** Line 1096 in PluginProcessor.cpp  
**Code:**
```cpp
int phaseModeValue = static_cast<int>(phaseModeParam->load() * 2.0f);
```
**Problem:**
- Parameter created as `AudioParameterInt("phaseMode", 1), "Phase Mode", 0, 2, 0)` (line 140)
- This means parameter value is ALREADY `0, 1, or 2`
- Code multiplies by 2.0 → produces `0, 2, or 4`
- Enum only has 3 values (0-2), so value `4` is undefined behavior
- **Result: Random enum state, phase mode broken**

**Fix:** Remove multiplication, use direct cast

---

### Bug #2: OFF-BY-ONE CHARACTER MODE CALCULATION 🔴 CRITICAL  
**Location:** Line 1107 in PluginProcessor.cpp  
**Code:**
```cpp
int charModeValue = static_cast<int>(charModeParam->load() * 2.0f);
```
**Problem:** Same as Bug #1 - parameter already 0-2, multiplying breaks it  
**Fix:** Remove multiplication

---

### Bug #3: INCONSISTENT MODE LOADING IN updateFilters() 🔴 CRITICAL
**Location:** Line 396 in PluginProcessor.cpp  
**Code:**
```cpp
phaseMode = (PhaseMode) treeState.getRawParameterValue ("phaseMode")->load ();
```
**Problem:**
- Direct cast of float 0-1 range to enum
- ALSO: updateFilters() called EVERY sample at line 1043
- This conflicts with the processBlock mode loading
- **Result: Mode switching doesn't work, only first mode accessible**

**Fix:** Use consistent conversion, only update when parameter changes

---

### Bug #4: READ-AFTER-CLEAR IN LINEAR PHASE 🔴 CRITICAL
**Location:** Lines 567-572 in PluginProcessor.cpp  
**Code:**
```cpp
void CleanSlateAudioProcessor::processLinearPhaseChannel(...)
{
    std::fill (channelData, channelData + numSamples, 0.0f);  // Line 567: CLEAR
    
    for (int i = 0; i < numSamples; ++i)
    {
        float sample = channelData[i];  // Line 572: READ CLEARED DATA = 0.0f
```
**Problem:**
- Clears the input buffer FIRST
- Then reads from the cleared buffer (always 0.0f)
- Convolution with zeros = silence
- **Result: Linear phase mode produces NO AUDIO**

**Fix:** Save input BEFORE clearing, or don't clear at all

---

### Bug #5: NO NULL CHECK ON MONO CHANNELS 🔴 CRITICAL
**Location:** Lines 439-440 in PluginProcessor.cpp  
**Code:**
```cpp
auto* leftData = buffer.getWritePointer (0);
auto* rightData = buffer.getWritePointer (1);  // CRASH if mono!
```
**Problem:**
- If input is mono, getWritePointer(1) returns nullptr
- Line 546: `rightData[sample] = rightSample;` → **NULL POINTER CRASH**

**Fix:** Add channel count check or duplicate mono channel

---

### Bug #6: MASSIVE PER-SAMPLE BUFFER ALLOCATION 🔴 CRITICAL
**Location:** Lines 469-472 in PluginProcessor.cpp  
**Code:**
```cpp
for (int sample = 0; sample < numSamples; ++sample)
{
    juce::AudioBuffer<float> bufL(1, 1);  // NEW ALLOCATION EVERY SAMPLE!
    juce::AudioBuffer<float> bufR(1, 1);
    juce::AudioBuffer<float> stereoBuffer(2, 1);
```
**Problem:**
- For 512-sample block: **4,096+ allocations per block**
- At 44.1kHz: **180,000+ allocations per second**
- Causes severe CPU spikes and audio dropouts
- **Should allocate ONCE in prepareToPlay**

**Fix:** Pre-allocate buffers, reuse them

---

## HIGH PRIORITY BUGS (Audio Doesn't Process Correctly)

### Bug #7: updateFilters() CALLED EVERY SAMPLE 🟠 HIGH
**Location:** Line 1043  
**Problem:**
- Called 705,600 times/second at 44.1kHz stereo
- Reloads ALL 32 filters from tree state
- Should update on parameter change only
- **Result: CPU load 50x higher than necessary**

**Fix:** Use `AudioProcessorValueTreeState::Listener`

---

### Bug #8: FILTER STATE LOST BETWEEN BANDS 🟠 HIGH
**Location:** Lines 510-518  
**Problem:**
- Each band creates new `stereoBuffer`
- Processes band N independently
- No cascade/accumulation of filter states
- **Result: EQ curve doesn't match design**

**Fix:** Process bands sequentially through same buffer

---

### Bug #9: NATURAL PHASE IS JUST A STUB 🟠 HIGH
**Location:** Line 556  
**Code:**
```cpp
void CleanSlateAudioProcessor::processWithNaturalPhase(...)
{
    processWithZeroLatency(buffer);  // NOT ACTUAL NATURAL PHASE!
}
```
**Fix:** Implement proper minimum-phase response

---

### Bug #10: DELTA MODE DRY BUFFER RESIZE 🟠 HIGH
**Location:** Lines 1060-1075  
**Problem:**
- Resizes buffer if size doesn't match
- Resizing clears the buffer
- Loses historical data for comparison
- **Result: Delta mode can't compare properly**

**Fix:** Pre-allocate in prepareToPlay with guaranteed size

---

### Bug #11: UNUSED SPECTRUM VIEW & ANALOG MODEL 🟠 HIGH
**Location:** Lines 1083, 1086  
**Problem:**
- Parameters loaded but never used
- GUI has buttons but no actual implementation
- **Result: Features don't work**

**Fix:** Either remove parameters or implement functionality

---

### Bug #12: A/B BUFFER LOGIC INEFFICIENT 🟠 HIGH
**Location:** Lines 985-1041  
**Problem:**
- Creates temp std::vector every block
- Copies data 3 times per block
- Confusing swap logic
- **Result: CPU waste + confusing behavior**

**Fix:** Use circular buffer or simpler pattern

---

## MEDIUM PRIORITY BUGS

### Bug #13: BUFFER CHANNEL COUNT NOT VALIDATED 🟡 MEDIUM
**Location:** Line 1088-1137  
**Problem:**
- Assumes 2+ channels exist
- If mono, processing skipped entirely
- **Result: Silent processing for mono**

**Fix:** Handle mono case explicitly

---

### Bug #14: STEREO PROCESSING BUG 🟡 MEDIUM
**Location:** Lines 509-518  
**Problem:**
- Uses `filtersL` for stereo mode
- Right channel doesn't get properly filtered
- **Result: Stereo processing broken**

**Fix:** Use appropriate filter for each channel

---

### Bug #15: CHARACTER MODE CAST UNSAFE 🟡 MEDIUM
**Location:** Line 398  
**Problem:**
- Direct cast of 0-1 float to enum
- If parameter is 0.5, behavior undefined
- **Result: Unpredictable character mode**

**Fix:** Use bounds checking

---

### Bug #16: PHASEMODE CAST UNSAFE 🟡 MEDIUM
**Location:** Line 396  
**Problem:** Same as Bug #15

**Fix:** Use bounds checking

---

## ARCHITECTURE ISSUES

### Bug #17: NO PARAMETER LISTENER 
- Every filter parameter update requires manual updateFilters() call
- Called every sample = massive waste
- Should use ValueTree::Listener pattern

### Bug #18: FILTER ALLOCATION NOT OPTIMAL
- 32 filters (8 bands × 4 types) allocated
- All prepared in prepareToPlay
- Better: Only allocate active filters

### Bug #19: NO THREAD SAFETY ON bandStates
- Multiple threads access bandStates
- No locks protecting array
- Could have race conditions

### Bug #20: CONVOLUTION BUFFER LOGIC UNCLEAR
- Complex circular buffer indexing
- Lines 747: `& (linearPhaseFFTSize - 1)` assumes power of 2
- Not validated

---

## FXSOUND COMPARISON

FXSound does:
✅ Modular DSP (separate module)  
✅ Parameter listeners (not every sample updates)  
✅ Efficient buffer management  
✅ Proper phase mode implementation  
✅ Clear audio processing pipeline  

We do:
❌ Monolithic processing  
❌ Per-sample updates  
❌ Wasteful allocations  
❌ Stub implementations  
❌ Unclear data flow  

---

## RECOMMENDED FIX PRIORITY

**PHASE 1 - CRITICAL (Must fix first):**
1. Fix mode value calculations (Bugs #1, #2)
2. Fix linear phase read-after-clear (Bug #4)
3. Add null checks for mono (Bug #5)
4. Move buffer allocations (Bug #6)
5. Fix inconsistent mode loading (Bug #3)

**PHASE 2 - HIGH (Before shipping):**
6. Implement parameter listener for updateFilters (Bug #7)
7. Fix filter state accumulation (Bug #8)
8. Implement Natural Phase properly (Bug #9)
9. Fix delta mode buffer handling (Bug #10)
10. Add channel count validation (Bug #13)

**PHASE 3 - MEDIUM (Nice to have):**
11-20: Polish and optimization

---

## ESTIMATED IMPACT

- **Bugs #1-6:** Plugin is non-functional without fixes
- **Bugs #7-12:** Audio works but wrong/inefficient
- **Bugs #13-20:** Edge cases and optimization

Total: **20+ bugs found**
