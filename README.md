# Pan Loop

A Linux-first standalone live looping application built with **C++20** and **JUCE**.

Pan Loop is intended to combine:
- JACK audio input and output
- live looping with overdub layers
- a sequence of backing audio files (WAV / FLAC / MP3) that loops as a playlist
- MIDI footswitch control, including the Blackstar Live Logic

The full design is in [PLAN.md](PLAN.md).

## Project status

**Phase 6 complete: sessions, performance view and packaging**

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
- **ALSA raw-MIDI fallback** for USB devices that PipeWire takes over and therefore never appears as an ALSA sequencer client (shown with a `(raw MIDI)` suffix)
- **Named MIDI presets** that save the selected input device together with the footswitch mappings, selectable from a preset list
- Automatic selection of a device whose name contains “Live Logic”
- Dark, stage-oriented UI with Live, Playlist, Layers, Audio, MIDI and Session tabs
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
- **Live first-loop recording** with optional bar snapping
- **Overdub layers** that start at the cycle boundary, auto-stop at the loop end, and loop in sync with the backing playlist or first loop
- **Stop Rec** finalizes the current take; **Discard** aborts a take or removes the latest layer
- **Latency-compensated recording** using JACK input/output latency plus an adjustable record offset
- **Layers tab** with per-layer gain, **pan**, mute, solo, delete and clear controls, plus memory usage
- First loops are trimmed to their actual length after finalization instead of keeping the recording-buffer allocation
- Barrier against changing playlist structure while live layers exist, since the playlist defines the master cycle
- Keyboard shortcuts: `Space` play/pause, `S` stop, `R` record, `T` stop recording, `D` discard
- Live looping verified with JACK loopback: first loop, bar-snapped loop, overdub, auto-stop, discard, clear, latency compensation, and playlist-plus-layer synchronization
- **MIDI Learn** for CC, Note On and Program Change messages
- Assignable actions: Record, Stop Recording, Discard Layer, Play/Pause, Clear Layers, Backing Mute, Input Monitor and Metronome
- Mapping table with per-action Learn/Clear controls and live mapping descriptions
- Automatic mapping persistence in `~/.config/pan-loop/midi-map.txt`
- MIDI actions dispatched safely on the message thread; repeated messages are debounced
- Backing mute toggle in the Playlist tab and input-monitor synchronization in the Audio tab
- MIDI Learn and action dispatch verified with injected CC, Note and Program Change messages, including mapping persistence across restarts
- **Sample rate and buffer size selection** in the Audio tab for non-JACK backends
- On JACK, the sample rate is displayed read-only; the buffer size can be changed when the JACK server is PipeWire (via `pw-metadata`) or when the native JACK `jack_bufsize` tool is available
- **Session save/load** (`.panloop` JSON plus a `<name>.layers/` sidecar directory of 32-bit float WAV layer files)
- Sessions restore the backing playlist, live layers, layer gain/pan/mute/solo, cycle length, BPM/time signature, bar snap, metronome, input monitoring, backing mute, record offset and MIDI mappings
- Session loading rejects a sample-rate mismatch instead of silently resampling live layers
- **Mixdown export** to a stereo 32-bit float WAV file covering exactly one master cycle, including the audible backing/layer mix and output gain
- **Performance view** with oversized Record, Stop Rec, Discard and Play/Pause controls, cycle progress, backing/layer status and large input/output meters; toggle with the header button or `F11`, exit with `Esc`
- **AppImage packaging** via `scripts/build-appimage.sh`, plus desktop entry and scalable application icon
- **Automated tests** for transport, playlist playback, live-layer save/restore, panning and offline mixing, MIDI mapping/preset/raw-MIDI helpers, PipeWire metadata parsing, session round-trips, sample-rate mismatch handling and mixdown export (`ctest`)

### Backing playlist notes

- The playlist is preloaded into RAM at the current JACK sample rate. Roughly 23 MB is used per minute of stereo audio at 48 kHz, 32-bit float. The Playlist tab shows the current total.
- Structural playlist edits are only available while the transport is stopped. Per-file gain can be changed while playing.
- Changing the JACK sample rate clears the preloaded playlist, because the decoded audio would be at the wrong rate. Re-add the files after changing sample rates.
- MP3 files may include a few milliseconds of encoder delay/padding, so their decoded duration can be slightly longer than the nominal file length. WAV and FLAC decode sample-accurately.
- Multichannel files currently use the first two channels rather than a proper surround downmix.

### Live loop notes

