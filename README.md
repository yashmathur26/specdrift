# SPECDRIFT

**A spectral blur plugin. Instead of each frequency bin jumping straight to the volume it needs, it gets there slowly. That's the blur.**

---

## About

When you look at audio spectrally, every point in the field is a bin. When that bin needs volume, it rises to a certain magnitude. Specdrift blurs that process. Instead of going straight to the magnitude needed, it slows down, so the sound smears and feels more blurred.

**Blur** is how slow that process is. **Tilt** lets you blur more on the lows or the highs, gradually across the spectrum. **Random** is important because it randomizes how much each bin gets blurred, so every frequency reacts a little differently. **Gate** is a spectral gate built into the plugin. **Mix** blends the blurred audio with the normal dry signal. **Low Cut** and **High Cut** keep the blurring from messing up your subs and highs. **Input** and **Output** are just gain. There's also an expandable spectral view with a live spectrogram so you can see what's happening.

Still want to add a feedback loop, saturation, delay, and more spectral processing stuff like attack/release, grain, stereo width, and a drawable curve that actually works over frequency.

**Status:** Beta. macOS (VST3, AU, Standalone). Windows not tested yet.

---

## Demo

- **Video / audio:** [Add link]
- **Screenshots:** [Add link]

---

## Download (beta)

**[Download Specdrift v1.0.0-beta for macOS](https://github.com/yashmathur26/specdrift/releases/download/v1.0.0-beta/Specdrift-v1.0.0-beta-macOS.zip)** (VST3, AU, Standalone in one zip)

| Platform | Format | Notes |
|----------|--------|-------|
| macOS | VST3 / AU / Standalone | [Latest release](https://github.com/yashmathur26/specdrift/releases/latest) or build from source below |

**Install (macOS):**

1. Download and unzip `Specdrift-v1.0.0-beta-macOS.zip`
2. Copy `Specdrift.vst3` to `~/Library/Audio/Plug-Ins/VST3/`
3. Copy `Specdrift.component` to `~/Library/Audio/Plug-Ins/Components/` (AU)
4. Or run `Specdrift.app` as the standalone version
5. Rescan plugins in your DAW
6. If macOS blocks the plugin: System Settings → Privacy & Security → Allow, or right-click → Open

Unsigned beta build. macOS may warn on first open.

**Tested in:** [Add DAWs you used, e.g. Logic, Reaper]

---

## Build from source

### Requirements

- CMake 3.22+
- C++17 compiler (Xcode on macOS)
- [JUCE 7+](https://github.com/juce-framework/JUCE) — clone separately; not included in this repo

### JUCE location

Clone JUCE as a sibling folder:

```
parent/
├── SPECDRIFT/    ← this repo
└── JUCE/         ← git clone https://github.com/juce-framework/JUCE.git
```

Or pass the path explicitly:

```bash
cmake -B build -S . -DJUCE_PATH=/path/to/JUCE
```

### macOS

```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Output:

```
build/Specdrift_artefacts/Release/VST3/Specdrift.vst3
build/Specdrift_artefacts/Release/AU/Specdrift.component
build/Specdrift_artefacts/Release/Standalone/Specdrift.app
```

---

## Project structure

```
Source/
├── Core/     PluginProcessor, PluginEditor, Parameters (APVTS)
├── DSP/      SpectralEngine, CurveState, SpectrogramFIFO
└── UI/       Orb knobs, spectrogram, footer sliders
```

---

## Tech stack

- JUCE 7+
- CMake
- Formats: VST3, AU, Standalone

---

## Known issues

- FL Studio (macOS): possible crash on insert; mitigated in editor fonts — try AU or Standalone if VST3 fails. See `BUG_REPORT.md`.

---

## License

**[Choose: MIT / GPL-3.0 / All rights reserved]**

---

## Contact

**[Your portfolio, LinkedIn, or email]**
