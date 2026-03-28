# Product Requirements Document (PRD)
## CLEAN Slate EQ Premium - VST Audio Plugin

**Version:** 1.0  
**Status:** In Development  
**Last Updated:** [Current Date]  
**Audience:** Development Team, QA, Marketing

---

## 1. Executive Summary

**CLEAN Slate EQ Premium** is a professional-grade parametric equalizer VST plugin designed for mixing, mastering, and production. It combines 8 surgical frequency bands with advanced processing modes, dynamic compression, M/S processing, and AI-assisted tools for resonance detection and spectral matching.

**Target User:** Audio engineers, producers, mastering professionals, music producers

**Platform:** Windows VST3 (VST2 optional)

**Licensing:** Commercial/Professional

---

## 2. Core Features

### 2.1 EQ Engine
- **8 Parametric Bands** with independent control
  - Frequency: 20 Hz - 20 kHz
  - Gain: Â±24 dB
  - Q (Sharpness): 0.1 - 10.0
  - Slope: 0 - 4 (6dB to 96dB/octave)
  
- **7 Filter Types:**
  - High Pass
  - Peak/Bell
  - Low Pass
  - Notch
  - Low Shelf
  - High Shelf
  - All Pass

### 2.2 Processing Modes
- **Zero-Latency:** Immediate processing (minimum phase)
- **Natural Phase:** Transparent minimum-phase approximation
- **Linear Phase:** Zero-phase distortion with controlled latency

### 2.3 Dynamic EQ
- Per-band compression/expansion
- Attack and Release controls (1-1000 ms)
- Threshold (-60 to 0 dB)
- Ratio (0.1:1 to 10:1)
- Lookahead for anticipatory compression

### 2.4 Stereo/M/S Processing
- **Per-Band Mode Selection:**
  - Stereo (both channels)
  - Left only
  - Right only
  - Mid component
  - Side component
- Mid/Side solo for surgical stereo work
- Independent M/S EQ curves

### 2.5 Character Modes
- **Clean:** No saturation (pure digital)
- **Subtle:** Light tape-like warmth (tanh Ã— 1.2)
- **Warm:** Rich harmonic saturation (asymmetric tanh)
- **Analog Models:**
  - Clean (standard)
  - Neve-style (asymmetric, subtle)
  - API-style (aggressive + harmonics)

### 2.6 Advanced Features
- **Delta Mode:** Hear only the changes (wet - dry signal)
- **External Sidechain:** Channels 2&3 for dynamic control
- **Lookahead Buffering:** Configurable delay for dynamic EQ anticipation
- **A/B Comparison:** Quick switching between two EQ snapshots
- **Spectrum Analysis:** Input/Output/Both/Difference views with:
  - Peak hold with decay
  - Tilt control (Â±6 dB/octave)
  - FFT-based magnitude analysis

### 2.7 Automation & Learning
- **Auto Gain:** Automatic makeup gain based on EQ changes
- **Resonance Detection:** AI-assisted harsh frequency identification
  - Threshold-based peak detection
  - Automatic notch placement
  - Configurable sensitivity
- **EQ Matching:** Capture reference spectrum and match to it
- **Sketch-Based EQ:** Draw EQ curve, plugin places bands automatically

### 2.8 User Interface Features
- Real-time spectrum display
- Per-band visual feedback
- Parameter preset/save system
- XML-based EQ curve import/export
- Copy/paste individual band settings

---

## 3. Technical Specifications

### 3.1 Plugin Specifications
| Feature | Value |
|---------|-------|
| Format | VST3 (Windows) |
| Architecture | x64 |
| I/O | 2 in (Main) + 2 in (Sidechain) / 2 out |
| Latency (Zero) | 0 samples |
| Latency (Natural) | 0 samples |
| Latency (Linear) | FFTSize / 2 samples |
| Processing | 32-bit float |
| Sample Rates | 44.1k, 48k, 96k, 192k Hz |
| Block Sizes | 64, 128, 256, 512, 1024, 2048 samples |

### 3.2 Buffer/Processing
- **FFT Size:** 4096 samples (12-bit order)
- **Scope Display:** 2048 bins
- **Linear Phase FFT:** Dynamic size (power of 2)
- **Lookahead Buffer:** 4800 samples (@48kHz)
- **CPU Target:** < 20% single instance (full load, modern CPU)

### 3.3 Parameter Counts
- **Band Parameters:** 13 per band Ã— 8 = 104 parameters
- **Global Parameters:** 15 parameters
- **Total:** ~120 parameters (automatable)

---

## 4. Functional Requirements

### 4.1 EQ Processing
- [ ] All 8 bands process independently
- [ ] Frequency changes apply in real-time
- [ ] Gain changes apply in real-time
- [ ] Q changes apply in real-time
- [ ] Filter type switching is seamless
- [ ] No aliasing artifacts
- [ ] Accurate frequency response matching published specifications

### 4.2 Phase Modes
- [ ] Zero-Latency processes with minimal latency
- [ ] Natural Phase sounds transparent
- [ ] Linear Phase has zero phase distortion
- [ ] Mode switching doesn't cause clicks/pops
- [ ] Latency reporting is accurate to DAW

