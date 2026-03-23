import React from 'react';
import { createPortal } from 'react-dom';
import { motion } from 'framer-motion';
import { Power, Trash2, X } from 'lucide-react';
import { EQBand, getBandColor } from '../utils/EQGraphLogic';

interface FloatingHUDProps {
  band: EQBand;
  position: { x: number; y: number };
  onClose: () => void;
  onUpdate: (id: string, updates: Partial<EQBand>) => void;
  onDelete: (id: string) => void;
  getNoteFromFreq: (f: number) => string;
}

export const FloatingBandControl = ({ 
  band, 
  position, 
  onClose, 
  onUpdate, 
  onDelete,
  getNoteFromFreq 
}: FloatingHUDProps) => {
  return createPortal(
    <motion.div
      initial={{ opacity: 0, y: 10, scale: 0.95 }}
      animate={{ opacity: 1, y: 0, scale: 1 }}
      exit={{ opacity: 0, y: 10, scale: 0.95 }}
      style={{
        position: 'fixed',
        top: position.y + 20, 
        left: position.x - 110, 
        zIndex: 1000,
      }}
      className="w-[220px] bg-zinc-900/95 border border-zinc-800 backdrop-blur-xl rounded-xl p-4 shadow-2xl flex flex-col gap-4"
    >
      {/* Header Info */}
      <div className="flex justify-between items-center border-b border-zinc-800 pb-2">
        <div className="flex flex-col">
          <span className="text-[10px] font-black uppercase tracking-widest" style={{ color: getBandColor(band.channelMode) }}>
            {getNoteFromFreq(band.f)}
          </span>
          <span className="text-[8px] text-zinc-500 uppercase font-bold">Band {band.id.slice(0, 4)}</span>
        </div>
        <div className="flex gap-2">
          <button 
            onClick={() => onUpdate(band.id, { isActive: !band.isActive })}
            className={`p-1 rounded transition-colors ${band.isActive ? 'text-gold' : 'text-zinc-600'}`}
          >
            <Power size={12} />
          </button>
          <button 
            onClick={() => onDelete(band.id)}
            className="p-1 text-zinc-600 hover:text-red-500 transition-colors"
          >
            <Trash2 size={12} />
          </button>
          <button onClick={onClose} className="p-1 text-zinc-500 hover:text-white transition-colors">
            <X size={12} />
          </button>
        </div>
      </div>

      {/* Main Control Group */}
      <div className="grid grid-cols-2 gap-4">
        <div className="flex flex-col items-center gap-1">
          <div className="w-12 h-12 rounded-full border-2 border-zinc-700 flex items-center justify-center bg-zinc-800/50">
            <span className="text-[10px] font-mono text-white">{Math.round(band.f)}Hz</span>
          </div>
          <span className="text-[7px] text-zinc-500 uppercase font-bold">Freq</span>
        </div>
        
        {/* Dynamic Range Ring around Gain */}
        <div className="flex flex-col items-center gap-1 relative">
           <svg className="absolute inset-0 w-12 h-12 -rotate-90 pointer-events-none">
             <circle cx="24" cy="24" r="20" fill="none" stroke="rgba(255,255,255,0.05)" strokeWidth="2" />
             <circle 
                cx="24" cy="24" r="20" 
                fill="none" 
                stroke={getBandColor(band.channelMode)} 
                strokeWidth="2" 
                strokeDasharray="125.6" 
                strokeDashoffset={125.6 - (band.dynamicThreshold * 125.6)} 
                className="transition-all duration-300"
             />
           </svg>
          <div className="w-12 h-12 rounded-full flex items-center justify-center bg-zinc-800/50 border border-zinc-700">
            <span className="text-[10px] font-mono text-white">{band.g.toFixed(1)}dB</span>
          </div>
          <span className="text-[7px] text-zinc-500 uppercase font-bold">Gain</span>
        </div>
      </div>

      {/* Parameters Grid */}
      <div className="grid grid-cols-2 gap-2">
        <div className="flex flex-col gap-1">
          <span className="text-[7px] text-zinc-500 uppercase font-bold">Shape</span>
          <select 
            value={band.type}
            onChange={(e) => onUpdate(band.id, { type: e.target.value as any })}
            className="bg-zinc-800 text-[9px] text-white p-1 rounded border border-zinc-700 outline-none cursor-pointer hover:border-zinc-600 transition-colors"
          >
            <option value="bell">Bell</option>
            <option value="lowshelf">Low Shelf</option>
            <option value="highshelf">High Shelf</option>
            <option value="notch">Notch</option>
            <option value="brickwall">Brickwall</option>
          </select>
        </div>
        <div className="flex flex-col gap-1">
          <span className="text-[7px] text-zinc-500 uppercase font-bold">Placement</span>
          <select 
            value={band.channelMode}
            onChange={(e) => onUpdate(band.id, { channelMode: e.target.value as any })}
            className="bg-zinc-800 text-[9px] text-white p-1 rounded border border-zinc-700 outline-none cursor-pointer hover:border-zinc-600 transition-colors"
          >
            <option value="stereo">Stereo</option>
            <option value="mid">Mid (M)</option>
            <option value="side">Side (S)</option>
          </select>
        </div>
      </div>

      <div className="grid grid-cols-2 gap-2">
        <div className="flex flex-col gap-1">
          <span className="text-[7px] text-zinc-500 uppercase font-bold">Slope</span>
          <select 
            value={band.slope}
            onChange={(e) => onUpdate(band.id, { slope: parseInt(e.target.value) })}
            className="bg-zinc-800 text-[9px] text-white p-1 rounded border border-zinc-700 outline-none cursor-pointer hover:border-zinc-600 transition-colors"
          >
            <option value="6">6 dB/oct</option>
            <option value="12">12 dB/oct</option>
            <option value="24">24 dB/oct</option>
            <option value="48">48 dB/oct</option>
            <option value="96">96 dB/oct</option>
          </select>
        </div>
        <div className="flex flex-col gap-1">
          <span className="text-[7px] text-zinc-500 uppercase font-bold">Dynamics</span>
          <button 
            onClick={() => onUpdate(band.id, { isSpectral: !band.isSpectral })}
            className={`text-[9px] p-1 rounded border transition-all ${band.isSpectral ? 'bg-gold/20 border-gold/50 text-gold' : 'bg-zinc-800 border-zinc-700 text-zinc-500'}`}
          >
            {band.isSpectral ? 'Spectral' : 'Standard'}
          </button>
        </div>
      </div>

      {/* Sliders */}
      <div className="flex flex-col gap-3 pt-2 border-t border-zinc-800">
        <div className="flex flex-col gap-1">
          <div className="flex justify-between text-[7px] text-zinc-500 uppercase font-bold">
            <span>Gain</span>
            <span className="text-gold">{band.g.toFixed(1)} dB</span>
          </div>
          <input 
            type="range" min="-18" max="18" step="0.1"
            value={band.g}
            onChange={(e) => onUpdate(band.id, { g: parseFloat(e.target.value) })}
            className="w-full accent-gold h-1 bg-zinc-800 rounded-full appearance-none cursor-pointer"
          />
        </div>

        <div className="flex flex-col gap-1">
          <div className="flex justify-between text-[7px] text-zinc-500 uppercase font-bold">
            <span>Dynamic Range</span>
            <span>{Math.round(band.dynamicThreshold * 100)}%</span>
          </div>
          <input 
            type="range" min="0" max="1" step="0.01"
            value={band.dynamicThreshold}
            onChange={(e) => onUpdate(band.id, { dynamicThreshold: parseFloat(e.target.value) })}
            className="w-full accent-gold h-1 bg-zinc-800 rounded-full appearance-none cursor-pointer"
          />
        </div>
      </div>
    </motion.div>,
    document.body
  );
};
