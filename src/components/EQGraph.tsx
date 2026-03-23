import React, { useState, useRef, useEffect, useMemo, useCallback } from 'react';
import { motion, useMotionValue, AnimatePresence } from 'motion/react';
import { EQBand, freqToX, xToFreq, gainToY, yToGain, calculateMagnitude, calculateBandMagnitude, snapToNote, MIN_FREQ, MAX_FREQ, getBandColor } from '../utils/EQGraphLogic';
import { Sliders, Activity, Target, Trash2, Power, Zap, Eye, EyeOff, ChevronDown, Move } from 'lucide-react';
import { FloatingBandControl } from './FloatingBandControl';

interface EQGraphProps {
  bands: EQBand[];
  ghostBands?: EQBand[];
  onUpdateBands: (bands: EQBand[]) => void;
  width?: number;
  height?: number;
  displayRange?: number;
  proportionalQ?: boolean;
  isBypassed?: boolean;
  isDeltaMode?: boolean;
}

const NOTES = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"];
const getNoteFromFreq = (f: number) => {
  const n = 12 * (Math.log2(f / 440.0)) + 69;
  const noteIdx = Math.floor(n) % 12;
  const octave = Math.floor(n / 12 - 1);
  return `${NOTES[noteIdx]}${octave}`;
};

