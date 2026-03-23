import React, { useRef, useState, useEffect, useCallback } from 'react';
import { Play, Pause, Square, Ear, Repeat, Volume2, VolumeX } from 'lucide-react';
import { EQBand } from '../utils/EQGraphLogic';

interface DevTransportProps {
  audioFile: File | null;
  bands: EQBand[];
  isBypassed: boolean;
  isDeltaMode: boolean;
  onToggleBypass: () => void;
  onToggleDelta: () => void;
  onAnalysisUpdate?: (analysis: { low: number, high: number }) => void;
}

export const DevTransport = ({ 
  audioFile, 
  bands, 
  isBypassed, 
  isDeltaMode,
  onToggleBypass,
  onToggleDelta,
  onAnalysisUpdate
}: DevTransportProps) => {
  const audioContext = useRef<AudioContext | null>(null);
  const [isPlaying, setIsPlaying] = useState(false);
  const [isPaused, setIsPaused] = useState(false);
  const [offset, setOffset] = useState(0);
  const [startTime, setStartTime] = useState(0);
  const [audioBuffer, setAudioBuffer] = useState<AudioBuffer | null>(null);
  const [isLooping, setIsLooping] = useState(true);
  const [is4BarLoop, setIs4BarLoop] = useState(false);
  const [bpm, setBpm] = useState(128);
  const sourceNode = useRef<AudioBufferSourceNode | null>(null);
  const analyserNode = useRef<AnalyserNode | null>(null);
  const filterNodes = useRef<BiquadFilterNode[]>([]);
  const gainNode = useRef<GainNode | null>(null);
  const [loadProgress, setLoadProgress] = useState(0);
  const animationRef = useRef<number | null>(null);

  // Initialize Audio Context on first interaction
  const initAudio = useCallback(() => {
    if (!audioContext.current) {
      audioContext.current = new (window.AudioContext || (window as any).webkitAudioContext)();
    }
    if (audioContext.current.state === 'suspended') {
      audioContext.current.resume();
    }
  }, []);

  // Memory Leak Protection: Close AudioContext on unmount
  useEffect(() => {
    return () => {
      if (audioContext.current) {
        audioContext.current.close();
      }
      if (animationRef.current) {
        cancelAnimationFrame(animationRef.current);
      }
    };
  }, []);

  const stopPlayback = useCallback(() => {
    if (animationRef.current) {
      cancelAnimationFrame(animationRef.current);
      animationRef.current = null;
    }
    if (sourceNode.current) {
      try {
        sourceNode.current.stop();
      } catch (e) {
        // Already stopped
      }
      sourceNode.current.disconnect();
      sourceNode.current = null;
    }
    setIsPlaying(false);
    setIsPaused(false);
  }, []);

  const handleStop = useCallback(() => {
    stopPlayback();
    setOffset(0);
  }, [stopPlayback]);

  const handlePause = useCallback(() => {
    if (isPlaying && audioContext.current) {
      const elapsed = audioContext.current.currentTime - startTime;
      setOffset(prev => (prev + elapsed) % (audioBuffer?.duration || 1));
      stopPlayback();
      setIsPaused(true);
    }
  }, [isPlaying, startTime, audioBuffer, stopPlayback]);

  const loadAudio = async () => {
    if (!audioFile) return null;
    try {
      setLoadProgress(10);
      const arrayBuffer = await audioFile.arrayBuffer();
      setLoadProgress(40);
      const ctx = audioContext.current || new (window.AudioContext || (window as any).webkitAudioContext)();
      if (!audioContext.current) audioContext.current = ctx;
      const buffer = await ctx.decodeAudioData(arrayBuffer);
      setAudioBuffer(buffer);
      setLoadProgress(100);
      return buffer;
    } catch (error) {
      console.error("Load failed:", error);
      setLoadProgress(0);
      return null;
    }
  };

  const buildFilterChain = (source: AudioNode) => {
    if (!audioContext.current) return source;
    
    let lastNode = source;
    filterNodes.current = [];

    bands.filter(b => b.isActive).forEach(band => {
      const filter = audioContext.current!.createBiquadFilter();
      let type: BiquadFilterType = 'peaking';
      if (band.type === 'highpass') type = 'highpass';
      else if (band.type === 'lowpass') type = 'lowpass';
      else if (band.type === 'highshelf') type = 'highshelf';
      else if (band.type === 'lowshelf') type = 'lowshelf';
      else if (band.type === 'notch') type = 'notch';
      
      filter.type = type;
      filter.frequency.value = band.f;
      filter.Q.value = band.q;
      
      if (isBypassed) filter.gain.value = 0;
      else if (isDeltaMode) filter.gain.value = -band.g;
      else filter.gain.value = band.g;

      lastNode.connect(filter);
      lastNode = filter;
      filterNodes.current.push(filter);
    });

    return lastNode;
  };

  const handlePlay = async () => {
    if (!audioFile) return;
    initAudio();

    // If already playing, treat this as a "Pause" toggle (useful for spacebar)
    if (isPlaying) {
      handlePause();
      return;
    }

    let buffer = audioBuffer;
    if (!buffer) {
      buffer = await loadAudio();
    }
    if (!buffer) return;

    let currentOffset = offset;
    if (isPlaying && audioContext.current) {
      const elapsed = audioContext.current.currentTime - startTime;
      currentOffset = (offset + elapsed) % buffer.duration;
    }

    stopPlayback();

    const source = audioContext.current!.createBufferSource();
    source.buffer = buffer;
    source.loop = isLooping;

    if (isLooping && is4BarLoop) {
      const beatDuration = 60 / bpm;
      const fourBarDuration = beatDuration * 16;
      source.loopStart = 0;
      source.loopEnd = Math.min(fourBarDuration, buffer.duration);
    }

    const lastFilterNode = buildFilterChain(source);
    
    gainNode.current = audioContext.current!.createGain();
    analyserNode.current = audioContext.current!.createAnalyser();
    analyserNode.current.fftSize = 2048;

    lastFilterNode.connect(gainNode.current);
    gainNode.current.connect(analyserNode.current);
    analyserNode.current.connect(audioContext.current!.destination);

    source.start(0, currentOffset);
    sourceNode.current = source;
    setStartTime(audioContext.current!.currentTime);
    setOffset(currentOffset);
    setIsPlaying(true);
    setIsPaused(false);

    // Start analysis loop
    const updateAnalysis = () => {
      if (analyserNode.current && onAnalysisUpdate && sourceNode.current) {
        const dataArray = new Uint8Array(analyserNode.current.frequencyBinCount);
        analyserNode.current.getByteFrequencyData(dataArray);
        
        // Simple low/high energy calculation
        let lowSum = 0;
        let highSum = 0;
        const midPoint = Math.floor(dataArray.length / 4); // Focus on lower frequencies for "low"
        
        for (let i = 0; i < midPoint; i++) lowSum += dataArray[i];
        for (let i = midPoint; i < dataArray.length; i++) highSum += dataArray[i];
        
        onAnalysisUpdate({
          low: (lowSum / midPoint) / 255,
          high: (highSum / (dataArray.length - midPoint)) / 255
        });
        animationRef.current = requestAnimationFrame(updateAnalysis);
      } else {
        if (onAnalysisUpdate) onAnalysisUpdate({ low: 0, high: 0 });
        animationRef.current = null;
      }
    };
    updateAnalysis();

    source.onended = () => {
      if (!source.loop) {
        setIsPlaying(false);
        setOffset(0);
      }
    };
  };

  // Update filters in real-time if playing
  useEffect(() => {
    const activeBands = bands.filter(b => b.isActive);
    
    // If number of active bands changed, we need to rebuild the chain
    if (isPlaying && activeBands.length !== filterNodes.current.length) {
      handlePlay(); // Restart with new chain at current offset
      return;
    }

    if (isPlaying && filterNodes.current.length === activeBands.length) {
      activeBands.forEach((band, i) => {
        const filter = filterNodes.current[i];
        if (filter) {
          const now = audioContext.current!.currentTime;
          filter.frequency.setTargetAtTime(band.f, now, 0.05);
          filter.Q.setTargetAtTime(band.q, now, 0.05);
          
          let targetGain = band.g;
          if (isBypassed) targetGain = 0;
          else if (isDeltaMode) targetGain = -band.g;
          
          filter.gain.setTargetAtTime(targetGain, now, 0.05);
        }
      });
    }
  }, [bands, isBypassed, isDeltaMode, isPlaying]);

  // Reset buffer if file changes
  useEffect(() => {
    handleStop();
    setAudioBuffer(null);
  }, [audioFile, handleStop]);

  // Handle Spacebar
  useEffect(() => {
    const handleKeyDown = (e: KeyboardEvent) => {
      if (e.code === 'Space' && audioFile) {
        e.preventDefault();
        handlePlay();
      }
    };
    window.addEventListener('keydown', handleKeyDown);
    return () => window.removeEventListener('keydown', handleKeyDown);
  }, [audioFile, isPlaying, bands, isBypassed, isDeltaMode, isLooping]);

  return (
    <div className="flex items-center gap-4 p-3 bg-black/40 rounded-xl border border-white/5">
      <div className="flex items-center gap-2">
        <button 
          onClick={handlePlay} 
          disabled={!audioFile || isPlaying}
          className={`w-10 h-10 rounded-full flex items-center justify-center transition-all ${(!audioFile || isPlaying) ? 'bg-zinc-800 text-zinc-600' : 'bg-gold text-black hover:scale-105 active:scale-95'}`}
          title="Play"
        >
          <Play size={20} fill="currentColor" className="ml-1" />
        </button>

        <button 
          onClick={handlePause} 
          disabled={!audioFile || !isPlaying}
          className={`w-10 h-10 rounded-full flex items-center justify-center transition-all ${(!audioFile || !isPlaying) ? 'bg-zinc-800 text-zinc-600' : 'bg-zinc-800 text-gold hover:bg-zinc-700'}`}
          title="Pause"
        >
          <Pause size={20} fill="currentColor" />
        </button>

        <button 
          onClick={handleStop} 
          disabled={!audioFile || (!isPlaying && !isPaused && offset === 0)}
          className={`w-10 h-10 rounded-full flex items-center justify-center transition-all ${(!audioFile || (!isPlaying && !isPaused && offset === 0)) ? 'bg-zinc-800 text-zinc-600' : 'bg-zinc-800 text-red-500 hover:bg-zinc-700'}`}
          title="Stop"
        >
          <Square size={20} fill="currentColor" />
        </button>
        
        <div className="w-[1px] h-6 bg-zinc-800 mx-1" />

        <button 
          onClick={() => setIsLooping(!isLooping)}
          className={`p-2 rounded-lg transition-all ${isLooping ? 'text-gold bg-gold/10' : 'text-zinc-600 hover:text-zinc-400'}`}
          title="Toggle Loop"
        >
          <Repeat size={16} />
        </button>

        <button 
          onClick={() => setIs4BarLoop(!is4BarLoop)}
          className={`px-2 py-1 rounded text-[7px] font-black uppercase tracking-tighter border transition-all ${is4BarLoop ? 'bg-gold text-black border-gold' : 'border-zinc-800 text-zinc-600'}`}
          title="Loop exactly 4 bars based on BPM"
        >
          4-BAR
        </button>
      </div>

      <div className="h-8 w-[1px] bg-zinc-800" />

      <div className="flex flex-col gap-1 min-w-[80px]">
        <div className="flex justify-between text-[7px] font-bold text-zinc-500 uppercase">
          <span>BPM</span>
          <span className="text-gold">{bpm}</span>
        </div>
        <input 
          type="range" min="60" max="200" step="1"
          value={bpm}
          onChange={(e) => setBpm(parseInt(e.target.value))}
          className="w-full accent-gold h-1 bg-zinc-800 rounded-full appearance-none"
        />
      </div>

      <div className="h-8 w-[1px] bg-zinc-800" />

      <div className="flex items-center gap-2">
        <button 
          onClick={onToggleBypass}
          className={`px-4 py-2 rounded-lg text-[9px] font-black uppercase tracking-widest border transition-all ${isBypassed ? 'bg-red-500 border-red-400 text-white' : 'bg-zinc-900 border-zinc-800 text-zinc-500 hover:text-white'}`}
        >
          {isBypassed ? "Bypassed (Dry)" : "AI Processing (Wet)"}
        </button>

        <button 
          onClick={onToggleDelta}
          className={`p-2 rounded-lg border transition-all ${isDeltaMode ? 'border-red-500 text-red-500 bg-red-500/10' : 'border-zinc-800 text-zinc-500 hover:text-white'}`}
          title="Delta Mode: Hear only what is being cut/boosted"
        >
          <Ear size={16} />
        </button>
      </div>

      {loadProgress > 0 && loadProgress < 100 && (
        <div className="flex-1 h-1 bg-zinc-800 rounded-full overflow-hidden">
          <div className="h-full bg-gold transition-all duration-300" style={{ width: `${loadProgress}%` }} />
        </div>
      )}

      <div className="ml-auto flex items-center gap-2 text-[8px] font-mono text-zinc-600 uppercase tracking-widest">
        <Volume2 size={12} />
        <span>Web Audio Engine Active</span>
      </div>
    </div>
  );
};
