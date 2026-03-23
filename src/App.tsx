import React, { useState, useEffect, useCallback, useRef } from 'react';
import { motion, AnimatePresence } from 'motion/react';
import { 
  Settings, Code, X, Zap, Activity, Waves, Maximize2, Music, 
  Save, ChevronDown, Layers, Target, Sliders, Volume2, VolumeX, 
  Grid, Scissors, Copy, RefreshCw, Box, Minimize2, RotateCcw, 
  Database, Cpu, Flame, Sparkles, SlidersHorizontal, Radio,
  Terminal, Upload, Play, Ear, Check, Trash
} from 'lucide-react';
import { GoogleGenAI, Type } from "@google/genai";
import { EQGraph } from './components/EQGraph';
import { SpectrumAnalyzer } from './components/SpectrumAnalyzer';
import { WaveformDisplay } from './components/WaveformDisplay';
import { DevTransport } from './components/DevTransport';
import { EQBand, snapToNote, MIN_FREQ, MAX_FREQ } from './utils/EQGraphLogic';
import { HISE_SCRIPT, DSP_CPP, UI_JSON, SPECTRUM_GLSL } from './constants/hiseCode';
import { PRESETS, Preset } from './utils/PresetData';

type PhaseMode = 'Natural' | 'Linear' | 'Zero Latency';
type LinearResolution = 'Low' | 'Medium' | 'High' | 'Max';
type CharacterMode = 'Clean' | 'Subtle' | 'Warm' | 'Bounce';

interface EQState {
  bands: EQBand[];
  outputGain: number;
  phaseMode: PhaseMode;
  linearRes: LinearResolution;
  character: CharacterMode;
  proportionalQ: boolean;
  autoGain: boolean;
  sidechain: boolean;
}

const DEFAULT_STATE: EQState = {
  bands: [],
  outputGain: 0.5,
  phaseMode: 'Natural',
  linearRes: 'High',
  character: 'Clean',
  proportionalQ: false,
  autoGain: true,
  sidechain: false
};