### 4.3 Dynamic EQ
- [ ] Compression/expansion works per-band
- [ ] Attack time is accurate (Â±10%)
- [ ] Release time is accurate (Â±10%)
- [ ] Threshold detection works correctly
- [ ] Ratio changes are smooth
- [ ] Lookahead provides anticipatory gain reduction

### 4.4 Stereo/M/S
- [ ] Stereo mode processes both channels
- [ ] Left-only mode mutes right channel from processing
- [ ] Right-only mode mutes left channel from processing
- [ ] Mid mode processes only M component
- [ ] Side mode processes only S component
- [ ] M/S encoding/decoding is transparent
- [ ] M/S solo works correctly

### 4.5 Character Modes
- [ ] Clean mode has no audible saturation
- [ ] Subtle mode adds light warmth
- [ ] Warm mode adds rich saturation
- [ ] Neve model sounds asymmetric
- [ ] API model has audible harmonics
- [ ] Mode switching is smooth

### 4.6 Spectrum Analysis
- [ ] Real-time FFT display of input
- [ ] Accurate frequency bin resolution
- [ ] Peak hold displays correctly
- [ ] Peak decay rate is smooth (0.985 per frame)
- [ ] Tilt application is correct (6dB/octave)
- [ ] Multiple view modes display correctly

### 4.7 AI Features
- [ ] Resonance detection identifies peaks above threshold
- [ ] Detected frequencies are accurate (Â±5% tolerance)
- [ ] Automatic notch placement is surgical (Q=8)
- [ ] EQ matching captures spectrum accurately
- [ ] EQ matching applies correct gain differences
- [ ] Sketch-to-EQ finds peaks/dips correctly

### 4.8 Advanced Features
- [ ] Delta mode subtracts dry from wet correctly
- [ ] External sidechain input works on channels 2&3
- [ ] Lookahead delay is accurate
- [ ] A/B swapping is reliable
- [ ] A/B snapshots are independent
- [ ] Auto Gain makeup is accurate

---

## 5. Non-Functional Requirements

### 5.1 Performance
- **CPU Usage:** < 20% for full EQ (8 bands active)
- **Memory:** < 50 MB total footprint
- **Memory Leaks:** Zero detected after 1 hour use
- **Buffer Allocation:** No heap allocations in audio thread
- **Real-Time Safety:** RTOS-compliant (no blocking operations)

### 5.2 Reliability
- **Stability:** No crashes in 8-hour continuous operation
- **State Persistence:** Plugin state saved/restored correctly
- **Parameter Ranges:** All values within specified limits
- **Error Handling:** Graceful fallback if parameters invalid
- **Thread Safety:** Safe concurrent parameter access

### 5.3 Compatibility
- **DAWs:** Reaper, FL Studio, Ableton Live, Logic Pro, Studio One
- **OS:** Windows 10, 11
- **VST Version:** VST3.x compliant
- **Unicode:** Full UTF-8 support for parameters

### 5.4 Audio Quality
- **Frequency Response:** Â±0.5 dB error (20Hz-20kHz)
- **THD:** < 0.01% (clean mode, no saturation)
- **Noise Floor:** < -120 dBFS
- **Phase Distortion:** < 5Â° (zero-latency mode)
- **Linear Phase:** Zero phase distortion (linear phase mode)

---

## 6. User Stories

### User Story 1: Basic Mixing
**As a** producer mixing a vocal  
**I want to** remove harsh frequencies and add warmth  
**So that** the vocal sits better in the mix

**Acceptance Criteria:**
- [ ] Can identify harsh frequency with resonance detection
- [ ] Can place notch with surgical precision
- [ ] Can add warmth with character mode
- [ ] Hear changes in real-time

### User Story 2: Mastering
**As a** mastering engineer  
**I want to** ensure linear phase processing with zero phase distortion  
**So that** the master maintains phase coherence

**Acceptance Criteria:**
- [ ] Linear phase mode available
- [ ] Zero phase distortion in linear phase
- [ ] Accurate latency reporting
- [ ] No phase interactions between bands

### User Story 3: Surgical EQ
**As a** mixing engineer  
**I want to** process only the mid or side component of a track  
**So that** I can control stereo imaging precisely

**Acceptance Criteria:**
- [ ] Mid/Side mode works correctly
- [ ] Mid solo isolates middle component
- [ ] Side solo isolates stereo component
- [ ] Independent M/S processing

### User Story 4: Reference Matching
**As a** producer  
**I want to** match my mix to a reference track's spectrum  
**So that** I have consistent spectral balance

**Acceptance Criteria:**
- [ ] Can capture reference spectrum
- [ ] Can match current mix to reference
- [ ] Matching suggests appropriate EQ changes
- [ ] Visualize difference between current and target

### User Story 5: Dynamic Control
**As a** engineer using dynamic EQ  
**I want to** compress specific frequencies when they get too loud  
**So that** I can tame problematic frequencies dynamically

