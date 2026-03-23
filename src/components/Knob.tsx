import React, { useState, useEffect, useRef } from 'react';
import { motion } from 'motion/react';

interface KnobProps {
  label: string;
  min?: number;
  max?: number;
  value: number;
  onChange: (val: number) => void;
  unit?: string;
  color?: string;
  texture?: 'none' | 'wood' | 'vibrate';
}

export const Knob = ({ 
  label, 
  min = 0, 
  max = 1, 
  value, 
  onChange, 
  unit = '', 
  color = '#D4AF37',
  texture = 'none'
}: KnobProps) => {
  const [isDragging, setIsDragging] = useState(false);
  const startY = useRef(0);
  const startVal = useRef(0);

  const handleMouseDown = (e: React.MouseEvent) => {
    setIsDragging(true);
    startY.current = e.clientY;
    startVal.current = value;
  };

  useEffect(() => {
    const handleMouseMove = (e: MouseEvent) => {
      if (!isDragging) return;
      const delta = startY.current - e.clientY;
      const range = max - min;
      const step = range / 200;
      const newVal = Math.min(max, Math.max(min, startVal.current + delta * step));
      onChange(newVal);
    };

    const handleMouseUp = () => setIsDragging(false);

    if (isDragging) {
      window.addEventListener('mousemove', handleMouseMove);
      window.addEventListener('mouseup', handleMouseUp);
    }
    return () => {
      window.removeEventListener('mousemove', handleMouseMove);
      window.removeEventListener('mouseup', handleMouseUp);
    };
  }, [isDragging, max, min, onChange]);

  const percentage = ((value - min) / (max - min)) * 100;
  const rotation = (percentage / 100) * 270 - 135;

  return (
    <div className="flex flex-col items-center gap-2 select-none">
      <span className="text-[8px] uppercase tracking-[0.3em] text-white/40 font-black">{label}</span>
      <div 
        className="relative w-20 h-20 cursor-ns-resize group"
        onMouseDown={handleMouseDown}
      >
        {/* Outer Glow */}
        <div 
          className="absolute inset-0 rounded-full blur-md opacity-0 group-hover:opacity-20 transition-opacity"
          style={{ backgroundColor: color }}
        />

        {/* Outer Ring */}
        <svg className="w-full h-full -rotate-90">
          <circle
            cx="40" cy="40" r="36"
            fill="none"
            stroke="rgba(255,255,255,0.03)"
            strokeWidth="4"
          />
          <circle
            cx="40" cy="40" r="36"
            fill="none"
            stroke={color}
            strokeWidth="4"
            strokeDasharray={`${(percentage / 100) * 226} 226`}
            className="transition-all duration-75"
          />
        </svg>

        {/* Knob Body */}
        <motion.div 
          animate={texture === 'vibrate' && isDragging ? { x: [0, -1, 1, 0], y: [0, 1, -1, 0] } : {}}
          transition={{ repeat: Infinity, duration: 0.1 }}
          className={`absolute inset-3 rounded-full shadow-2xl border border-white/5 flex items-center justify-center overflow-hidden ${
            texture === 'wood' ? 'bg-[#2a1a0a]' : 'bg-zinc-800'
          }`}
          style={{ transform: `rotate(${rotation}deg)` }}
        >
          {texture === 'wood' && (
            <div className="absolute inset-0 opacity-20 pointer-events-none bg-[url('https://www.transparenttextures.com/patterns/wood-pattern.png')]" />
          )}
          
          {/* Indicator Line */}
          <div 
            className="w-1 h-5 rounded-full -translate-y-4"
            style={{ backgroundColor: color }}
          />
          
          {/* Center Cap */}
          <div className="absolute inset-6 rounded-full bg-black/20 border border-white/5" />
        </motion.div>
      </div>
      
      <div className="bg-black/60 px-3 py-1 rounded-md border border-white/5 min-w-[60px] text-center shadow-inner">
        <span className="text-[10px] font-mono font-bold" style={{ color }}>
          {unit === '%' ? (value * 100).toFixed(0) : value.toFixed(2)}{unit}
        </span>
      </div>
    </div>
  );
};
