import React, { useState, useEffect, useRef } from 'react';

interface SliderProps {
  label: string;
  value: number;
  onChange: (val: number) => void;
  min?: number;
  max?: number;
  unit?: string;
}

export const LinearSlider = ({ label, value, onChange, min = 0, max = 1, unit = '' }: SliderProps) => {
  const [isDragging, setIsDragging] = useState(false);
  const trackRef = useRef<HTMLDivElement>(null);

  const handleUpdate = (clientX: number) => {
    if (!trackRef.current) return;
    const rect = trackRef.current.getBoundingClientRect();
    const pos = (clientX - rect.left) / rect.width;
    const newVal = Math.min(max, Math.max(min, min + pos * (max - min)));
    onChange(newVal);
  };

  const handleMouseDown = (e: React.MouseEvent) => {
    setIsDragging(true);
    handleUpdate(e.clientX);
  };

  useEffect(() => {
    const handleMouseMove = (e: MouseEvent) => {
      if (isDragging) handleUpdate(e.clientX);
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
  }, [isDragging]);

  const percentage = ((value - min) / (max - min)) * 100;

  return (
    <div className="flex flex-col gap-2 w-full max-w-md select-none">
      <div className="flex justify-between items-center px-1">
        <span className="text-[8px] uppercase tracking-[0.3em] text-white/40 font-black">{label}</span>
        <span className="text-[10px] font-mono text-gold font-bold">{(value * 100).toFixed(0)}{unit}</span>
      </div>
      <div 
        ref={trackRef}
        className="relative h-6 bg-black/60 rounded-lg border border-white/5 cursor-pointer overflow-hidden shadow-inner"
        onMouseDown={handleMouseDown}
      >
        {/* Active Track */}
        <div 
          className="absolute inset-y-0 left-0 bg-gradient-to-r from-gold/20 to-gold/60 border-r border-gold/50"
          style={{ width: `${percentage}%` }}
        />
        
        {/* Handle */}
        <div 
          className="absolute top-0 bottom-0 w-1 bg-white shadow-[0_0_10px_white]"
          style={{ left: `${percentage}%`, transform: 'translateX(-50%)' }}
        />

        {/* Grid lines */}
        <div className="absolute inset-0 flex justify-between px-4 pointer-events-none opacity-10">
          {[...Array(10)].map((_, i) => (
            <div key={i} className="w-[1px] h-full bg-white" />
          ))}
        </div>
      </div>
    </div>
  );
};