**Acceptance Criteria:**
- [ ] Per-band compression available
- [ ] Lookahead anticipates peaks
- [ ] Attack/release times are accurate
- [ ] Smooth, musical compression

---

## 7. Success Metrics

### Phase 1: Development
- [x] All 12 critical bugs identified
- [ ] All 6 remaining bugs fixed
- [ ] 100% parameter validation
- [ ] Zero memory leaks
- [ ] Build compiles without warnings

### Phase 2: Testing
- [ ] 95%+ test pass rate
- [ ] CPU usage < 20% (target)
- [ ] Zero crashes in 8-hour session
- [ ] All audio quality metrics met
- [ ] Compatible with 5+ DAWs

### Phase 3: Release
- [ ] User manual complete
- [ ] Video tutorials created
- [ ] Preset library (50+ presets)
- [ ] Beta feedback < 5 issues per tester
- [ ] A/B comparison with competitors

### Phase 4: Post-Release
- [ ] User satisfaction > 4.5/5 stars
- [ ] Support response time < 24 hours
- [ ] Feature requests tracked and prioritized
- [ ] Quarterly updates planned
- [ ] Community presets shared

---

## 8. Known Limitations

### Current Limitations
- No MIDI learn functionality (future)
- No audio input for sidechain automation (future)
- Limited to 8 bands (architectural)
- Linear phase mode has latency (by design)
- No multi-band compression (future)

### Platform Limitations
- Windows only (Mac version planned)
- VST3 only (VST2 compatibility planned)
- 64-bit only (32-bit not supported)

---

## 9. Roadmap

### Version 1.0 (Current)
- âœ… Core EQ engine
- âœ… 8 parametric bands
- âœ… Dynamic EQ
- âœ… M/S processing
- âœ… Character modes
- âœ… Spectrum analysis
- âœ… AI resonance detection
- âœ… A/B comparison
- âš ï¸ Full testing/debugging

### Version 1.1 (Q2 2025)
- [ ] VST2 compatibility
- [ ] Additional analog models
- [ ] MIDI learn
- [ ] Preset library expansion
- [ ] Performance optimizations

### Version 2.0 (Q4 2025)
- [ ] Multi-band compression
- [ ] Convolution reverb integration
- [ ] Machine learning EQ matching
- [ ] Sidechain from audio input
- [ ] Mac support

### Version 3.0 (2026)
- [ ] Parallel processing
- [ ] Mid/side dynamics
- [ ] Advanced metering
- [ ] Plugin ecosystem integration

---

## 10. Risk Assessment

| Risk | Impact | Likelihood | Mitigation |
|------|--------|------------|-----------|
| Audio quality issues | High | Medium | Rigorous testing, A/B with competitors |
| Performance problems | High | Low | Comprehensive profiling, optimization |
| DAW incompatibility | Medium | Low | Test with 5+ DAWs early |
| User learning curve | Medium | Medium | Detailed manual, video tutorials |
| Market competition | Medium | High | Focus on AI features, ease of use |

---

## 11. Success Criteria for Testing

âœ… **MUST HAVE (Blocking Release):**
- [ ] All 6 remaining critical bugs fixed
- [ ] Parameter updates work in real-time
- [ ] Zero crashes in extended session
- [ ] CPU < 20% at full processing
- [ ] All frequency responses accurate
- [ ] M/S processing transparent
- [ ] Dynamic EQ stable

âœ… **SHOULD HAVE (Blocking Beta):**
- [ ] Spectrum analysis accurate
- [ ] Resonance detection functional
- [ ] Character modes sound good
- [ ] All phase modes work correctly
- [ ] A/B comparison reliable

âš ï¸ **NICE TO HAVE (Post-Release):**
- [ ] Performance optimizations
- [ ] Additional presets
- [ ] UI polish
- [ ] Documentation perfection

---

## 12. Acceptance Criteria

The plugin is **READY FOR RELEASE** when:

1. âœ… All critical bugs fixed and verified
2. âœ… All tests pass (95%+ success rate)
3. âœ… Audio quality benchmarks met
4. âœ… Performance targets achieved
5. âœ… Documentation complete
6. âœ… Tested on 5+ DAWs
7. âœ… Zero known issues
8. âœ… Beta feedback < 3 critical items
9. âœ… User guide written
10. âœ… Preset library created (50+ presets)

---

## 13. Support & Maintenance

### Support Channels
- Email: support@cleanslateaq.com (planned)
- Forum: community.cleanslateq.com (planned)
- Documentation: online wiki (planned)
- Video tutorials: YouTube channel (planned)

### Maintenance Schedule
- Monthly bug fix releases
- Quarterly feature updates
- Annual major version updates
- Continuous DAW compatibility testing

---

## Document Control

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0 | [Date] | Dev Team | Initial PRD from code analysis |
| 1.1 | TBD | TBD | Post-testing revisions |

---

**Next Steps:**
1. Review PRD with stakeholders
2. Prioritize remaining bug fixes
3. Create detailed test plan (based on section 6 & 7)
4. Begin testing phase
5. Track success metrics throughout development


