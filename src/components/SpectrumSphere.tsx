import React, { useMemo, useRef } from 'react';
import { Canvas, useFrame } from '@react-three/fiber';
import { Sphere } from '@react-three/drei';
import * as THREE from 'three';

const vertexShader = `
uniform float uLowEnergy;
uniform float uHighEnergy;
uniform float uTime;

varying vec3 vNormal;
varying vec3 vPosition;

void main() {
    vNormal = normal;
    vec3 pos = position;
    
    // Bottom displacement for BASS (Gold)
    if (pos.y < 0.0) {
        pos += normal * (uLowEnergy * sin(uTime * 5.0) * 0.2);
    }
    
    // Top spikes for THE TWEAK (Blue)
    if (pos.y > 0.0) {
        float spikes = pow(sin(pos.x * 10.0) * cos(pos.z * 10.0), 2.0);
        pos += normal * (spikes * uHighEnergy * 0.4);
    }
    
    vPosition = pos;
    gl_Position = projectionMatrix * modelViewMatrix * vec4(pos, 1.0);
}
`;

const fragmentShader = `
uniform float uTime;
uniform float uHeat;
varying vec3 vNormal;
varying vec3 vPosition;

void main() {
    vec3 color = vec3(0.1);
    
    // Gold/Yellow for low energy
    vec3 lowColor = vec3(0.83, 0.69, 0.22); // Gold
    // Blue for high energy
    vec3 highColor = vec3(0.0, 0.75, 1.0); // Sky Blue
    
    float mixFactor = smoothstep(-0.5, 0.5, vPosition.y);
    color = mix(lowColor, highColor, mixFactor);
    
    // Add heat glow
    color += vec3(1.0, 0.3, 0.0) * uHeat * 0.5;
    
    // Basic lighting
    float diff = max(dot(vNormal, vec3(1.0, 1.0, 1.0)), 0.0);
    color *= (diff + 0.2);
    
    gl_FragColor = vec4(color, 1.0);
}
`;

const SphereVisualizer = ({ low, high, heat }: { low: number, high: number, heat: number }) => {
  const materialRef = useRef<THREE.ShaderMaterial>(null);
  
  const uniforms = useMemo(() => ({
    uLowEnergy: { value: low },
    uHighEnergy: { value: high },
    uHeat: { value: heat },
    uTime: { value: 0 }
  }), []);

  useFrame((state) => {
    if (materialRef.current) {
      materialRef.current.uniforms.uTime.value = state.clock.getElapsedTime();
      materialRef.current.uniforms.uLowEnergy.value = low;
      materialRef.current.uniforms.uHighEnergy.value = high;
      materialRef.current.uniforms.uHeat.value = heat;
    }
  });

  return (
    <Sphere args={[1.2, 128, 128]}>
      <shaderMaterial
        ref={materialRef}
        vertexShader={vertexShader}
        fragmentShader={fragmentShader}
        uniforms={uniforms}
        wireframe={false}
      />
    </Sphere>
  );
};

export const SpectrumSphere = ({ low, high, heat }: { low: number, high: number, heat: number }) => {
  return (
    <div className="w-full h-full absolute inset-0 pointer-events-none">
      <Canvas camera={{ position: [0, 0, 3.5] }}>
        <ambientLight intensity={0.4} />
        <pointLight position={[10, 10, 10]} intensity={1} />
        <SphereVisualizer low={low} high={high} heat={heat} />
      </Canvas>
    </div>
  );
};
