# Pan Loop

A Linux-first standalone live looping application built with **C++20** and **JUCE**.

Pan Loop is intended to combine:
- JACK audio input and output
- live looping with overdub layers
- a sequence of backing audio files (WAV / FLAC / MP3) that loops as a playlist
- MIDI footswitch control, including the Blackstar Live Logic

The full design is in [PLAN.md](PLAN.md).

## Project status

**Phase 3 complete: backing playlist**

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
- Dark, stage-oriented UI with Live, Playlist, Audio, MIDI and Session tabs
- File logging and per-user config paths
- **Sample-accurate transport clock** with BPM, time signature, bar/beat position, play/pause and stop/rewind
- **Tap tempo** and BPM controls
- **Metronome** with beat/bar accents, enable toggle and level control
- **Live transport panel** with large play/pause and stop controls, beat indicator and bar progress
- **Round-trip latency readout**
- Transport state is preserved across JACK port connection changes (the JUCE JACK backend restarts the audio device when ports change)
- Metronome audio verified at 120 BPM: 0.5 s click intervals with accents every four beats
- **Backing playlist** with ordered WAV / FLAC / MP3 / AIFF / OGG files
- Background decoding and resampling to the JACK sample rate
- Playlist total duration becomes the master transport cycle; playback is sample-accurate and loops back to the first file
- Add files with the file chooser or drag-and-drop, plus remove, reorder, clear and per-file gain controls
- Preloaded memory and cycle-duration display
- Mono files are duplicated to stereo; files with more than two channels use the first two channels
- Loading progress, error reporting and a list of loaded files
- Playlist playback verified with WAV, FLAC and MP3 files, including a 44.1 kHz mono file resampled to 48 kHz; the recorded sequence repeated at the expected cycle length

The live loop engine, session persistence and MIDI learn are the next phases.

### Backing playlist notes

- The playlist is preloaded into RAM at the current JACK sample rate. Roughly 23 MB is used per minute of stereo audio at 48 kHz, 32-bit float. The Playlist tab shows the current total.
- Structural playlist edits are only available while the transport is stopped. Per-file gain can be changed while playing.
- Changing the JACK sample rate clears the preloaded playlist, because the decoded audio would be at the wrong rate. Re-add the files after changing sample rates.
- MP3 files may include a few milliseconds of encoder delay/padding, so their decoded duration can be slightly longer than the nominal file length. WAV and FLAC decode sample-accurately.
- Multichannel files currently use the first two channels rather than a proper surround downmix.

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
- The MIDI page currently only monitors incoming **CC**, **Note On/Off** and **Program Change** messages. MIDI Learn and transport actions arrive in Phase 5.
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
  app/                         main window, live transport, playlist UI, look and feel, tabs
  audio/                       audio engine, transport, metronome, playlist, file loader, device setup
  core/                        config paths and logging
  midi/                        MIDI device management and monitor UI
  ui/                          shared widgets (level meter)
PLAN.md                        full product and implementation plan
CMakeLists.txt                 build definition
```

## Development smoke tests

Debug builds support two development-only hooks:

```bash
# Start the transport and metronome after two seconds.
PANLOOP_DEMO=1 ./build/pan-loop_artefacts/Debug/Pan\ Loop

# Load a playlist at startup and start it after JACK connections settle.
PANLOOP_DEMO=1 \
PANLOOP_TEST_PLAYLIST="/path/to/one.wav;/path/to/two.flac" \
./build/pan-loop_artefacts/Debug/Pan\ Loop
```

`PANLOOP_DEMO` is useful for checking the transport clock, metronome audio and UI without touching the mouse. `PANLOOP_TEST_PLAYLIST` separates file paths with semicolons and loads them through the normal background loader. When a test playlist is present, the demo hook leaves the metronome off so recorded output contains only the backing files; set `PANLOOP_DEMO_METRONOME=1` to keep the click on while testing a playlist. These hooks are compiled out of release builds.

## Roadmap

1. **Phase 1 — skeleton**: build, JACK I/O, MIDI enumeration, basic monitoring. *(complete)*
2. **Phase 2 — transport and metronome**: transport clock, BPM/time signature, metronome, latency readout, Live transport panel. *(complete)*
3. **Phase 3 — backing playlist**: file decoding/resampling, ordered playlist, gapless looping, per-file gain, memory display. *(complete)*
4. **Phase 4 — live looper**: first-loop capture, bar snap, overdub auto-stop, Stop/Discard state machine, latency compensation.
5. **Phase 5 — MIDI control**: MIDI Learn, action mappings, persistence, Blackstar quick start.
6. **Phase 6 — session and polish**: save/load, layer WAVs, mixdown export, performance view, AppImage packaging, tests.

## License

Pan Loop uses JUCE under its **AGPLv3** open-source license. The project is therefore distributed under the GNU Affero General Public License v3.0. See [LICENSE](LICENSE).

If you need a non-AGPL distribution, JUCE offers commercial licenses; the license choice should be revisited before distributing binaries.
