import React, { useEffect, useRef, useState } from 'react';
import { EQBand } from '../utils/EQGraphLogic';

interface WaveformDisplayProps {
  files: File[];
  bands: EQBand[];
  isBypassed?: boolean;
}

export const WaveformDisplay = ({ files, bands, isBypassed = false }: WaveformDisplayProps) => {
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const [dryData, setDryData] = useState<Float32Array | null>(null);
  const [wetData, setWetData] = useState<Float32Array | null>(null);

  useEffect(() => {
    const processAudio = async () => {
      if (files.length === 0) {
        setDryData(null);
        setWetData(null);
        return;
      }

      try {
        const file = files[0];
        const arrayBuffer = await file.arrayBuffer();
        const audioCtx = new (window.AudioContext || (window as any).webkitAudioContext)();
        const audioBuffer = await audioCtx.decodeAudioData(arrayBuffer);
        
        // Get dry data (downsampled for visualization)
        const channelData = audioBuffer.getChannelData(0);
        setDryData(channelData);

        // Render wet data using OfflineAudioContext
        const offlineCtx = new OfflineAudioContext(
          audioBuffer.numberOfChannels,
          audioBuffer.length,
          audioBuffer.sampleRate
        );

        const source = offlineCtx.createBufferSource();
        source.buffer = audioBuffer;

        let lastNode: AudioNode = source;
        
        if (!isBypassed) {
          bands.filter(b => b.isActive).forEach(band => {
            const filter = offlineCtx.createBiquadFilter();
            let type: BiquadFilterType = 'peaking';
            if (band.type === 'highpass') type = 'highpass';
            else if (band.type === 'lowpass') type = 'lowpass';
            else if (band.type === 'highshelf') type = 'highshelf';
            else if (band.type === 'lowshelf') type = 'lowshelf';
            else if (band.type === 'notch') type = 'notch';
            
            filter.type = type;
            filter.frequency.value = band.f;
            filter.Q.value = band.q;
            filter.gain.value = band.g;
            
            lastNode.connect(filter);
            lastNode = filter;
          });
        }

        lastNode.connect(offlineCtx.destination);
        source.start();
        
        const renderedBuffer = await offlineCtx.startRendering();
        setWetData(renderedBuffer.getChannelData(0));
        
        audioCtx.close();
      } catch (error) {
        console.error("Waveform processing failed:", error);
      }
    };

    processAudio();
  }, [files, bands, isBypassed]);

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    if (!ctx) return;

    const draw = () => {
      ctx.clearRect(0, 0, canvas.width, canvas.height);
      
      const drawWave = (data: Float32Array | null, yOffset: number, height: number, color: string) => {
        if (!data) return;
        
        ctx.beginPath();
        ctx.strokeStyle = color;
        ctx.lineWidth = 1;
        
        const step = Math.ceil(data.length / canvas.width);
        const amp = height / 2;
        
        for (let i = 0; i < canvas.width; i++) {
          let min = 1.0;
          let max = -1.0;
          for (let j = 0; j < step; j++) {
            const datum = data[i * step + j];
            if (datum < min) min = datum;
            if (datum > max) max = datum;
          }
          ctx.moveTo(i, yOffset + min * amp);
          ctx.lineTo(i, yOffset + max * amp);
        }
        ctx.stroke();
      };

      if (dryData) {
        // Dry Waveform (Top)
        drawWave(dryData, canvas.height * 0.25, 40, '#D4AF37');
        
        // Wet Waveform (Bottom)
        const wetColor = isBypassed ? '#666' : '#FFFFFF';
        drawWave(wetData || dryData, canvas.height * 0.75, 40, wetColor);

        // Labels
        ctx.font = '10px Inter';
        ctx.fillStyle = '#666';
        ctx.fillText('DRY', 10, canvas.height * 0.25 - 25);
        ctx.fillText('WET (AI PROPOSAL)', 10, canvas.height * 0.75 - 25);
      } else {
        ctx.font = '12px Inter';
        ctx.fillStyle = '#444';
        ctx.textAlign = 'center';
        ctx.fillText('Drop audio files to visualize waveform', canvas.width / 2, canvas.height / 2);
      }
    };

    draw();
  }, [dryData, wetData, isBypassed]);

  return (
    <div className="w-full h-32 bg-black/40 rounded-lg border border-white/5 overflow-hidden">
      <canvas 
        ref={canvasRef} 
        width={1300} 
        height={128} 
        className="w-full h-full"
      />
    </div>
  );
};
