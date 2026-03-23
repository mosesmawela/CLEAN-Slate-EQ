/**
 * EQ Magnitude Response Calculation
 * Based on standard peaking and shelving filter formulas
 */

export interface EQBand {
  id: string;
  f: number;      // Frequency (Hz)
  g: number;      // Gain (dB)
  q: number;      // Q Factor
  type: 'bell' | 'lowshelf' | 'highshelf' | 'lowpass' | 'highpass' | 'notch' | 'brickwall';
  channelMode: 'stereo' | 'mid' | 'side'; // Stereo placement
  dynamicThreshold: number; // 0 to 1
  dynamicAmount: number;    // -1 to 1
  slope: number;            // 6, 12, 24, 48, 96
  isActive: boolean;
  isSolo: boolean;
  isSpectral: boolean;      // Spectral Dynamics mode
}

export const MIN_FREQ = 10;
export const MAX_FREQ = 30000;

export const freqToX = (freq: number, width: number) => {
  return (Math.log10(freq) - Math.log10(MIN_FREQ)) / (Math.log10(MAX_FREQ) - Math.log10(MIN_FREQ)) * width;
};

export const xToFreq = (x: number, width: number) => {
  return Math.pow(10, (x / width) * (Math.log10(MAX_FREQ) - Math.log10(MIN_FREQ)) + Math.log10(MIN_FREQ));
};

export const gainToY = (gain: number, height: number, range: number = 30) => {
  const maxGain = range / 2;
  const minGain = -range / 2;
  return height / 2 - (gain / (maxGain - minGain)) * height;
};

export const yToGain = (y: number, height: number, range: number = 30) => {
  const maxGain = range / 2;
  const minGain = -range / 2;
  return ((height / 2 - y) / height) * (maxGain - minGain);
};

export const snapToNote = (freq: number) => {
  const n = 12 * Math.log2(freq / 440) + 69;
  const snappedN = Math.round(n);
  return 440 * Math.pow(2, (snappedN - 69) / 12);
};

export const getBandColor = (mode: 'stereo' | 'mid' | 'side') => {
  if (mode === 'mid') return "#4ADE80"; // Green for Mid
  if (mode === 'side') return "#60A5FA"; // Blue for Side
  return "#D4AF37"; // Yellow/Gold for Stereo
};

export const calculateBandMagnitude = (freq: number, band: EQBand, proportionalQ: boolean = false) => {
  let { f, g, q, type } = band;
  
  if (proportionalQ && type === 'bell') {
    const gainFactor = Math.abs(g) / 6.0;
    q = q * (1.0 + gainFactor);
  }

  const distance = Math.abs(Math.log10(freq) - Math.log10(f));
  
  if (type === 'bell') {
    const influence = Math.exp(-Math.pow(distance * q * 5, 2));
    return g * influence;
  } else if (type === 'lowshelf') {
    if (freq < f) return g;
    const influence = Math.exp(-Math.pow(distance * 3, 2));
    return g * influence;
  } else if (type === 'highshelf') {
    if (freq > f) return g;
    const influence = Math.exp(-Math.pow(distance * 3, 2));
    return g * influence;
  } else if (type === 'lowpass' || (type === 'brickwall' && freq > f)) {
    const slopeFactor = type === 'brickwall' ? 120 : 20;
    return -Math.pow(distance * slopeFactor, 2);
  } else if (type === 'highpass' || (type === 'brickwall' && freq < f)) {
    const slopeFactor = type === 'brickwall' ? 120 : 20;
    return -Math.pow(distance * slopeFactor, 2);
  } else if (type === 'notch') {
    const influence = Math.exp(-Math.pow(distance * q * 20, 2));
    return -Math.abs(g) * influence * 2;
  }
  return 0;
};

export const calculateMagnitude = (freq: number, bands: EQBand[], proportionalQ: boolean = false) => {
  let totalMagnitude = 0;
  const soloActive = bands.some(b => b.isSolo);
  
  bands.forEach(band => {
    if (!band.isActive) return;
    if (soloActive && !band.isSolo) return;
    totalMagnitude += calculateBandMagnitude(freq, band, proportionalQ);
  });
  
  return totalMagnitude;
};