- The first free-form loop can be up to 60 seconds. It is recorded with latency compensation, and its buffer is trimmed to the final loop length.
- Bar snap is optional and off by default. When enabled, Stop Rec targets the nearest bar at the current BPM/time signature; recording continues or is truncated to that target.
- With an existing cycle, each overdub records one full cycle and auto-stops at the loop boundary. Stop Rec can finalize early; silence remains after the stop point for that layer.
- The playlist defines the cycle when present. Adding, removing or reordering playlist files is disabled while live layers exist. Per-file gain remains adjustable.
- Changing the JACK sample rate clears both the preloaded playlist and live layers.
- The Audio tab's **Record offset** slider adjusts latency compensation in milliseconds. Adjust it if recorded layers feel early or late relative to the click/backing. The default uses the reported JACK input + output latency.
- The first loop, overdub and discard paths were verified through JACK loopback; layers can be inspected, panned, muted, soloed and deleted in the Layers tab.
- Layer **Pan** is a stereo balance: `C` keeps both channels at full level, `L 100` keeps only the left channel and `R 100` keeps only the right. Pan is applied to live playback and mixdown export, and saved with the session.

### Sessions

Open the **Session** tab to save, load, export or start a new session.

- A session is a single `.panloop` JSON file. Recorded live layers are stored next to it in `<session-name>.layers/` as 32-bit float stereo WAV files.
- Saving keeps the current playlist file paths, per-file gains, layer gain/pan/mute/solo, BPM, time signature, bar-snap setting, metronome state, monitoring/backing-mute state, record offset and MIDI mappings.
- Loading requires the audio device to be running at the same sample rate as the session. If the rates differ, the load is rejected with a clear message; change the JACK server/PipeWire rate first.
- **Export Mixdown** renders exactly one master cycle to a 32-bit float WAV file. It uses the current audible mix: backing (unless backing mute is on), all non-muted live layers with solo rules applied, and the output gain. Metronome and test tone are not included.
- Loading a session stops the transport first. Missing backing files, missing layer files and malformed sessions produce an error without partially replacing the current playlist or layers.

### Performance view

Press **F11** or click **Performance** in the header to enter the stage view. It provides:

- Large **Record**, **Stop Rec**, **Discard** and **Play/Pause** buttons
- Big transport state and position readouts
- Cycle progress bar with bar markers
- Backing-track and live-layer status
- Vertical input/output meters
- Backing mute, clear layers, metronome and full-screen controls

Keyboard shortcuts in the performance view: `Space` play/pause, `R` record, `T` stop recording, `D` discard, `B` backing mute, `C` clear layers, `F11` toggle full screen and `Esc` exit.

### AppImage packaging

```bash
./scripts/build-appimage.sh
```

The script builds a Release binary, creates an AppDir with the desktop file and icon, collects runtime libraries with `linuxdeploy`, and writes:

```text
dist/Pan-Loop-<version>-x86_64.AppImage
```

`linuxdeploy` and `appimagetool` are downloaded on demand into `.cache/appimage-tools/`. Set `PANLOOP_SKIP_BUILD=1` to package an existing `build-release` tree, or override `BUILD_DIR`, `DIST_DIR`, `TOOLS_DIR` and `APPDIR` with environment variables. `rsvg-convert` or ImageMagick is used to render the icon.

The same files can be installed system-wide without AppImage:

```bash
cmake --install build-release --prefix /usr/local
```

### Tests

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j"$(nproc)"
ctest --test-dir build --output-on-failure
```

The test target is built by default. Disable it with `-DPANLOOP_BUILD_TESTS=OFF`. Tests cover transport timing/cycle wrap, playlist ordering/gain/wrap behavior, live-layer offline mixing and save/restore, MIDI mapping helpers, and a full session save/load/export round-trip including sample-rate mismatch and error paths.

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

For AppImage packaging, `wget` and either `rsvg-convert` (`librsvg2-bin`) or ImageMagick are also needed. The AppImage helper tools are downloaded by the packaging script.

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
./build/pan-loop_artefacts/Release/pan-loop
```

or, for the debug build:

