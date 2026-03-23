# CLEAN Slate EQ (Build Experiment v1.0)

![CLEAN Slate EQ](https://img.shields.io/badge/Status-Experimental-yellow) ![JUCE](https://img.shields.io/badge/Powered%20By-JUCE-blue) ![C++](https://img.shields.io/badge/Language-C%2B%2B-orange)

**A High-End EQ Build Experiment.**  
CLEAN Slate EQ is a high-performance audio plugin project focused on building a clean, transparent 8-band surgical EQ engine with real-time OpenGL visualization. This is an ongoing experiment in bridgeable DSP and high-performance UI rendering.

---

## 🚀 Key Features

### 1. **Core 8-Band DSP Engine**
- **Surgical Precision**: 8 independent bands with switchable types (Highpass, Peak, Lowpass, Notch, Low-Shelf, High-Shelf).
- **Pure Transparency**: Designed with Natural Phase algorithms for zero coloration across the spectrum.
- **Auto-Gain (Alpha)**: An experimental compensation logic to maintain perceived loudness as you cut or boost.

### 2. **OpenGL Spectrum Visualizer**
- **Zero-Latency Rendering**: Powered by raw GLSL shaders for ultra-smooth visualization with minimal CPU impact.
- **Collision Glow**: Experimental visual feedback that illuminates overlapping frequency zones for better conflict detection.
- **Dynamic Response**: Real-Time FFT analysis for high-fidelity audio monitoring.

### 3. **Modern UI Design**
- **Premium Aesthetics**: A "Bento Box" inspired dark luxury design using custom LookAndFeel classes.
- **Interactive Controls**: Smooth rotary sliders and intuitive band management.

---

## ⚡ Coming Soon (Roadmap)

The project will be updated over time with advanced "Surgical AI" features:
- **Res-Kill (Auto-Notch)**: Intelligent FFT-based harshness detection.
- **Smart Learn**: Automated frequency balancing suggestions.
- **Multi-Channel Modes**: M/S and L/R processing.

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
4. **Build**:
   - Go to **Build** -> **Build All** (or press `Ctrl+Shift+B`).

---

## ⚙️ Requirements
- **OS**: Windows (x64)
- **Host**: VST3 compatible DAW
- **C++ Standard**: C++17 or higher

---

*Developed by **Anti-Gravity / Moses Mawela***  
*This project is an ongoing build experiment.*
