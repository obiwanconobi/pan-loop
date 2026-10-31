# Pan Loop

A Linux-first standalone live looping application built with **C++20** and **JUCE**.

Pan Loop is intended to combine:
- JACK audio input and output
- live looping with overdub layers
- a sequence of backing audio files (WAV / FLAC / MP3) that loops as a playlist
- MIDI footswitch control, including the Blackstar Live Logic

The full design is in [PLAN.md](PLAN.md).

## Project status

**Phase 1 complete: audio + MIDI skeleton**

Implemented so far:

- CMake build with JUCE 8.0.15 fetched automatically
- JACK backend enabled (`JUCE_JACK=1`), with JACK preferred on startup
- `pan-loop` JACK client with stereo `in_1` / `in_2` and `out_1` / `out_2`
- JACK / ALSA / PulseAudio backend selection as a fallback
- Stereo input monitoring with smoothed gain controls (on by default; disable it if your interface already provides direct monitoring or you are using speakers)
- 440 Hz test tone
- Input/output peak meters and clip indicator
- Sample rate, buffer size, latency, xrun and CPU display
- MIDI input enumeration, selection, hot-plug rescan and activity monitor
- Automatic selection of a device whose name contains “Live Logic”
- Dark, stage-oriented UI with Live, Audio, MIDI and Session tabs
- File logging and per-user config paths

The loop engine, playlist playback, session persistence and MIDI learn are the next phases.

## Requirements

### Linux

A C++20 compiler, CMake 3.22+, Ninja (recommended), JACK development headers and the usual JUCE Linux dependencies.

On Debian / Ubuntu:

```bash
sudo apt install build-essential cmake ninja-build git pkg-config \
  libjack-jackd2-dev libasound2-dev \
  libfreetype-dev libfontconfig1-dev \
  libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev \
  libgl1-mesa-dev libcurl4-openssl-dev
```

For PipeWire’s JACK compatibility, install `pipewire-jack` (often already present). The app can also use JACK2 or PipeWire’s JACK server.

JUCE is downloaded by CMake via `FetchContent`; no system JUCE package is required.

## Build

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
```

For development:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j"$(nproc)"
```

### Run

```bash
./build/pan-loop_artefacts/Release/Pan\ Loop
```

or, for the debug build:

```bash
./build/pan-loop_artefacts/Debug/Pan\ Loop
```

## JACK setup

1. Start JACK or make sure PipeWire is running.
2. Launch Pan Loop.
3. The app prefers the JACK backend and opens the first available input/output client.
4. It creates a JACK client named `pan-loop` with:
   - `pan-loop:in_1`, `pan-loop:in_2`
   - `pan-loop:out_1`, `pan-loop:out_2`
5. JUCE auto-connects the selected JACK device to those ports. You can inspect or change connections with `qjackctl`, `Helvum`, `pw-link`, or `jack_lsp` / `jack_connect`.

Example check with PipeWire:

```bash
pw-link -l | grep -A4 -B4 pan-loop
```

If JACK is not running, the Audio tab lets you select another backend (ALSA / PulseAudio) for basic testing. JACK is the intended backend for the final application.

### JACK limitations

JUCE’s JACK backend auto-connects whole devices and does not expose full per-port patchbay management. If arbitrary port routing is required for your setup, the plan includes adding a small direct `libjack` backend behind the same audio-engine interface.

## MIDI / Blackstar Live Logic

- Connect the Live Logic over USB.
- Open the **MIDI** tab and select the device, or press **Rescan**.
- If a device whose name contains “Live Logic” is present at startup, it is selected automatically.
- Phase 1 only monitors incoming **CC**, **Note On/Off** and **Program Change** messages. MIDI Learn and transport actions arrive in Phase 5.
- MIDI clock and active sensing are ignored so they do not flood the monitor.
- MIDI input uses ALSA (through JUCE), not JACK MIDI, in this phase.

## Configuration and logs

- Config directory: `~/.config/pan-loop/`
- Log file: `~/.config/pan-loop/pan-loop.log`
- Sessions directory: `~/.config/pan-loop/sessions/` (reserved for future sessions)

The Session tab shows these paths and can copy a diagnostics block to the clipboard.

## Source layout

```text
src/
  Main.cpp                     JUCE application entry point
  app/                         main window, look and feel, tabs
  audio/                       audio engine, device setup, audio settings UI
  core/                        config paths and logging
  midi/                        MIDI device management and monitor UI
  ui/                          shared widgets (level meter)
PLAN.md                        full product and implementation plan
CMakeLists.txt                 build definition
```

## Roadmap

1. **Phase 1 — skeleton**: build, JACK I/O, MIDI enumeration, basic monitoring. *(current)*
2. **Phase 2 — audio engine**: transport clock, metronome, latency reporting, mixing foundation.
3. **Phase 3 — backing playlist**: file decoding/resampling, ordered playlist, gapless looping.
4. **Phase 4 — live looper**: first-loop capture, bar snap, overdub auto-stop, Stop/Discard state machine, latency compensation.
5. **Phase 5 — MIDI control**: MIDI Learn, action mappings, persistence, Blackstar quick start.
6. **Phase 6 — session and polish**: save/load, layer WAVs, mixdown export, performance view, AppImage packaging, tests.

## License

Pan Loop uses JUCE under its **AGPLv3** open-source license. The project is therefore distributed under the GNU Affero General Public License v3.0. See [LICENSE](LICENSE).

If you need a non-AGPL distribution, JUCE offers commercial licenses; the license choice should be revisited before distributing binaries.