```bash
./build/pan-loop_artefacts/Debug/pan-loop
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

### Sample rate and buffer size

The Audio tab has separate **Sample rate** and **Buffer size** selectors. Both use the same **Apply** button.

- **JACK owns the sample rate.** The Audio tab shows the current JACK server rate read-only and explains that it must be changed in the JACK server/PipeWire configuration (for example with `qjackctl` or PipeWire metadata).
- **Buffer size is the number of samples per audio callback/period.** For **ALSA / PulseAudio** fallback backends, choose an available buffer size and press **Apply**.
- When JACK is provided by **PipeWire**, the buffer size selector is enabled and changes the PipeWire graph quantum with `pw-metadata -n settings 0 clock.force-quantum <samples>`. This is the usual way to lower the JACK period below the 1024-sample default on PipeWire systems. Selecting **Auto (server)** clears the forced quantum (`0`).
- On native JACK, the app uses the `jack_bufsize` tool when it is installed. If neither PipeWire metadata nor the JACK tool is available, the period remains server-owned and the control is disabled with an explanatory hint.
- If the JACK server rate changes and the app reopens its audio device, the preloaded playlist and live layers are cleared because those buffers are tied to the previous rate. Restart the app or reselect the device after changing the JACK server rate.
- Session files record the sample rate and refuse to load at a different rate, so live layers stay sample-aligned. Audio device selection itself is not persisted yet; choose the JACK device again after restarting the app.

### JACK limitations

JUCE’s JACK backend auto-connects whole devices and does not expose full per-port patchbay management. If arbitrary port routing is required for your setup, the plan includes adding a small direct `libjack` backend behind the same audio-engine interface.

## MIDI / Blackstar Live Logic

Quick start:

1. Connect the Live Logic over USB.
2. Open the **MIDI** tab. If a device whose name contains “Live Logic” is present at startup, it is selected automatically; otherwise select it and press **Rescan**.
3. In **Footswitch mappings**, click **Learn** next to an action.
4. Press the footswitch. The incoming message is captured and saved automatically.
5. Repeat for the other actions. **Clear** removes a mapping.

Suggested footswitch layout:

- FS1: Record
- FS2: Stop Recording
- FS3: Discard Layer
- FS4: Play / Pause
- FS5: Backing Mute
- FS6: Clear Layers (or Input Monitor / Metronome)

The Live Logic can be configured to send CC, Note or Program Change messages. MIDI Learn accepts all three; no fixed CC numbers are required. MIDI clock and active sensing are ignored, and repeated messages are debounced.

On PipeWire systems, a USB MIDI device is often grabbed by PipeWire and never appears as an ALSA sequencer client. JUCE's normal `MidiInput` enumeration cannot see those devices, so Pan Loop also scans `/dev/snd/midiC*D*` and opens them directly with ALSA raw-MIDI. Such devices appear in the device list with a `(raw MIDI)` suffix, for example:

```text
Live Logic MIDI Controller (raw MIDI)
GP-200 (raw MIDI)
```

Both routes use the same Learn/mapping/action path. One MIDI input is active at a time; choose the device from the **MIDI** tab.

### MIDI presets

Use the **Preset** row in the MIDI tab to keep multiple setups:

1. Select the input device and learn the footswitch mappings you want.
2. Type a name into the **Preset name** box and press **Save** (or press Enter).
3. Pick the preset from the list later to restore both the input device and its mappings.
4. Use **Delete** to remove the selected preset.

Saving with an existing name replaces that preset. Selecting a preset fills the name box, so you can load it, tweak mappings, and press **Save** again to update it. Presets are stored in `~/.config/pan-loop/midi-profiles.json`. If a preset’s device is not connected when it is selected, the mappings are still restored and the app reports that the device is unavailable.

Mappings are also stored globally at:

`~/.config/pan-loop/midi-map.txt`

The file is human-readable and can be edited manually:

```text
# action type channel number
record cc 1 20
stop cc 1 21
discard cc 1 22
playpause note 1 60
metronome program 1 5
```

Each mapping stores the MIDI channel and number. For CC messages, values 64-127 trigger the action; Note On messages trigger on velocity greater than zero; Program Change messages trigger on receipt. MIDI input uses ALSA (the sequencer through JUCE, with a raw-MIDI fallback), not JACK MIDI.

## Configuration and logs

- Config directory: `~/.config/pan-loop/`
- Log file: `~/.config/pan-loop/pan-loop.log`
- MIDI map: `~/.config/pan-loop/midi-map.txt`
- MIDI presets: `~/.config/pan-loop/midi-profiles.json`
- Sessions directory: `~/.config/pan-loop/sessions/` (default location for saved `.panloop` files)

The Session tab shows these paths, saves/loads sessions, exports mixdowns and can copy a diagnostics block to the clipboard.

## Source layout

```text
src/
  Main.cpp                     JUCE application entry point
  app/                         main window, live transport, performance view, playlist/layer UI, look and feel, tabs
  audio/                       audio engine, live looper, transport, metronome, playlist, file loader, device setup
  core/                        config paths and logging
  midi/                        MIDI device management, learn/mapping model and mapping UI
  session/                     `.panloop` save/load and mixdown export
  ui/                          shared widgets (level meter)
tests/                         unit and integration tests (CTest)
packaging/                     desktop entry and application icon
scripts/                       AppImage packaging script
PLAN.md                        full product and implementation plan
CMakeLists.txt                 build definition
```

## Development smoke tests

Debug builds support development-only hooks:

```bash
# Start the transport and metronome after two seconds.
PANLOOP_DEMO=1 ./build/pan-loop_artefacts/Debug/pan-loop

