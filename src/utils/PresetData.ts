import { EQBand } from './EQGraphLogic';

export interface Preset {
  name: string;
  category: string;
  description: string;
  bands: EQBand[];
  outputGain: number;
}

export const PRESETS: Preset[] = [
  // FACTORY & UTILITY
  {
    name: "Default (Total Flat)",
    category: "FACTORY",
    description: "100% Flat. The starting point for surgical precision.",
    bands: [],
    outputGain: 0.5
  },
  {
    name: "Sub-Clear 30Hz",
    category: "UTILITY",
    description: "Cleans up subsonic rumble to save headroom for the mix.",
    bands: [
      { id: 'util-hp-30', f: 30, g: 0, q: 0.707, type: 'highpass', channelMode: 'mid', dynamicThreshold: 0.5, dynamicAmount: 0, slope: 24, isActive: true, isSolo: false, isSpectral: false }
    ],
    outputGain: 0.5
  },
  {
    name: "Bass Focus 60Hz",
    category: "UTILITY",
    description: "Moves the energy out of the sub-zone for cleaner bass clarity.",
    bands: [
      { id: 'util-hp-60', f: 60, g: 0, q: 0.707, type: 'highpass', channelMode: 'mid', dynamicThreshold: 0.5, dynamicAmount: 0, slope: 12, isActive: true, isSolo: false, isSpectral: false }
    ],
    outputGain: 0.5
  },
  {
    name: "Rumble Killer 80Hz",
    category: "UTILITY",
    description: "Aggressive cut for shakers or guitars to remove unwanted mud.",
    bands: [
      { id: 'util-hp-80', f: 80, g: 0, q: 0.707, type: 'highpass', channelMode: 'stereo', dynamicThreshold: 0.5, dynamicAmount: 0, slope: 18, isActive: true, isSolo: false, isSpectral: false }
    ],
    outputGain: 0.5
  },
  {
    name: "Harshness Kill 4kHz",
    category: "UTILITY",
    description: "Targets that painful 'whistle' often found in shakers or bad recordings.",
    bands: [
      { id: 'util-notch-4k', f: 4000, g: -12, q: 8, type: 'notch', channelMode: 'stereo', dynamicThreshold: 0.5, dynamicAmount: 0, slope: 24, isActive: true, isSolo: false, isSpectral: false }
    ],
    outputGain: 0.5
  },
  {
    name: "50Hz AC Hum Remover",
    category: "UTILITY",
    description: "Removes electrical hum from old gear or bad cables.",
    bands: [
      { id: 'util-notch-50', f: 50, g: -24, q: 10, type: 'notch', channelMode: 'mid', dynamicThreshold: 0.5, dynamicAmount: 0, slope: 24, isActive: true, isSolo: false, isSpectral: false }
    ],
    outputGain: 0.5
  },
  {
    name: "Air Seal 18kHz",
    category: "UTILITY",
    description: "Removes high-frequency digital noise while keeping the 'air'.",
    bands: [
      { id: 'util-lp-18k', f: 18000, g: 0, q: 0.707, type: 'lowpass', channelMode: 'stereo', dynamicThreshold: 0.5, dynamicAmount: 0, slope: 12, isActive: true, isSolo: false, isSpectral: false }
    ],
    outputGain: 0.5
  },
  {
    name: "Hiss Cut 15kHz",
    category: "UTILITY",
    description: "Tames noisy recordings or high-end 'fizz' from distorted guitars.",
    bands: [
      { id: 'util-lp-15k', f: 15000, g: 0, q: 0.707, type: 'lowpass', channelMode: 'stereo', dynamicThreshold: 0.5, dynamicAmount: 0, slope: 24, isActive: true, isSolo: false, isSpectral: false }
    ],
    outputGain: 0.5
  },
  {
    name: "Vintage Roll-off",
    category: "UTILITY",
    description: "Creates a warm, 'analog tape' feel by gently smoothing the top end.",
    bands: [
      { id: 'util-lp-10k', f: 10000, g: 0, q: 0.5, type: 'lowpass', channelMode: 'stereo', dynamicThreshold: 0.5, dynamicAmount: 0, slope: 6, isActive: true, isSolo: false, isSpectral: false }
    ],
    outputGain: 0.52
  },
  {
    name: "Underwater Effect",
    category: "UTILITY",
    description: "Extreme cut that makes the sound feel submerged.",
    bands: [
      { id: 'util-lp-300', f: 300, g: 0, q: 1.0, type: 'lowpass', channelMode: 'stereo', dynamicThreshold: 0.5, dynamicAmount: 0, slope: 48, isActive: true, isSolo: false, isSpectral: false }
    ],
    outputGain: 0.6
  },

  // CREATIVE FX
  {
    name: "Telephone Vocal",
    category: "FX",
    description: "The classic nasal effect for intros or special vocal ad-libs.",
    bands: [
      { id: 'fx-tel-1', f: 400, g: 0, q: 0.707, type: 'highpass', channelMode: 'stereo', dynamicThreshold: 0.5, dynamicAmount: 0, slope: 24, isActive: true, isSolo: false, isSpectral: false },
      { id: 'fx-tel-2', f: 3000, g: 0, q: 0.707, type: 'lowpass', channelMode: 'stereo', dynamicThreshold: 0.5, dynamicAmount: 0, slope: 24, isActive: true, isSolo: false, isSpectral: false },
      { id: 'fx-tel-3', f: 1500, g: 6, q: 1.5, type: 'bell', channelMode: 'stereo', dynamicThreshold: 0.5, dynamicAmount: 0, slope: 24, isActive: true, isSolo: false, isSpectral: false }
    ],
    outputGain: 0.45
  },
  {
    name: "AM Radio",
    category: "FX",
    description: "Narrow, thin sound that mimics a small old speaker.",
    bands: [
      { id: 'fx-radio-1', f: 600, g: 0, q: 0.707, type: 'highpass', channelMode: 'stereo', dynamicThreshold: 0.5, dynamicAmount: 0, slope: 12, isActive: true, isSolo: false, isSpectral: false },
      { id: 'fx-radio-2', f: 2000, g: 0, q: 0.707, type: 'lowpass', channelMode: 'stereo', dynamicThreshold: 0.5, dynamicAmount: 0, slope: 12, isActive: true, isSolo: false, isSpectral: false }
    ],
    outputGain: 0.5
  },
  {
    name: "Walkie Talkie",
    category: "FX",
    description: "Lo-fi texture perfect for creative sound design.",
    bands: [
      { id: 'fx-walkie-1', f: 1000, g: 0, q: 0.707, type: 'highpass', channelMode: 'stereo', dynamicThreshold: 0.5, dynamicAmount: 0, slope: 24, isActive: true, isSolo: false, isSpectral: false },
      { id: 'fx-walkie-2', f: 4000, g: 0, q: 0.707, type: 'lowpass', channelMode: 'stereo', dynamicThreshold: 0.5, dynamicAmount: 0, slope: 24, isActive: true, isSolo: false, isSpectral: false },
      { id: 'fx-walkie-3', f: 2500, g: 8, q: 2.0, type: 'bell', channelMode: 'stereo', dynamicThreshold: 0.5, dynamicAmount: 0, slope: 24, isActive: true, isSolo: false, isSpectral: false }
    ],
    outputGain: 0.42
  },

  // CHARACTER
  {
    name: "Tilt: Brighten",
    category: "CHARACTER",
    description: "Gently brightens everything above 1kHz while darkening the lows.",
    bands: [
      { id: 'char-tilt-b1', f: 1000, g: -2, q: 0.5, type: 'lowshelf', channelMode: 'stereo', dynamicThreshold: 0.5, dynamicAmount: 0, slope: 6, isActive: true, isSolo: false, isSpectral: false },
      { id: 'char-tilt-b2', f: 1000, g: 2, q: 0.5, type: 'highshelf', channelMode: 'stereo', dynamicThreshold: 0.5, dynamicAmount: 0, slope: 6, isActive: true, isSolo: false, isSpectral: false }
    ],
    outputGain: 0.5
  },
  {
    name: "Tilt: Darken",
    category: "CHARACTER",
    description: "The opposite; makes the mix feel warm and 'moody'.",
    bands: [
      { id: 'char-tilt-d1', f: 1000, g: 2, q: 0.5, type: 'lowshelf', channelMode: 'stereo', dynamicThreshold: 0.5, dynamicAmount: 0, slope: 6, isActive: true, isSolo: false, isSpectral: false },
      { id: 'char-tilt-d2', f: 1000, g: -2, q: 0.5, type: 'highshelf', channelMode: 'stereo', dynamicThreshold: 0.5, dynamicAmount: 0, slope: 6, isActive: true, isSolo: false, isSpectral: false }
    ],
    outputGain: 0.5
  },

  // AMAPIANO UTILITIES
  {
    name: "Log Thump Stabilizer",
    category: "AMAPIANO",
    description: "Surgical control for the 40-60Hz Log Drum pocket.",
    bands: [
      { id: 'ama-1', f: 50, g: -2, q: 4, type: 'bell', channelMode: 'mid', dynamicThreshold: 0.3, dynamicAmount: 0.4, slope: 24, isActive: true, isSolo: false, isSpectral: true }
    ],
    outputGain: 0.5
  },
  {
    name: "Shaker Air",
    category: "AMAPIANO",
    description: "12kHz Side-Boost for wide, shimmering shakers.",
    bands: [
      { id: 'ama-2', f: 12000, g: 3, q: 0.7, type: 'highshelf', channelMode: 'side', dynamicThreshold: 0.5, dynamicAmount: 0, slope: 24, isActive: true, isSolo: false, isSpectral: false }
    ],
    outputGain: 0.48
  },
  {
    name: "Vocal Clarity Focus",
    category: "AMAPIANO",
    description: "Dynamic Mid-Cut at 300Hz to remove vocal mud.",
    bands: [
      { id: 'ama-3', f: 300, g: -3, q: 2, type: 'bell', channelMode: 'mid', dynamicThreshold: 0.4, dynamicAmount: 0.3, slope: 24, isActive: true, isSolo: false, isSpectral: true }
    ],
    outputGain: 0.52
  },

  // DRUMS
  {
    name: "KICK | Sub Weight",
    category: "DRUMS",
    description: "40Hz Thump for deep Amapiano kicks.",
    bands: [
      { id: 'kick-1', f: 40, g: 4, q: 1.5, type: 'bell', channelMode: 'mid', dynamicThreshold: 0.5, dynamicAmount: 0, slope: 24, isActive: true, isSolo: false, isSpectral: false }
    ],
    outputGain: 0.45
  },
  {
    name: "KICK | Click Forward",
    category: "DRUMS",
    description: "1.5kHz Snaps to help the kick cut through the mix.",
    bands: [
      { id: 'kick-2', f: 1500, g: 3, q: 2.5, type: 'bell', channelMode: 'stereo', dynamicThreshold: 0.5, dynamicAmount: 0, slope: 24, isActive: true, isSolo: false, isSpectral: false }
    ],
    outputGain: 0.48
  },
  {
    name: "KICK | Mud Killer",
    category: "DRUMS",
    description: "250Hz Wide Cut to remove boxiness.",
    bands: [
      { id: 'kick-3', f: 250, g: -4, q: 0.8, type: 'bell', channelMode: 'stereo', dynamicThreshold: 0.5, dynamicAmount: 0, slope: 24, isActive: true, isSolo: false, isSpectral: false }
    ],
    outputGain: 0.52
  },
  {
    name: "SNARE | Rim Bite",
    category: "DRUMS",
    description: "2kHz Snap for crisp snare rims.",
    bands: [
      { id: 'snare-1', f: 2000, g: 3.5, q: 3, type: 'bell', channelMode: 'stereo', dynamicThreshold: 0.5, dynamicAmount: 0, slope: 24, isActive: true, isSolo: false, isSpectral: false }
    ],
    outputGain: 0.47
  },
  {
    name: "SNARE | Body Enhancer",
    category: "DRUMS",
    description: "200Hz Weight for a thick snare sound.",
    bands: [
      { id: 'snare-2', f: 200, g: 2.5, q: 1.2, type: 'bell', channelMode: 'stereo', dynamicThreshold: 0.5, dynamicAmount: 0, slope: 24, isActive: true, isSolo: false, isSpectral: false }
    ],
    outputGain: 0.48
  },
  {
    name: "OH | Shimmer & Air",
    category: "DRUMS",
    description: "12kHz High Shelf for overhead clarity.",
    bands: [
      { id: 'oh-1', f: 12000, g: 4, q: 0.7, type: 'highshelf', channelMode: 'stereo', dynamicThreshold: 0.5, dynamicAmount: 0, slope: 24, isActive: true, isSolo: false, isSpectral: false }
    ],
    outputGain: 0.45
  },

  // LOG DRUM & BASS
  {
    name: "LOG | The Signature Bounce",
    category: "LOG & BASS",
    description: "Dynamic Mix for the ultimate Log Drum bounce.",
    bands: [
      { id: 'log-1', f: 30, g: 0, q: 1, type: 'highpass', channelMode: 'mid', dynamicThreshold: 0.5, dynamicAmount: 0, slope: 24, isActive: true, isSolo: false, isSpectral: false },
      { id: 'log-2', f: 1200, g: 3.5, q: 2.5, type: 'bell', channelMode: 'stereo', dynamicThreshold: 0.4, dynamicAmount: 0.3, slope: 24, isActive: true, isSolo: false, isSpectral: true },
      { id: 'log-3', f: 350, g: -2, q: 1.5, type: 'bell', channelMode: 'mid', dynamicThreshold: 0.5, dynamicAmount: 0.2, slope: 24, isActive: true, isSolo: false, isSpectral: true }
    ],
    outputGain: 0.48
  },
  {
    name: "LOG | Wood Knock",
    category: "LOG & BASS",
    description: "1.2kHz Peak for that classic wooden log sound.",
    bands: [
      { id: 'log-4', f: 1200, g: 5, q: 3.5, type: 'bell', channelMode: 'stereo', dynamicThreshold: 0.5, dynamicAmount: 0, slope: 24, isActive: true, isSolo: false, isSpectral: false }
    ],
    outputGain: 0.45
  },
  {
    name: "LOG | Head Cold Fix",
    category: "LOG & BASS",
    description: "250Hz Notch to fix muffled log drums.",
    bands: [
      { id: 'log-5', f: 250, g: -6, q: 5, type: 'notch', channelMode: 'mid', dynamicThreshold: 0.5, dynamicAmount: 0, slope: 24, isActive: true, isSolo: false, isSpectral: false }
    ],
    outputGain: 0.53
  },
  {
    name: "BASS | Sub Enhancer",
    category: "LOG & BASS",
    description: "Low Shelf Boost for deep sub bass.",
    bands: [
      { id: 'bass-1', f: 80, g: 3.5, q: 0.7, type: 'lowshelf', channelMode: 'mid', dynamicThreshold: 0.5, dynamicAmount: 0, slope: 24, isActive: true, isSolo: false, isSpectral: false }
    ],
    outputGain: 0.46
  },
  {
    name: "BASS | DI Clean Sculpt",
    category: "LOG & BASS",
    description: "Surgical Mids for a clean DI bass sound.",
    bands: [
      { id: 'bass-2', f: 600, g: -3, q: 2, type: 'bell', channelMode: 'mid', dynamicThreshold: 0.5, dynamicAmount: 0, slope: 24, isActive: true, isSolo: false, isSpectral: false }
    ],
    outputGain: 0.51
  },

  // MIX BUS & MASTERING
  {
    name: "MIX BUS | Amapiano Master",
    category: "MASTERING",
    description: "Final Glue for the entire Amapiano mix.",
    bands: [
      { id: 'mast-1', f: 30, g: 0, q: 1, type: 'brickwall', channelMode: 'mid', dynamicThreshold: 0.5, dynamicAmount: 0, slope: 96, isActive: true, isSolo: false, isSpectral: false },
      { id: 'mast-2', f: 120, g: 1.2, q: 0.7, type: 'lowshelf', channelMode: 'mid', dynamicThreshold: 0.5, dynamicAmount: 0, slope: 24, isActive: true, isSolo: false, isSpectral: false },
      { id: 'mast-3', f: 10000, g: 1.5, q: 0.7, type: 'highshelf', channelMode: 'side', dynamicThreshold: 0.5, dynamicAmount: 0, slope: 24, isActive: true, isSolo: false, isSpectral: false }
    ],
    outputGain: 0.49
  },
  {
    name: "MIX BUS | 30Hz Safe-Cut",
    category: "MASTERING",
    description: "Brickwall Low Cut to protect speakers.",
    bands: [
      { id: 'mast-4', f: 30, g: 0, q: 0.707, type: 'brickwall', channelMode: 'mid', dynamicThreshold: 0.5, dynamicAmount: 0, slope: 96, isActive: true, isSolo: false, isSpectral: false }
    ],
    outputGain: 0.5
  },
  {
    name: "MIX BUS | Sparkle Polish",
    category: "MASTERING",
    description: "High-End Clarity for a polished master.",
    bands: [
      { id: 'mast-5', f: 15000, g: 2.5, q: 0.6, type: 'highshelf', channelMode: 'side', dynamicThreshold: 0.5, dynamicAmount: 0, slope: 24, isActive: true, isSolo: false, isSpectral: false }
    ],
    outputGain: 0.48
  },

  // AI SURGICAL TEMPLATES
  {
    name: "[AI] | Bad Vocal Rescue",
    category: "AI SURGICAL",
    description: "AI proposes a 350Hz cut and an 8kHz air-lift to fix low-quality mic recordings.",
    bands: [
      { id: 'ai-vocal-1', f: 350, g: -4.5, q: 1.2, type: 'bell', channelMode: 'mid', dynamicThreshold: 0.4, dynamicAmount: 0.3, slope: 24, isActive: true, isSolo: false, isSpectral: true },
      { id: 'ai-vocal-2', f: 8000, g: 3.2, q: 0.7, type: 'highshelf', channelMode: 'stereo', dynamicThreshold: 0.5, dynamicAmount: 0, slope: 12, isActive: true, isSolo: false, isSpectral: false }
    ],
    outputGain: 0.5
  },
  {
    name: "[AI] | Log Thump Stabilizer",
    category: "AI SURGICAL",
    description: "Smart-detects the moving sub-harmonic and locks the EQ to the fundamental.",
    bands: [
      { id: 'ai-log-1', f: 55, g: -2.5, q: 4.5, type: 'bell', channelMode: 'mid', dynamicThreshold: 0.3, dynamicAmount: 0.5, slope: 24, isActive: true, isSolo: false, isSpectral: true }
    ],
    outputGain: 0.5
  },
  {
    name: "[AI] | Shaker Air Focus",
    category: "AI SURGICAL",
    description: "Dynamically boosts highs only when the shaker transients are detected.",
    bands: [
      { id: 'ai-shaker-1', f: 10000, g: 4.0, q: 0.8, type: 'highshelf', channelMode: 'side', dynamicThreshold: 0.6, dynamicAmount: 0.4, slope: 12, isActive: true, isSolo: false, isSpectral: true }
    ],
    outputGain: 0.48
  },
  {
    name: "[UTILITY] | AC Hum 60Hz",
    category: "AI SURGICAL",
    description: "Pre-calculated surgical notch for electrical interference.",
    bands: [
      { id: 'util-hum-60', f: 60, g: -30, q: 10, type: 'notch', channelMode: 'mid', dynamicThreshold: 0.5, dynamicAmount: 0, slope: 24, isActive: true, isSolo: false, isSpectral: false }
    ],
    outputGain: 0.5
  },
  {
    name: "[UTILITY] | Telephone Intro",
    category: "AI SURGICAL",
    description: "High-Pass 400Hz + Low-Pass 3kHz for classic lo-fi effect.",
    bands: [
      { id: 'util-tel-1', f: 400, g: 0, q: 0.707, type: 'highpass', channelMode: 'stereo', dynamicThreshold: 0.5, dynamicAmount: 0, slope: 24, isActive: true, isSolo: false, isSpectral: false },
      { id: 'util-tel-2', f: 3000, g: 0, q: 0.707, type: 'lowpass', channelMode: 'stereo', dynamicThreshold: 0.5, dynamicAmount: 0, slope: 24, isActive: true, isSolo: false, isSpectral: false }
    ],
    outputGain: 0.5
  }
];
