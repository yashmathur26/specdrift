# SPECDRIFT

**[Add your one-line description here.]**

---

## About

**[Add 2–4 sentences in your own words: why you built this, what you were going for, what stage it's at.]**

**Status:** Beta — macOS (VST3, AU, Standalone). Windows build not tested yet.

---

## Demo

- **Video / audio:** [Add link]
- **Screenshots:** [Add link]

---

## Download (beta)

| Platform | Format | Notes |
|----------|--------|-------|
| macOS | VST3 / AU / Standalone | Build from source below, or [add GitHub Release link if you upload builds] |

**Install (macOS):**

1. Copy `Specdrift.vst3` to `~/Library/Audio/Plug-Ins/VST3/`
2. Copy `Specdrift.component` to `~/Library/Audio/Plug-Ins/Components/` (AU)
3. Rescan plugins in your DAW
4. If macOS blocks the plugin: System Settings → Privacy & Security → Allow, or right-click → Open (Standalone `.app`)

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
