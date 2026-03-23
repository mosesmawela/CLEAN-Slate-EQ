import React, { useMemo, useEffect, useState } from 'react';

interface EQCurveProps {
  subClear: number; // HPF freq
  body: number;     // Low shelf gain
  antiBox: number;  // Dynamic bell sensitivity
  theTweak: number; // High peak gain
  isPowerOn: boolean;
}

export const EQCurveVisualizer = ({ subClear, body, antiBox, theTweak, isPowerOn }: EQCurveProps) => {
  const width = 800;
  const height = 200;
  const centerY = height / 2;

  // Simulated Spectrum Data
  const [spectrum, setSpectrum] = useState<number[]>(new Array(60).fill(0));

  useEffect(() => {
    if (!isPowerOn) {
      setSpectrum(new Array(60).fill(0));
      return;
    }
    const interval = setInterval(() => {
      setSpectrum(prev => prev.map((_, i) => {
        const base = Math.random() * 20;
        // Boost low end if body is high
        if (i < 15) return base + body * 30 + Math.random() * 10;
        // Boost high end if tweak is high
        if (i > 40) return base + theTweak * 15 + Math.random() * 10;
        // Dip mid if antiBox is high
        if (i > 20 && i < 30) return Math.max(0, base - antiBox * 20);
        return base + Math.random() * 5;
      }));
    }, 80);
    return () => clearInterval(interval);
  }, [isPowerOn, body, theTweak, antiBox]);

  const points = useMemo(() => {
    const p = [];
    const numPoints = 100;
    
    for (let i = 0; i <= numPoints; i++) {
      const x = (i / numPoints) * width;
      const freq = Math.pow(10, (i / numPoints) * 3 + 1.3); // 20Hz to 20kHz approx
      
      let gain = 0;
      
      // HPF (Sub-Clear) - normalized 0-1 to freq
      const hpfFreq = 20 + subClear * 180;
      if (freq < hpfFreq) {
        gain -= Math.min(60, (hpfFreq - freq) * 0.5);
      }
      
      // Low Shelf (Body) - 150Hz
      if (freq < 150) {
        gain += body * 12;
      } else if (freq < 300) {
        gain += (body * 12) * (1 - (freq - 150) / 150);
      }
      
      // Dynamic Bell (Anti-Box) - 350Hz
      const dist350 = Math.abs(Math.log10(freq) - Math.log10(350));
      if (dist350 < 0.2) {
        gain -= (1 - dist350 / 0.2) * antiBox * 12;
      }
      
      // High Peak (The Tweak) - 1.5kHz
      const dist1500 = Math.abs(Math.log10(freq) - Math.log10(1500));
      if (dist1500 < 0.3) {
        gain += (1 - dist1500 / 0.3) * theTweak * 12;
      }

      const y = centerY - (gain * 4);
      p.push(`${x},${y}`);
    }
    return p.join(' ');
  }, [subClear, body, antiBox, theTweak]);

  return (
    <div className="w-full h-48 bg-black/60 rounded-xl border border-white/5 overflow-hidden relative">
      {/* Spectrum Analyzer Bars */}
      <div className="absolute inset-0 flex items-end justify-between px-2 opacity-30 pointer-events-none">
        {spectrum.map((val, i) => (
          <div 
            key={i}
            className="w-1 bg-gold/50 rounded-t-sm transition-all duration-75"
            style={{ height: `${val}%` }}
          />
        ))}
      </div>

      <div className="absolute inset-0 flex justify-between px-4 items-end pb-2 opacity-20 pointer-events-none">
        <span className="text-[10px] font-mono">20Hz</span>
        <span className="text-[10px] font-mono">100Hz</span>
        <span className="text-[10px] font-mono">1kHz</span>
        <span className="text-[10px] font-mono">10kHz</span>
      </div>
      
      <svg width="100%" height="100%" viewBox={`0 0 ${width} ${height}`} preserveAspectRatio="none" className="relative z-10">
        <line x1="0" y1={centerY} x2={width} y2={centerY} stroke="rgba(212, 175, 55, 0.1)" strokeWidth="1" />
        
        <polyline
          points={points}
          fill="none"
          stroke="#D4AF37"
          strokeWidth="3"
          className="drop-shadow-[0_0_8px_rgba(212,175,55,0.5)]"
        />
        
        <path
          d={`M 0 ${height} L ${points} L ${width} ${height} Z`}
          fill="url(#curveGradient)"
          className="opacity-10"
        />
        
        <defs>
          <linearGradient id="curveGradient" x1="0" y1="0" x2="0" y2="1">
            <stop offset="0%" stopColor="#D4AF37" />
            <stop offset="100%" stopColor="transparent" />
          </linearGradient>
        </defs>
      </svg>
    </div>
  );
};
