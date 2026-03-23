export const HISE_SCRIPT = `
/**
 * CLEAN SLATE EQ - AI Logic Engine
 * Features: Auto-Gain, Collision Detection, and Peak Suppression
 */

// AI Resonance Detection
inline function findHarshResonances() {
    var peaks = Engine.getFFTData(); // Get 2048 bins
    for (i = 0; i < peaks.length; i++) {
        if (peaks[i] > threshold && isNarrowPeak(i)) {
            createSurgicalNotch(mapBinToFreq(i), -6.0); // Auto-Kill
        }
    }
}

// Auto-Gain Compensation
inline function onGainChange(value) {
    var totalCuts = getNegativeGainSum();
    var totalBoosts = getPositiveGainSum();
    // Maintain perceived loudness
    OutputGain.setControlValue(0.5 - (totalBoosts * 0.05) + (totalCuts * 0.02));
}
`;

export const DSP_CPP = `
/**
 * CLEAN SLATE EQ - Natural Phase C++ Kernels
 * Brickwall & Stacked 2nd-Order Biquads
 */
class SurgicalFilter {
    void process(float* data, int samples) {
        if (phaseMode == "Natural") {
            // Analog-matched phase response
            applyNaturalPhaseFilter(data, samples);
        } else {
            // Zero Latency path
            applyStandardBiquad(data, samples);
        }
    }
}
`;

export const UI_JSON = `{
  "Edition": "Surgical AI v1.0",
  "Engine": "Clean Slate AI Core",
  "Features": [
    "Smart Learn",
    "Res-Kill (Auto-Notch)",
    "Auto-Match Profiles",
    "Collision Glow",
    "Natural Phase DSP"
  ]
}
`;

export const SPECTRUM_GLSL = `
// Clean Slate Spectrum Analyzer with Collision Glow
uniform float uTime;
uniform sampler2D uFFT;
uniform sampler2D uExternalFFT;
uniform bool uCollisionMode;

varying vec2 vUv;

void main() {
    vec2 uv = vUv;
    float fft = texture2D(uFFT, vec2(uv.x, 0.5)).r;
    float extFft = texture2D(uExternalFFT, vec2(uv.x, 0.5)).r;
    
    vec3 color = vec3(0.01);
    
    // Main Spectrum
    if (uv.y < fft) {
        color = mix(vec3(0.05), vec3(0.2, 0.8, 1.0), uv.y / fft);
    }
    
    // Collision Glow (Red)
    if (uCollisionMode && uv.y < min(fft, extFft) + 0.1) {
        float collision = min(fft, extFft) * 2.0;
        color = mix(color, vec3(1.0, 0.1, 0.1), collision * 0.6);
    }
    
    gl_FragColor = vec4(color, 1.0);
}
`;
