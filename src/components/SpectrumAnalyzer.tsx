import React, { useMemo, useRef } from 'react';
import { Canvas, useFrame } from '@react-three/fiber';
import * as THREE from 'three';

const fragmentShader = `
uniform float uTime;
uniform float uLowEnergy;
uniform float uHighEnergy;
uniform bool uCollisionMode;
uniform bool uDeltaMode;
varying vec2 vUv;

void main() {
    vec2 uv = vUv;
    
    // Create a scrolling FFT-like visualization
    float fft = 0.0;
    fft += sin(uv.x * 10.0 - uTime * 2.0) * uLowEnergy * 0.5;
    fft += sin(uv.x * 50.0 + uTime * 5.0) * uHighEnergy * 0.2;
    
    float h = (sin(uv.x * 3.14159) * 0.5 + 0.5) * (uLowEnergy + uHighEnergy + 0.2);
    float mask = step(uv.y, h + fft * 0.1);
    
    vec3 color = mix(vec3(0.05), vec3(0.83, 0.69, 0.22), mask * 0.3);
    
    // Delta Mode (Red Highlight)
    if (uDeltaMode) {
        float delta = step(uv.y, (h + fft * 0.1) * 0.5) * (0.5 + 0.5 * sin(uTime * 15.0));
        color = mix(color, vec3(1.0, 0.0, 0.0), delta * 0.4);
    }
    
    // Collision Detection Glow (Red)
    if (uCollisionMode && !uDeltaMode) {
        float collision = step(uv.y, (h + fft * 0.1) * 0.8) * sin(uTime * 10.0) * 0.2;
        color = mix(color, vec3(1.0, 0.1, 0.1), collision);
    }
    
    gl_FragColor = vec4(color, 1.0);
}
`;

const vertexShader = `
varying vec2 vUv;
void main() {
    vUv = uv;
    gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0);
}
`;

const AnalyzerPlane = ({ low, high, collision, isDeltaMode }: { low: number, high: number, collision: boolean, isDeltaMode: boolean }) => {
  const materialRef = useRef<THREE.ShaderMaterial>(null);
  
  useFrame((state) => {
    if (materialRef.current) {
      materialRef.current.uniforms.uTime.value = state.clock.getElapsedTime();
      materialRef.current.uniforms.uLowEnergy.value = low;
      materialRef.current.uniforms.uHighEnergy.value = high;
      materialRef.current.uniforms.uCollisionMode.value = collision;
      materialRef.current.uniforms.uDeltaMode.value = isDeltaMode;
    }
  });

  const uniforms = useMemo(() => ({
    uTime: { value: 0 },
    uLowEnergy: { value: low },
    uHighEnergy: { value: high },
    uCollisionMode: { value: collision },
    uDeltaMode: { value: isDeltaMode }
  }), []);

  return (
    <mesh position={[0, 0, 0]}>
      <planeGeometry args={[10, 4]} />
      <shaderMaterial
        ref={materialRef}
        vertexShader={vertexShader}
        fragmentShader={fragmentShader}
        uniforms={uniforms}
        transparent
      />
    </mesh>
  );
};

export const SpectrumAnalyzer = ({ low, high, collision = false, isDeltaMode = false }: { low: number, high: number, collision?: boolean, isDeltaMode?: boolean }) => {
  return (
    <div className="absolute inset-0 w-full h-full pointer-events-none opacity-40">
      <Canvas camera={{ position: [0, 0, 2] }}>
        <AnalyzerPlane low={low} high={high} collision={collision} isDeltaMode={isDeltaMode} />
      </Canvas>
    </div>
  );
};
