# SPECDRIFT

**A spectral blur effect that slows how frequency bins reach their target magnitude — smearing transients into hazy, diffused textures.**

---

## About

Specdrift works in the frequency domain: audio is split into bins, and each bin's magnitude is smoothed toward its target level instead of jumping instantly. That slowdown is what creates the blur — transients smear and the sound takes on a washed-out, spectral character.

**Blur** controls how slowly magnitudes follow the input. **Tilt** biases that smoothing toward lower or higher frequencies. **Random** gives each bin its own blur amount so neighboring frequencies move at different rates, making the texture feel alive rather than uniform. **Gate** is a spectral noise gate that quiets bins below a threshold. **Mix** blends the processed signal with the dry input (delay-compensated so they stay in phase). **Low Cut** and **High Cut** trim the wet path and output so blur doesn't muddy your subs or harsh highs. **Input** and **Output** gain round out level control at the ends of the chain. An expandable spectral view shows a live spectrogram for monitoring what the effect is doing to the spectrum.

Planned next: a feedback loop, saturation, delay, and more spectral processing — attack/release shaping, grain, stereo width, and a fully working drawable blur curve over frequency.

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
