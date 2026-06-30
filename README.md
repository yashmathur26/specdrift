# SPECDRIFT

<!-- Fill in: one line that names what this is (your words, not marketing fluff) -->
**[One-line description — e.g. "A spectral blur audio plugin I built with JUCE."]**

---

## About

<!-- 2–4 sentences: why you made it, what problem or sound you were going for, what makes it yours -->
[Your paragraph here.]

<!-- Optional: 1–2 sentences on where you are in development -->
**Status:** [e.g. Beta — macOS only for now / v0.1 / etc.]

---

## Demo

<!-- Portfolio links recruiters actually click -->
- **Video / audio:** [YouTube, SoundCloud, or portfolio page URL]
- **Screenshots:** [Link or add images to a `/docs` folder and link them here]

---

## Download (beta)

<!-- Only if you are hosting builds. If source-only, delete this section. -->
| Platform | Format | Notes |
|----------|--------|-------|
| macOS | VST3 / AU / Standalone | [e.g. Apple Silicon, macOS 13+, unsigned — see install note below] |

**Install (macOS):** [Your steps — e.g. copy `.vst3` to `~/Library/Audio/Plug-Ins/VST3/`, rescan in DAW, Gatekeeper / right-click Open if needed.]

**Tested in:** [e.g. Logic, Reaper, Ableton — list what you actually used.]

---

## Build from source

### Requirements

- CMake 3.22+
- C++17 compiler (Xcode on macOS, Visual Studio on Windows)
- [JUCE 7+](https://github.com/juce-framework/JUCE) — clone separately; this repo does not bundle JUCE

### JUCE location

Place JUCE as a sibling folder, or pass the path explicitly:

```
SPECDRIFT/          ← this repo
JUCE/               ← clone JUCE here (sibling)
```

Or:

```bash
cmake -B build -S . -DJUCE_PATH=/path/to/JUCE
```

### macOS

```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Built plugins (with default JUCE settings):

```
build/Specdrift_artefacts/Release/VST3/Specdrift.vst3
build/Specdrift_artefacts/Release/AU/Specdrift.component
build/Specdrift_artefacts/Release/Standalone/Specdrift.app
```

### Windows

```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

<!-- Fill in if you have actually built on Windows; otherwise delete or mark "not tested yet" -->
[Windows notes — Visual Studio version, VST3 output path, etc.]

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

<!-- Factual list — adjust to match what you actually used -->
- JUCE 7+
- CMake
- Formats: VST3, AU, Standalone

---

## Known issues

<!-- Honest list for beta / portfolio — builds trust -->
- [Issue 1]
- [Issue 2]

---

## License

<!-- Pick one and replace the placeholder -->
[MIT / GPL-3.0 / All rights reserved — contact for permission / etc.]

---

## Contact

[Your name, portfolio site, email, or LinkedIn — whatever you want public.]