export default function App() {
  // A/B States
  const [stateA, setStateA] = useState<EQState>(DEFAULT_STATE);
  const [stateB, setStateB] = useState<EQState>(DEFAULT_STATE);
  const [activeState, setActiveState] = useState<'A' | 'B'>('A');

  const [isPowerOn, setIsPowerOn] = useState(true);
  const [displayRange, setDisplayRange] = useState<6 | 30>(30);
  const [isSnapOn, setIsSnapOn] = useState(true);
  const [showCode, setShowCode] = useState<'none' | 'hise' | 'dsp' | 'json' | 'glsl' | 'presets'>('none');
  const [analysis, setAnalysis] = useState({ low: 0, high: 0 });
  const [isMuted, setIsMuted] = useState(false);
  const [isMonoSub, setIsMonoSub] = useState(false);
  const [isPhaseFlip, setIsPhaseFlip] = useState(false);
  const [currentPresetName, setCurrentPresetName] = useState("Default (Total Flat)");
  const [showPresets, setShowPresets] = useState(false);

  const [isAiProcessing, setIsAiProcessing] = useState(false);
  const [showCollision, setShowCollision] = useState(true);

  // Dev Sandbox State
  const [isDevPortalOpen, setIsDevPortalOpen] = useState(false);
  const [audioFiles, setAudioFiles] = useState<File[]>([]);
  const [ghostBands, setGhostBands] = useState<EQBand[]>([]);
  const [isBypassed, setIsBypassed] = useState(false);
  const [isDeltaMode, setIsDeltaMode] = useState(false);
  const [aiAnalysisResult, setAiAnalysisResult] = useState<string | null>(null);

  const currentState = activeState === 'A' ? stateA : stateB;

  // AI Hook: onAiComplete
  const hook_onAiComplete = (proposal: { bands: EQBand[], analysis: string }) => {
    setGhostBands(proposal.bands);
    setAiAnalysisResult(proposal.analysis);
  };

  // AI Hook: applyMatch
  const hook_applyMatch = () => {
    if (ghostBands.length > 0) {
      handleUpdateBands([...currentState.bands, ...ghostBands]);
      setGhostBands([]);
      setAiAnalysisResult(null);
    }
  };

  // AI Logic: Gemini 3 Pro Backend
  const triggerAiAnalyze = async () => {
    if (audioFiles.length === 0) return;
    setIsAiProcessing(true);

    try {
      const ai = new GoogleGenAI({ apiKey: process.env.GEMINI_API_KEY! });
      
      const file = audioFiles[0];
      
      const getBase64 = (file: File): Promise<string> => {
        return new Promise((resolve, reject) => {
          const reader = new FileReader();
          reader.readAsDataURL(file);
          reader.onload = () => {
            const result = reader.result as string;
            resolve(result.split(',')[1]);
          };
          reader.onerror = error => reject(error);
        });
      };

      const base64Data = await getBase64(file);

      const prompt = `Analyze this audio for EQ improvements. 
      Identify resonant peaks and propose surgical notches. 
      Also suggest a 'Target Tone' curve for an 'Amapiano Master' profile.
      Return a JSON object with 'bands' (array of EQBand objects) and 'analysis' (string description). 
      EQBand should have: f (freq), g (gain), q (quality), type ('bell' or 'notch'), isActive (true).
      Ensure frequencies are between 20 and 20000. 
      Gains should be between -12 and 12. 
      Q should be between 0.1 and 10.`;

      const response = await ai.models.generateContent({
        model: "gemini-3-flash-preview",
        contents: [
          {
            parts: [
              { text: prompt },
              {
                inlineData: {
                  mimeType: file.type,
                  data: base64Data
                }
              }
            ]
          }
        ],
        config: {
          responseMimeType: "application/json",
          responseSchema: {
            type: Type.OBJECT,
            properties: {
              bands: {
                type: Type.ARRAY,
                items: {
                  type: Type.OBJECT,
                  properties: {
                    f: { type: Type.NUMBER },
                    g: { type: Type.NUMBER },
                    q: { type: Type.NUMBER },
                    type: { type: Type.STRING },
                    isActive: { type: Type.BOOLEAN }
                  },
                  required: ["f", "g", "q", "type", "isActive"]
                }
              },
              analysis: { type: Type.STRING }
            },
            required: ["bands", "analysis"]
          }
        }
      });

      const result = JSON.parse(response.text);
      
      // Map AI response to our EQBand structure (adding IDs and defaults)
      const proposalBands = result.bands.map((b: any, i: number) => ({
        ...b,
        id: `ai-${Date.now()}-${i}`,
        channelMode: 'stereo',
        dynamicThreshold: 0.5,
        dynamicAmount: 0,
        slope: 24,
        isSolo: false,
        isSpectral: b.type === 'notch'
      }));

      hook_onAiComplete({
        bands: proposalBands,
        analysis: result.analysis
      });
      setIsAiProcessing(false);

    } catch (error) {
      console.error("AI Analysis failed:", error);
      setIsAiProcessing(false);
      // Fallback to mock if real fails for some reason (e.g. quota)
      setAiAnalysisResult("AI Analysis failed. Please check your connection or try again later.");
    }
  };

  const handleFileUpload = (e: React.ChangeEvent<HTMLInputElement>) => {
    if (e.target.files) {
      const newFiles = Array.from(e.target.files);
      setAudioFiles(prev => [...prev, ...newFiles].slice(0, 2));
    }
  };

  // AI Hook: Smart Resonance Suppression
  const triggerResKill = () => {
    setIsAiProcessing(true);
    // Simulate AI action
    setTimeout(() => {
      setIsAiProcessing(false);
      // In a real app, this would call into the DSP engine
      console.log("AI Action: ResonanceKill triggered");
    }, 800);
  };

  // AI Hook: Smart Learn
  const triggerSmartLearn = () => {
    setIsAiProcessing(true);
    setTimeout(() => {
      setIsAiProcessing(false);
      // Suggest 3 essential bands
      const suggestedBands: EQBand[] = [
        { id: 'ai-sub', f: 30, g: 0, q: 0.707, type: 'highpass', channelMode: 'mid', dynamicThreshold: 0.5, dynamicAmount: 0, slope: 24, isActive: true, isSolo: false, isSpectral: false },
        { id: 'ai-box', f: 400, g: -3, q: 1.5, type: 'bell', channelMode: 'mid', dynamicThreshold: 0.4, dynamicAmount: 0.2, slope: 24, isActive: true, isSolo: false, isSpectral: true },
        { id: 'ai-air', f: 12000, g: 2, q: 0.7, type: 'highshelf', channelMode: 'stereo', dynamicThreshold: 0.5, dynamicAmount: 0, slope: 12, isActive: true, isSolo: false, isSpectral: false }
      ];
      handleUpdateBands(suggestedBands);
      setCurrentPresetName("AI Suggested Curve");
    }, 1200);
  };

  const updateCurrentState = (updates: Partial<EQState>) => {
    if (activeState === 'A') {
      setStateA(prev => ({ ...prev, ...updates }));
    } else {
      setStateB(prev => ({ ...prev, ...updates }));
    }
  };

  const resetToDefault = () => {
    const defaultPreset = PRESETS[0];
    updateCurrentState({ bands: defaultPreset.bands, outputGain: defaultPreset.outputGain });
    setCurrentPresetName(defaultPreset.name);
  };

  const loadPreset = (preset: Preset) => {
    updateCurrentState({ bands: preset.bands, outputGain: preset.outputGain });
    setCurrentPresetName(preset.name);
    setShowPresets(false);
  };

  const categories = Array.from(new Set(PRESETS.map(p => p.category)));

  const toggleAB = () => {
    setActiveState(prev => prev === 'A' ? 'B' : 'A');
  };

  const copyAToB = () => {
    if (activeState === 'A') setStateB({ ...stateA });
    else setStateA({ ...stateB });
  };

  // Auto-Gain Compensation Logic
  const calculateAutoGain = (bands: EQBand[]) => {
    const totalBoost = bands.reduce((acc, b) => b.isActive ? acc + (b.g > 0 ? b.g : 0) : acc, 0);
    const totalCut = bands.reduce((acc, b) => b.isActive ? acc + (b.g < 0 ? b.g : 0) : acc, 0);
    // Simple compensation: reduce output gain as boost increases
    const compensation = Math.max(0.1, 0.5 - (totalBoost * 0.01) + (Math.abs(totalCut) * 0.005));
    return compensation;
  };

  const handleUpdateBands = (newBands: EQBand[]) => {
    const bandsToSet = isSnapOn ? newBands.map(b => ({ ...b, f: snapToNote(b.f) })) : newBands;
    const autoGain = calculateAutoGain(bandsToSet);
    updateCurrentState({ bands: bandsToSet, outputGain: autoGain });
  };

  return (
    <div className="min-h-screen bg-[#050505] text-white font-sans selection:bg-gold/30 overflow-hidden flex items-center justify-center p-4">
      <div className="fixed inset-0 bg-[radial-gradient(circle_at_50%_50%,#111_0%,#000_100%)] pointer-events-none" />
      
      <motion.div 
        initial={{ opacity: 0, scale: 0.98 }}
        animate={{ opacity: 1, scale: 1 }}
        className="relative w-[1400px] h-[920px] bg-[#0c0c0c] rounded-2xl border border-zinc-800 shadow-[0_0_150px_rgba(0,0,0,1)] overflow-hidden flex flex-col"
      >
        {/* Header */}
        <div className="h-16 border-b border-zinc-800/50 flex items-center justify-between px-10 bg-black/40">
          <div className="flex flex-col">
            <span className="text-2xl font-black text-white tracking-tighter uppercase italic">Clean Slate</span>
            <span className="text-[8px] font-bold text-gold tracking-[0.4em] uppercase">Surgical AI EQ</span>
          </div>

          <div className="flex items-center gap-6">
            {/* AI Control Hub */}
            <div className="flex items-center gap-3">
              <button 
                onClick={triggerResKill}
                className={`px-4 py-2 rounded-lg border text-[9px] font-black uppercase tracking-widest transition-all ${isAiProcessing ? 'bg-red-500 border-red-400 text-white' : 'bg-zinc-900 border-zinc-800 text-zinc-400 hover:text-white'}`}
              >
                <Zap size={12} className="inline mr-2" /> Res-Kill
              </button>
              <button 
                onClick={triggerSmartLearn}
                className="px-4 py-2 bg-zinc-900 border border-zinc-800 rounded-lg text-[9px] font-black uppercase tracking-widest text-zinc-400 hover:text-gold transition-all"
              >
                <Sparkles size={12} className="inline mr-2" /> Smart Learn
              </button>
              <button 
                onClick={() => setShowCollision(!showCollision)}
                className={`p-2 rounded-lg border transition-all ${showCollision ? 'border-gold/50 text-gold bg-gold/5' : 'border-zinc-800 text-zinc-500'}`}
                title="Toggle Collision Glow"
              >
                <Target size={16} />
              </button>
            </div>

            <div className="w-[1px] h-8 bg-zinc-800 mx-2" />

            {/* Preset Selector */}
            <div className="relative">
              <button 
                onClick={() => setShowPresets(!showPresets)}
                className="flex items-center gap-3 px-4 py-2 bg-zinc-900 border border-zinc-800 rounded-lg text-[10px] font-bold text-zinc-300 hover:text-gold transition-all uppercase tracking-widest"
              >
                <Database size={14} className="text-gold/50" />
                {currentPresetName}
                <ChevronDown size={12} />
              </button>
              
              <AnimatePresence>
                {showPresets && (
                  <motion.div 
                    initial={{ opacity: 0, y: 10 }}
                    animate={{ opacity: 1, y: 0 }}
                    exit={{ opacity: 0, y: 10 }}
                    className="absolute top-full left-0 mt-2 w-72 bg-zinc-900 border border-zinc-800 rounded-xl shadow-2xl z-[110] overflow-hidden max-h-[500px] overflow-y-auto"
                  >
                    {categories.map(cat => (
                      <div key={cat} className="flex flex-col">
                        <div className="px-4 py-2 bg-black/40 text-[7px] font-black text-gold/40 uppercase tracking-[0.3em] border-b border-zinc-800/50">
                          {cat}
                        </div>
                        {PRESETS.filter(p => p.category === cat).map((p) => (
                          <button 
                            key={p.name}
                            onClick={() => loadPreset(p)}
                            className="w-full px-6 py-3 text-left group hover:bg-gold transition-all border-b border-zinc-800/50 last:border-0"
                          >
                            <div className="flex flex-col gap-0.5">
                              <span className="text-[9px] font-bold uppercase tracking-widest text-zinc-300 group-hover:text-black">{p.name}</span>
                              <span className="text-[7px] font-medium text-zinc-600 group-hover:text-black/60 lowercase italic">{p.description}</span>
                            </div>
                          </button>
                        ))}
                      </div>
                    ))}
                  </motion.div>
                )}
              </AnimatePresence>
            </div>

            <div className="flex bg-zinc-900 rounded-lg p-1 border border-zinc-800">
              {(['Natural', 'Linear', 'Zero Latency'] as PhaseMode[]).map(m => (
                <button 
                  key={m}
                  onClick={() => updateCurrentState({ phaseMode: m })}
                  className={`px-3 py-1 text-[9px] font-bold uppercase tracking-widest rounded transition-all ${currentState.phaseMode === m ? 'bg-gold text-black' : 'text-zinc-500'}`}
                >
                  {m}
                </button>
              ))}
            </div>

            <div className="flex bg-zinc-900 rounded-lg p-1 border border-zinc-800">
              <button 
                onClick={() => setDisplayRange(6)}
                className={`px-3 py-1 text-[9px] font-bold uppercase tracking-widest rounded transition-all ${displayRange === 6 ? 'bg-gold text-black' : 'text-zinc-500'}`}
              >
                6dB
              </button>
              <button 
                onClick={() => setDisplayRange(30)}
                className={`px-3 py-1 text-[9px] font-bold uppercase tracking-widest rounded transition-all ${displayRange === 30 ? 'bg-gold text-black' : 'text-zinc-500'}`}
              >
                30dB
              </button>
            </div>
          </div>

          <div className="flex items-center gap-4">
            <button 
              onClick={resetToDefault}
              className="p-2 text-zinc-500 hover:text-gold transition-colors"
              title="Reset to Flat"
            >
              <RotateCcw size={18} />
            </button>
            <button 
              onClick={() => setIsSnapOn(!isSnapOn)}
              className={`p-2 transition-colors ${isSnapOn ? 'text-gold' : 'text-zinc-500 hover:text-white'}`}
              title="Piano Roll Snap"
            >
              <Grid size={18} />
            </button>
            <button onClick={() => setIsMuted(!isMuted)} className="p-2 text-zinc-500 hover:text-gold transition-colors">
              {isMuted ? <VolumeX size={20} /> : <Volume2 size={20} />}
            </button>
            <button onClick={() => setShowCode('dsp')} className="p-2 text-zinc-500 hover:text-gold transition-colors">
              <Settings size={20} />
            </button>
            <button 
              onClick={() => setIsDevPortalOpen(!isDevPortalOpen)}
              className={`p-2 transition-colors ${isDevPortalOpen ? 'text-gold' : 'text-zinc-500 hover:text-white'}`}
              title="Developer Portal"
            >
              <Terminal size={20} />
            </button>
            <button 
              onClick={() => setIsPowerOn(!isPowerOn)}
              className={`w-10 h-10 rounded-xl border flex items-center justify-center transition-all ${isPowerOn ? 'border-gold/50 text-gold bg-gold/5' : 'border-zinc-800 text-zinc-700'}`}
            >
              <Zap size={18} />
            </button>
          </div>
        </div>

        {/* Main Display Area */}
        <div className="flex-1 relative p-12 flex flex-col gap-8">
          <div className="relative flex-1 rounded-2xl overflow-hidden border border-zinc-800 bg-black/40">
            {showCollision && (
              <div className="absolute inset-0 bg-[radial-gradient(circle_at_30%_50%,rgba(239,68,68,0.05)_0%,transparent_50%)] animate-pulse pointer-events-none z-0" />
            )}
            <SpectrumAnalyzer 
              low={analysis.low} 
              high={analysis.high} 
              collision={showCollision && currentState.sidechain}
              isDeltaMode={isDeltaMode}
            />
            <EQGraph 
              bands={currentState.bands} 
              ghostBands={ghostBands}
              onUpdateBands={handleUpdateBands} 
              width={1304} 
              height={isDevPortalOpen ? 380 : 540} 
              displayRange={displayRange}
              proportionalQ={currentState.proportionalQ}
              isBypassed={isBypassed}
              isDeltaMode={isDeltaMode}
            />
            
            {/* Dev Portal Overlay */}
            <AnimatePresence>
              {isDevPortalOpen && (
                <motion.div 
                  initial={{ opacity: 0, y: 20 }}
                  animate={{ opacity: 1, y: 0 }}
                  exit={{ opacity: 0, y: 20 }}
                  className="absolute bottom-0 left-0 right-0 bg-black/90 border-t border-zinc-800 p-6 z-50 flex flex-col gap-4"
                >
                  <div className="flex items-center justify-between">
                    <div className="flex items-center gap-4">
                      <span className="text-[10px] font-black text-gold uppercase tracking-[0.3em]">Developer Sandbox</span>
                      <div className="flex gap-2">
                        <button 
                          onClick={() => setIsBypassed(!isBypassed)}
                          className={`px-3 py-1 rounded text-[8px] font-bold uppercase tracking-widest border transition-all ${isBypassed ? 'bg-gold text-black border-gold' : 'border-zinc-800 text-zinc-500'}`}
                        >
                          Bypass
                        </button>
                        <button 
                          onClick={() => setIsDeltaMode(!isDeltaMode)}
                          className={`px-3 py-1 rounded text-[8px] font-bold uppercase tracking-widest border transition-all ${isDeltaMode ? 'bg-red-500 text-white border-red-400' : 'border-zinc-800 text-zinc-500'}`}
                        >
                          <Ear size={10} className="inline mr-1" /> Delta
                        </button>
                      </div>
                    </div>
                    <div className="flex items-center gap-3">
                      {ghostBands.length > 0 && (
                        <button 
                          onClick={hook_applyMatch}
                          className="px-4 py-1.5 bg-gold text-black rounded-lg text-[9px] font-black uppercase tracking-widest flex items-center gap-2"
                        >
                          <Check size={12} /> Apply AI Match
                        </button>
                      )}
                      <button onClick={() => setIsDevPortalOpen(false)} className="text-zinc-500 hover:text-white">
                        <X size={16} />
                      </button>
                    </div>
                  </div>

                  {/* Dev Transport & AI Analysis Control */}
                  <div className="flex gap-6 h-32">
                    <div className="w-1/3 flex flex-col gap-4">
                      {/* File Drop Zone */}
                      <div className="flex-1 border-2 border-dashed border-zinc-800 rounded-xl flex flex-col items-center justify-center gap-2 hover:border-gold/50 transition-all relative group">
                        <input 
                          type="file" 
                          multiple 
                          accept=".wav,.mp3" 
                          onChange={handleFileUpload}
                          className="absolute inset-0 opacity-0 cursor-pointer"
                        />
                        <Upload size={24} className="text-zinc-700 group-hover:text-gold transition-colors" />
                        <span className="text-[8px] font-bold text-zinc-600 uppercase tracking-widest">Drop .wav / .mp3</span>
                        {audioFiles.length > 0 && (
                          <div className="absolute top-2 right-2 flex gap-1">
                            {audioFiles.map((_, i) => (
                              <div key={i} className="w-2 h-2 bg-gold rounded-full" />
                            ))}
                          </div>
                        )}
                      </div>
                    </div>

                    {/* AI Analysis Control */}
                    <div className="flex-1 bg-zinc-900/40 rounded-xl border border-zinc-800 p-4 flex flex-col gap-3">
                      <div className="flex items-center justify-between">
                        <span className="text-[9px] font-bold text-zinc-500 uppercase tracking-widest">AI Analysis Engine</span>
                        <button 
                          onClick={triggerAiAnalyze}
                          disabled={isAiProcessing || audioFiles.length === 0}
                          className={`px-4 py-2 rounded-lg text-[9px] font-black uppercase tracking-widest transition-all ${isAiProcessing ? 'bg-zinc-800 text-zinc-600' : 'bg-white text-black hover:bg-gold'}`}
                        >
                          {isAiProcessing ? 'Analyzing...' : 'AI Analyze'}
                        </button>
                      </div>
                      <div className="flex-1 bg-black/40 rounded-lg p-3 overflow-y-auto">
                        <p className="text-[9px] font-mono text-zinc-400 leading-relaxed">
                          {aiAnalysisResult || (audioFiles.length === 0 ? "Upload audio files to begin AI analysis..." : `${audioFiles.length} file(s) ready. Click Analyze to detect resonances.`)}
                        </p>
                      </div>
                    </div>
                  </div>

                  {/* Audio Engine Transport */}
                  <DevTransport 
                    audioFile={audioFiles[0] || null}
                    bands={currentState.bands}
                    isBypassed={isBypassed}
                    isDeltaMode={isDeltaMode}
                    onToggleBypass={() => setIsBypassed(!isBypassed)}
                    onToggleDelta={() => setIsDeltaMode(!isDeltaMode)}
                    onAnalysisUpdate={(data) => {
                      if (isPowerOn && !isMuted) {
                        setAnalysis(data);
                      } else {
                        setAnalysis({ low: 0, high: 0 });
                      }
                    }}
                  />

                  {/* Waveform Display */}
                  <div className="h-32 bg-black/40 rounded-xl border border-zinc-800 overflow-hidden">
                    <WaveformDisplay files={audioFiles} bands={currentState.bands} isBypassed={isBypassed} />
                  </div>
                </motion.div>
              )}
            </AnimatePresence>
            
            {/* Status Overlays */}
            <div className="absolute top-6 left-6 flex flex-col gap-2">
              <div className="flex items-center gap-2 text-[9px] font-mono text-gold/40">
                <Activity size={12} />
                <span>DYNAMIC ENGINE: ACTIVE</span>
              </div>
              <div className="flex items-center gap-2 text-[9px] font-mono text-gold/40">
                <Layers size={12} />
                <span>FFT BINS: 2048</span>
              </div>
            </div>

            {/* A/B Switch Overlay */}
            <div className="absolute top-6 right-6 flex items-center gap-2 bg-black/60 p-1 rounded-lg border border-zinc-800">
              <button 
                onClick={() => setActiveState('A')}
                className={`px-3 py-1 text-[9px] font-bold rounded transition-all ${activeState === 'A' ? 'bg-gold text-black' : 'text-zinc-500'}`}
              >
                A
              </button>
              <button 
                onClick={() => setActiveState('B')}
                className={`px-3 py-1 text-[9px] font-bold rounded transition-all ${activeState === 'B' ? 'bg-gold text-black' : 'text-zinc-500'}`}
              >
                B
              </button>
              <div className="w-[1px] h-4 bg-zinc-800 mx-1" />
              <button 
                onClick={copyAToB}
                className="p-1 text-zinc-500 hover:text-white transition-colors"
                title="Copy to other state"
              >
                <Copy size={12} />
              </button>
            </div>
          </div>

          {/* Bento Rack & Bottom Controls */}
          <div className="h-48 flex gap-8">
            {/* Processing & Character */}
            <div className="w-1/4 bg-zinc-900/20 rounded-xl border border-zinc-800/50 p-6 flex flex-col gap-4">
              <div className="flex items-center gap-2 text-gold">
                <Flame size={14} />
                <span className="text-[10px] font-black uppercase tracking-widest">Character</span>
              </div>
              <div className="grid grid-cols-2 gap-2">
                {(['Clean', 'Subtle', 'Warm', 'Bounce'] as CharacterMode[]).map(m => (
                  <button 
                    key={m}
                    onClick={() => updateCurrentState({ character: m })}
                    className={`px-2 py-1.5 rounded-lg text-[8px] font-bold uppercase tracking-widest transition-all ${currentState.character === m ? 'bg-gold text-black' : 'bg-white/5 text-zinc-500 hover:bg-white/10'}`}
                  >
                    {m}
                  </button>
                ))}
              </div>
              <div className="mt-auto flex items-center justify-between bg-black/40 p-2 rounded-lg border border-white/5">
                <span className="text-[7px] font-bold text-zinc-500 uppercase">Sidechain</span>
                <button 
                  onClick={() => updateCurrentState({ sidechain: !currentState.sidechain })}
                  className={`w-8 h-4 rounded-full relative transition-colors ${currentState.sidechain ? 'bg-gold' : 'bg-zinc-800'}`}
                >
                  <div className={`absolute top-1 w-2 h-2 bg-white rounded-full transition-all ${currentState.sidechain ? 'right-1' : 'left-1'}`} />
                </button>
              </div>
            </div>

            {/* LP Resolution */}
            <div className="w-1/4 bg-zinc-900/20 rounded-xl border border-zinc-800/50 p-6 flex flex-col gap-4">
              <div className="flex items-center gap-2 text-gold">
                <SlidersHorizontal size={14} />
                <span className="text-[10px] font-black uppercase tracking-widest">LP Resolution</span>
              </div>
              <div className="flex flex-col gap-3">
                <div className="flex justify-between text-[8px] font-bold text-zinc-500 uppercase">
                  <span>Accuracy</span>
                  <span className="text-gold">{currentState.linearRes}</span>
                </div>
                <input 
                  type="range" min="0" max="3" step="1"
                  value={['Low', 'Medium', 'High', 'Max'].indexOf(currentState.linearRes)}
                  onChange={(e) => updateCurrentState({ linearRes: ['Low', 'Medium', 'High', 'Max'][parseInt(e.target.value)] as LinearResolution })}
                  className="w-full accent-gold h-1.5 bg-zinc-800 rounded-full appearance-none"
                />
                <p className="text-[8px] text-zinc-600 leading-relaxed">Higher resolution increases latency but improves low-frequency surgical precision.</p>
              </div>
            </div>

            {/* Bento Rack */}
            <div className="w-1/4 bg-zinc-900/20 rounded-xl border border-zinc-800/50 p-6 flex items-center justify-between">
              <div className="flex flex-col gap-4">
                <span className="text-[8px] font-bold text-white/20 uppercase tracking-widest">Bento Rack</span>
                <div className="flex gap-5">
                  <button 
                    onClick={() => setIsMonoSub(!isMonoSub)}
                    className={`flex flex-col items-center gap-2 p-3 rounded-lg border transition-all ${isMonoSub ? 'border-gold/50 bg-gold/5 text-gold' : 'border-zinc-800 text-zinc-500'}`}
                  >
                    <Minimize2 size={16} />
                    <span className="text-[7px] font-bold uppercase">Mono Sub</span>
                  </button>
                  <button 
                    onClick={() => setIsPhaseFlip(!isPhaseFlip)}
                    className={`flex flex-col items-center gap-2 p-3 rounded-lg border transition-all ${isPhaseFlip ? 'border-gold/50 bg-gold/5 text-gold' : 'border-zinc-800 text-zinc-500'}`}
                  >
                    <RefreshCw size={16} />
                    <span className="text-[7px] font-bold uppercase">Phase Flip</span>
                  </button>
                  <button 
                    onClick={() => updateCurrentState({ proportionalQ: !currentState.proportionalQ })}
                    className={`flex flex-col items-center gap-2 p-3 rounded-lg border transition-all ${currentState.proportionalQ ? 'border-gold/50 bg-gold/5 text-gold' : 'border-zinc-800 text-zinc-500'}`}
                  >
                    <Zap size={16} />
                    <span className="text-[7px] font-bold uppercase">Prop. Q</span>
                  </button>
                </div>
              </div>
            </div>

            {/* Main Controls */}
            <div className="flex-1 bg-zinc-900/20 rounded-xl border border-zinc-800/50 flex items-center justify-between px-10">
              <div className="flex gap-10">
                <div className="flex flex-col gap-2">
                  <span className="text-[8px] font-bold text-white/20 uppercase tracking-widest">Active Bands</span>
                  <span className="text-2xl font-black text-gold">{currentState.bands.filter(b => b.isActive).length}</span>
                </div>
                <div className="w-[1px] h-12 bg-zinc-800 self-center" />
                <div className="flex flex-col gap-2">
                  <span className="text-[8px] font-bold text-white/20 uppercase tracking-widest">Auto-Gain</span>
                  <span className="text-sm font-bold text-gold uppercase tracking-widest">
                    {currentState.outputGain.toFixed(2)}x
                  </span>
                </div>
              </div>

              <div className="flex items-center gap-12">
                <div className="flex flex-col items-center gap-3">
                  <span className="text-[8px] font-bold text-white/20 uppercase tracking-widest">Output Gain</span>
                  <div className="w-56 h-2 bg-zinc-800 rounded-full overflow-hidden">
                    <motion.div 
                      animate={{ width: `${currentState.outputGain * 100}%` }}
                      className="h-full bg-gold" 
                    />
                  </div>
                </div>
                <button className="p-4 bg-zinc-800 rounded-lg text-zinc-400 hover:text-gold transition-colors">
                  <Sliders size={20} />
                </button>
              </div>
            </div>
          </div>
        </div>

        {/* Footer */}
        <div className="h-10 border-t border-zinc-800/50 flex items-center justify-between px-10 text-[8px] text-white/10 uppercase tracking-[0.3em] font-black">
          <span>Clean Slate AI v1.0</span>
          <span>Surgical Problem Solver</span>
          <span>Natural Phase AI Core</span>
        </div>

        {/* Code Viewer */}
        <AnimatePresence>
          {showCode !== 'none' && (
            <motion.div 
              initial={{ opacity: 0 }}
              animate={{ opacity: 1 }}
              exit={{ opacity: 0 }}
              className="absolute inset-0 bg-black/98 z-[100] flex flex-col"
            >
              <div className="h-16 border-b border-zinc-800 flex items-center justify-between px-10">
                <div className="flex gap-6">
                  {(['dsp', 'hise', 'json', 'glsl', 'presets'] as const).map((tab) => (
                    <button 
                      key={tab}
                      onClick={() => setShowCode(tab)}
                      className={`text-[10px] font-black uppercase tracking-[0.3em] transition-colors ${showCode === tab ? 'text-gold' : 'text-zinc-500 hover:text-white'}`}
                    >
                      {tab === 'dsp' ? 'Core.cpp' : tab === 'hise' ? 'Interface.js' : tab === 'json' ? 'Layout.json' : tab === 'glsl' ? 'Shader.glsl' : 'Presets.json'}
                    </button>
                  ))}
                </div>
                <button onClick={() => setShowCode('none')} className="p-2 hover:bg-white/10 rounded-full transition-colors">
                  <X size={20} />
                </button>
              </div>
              <div className="flex-1 p-10 overflow-auto font-mono text-xs text-zinc-500 leading-relaxed">
                <pre>
                  {showCode === 'dsp' && DSP_CPP}
                  {showCode === 'hise' && HISE_SCRIPT}
                  {showCode === 'json' && UI_JSON}
                  {showCode === 'glsl' && SPECTRUM_GLSL}
                  {showCode === 'presets' && JSON.stringify({ Presets: PRESETS }, null, 2)}
                </pre>
              </div>
            </motion.div>
          )}
        </AnimatePresence>
      </motion.div>
    </div>
  );
}