# Load a playlist at startup and start it after JACK connections settle.
PANLOOP_DEMO=1 \
PANLOOP_TEST_PLAYLIST="/path/to/one.wav;/path/to/two.flac" \
./build/pan-loop_artefacts/Debug/pan-loop

# Capture the performance view offscreen without needing a mapped window.
PANLOOP_SCREENSHOT=/tmp/pan-loop-performance.png \
PANLOOP_SCREENSHOT_PERFORMANCE=1 \
PANLOOP_SCREENSHOT_DELAY_MS=3500 \
./build/pan-loop_artefacts/Debug/pan-loop

# Apply a PipeWire/JACK buffer size shortly after startup and log the result.
PANLOOP_TEST_BUFFER_SIZE=256 \
PANLOOP_SCREENSHOT=/tmp/pan-loop-audio.png \
PANLOOP_SCREENSHOT_TAB=3 \
PANLOOP_SCREENSHOT_DELAY_MS=4000 \
./build/pan-loop_artefacts/Debug/pan-loop
```

`PANLOOP_TEST_BUFFER_SIZE` uses the same path as the Audio tab's **Apply** button and changes the PipeWire/JACK server period; select **Auto (server)** in the Audio tab (or set `clock.force-quantum` to `0`) to restore automatic quantum selection. `PANLOOP_DEMO` is useful for checking the transport clock, metronome audio and UI without touching the mouse. `PANLOOP_TEST_PLAYLIST` separates file paths with semicolons and loads them through the normal background loader. When a test playlist is present, the demo hook leaves the metronome off so recorded output contains only the backing files; set `PANLOOP_DEMO_METRONOME=1` to keep the click on while testing a playlist.

Live-loop smoke test:

```bash
PANLOOP_TEST_LOOP=1 \
PANLOOP_TEST_LOOP_SNAP=1 \
PANLOOP_SCREENSHOT=/tmp/pan-loop-layers.png \
PANLOOP_SCREENSHOT_TAB=2 \
PANLOOP_SCREENSHOT_DELAY_MS=11000 \
./build/pan-loop_artefacts/Debug/pan-loop
```

This records a first loop at 3 s, stops at 5 s, records an overdub at 6 s, stops at 8.5 s, then arms and discards another take. `PANLOOP_TEST_LOOP_SNAP=1` enables bar snap. `PANLOOP_TEST_LOOP_EXTRA=1` adds a finalized-layer discard at 11.5 s and a full layer clear at 13 s. The screenshot hook renders the UI offscreen to a PNG, which is useful in headless or window-manager-constrained environments.

MIDI smoke test:

```bash
# Learn Record from a simulated CC 20 message.
PANLOOP_TEST_MIDI_LEARN=record \
PANLOOP_TEST_MIDI_INJECT="cc:20:127" \
PANLOOP_TEST_MIDI_DEVICE="Midi Through" \
./build/pan-loop_artefacts/Debug/pan-loop

# Trigger Record -> Stop -> Discard using three simulated CC messages.
PANLOOP_TEST_MIDI_INJECT="cc:20:127|cc:21:127|cc:22:127" \
./build/pan-loop_artefacts/Debug/pan-loop
```

`PANLOOP_TEST_MIDI_INJECT` separates messages with `|`; each token is `cc:number:value`, `note:number:velocity` or `pc:number:value`. `PANLOOP_TEST_MIDI_DEVICE` selects a device by name substring. `PANLOOP_TEST_MIDI_PROFILE` selects a saved preset after startup, and `PANLOOP_TEST_MIDI_PROFILE_SAVE` saves the current setup under the given name. These hooks use the same paths as the UI and hardware, and all development hooks are compiled out of release builds.

## Roadmap

1. **Phase 1 — skeleton**: build, JACK I/O, MIDI enumeration, basic monitoring. *(complete)*
2. **Phase 2 — transport and metronome**: transport clock, BPM/time signature, metronome, latency readout, Live transport panel. *(complete)*
3. **Phase 3 — backing playlist**: file decoding/resampling, ordered playlist, gapless looping, per-file gain, memory display. *(complete)*
4. **Phase 4 — live looper**: first-loop capture, bar snap, overdub auto-stop, Stop/Discard state machine, latency compensation, layers/mix controls, live-layer trimming. *(complete)*
5. **Phase 5 — MIDI control**: MIDI Learn, action mappings, persistence, Blackstar quick start. *(complete)*
6. **Phase 6 — session and polish**: session save/load, layer WAVs, mixdown export, performance view, AppImage packaging, automated tests. *(complete)*

## License

Pan Loop uses JUCE under its **AGPLv3** open-source license. The project is therefore distributed under the GNU Affero General Public License v3.0. See [LICENSE](LICENSE).

If you need a non-AGPL distribution, JUCE offers commercial licenses; the license choice should be revisited before distributing binaries.