export const EQGraph = ({ 
  bands, 
  ghostBands = [], 
  onUpdateBands, 
  width = 1000, 
  height = 400, 
  displayRange = 30, 
  proportionalQ = false,
  isBypassed = false,
  isDeltaMode = false
}: EQGraphProps) => {
  const svgRef = useRef<SVGSVGElement>(null);
  const [selectedBandId, setSelectedBandId] = useState<string | null>(null);
  const [isDragging, setIsDragging] = useState(false);
  const [dragMode, setDragMode] = useState<'node' | 'dynamic'>('node');
  const [ghostNode, setGhostNode] = useState<{ x: number, y: number, freq: number } | null>(null);
  const [hoverStartTime, setHoverStartTime] = useState<number | null>(null);
  const [isFrozen, setIsFrozen] = useState(false);
  const [hudPos, setHudPos] = useState({ x: 0, y: 0 });
  
  // EQ Sketching State
  const [isSketching, setIsSketching] = useState(false);
  const [sketchPoints, setSketchPoints] = useState<{ x: number, y: number }[]>([]);

  // Motion Values for the curve paths
  const pathData = useMotionValue("");
  const ghostPathData = useMotionValue("");
  const aiGhostPathData = useMotionValue("");
  const deltaPathData = useMotionValue("");

  const handleSketchEnd = useCallback(() => {
    if (!isSketching || sketchPoints.length < 10) {
      setIsSketching(false);
      setSketchPoints([]);
      return;
    }

    const windowSize = 8;
    const zeroY = height / 2;
    
    const extremas = sketchPoints.filter((pt, i, arr) => {
      if (i < windowSize || i >= arr.length - windowSize) return false;
      if (Math.abs(pt.y - zeroY) < 15) return false;

      let isPeak = true;
      let isValley = true;
      for (let j = 1; j <= windowSize; j++) {
        if (arr[i-j].y <= pt.y || arr[i+j].y <= pt.y) isPeak = false;
        if (arr[i-j].y >= pt.y || arr[i+j].y >= pt.y) isValley = false;
      }
      return isPeak || isValley;
    });

    const cleanExtremas: typeof sketchPoints = [];
    extremas.forEach(ext => {
      if (cleanExtremas.length === 0 || Math.abs(ext.x - cleanExtremas[cleanExtremas.length - 1].x) > 40) {
        cleanExtremas.push(ext);
      }
    });

    const newBands: EQBand[] = cleanExtremas.map(p => {
      const idx = sketchPoints.findIndex(sp => sp.x === p.x && sp.y === p.y);
      const peakGain = Math.abs(p.y - zeroY);
      
      let leftIdx = idx;
      while (leftIdx > 0 && Math.abs(sketchPoints[leftIdx].y - zeroY) > peakGain * 0.7) leftIdx--;
      
      let rightIdx = idx;
      while (rightIdx < sketchPoints.length - 1 && Math.abs(sketchPoints[rightIdx].y - zeroY) > peakGain * 0.7) rightIdx++;
      
      const widthPx = sketchPoints[rightIdx].x - sketchPoints[leftIdx].x;
      const calculatedQ = Math.max(0.3, Math.min(8, 80 / (widthPx || 20)));

      return {
        id: Math.random().toString(36).substr(2, 9),
        f: xToFreq(p.x, width),
        g: yToGain(p.y, height, displayRange),
        q: calculatedQ,
        type: 'bell',
        channelMode: 'stereo',
        dynamicThreshold: 0.5,
        dynamicAmount: 0,
        slope: 24,
        isActive: true,
        isSolo: false,
        isSpectral: false
      };
    });

    if (newBands.length > 0) {
      onUpdateBands([...bands, ...newBands]);
    }
    
    setIsSketching(false);
    setSketchPoints([]);
  }, [isSketching, sketchPoints, bands, width, height, displayRange, onUpdateBands]);

  useEffect(() => {
    if (isSketching && sketchPoints.length > 30) {
      const lastPoint = sketchPoints[sketchPoints.length - 1];
      const zeroY = height / 2;
      if (Math.abs(lastPoint.y - zeroY) < 5) {
        handleSketchEnd();
      }
    }
  }, [sketchPoints, isSketching, handleSketchEnd, height]);

  useEffect(() => {
    let frameId: number;
    const updateCurve = () => {
      const points = [];
      const deltaPoints = [];
      const aiPoints = [];
      const steps = 200; 
      
      for (let i = 0; i <= steps; i++) {
        const x = (i / steps) * width;
        const freq = xToFreq(x, width);
        
        // Main Curve
        const gain = calculateMagnitude(freq, bands, proportionalQ);
        const y = gainToY(gain, height, displayRange);
        points.push(`${x},${y}`);

        // AI Ghost Curve
        if (ghostBands.length > 0) {
          const aiGain = calculateMagnitude(freq, ghostBands, proportionalQ);
          const aiY = gainToY(aiGain, height, displayRange);
          aiPoints.push(`${x},${aiY}`);
        }

        // Delta Curve (Difference)
        if (isDeltaMode) {
          const processedGain = calculateMagnitude(freq, [...bands, ...ghostBands], proportionalQ);
          const deltaGain = Math.abs(gain - processedGain);
          const deltaY = gainToY(deltaGain, height, displayRange);
          deltaPoints.push(`${x},${deltaY}`);
        }
      }
      
      pathData.set(`M ${points.join(' L ')}`);
      if (aiPoints.length > 0) aiGhostPathData.set(`M ${aiPoints.join(' L ')}`);
      else aiGhostPathData.set("");

      if (deltaPoints.length > 0) deltaPathData.set(`M ${deltaPoints.join(' L ')}`);
      else deltaPathData.set("");

      frameId = requestAnimationFrame(updateCurve);
    };
    
    frameId = requestAnimationFrame(updateCurve);
    return () => cancelAnimationFrame(frameId);
  }, [bands, ghostBands, width, height, displayRange, proportionalQ, isDeltaMode]);

  const handleDoubleClick = (e: React.MouseEvent) => {
    if (!svgRef.current) return;
    const rect = svgRef.current.getBoundingClientRect();
    const x = e.clientX - rect.left;
    const y = e.clientY - rect.top;
    
    const newBand: EQBand = {
      id: Math.random().toString(36).substr(2, 9),
      f: xToFreq(x, width),
      g: yToGain(y, height, displayRange),
      q: 1.0,
      type: 'bell',
      channelMode: 'stereo',
      dynamicThreshold: 0.5,
      dynamicAmount: 0,
      slope: 24,
      isActive: true,
      isSolo: false,
      isSpectral: false
    };
    
    onUpdateBands([...bands, newBand]);
    setSelectedBandId(newBand.id);
  };

  const handleMouseDown = (e: React.MouseEvent, id: string, mode: 'node' | 'dynamic') => {
    e.stopPropagation();
    if (e.altKey) {
      const updatedBands = bands.map(b => ({
        ...b,
        isSolo: b.id === id ? !b.isSolo : false
      }));
      onUpdateBands(updatedBands);
      return;
    }
    setSelectedBandId(id);
    setHudPos({ x: e.clientX, y: e.clientY });
    setIsDragging(true);
    setDragMode(mode);
  };

  const handleMouseMove = (e: React.MouseEvent) => {
    if (!svgRef.current) return;
    const rect = svgRef.current.getBoundingClientRect();
    const x = e.clientX - rect.left;
    const y = e.clientY - rect.top;

    if (isSketching) {
      setSketchPoints(prev => {
        if (prev.length > 0 && x < prev[prev.length - 1].x) {
          return prev.filter(p => p.x <= x);
        }
        return [...prev, { x, y }];
      });
      return;
    }

    if (isDragging) return;
    
    const freq = xToFreq(x, width);
    const curveGain = calculateMagnitude(freq, bands, proportionalQ);
    const curveY = gainToY(curveGain, height, displayRange);
    
    if (Math.abs(y - curveY) < 30) {
      if (!ghostNode) {
        setGhostNode({ x, y: curveY, freq });
        setHoverStartTime(Date.now());
      } else {
        setGhostNode({ x, y: curveY, freq });
        if (hoverStartTime && Date.now() - hoverStartTime > 1000) {
          setIsFrozen(true);
        }
      }

      // Calculate Ghost Path
      const ghostBand: EQBand = {
        id: 'ghost',
        f: freq,
        g: yToGain(y, height, displayRange),
        q: 1.0,
        type: 'bell',
        channelMode: 'stereo',
        dynamicThreshold: 0.5,
        dynamicAmount: 0,
        slope: 24,
        isActive: true,
        isSolo: false,
        isSpectral: false
      };

      const points = [];
      const steps = 100;
      for (let i = 0; i <= steps; i++) {
        const gx = (i / steps) * width;
        const gfreq = xToFreq(gx, width);
        const gmag = calculateBandMagnitude(gfreq, ghostBand, proportionalQ);
        const gy = gainToY(gmag, height, displayRange);
        points.push(`${gx},${gy}`);
      }
      ghostPathData.set(`M ${points.join(' L ')}`);
    } else {
      setGhostNode(null);
      setHoverStartTime(null);
      setIsFrozen(false);
      ghostPathData.set("");
    }
  };

  const handleWheel = (e: React.WheelEvent, id: string) => {
    e.preventDefault();
    const delta = e.deltaY > 0 ? -0.1 : 0.1;
    const updatedBands = bands.map(b => {
      if (b.id !== id) return b;
      return { ...b, q: Math.max(0.1, Math.min(10, b.q + delta)) };
    });
    onUpdateBands(updatedBands);
  };

  const updateBandProperty = (id: string, updates: Partial<EQBand>) => {
    onUpdateBands(bands.map(b => b.id === id ? { ...b, ...updates } : b));
  };

  const deleteBand = (id: string) => {
    onUpdateBands(bands.filter(b => b.id !== id));
    setSelectedBandId(null);
  };

  useEffect(() => {
    const handleGlobalMouseMove = (e: MouseEvent) => {
      if (!isDragging || !selectedBandId || !svgRef.current) return;
      const rect = svgRef.current.getBoundingClientRect();
      const x = Math.max(0, Math.min(width, e.clientX - rect.left));
      const y = Math.max(0, Math.min(height, e.clientY - rect.top));
      
      const updatedBands = bands.map(b => {
        if (b.id !== selectedBandId) return b;
        if (dragMode === 'node') {
          return { ...b, f: xToFreq(x, width), g: yToGain(y, height, displayRange) };
        } else {
          const nodeX = freqToX(b.f, width);
          const nodeY = gainToY(b.g, height, displayRange);
          const dist = Math.sqrt(Math.pow(x - nodeX, 2) + Math.pow(y - nodeY, 2));
          return { ...b, dynamicThreshold: Math.max(0, Math.min(1, dist / 100)) };
        }
      });
      onUpdateBands(updatedBands);
    };

    const handleGlobalMouseUp = () => {
      setIsDragging(false);
      if (isSketching) handleSketchEnd();
    };

    window.addEventListener('mousemove', handleGlobalMouseMove);
    window.addEventListener('mouseup', handleGlobalMouseUp);
    return () => {
      window.removeEventListener('mousemove', handleGlobalMouseMove);
      window.removeEventListener('mouseup', handleGlobalMouseUp);
    };
  }, [isDragging, selectedBandId, bands, dragMode, displayRange, width, height, isSketching, sketchPoints, handleSketchEnd]);

  const selectedBand = bands.find(b => b.id === selectedBandId);

  // Calculate individual band fill paths
  const bandFills = useMemo(() => {
    const soloActive = bands.some(b => b.isSolo);
    return bands.filter(b => b.isActive && (!soloActive || b.isSolo)).map(band => {
      const points = [];
      const steps = 100;
      const zeroY = height / 2;
      
      for (let i = 0; i <= steps; i++) {
        const x = (i / steps) * width;
        const freq = xToFreq(x, width);
        const gain = calculateBandMagnitude(freq, band, proportionalQ);
        const y = gainToY(gain, height, displayRange);
        points.push(`${x},${y}`);
      }
      
      // Close the path along the zero line
      const path = `M 0,${zeroY} L ${points.join(' L ')} L ${width},${zeroY} Z`;
      return { id: band.id, path, color: getBandColor(band.channelMode) };
    });
  }, [bands, width, height, displayRange, proportionalQ]);

  return (
    <div className="relative w-full h-full bg-black/80 rounded-xl border border-white/5 overflow-hidden group">
      {/* Piano Keyboard Display */}
      <div className="absolute bottom-0 w-full h-12 bg-zinc-900/90 border-t border-white/10 flex items-end overflow-hidden">
        {Array.from({ length: 88 }).map((_, i) => {
          const note = (i + 21) % 12;
          const isBlack = [1, 3, 6, 8, 10].includes(note);
          return (
            <div 
              key={i} 
              className={`flex-1 border-r border-black/20 ${isBlack ? 'bg-zinc-800 h-8 z-10' : 'bg-zinc-100 h-12'}`}
              style={{ opacity: 0.1 }}
            />
          );
        })}
      </div>

      <svg 
        ref={svgRef}
        className="absolute inset-0 w-full h-full cursor-crosshair"
        onDoubleClick={handleDoubleClick}
        onMouseMove={handleMouseMove}
        onMouseDown={(e) => {
          if (e.shiftKey) setIsSketching(true);
        }}
      >
        <defs>
          {bands.map(band => (
            <linearGradient key={`grad-${band.id}`} id={`grad-${band.id}`} x1="0%" y1="0%" x2="0%" y2="100%">
              <stop offset="0%" stopColor={getBandColor(band.channelMode)} stopOpacity="0.3" />
              <stop offset="100%" stopColor={getBandColor(band.channelMode)} stopOpacity="0" />
            </linearGradient>
          ))}
        </defs>

        {/* Individual Band Fills with Blending */}
        <g style={{ mixBlendMode: 'screen' }}>
          {bandFills.map(fill => (
            <path 
              key={fill.id}
              d={fill.path}
              fill={`url(#grad-${fill.id})`}
              className="transition-all duration-300"
            />
          ))}
        </g>

        {/* Ghost Curve Preview */}
        {!selectedBandId && ghostNode && (
          <motion.path 
            d={ghostPathData}
            fill="none"
            stroke="#D4AF37"
            strokeWidth="1"
            strokeDasharray="4 2"
            className="opacity-20 pointer-events-none"
          />
        )}

        {/* Sketch Path */}
        {isSketching && sketchPoints.length > 1 && (
          <path 
        
            d={`M ${sketchPoints.map(p => `${p.x},${p.y}`).join(' L ')}`}
            fill="none" stroke="rgba(212,175,55,0.4)" strokeWidth="2" strokeDasharray="4 4"
          />
        )}

        <motion.path 
          d={pathData}
          fill="none"
          stroke={isFrozen ? "#FFFFFF" : "#D4AF37"}
          strokeWidth="3"
          initial={false}
          animate={{ 
            opacity: isBypassed ? 0.2 : 1,
            stroke: isBypassed ? "#666" : (isFrozen ? "#FFFFFF" : "#D4AF37")
          }}
          transition={{ duration: 0.2 }}
          className={`transition-colors duration-500 ${isFrozen ? 'drop-shadow-[0_0_15px_rgba(255,255,255,0.8)]' : 'drop-shadow-[0_0_12px_rgba(212,175,55,0.6)]'}`}
        />

        {/* AI Ghost Path (Dashed) */}
        {ghostBands.length > 0 && (
          <motion.path 
            d={aiGhostPathData}
            fill="none"
            stroke="#D4AF37"
            strokeWidth="2"
            strokeDasharray="6 4"
            className="opacity-40 pointer-events-none"
          />
        )}

        {/* Delta Mode Path (Highlighting Difference) */}
        {isDeltaMode && (
          <motion.path 
            d={deltaPathData}
            fill="none"
            stroke="#EF4444"
            strokeWidth="2"
            className="drop-shadow-[0_0_8px_rgba(239,68,68,0.5)] pointer-events-none"
          />
        )}
        
        {ghostNode && !isDragging && (
          <circle 
            cx={ghostNode.x} cy={ghostNode.y} r={isFrozen ? "10" : "6"}
            fill="none" stroke={isFrozen ? "#FFF" : "#D4AF37"} strokeWidth={isFrozen ? "2" : "1"} 
            className={isFrozen ? "animate-pulse" : ""}
          />
        )}

        {bands.map(band => {
          const x = freqToX(band.f, width);
          const y = gainToY(band.g, height, displayRange);
          const isSelected = selectedBandId === band.id;
          
          return (
            <g key={band.id}>
              {/* Dynamic EQ Ring (Expands/Contracts based on gain reduction) */}
              <motion.circle 
                cx={x} cy={y} 
                initial={{ r: 25 }}
                animate={{ 
                  r: 25 + band.dynamicThreshold * 50 + (Math.sin(Date.now() / 200) * 5),
                  opacity: isSelected ? 0.6 : 0.15
                }}
                fill="none"
                stroke={getBandColor(band.channelMode)}
                strokeWidth="1.5"
                strokeDasharray="3 3"
                className="cursor-pointer"
                onMouseDown={(e) => handleMouseDown(e, band.id, 'dynamic')}
              />
              <circle 
                cx={x} cy={y} r="8"
                fill={isSelected ? (band.isSolo ? "#FF4444" : getBandColor(band.channelMode)) : "#111"}
                stroke={band.isSolo ? "#FF4444" : getBandColor(band.channelMode)}
                strokeWidth="2"
                className="cursor-move"
                onMouseDown={(e) => handleMouseDown(e, band.id, 'node')}
                onWheel={(e) => handleWheel(e, band.id)}
              />
            </g>
          );
        })}
      </svg>

      {/* Contextual Floating Controls (Portal) */}
      <AnimatePresence>
        {selectedBand && !isDragging && (
          <FloatingBandControl 
            band={selectedBand}
            position={hudPos}
            onClose={() => setSelectedBandId(null)}
            onUpdate={updateBandProperty}
            onDelete={deleteBand}
            getNoteFromFreq={getNoteFromFreq}
          />
        )}
      </AnimatePresence>

      {/* Sketching Hint */}
      <div className="absolute top-4 right-4 text-[8px] text-zinc-600 uppercase tracking-widest font-bold pointer-events-none">
        Hold SHIFT to Sketch EQ
      </div>
    </div>
  );
};
