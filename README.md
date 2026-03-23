# CLEAN Slate EQ (Surgical AI v1.0)

![CLEAN Slate EQ](https://img.shields.io/badge/Status-Stable-brightgreen) ![JUCE](https://img.shields.io/badge/Powered%20By-JUCE-blue) ![C++](https://img.shields.io/badge/Language-C%2B%2B-orange)

**The Future of Surgical Mixing.**  
CLEAN Slate EQ is a high-performance audio plugin designed for precision surgical processing. It combines an 8-band parametric engine with a real-time AI core to detect and eliminate harsh resonances instantly.

---

## 🚀 Key Features

### 1. **Surgical AI Engine**
- **Res-Kill**: An intelligent FFT-based scanner that identifies high-Q resonances in real-time. Toggle "Res-Kill" to see the AI automatically apply narrow notches to harsh frequencies.
- **Smart Learn**: Analyzes incoming audio and suggests the best starting point for a clean, transparent mix.
- **Auto-Gain**: Maintains perceived loudness automatically using a custom precision formula (`0.5 - (boosts * 0.05) + (cuts * 0.02)`).

### 2. **8-Band Parametric DSP**
- **High-Precision Filters**: Switch between Peak, Notch, Highpass, Lowpass, Low-Shelf, and High-Shelf.
- **Pure Transparency**: Designed with Natural Phase algorithms for zero coloration across the spectrum.

### 3. **OpenGL Spectrum Analyzer**
- **High-Speed Rendering**: Powered by raw GLSL shaders for ultra-smooth visualization with zero CPU overhead.
- **Collision Glow**: A custom visual effect that illuminates overlapping frequency zones for better conflict detection.

### 4. **Modern UI/UX**
- **Luxury Look & Feel**: A "Bento Box" inspired design with a dark, premium aesthetic.
- **Quick Recall**: Includes Factory and Utility presets to jumpstart your surgical workflow.

---

## 🛠 Build Instructions (Visual Studio 2022)

This project uses **CMake** and the **JUCE** framework.

1. **Clone the Repo**:
   ```bash
   git clone https://github.com/MosesMawela/CLEAN-Slate-EQ.git
   ```
2. **Open in Visual Studio**: Open the root folder in VS 2022.
3. **Configure CMake**:
   - Right-click `CMakeLists.txt` -> **Delete Cache and Reconfigure**.
   - Wait for "CMake generation finished."
4. **Build**:
   - Go to **Build** -> **Build All** (or press `Ctrl+Shift+B`).
5. **Find your Binaries**:
   - The `.vst3` and `.exe` will be located in:
     `out/build/x64-Debug/CLEAN_Slate_EQ_artefacts/Debug/`

---

## ⚙️ Requirements
- **OS**: Windows (x64)
- **Host**: VST3 compatible DAW (FL Studio, Ableton, Logic, Pro Tools, etc.)
- **C++ Standard**: C++17 or higher

---

*Developed by **Anti-Gravity / Moses Mawela***  
*Copyright © 2026. All rights reserved.*
