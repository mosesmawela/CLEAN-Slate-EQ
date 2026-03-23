# CLEAN Slate EQ (Pro Surgical Build Experiment v2.0)

![CLEAN Slate EQ](https://img.shields.io/badge/Status-Pro--Experimental-gold) ![JUCE](https://img.shields.io/badge/Powered%20By-JUCE-blue) ![C++](https://img.shields.io/badge/Language-C%2B%2B-orange)

**The Ultimate Precision EQ Build Experiment.**  
CLEAN Slate EQ v2.0 is a professional-grade, surgical audio plugin that transforms the original slider-based UI into a fully interactive, mouse-driven EQ Graph. Featuring ultra-steep 96dB/octave slopes, natural phase processing, and a contextual floating HUD, this is a masterclass in modern JUCE UI/UX and advanced DSP.

---

## 🚀 Pro Features

### 1. **Interactive EQ Graph (Pro-Q Logic)**
- **Direct Manipulation**: Double-click to create bands, drag to adjust frequency and gain, and use the mouse wheel for Q factor control.
- **Contextual HUD**: A floating control center that follows your selection, providing instant access to filter shapes, slopes, and dynamic settings.
- **Sketching Mode**: Draw your desired frequency curve directly on the graph, and our intelligent algorithm will automatically fit EQ bands to your sketch.

### 2. **Surgical DSP & Dynamic EQ**
- **Ultra-Steep Slopes**: Cascaded IIR stages allowing for surgical 48dB, 72dB, and 96dB/octave cuts.
- **Dynamic Engine**: Each band features a dedicated compressor/expander for frequency-dependent dynamics.
- **Phase Modes**: Choose between Zero Latency, Analog-matched Natural Phase, and Linear Phase for maximum transparency.

### 3. **Workflow & Visualization**
- **Sleek Resizing**: Dynamic UI scaling (100% to 150%) and window resizing to fit any screen resolution.
- **OpenGL Spectrum**: Lab-grade real-time FFT visualization with sleek collision glow.
- **Curve Sync**: Copy and Paste EQ curves between plugin instances using a secure XML clipboard system.

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
