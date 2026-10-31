# Pan Loop — Implementation Plan

A Linux-first standalone live looping app in **C++20 + JUCE**, with JACK audio I/O and MIDI footswitch control.

## Progress

- [x] Phase 1 — project skeleton, JACK audio I/O, MIDI enumeration, basic monitoring
- [x] Phase 2 — transport clock, BPM/time signature, metronome, latency readout, Live transport panel
- [x] Phase 3 — backing playlist, file decoding/resampling, ordered sequence, gapless loop, per-file gain, memory display
- [x] Phase 4 — live looper, first loop/bar snap, overdub auto-stop, Stop/Discard, latency compensation, layers and mix controls
- [x] Phase 5 — MIDI learn, action mappings, persistence, Blackstar quick start
- [x] Audio settings follow-up — sample rate and buffer-size selectors (non-JACK editable; JACK sample rate read-only, PipeWire quantum or native `jack_bufsize` for the period)
- [x] Layer mix follow-up — per-layer stereo pan
- [x] MIDI follow-up — raw-MIDI fallback for USB devices hidden from the ALSA sequencer by PipeWire
- [x] Phase 6 — session persistence, performance view, AppImage packaging and tests

## 1. Core loop behavior

### Master cycle

- If the **backing playlist** has files, the cycle is the **sum of their durations**, resampled to the JACK sample rate. The files play in order, sample-accurately, then the whole sequence repeats.
- If the playlist is empty, the **first live recording** defines the cycle length.
- All live layers are exactly one cycle long and always start at cycle position 0. They repeat with the backing sequence.
- Consequence of the chosen model: a 3-minute playlist creates a **3-minute loop**, and every live layer is 3 minutes long. This is good for short multi-part loops; less suitable for full-length songs.

### Record / Stop / Discard

- **Record**
  - No cycle yet: starts recording the first layer immediately.
  - Cycle already playing: arms; recording begins at the next cycle start.
  - Cycle stopped/paused: starts playback from cycle 0 and records from the start.
  - If a playlist exists, the first live layer is recorded for one full cycle and auto-stops.
- **Stop Recording**
  - Finalizes the current take and keeps it as the current layer.
  - For the first free-form loop, this sets the cycle length. Optional bar snap can extend/cut it to the nearest bar.
  - For overdubs, it can finalize early; otherwise recording auto-stops at the loop boundary.
  - Playback continues, and the **next Record press creates a fresh channel** for the next layer. This is the “new fresh recording channel” behavior.
- **Discard**
  - While recording: aborts the take.
  - Otherwise: deletes the current/last recorded layer. If no layers and no playlist remain, the transport stops and the cycle is cleared.
- **Play/Pause** is a separate global transport action. It is disabled while recording so a foot stomp cannot accidentally cut a take.
- **Clear Layers** removes all live layers but keeps the backing playlist/cycle.

### Playlist editing

- Add, reorder, remove, and drag-and-drop files while playback is stopped.
- Per-file gain can be adjusted during playback.
- If live layers exist and a playlist change alters the cycle length, the app asks to clear the layers. If the total length matches sample-exactly, the layers can be kept.
- This prevents live layers from drifting out of sync with the backing sequence.

### First-loop timing

- BPM and time signature settings, optional click/metronome, and tap tempo.
- **Bar snap**: Stop targets the nearest bar; recording continues or cuts to that boundary, then loops. Off by default for free-form looping.
- BPM/click are mainly useful for free-form loops; with a playlist-defined cycle they are informational.

## 2. Audio architecture

- **JUCE `AudioDeviceManager` with the JACK backend enabled** (`JUCE_JACK=1`), client name `pan-loop`, stereo input and stereo output.
- JACK sample rate and buffer size are server-controlled; the app displays them and handles changes.
- Audio callback rules: no allocation, locks, file I/O, or GUI calls. Use preallocated buffers, atomics, and lock-free queues.
- Block processing is split at loop boundaries so record start/stop and cycle wrap are sample-accurate.
- **Recording latency compensation** using JACK-reported input/output latency plus a manual offset, with a small capture pre-roll so the first notes are not lost.
- **Live layer mixer**: per-layer gain, mute, solo, short fade ramps to avoid clicks.
- **Backing mixer**: per-file gain, global backing mute/gain.
- **Master mix**: backing + layers + optional input monitoring, master gain, clip indicator.
- Optional short loop-edge fade to reduce clicks at loop points.

### File handling

- WAV, FLAC, MP3 supported via JUCE `AudioFormatManager`; AIFF/OGG also available.
- Background decoding and high-quality offline resampling to the JACK sample rate.
- Mono files become dual mono; more than 2 channels downmixed to stereo.
- **V1 preloads the playlist into RAM** for gapless, glitch-free looping, with a memory estimate in the UI. Long playlists may need disk streaming later.
- Session files are a single `.panloop` JSON document plus a `<name>.layers/` sidecar directory containing each recorded layer as a stereo 32-bit float WAV.
- Mixdown export renders exactly one master cycle to a stereo 32-bit float WAV, including the audible backing/layer mix and output gain.

## 3. User interface

A dark, high-contrast, stage-friendly UI:

- **Live/Performance view**: large Record, Stop Rec, Discard, and Play/Pause buttons; cycle progress; layer indicators; backing track name; tempo/position; input/output meters; xrun/CPU status.
- **Playlist view**: ordered file list, add files, drag-and-drop, reorder, remove, per-file gain, total cycle duration.
- **Layers view**: waveform per layer, gain, stereo pan, mute/solo, delete, rename.
- **MIDI view**: device selector, activity indicator, mapping table with Learn/Clear buttons, last received message.
- **Audio view**: JACK input/output client selection, channel mapping, sample rate/buffer display, latency offset, input monitoring, gains.
- **Session view**: BPM, time signature, bar snap, click, save/load/export.
- Keyboard shortcuts for testing: Record, Stop, Discard, Play/Pause.
- Clear visual states: recording (red), armed/waiting (amber), looping (green), paused (blue).

## 4. MIDI / Blackstar Live Logic

- ALSA MIDI input through JUCE. The Live Logic appears as a USB MIDI device.
- Direct ALSA raw-MIDI fallback for USB devices that PipeWire owns and does not expose through the ALSA sequencer. These are shown with a `(raw MIDI)` suffix and use the same Learn/mapping path.
- Supports **CC, Note On/Off, and Program Change**, so it works whether the pedal is configured in live/CC mode or sends program changes.
- Ignores MIDI clock/active sensing to avoid noise.
- **MIDI learn**: click Learn next to an action, press the footswitch, mapping is stored. Handles press/release messages and debounce.
- Assignable actions: Record, Stop Recording, Discard, Play/Pause, Clear Layers, Backing Mute, Input Monitor toggle, Undo Discard.
- Mappings persist globally and per session; auto-selects a device whose name contains “Live Logic”.
- Named MIDI presets store the selected input device plus its mappings and can be restored from the preset list; presets are stored in `~/.config/pan-loop/midi-profiles.json`.
- Suggested footswitch layout:
  - FS1: Record
  - FS2: Stop Recording
  - FS3: Discard
  - FS4: Play/Pause
  - FS5: Backing Mute
  - FS6: Undo Discard / Clear Layers
- JACK MIDI is not included in v1; the Blackstar works over ALSA. A direct JACK MIDI backend can be added later.

## 5. JACK notes and limitations

JUCE’s JACK backend creates a `pan-loop` client with stereo `in_1/2` and `out_1/2` ports and auto-connects to the selected JACK device. It does **not** provide full arbitrary per-port patchbay control; that is a known JUCE limitation.

- If auto-connecting to `system` capture/playback is enough, the JUCE backend is fine.
- If arbitrary port patching to a DAW/interface is needed, add a small **direct libjack backend** behind the same audio-engine interface. The rest of the app stays the same.
- JACK shutdown, missing server, and sample-rate changes are handled with clear errors rather than crashes.

## 6. Implementation phases

1. [x] **Skeleton and build**: CMake, JUCE 8 with JACK, app shell, config paths, logging, MIDI/JACK enumeration, README. *(complete)*
2. [x] **Audio engine and transport foundation**: JACK stereo I/O, transport clock, input monitoring, meters, xruns, metronome/test tone, latency readout, Live transport panel. *(complete)*
3. [x] **Backing playlist**: file decoding/resampling, playlist UI, sequence playback, gapless loop, per-file gain, memory display. *(complete)*
4. [x] **Live looper**: first-loop capture and bar snap, overdub auto-stop, layer mixer, Stop/Discard state machine, latency compensation, layers and mix controls, trimmed first-loop buffers. *(complete)*
5. [x] **MIDI control**: learn UI, mappings for CC/Note/Program Change, action dispatch, persistence, Blackstar quick-start. *(complete)*
6. [x] **Session and polish**: save/load, layer WAVs, mixdown export, performance view, error handling, AppImage packaging, tests. *(complete)*

## 7. Acceptance criteria

- App appears as a JACK client with working stereo input/output.
- A playlist of WAV/FLAC/MP3 files plays in order and loops seamlessly.
- First loop length is set by Stop, with optional bar snap.
- Overdubs arm at the next cycle start and auto-stop at the loop boundary.
- Stop leaves the loop playing and ready for a new channel.
- Discard aborts a take or deletes the current/last layer.
- Blackstar footswitches can be learned for Record, Stop, and Discard, and mappings survive restart.
- Session save/load restores playlist, layers, MIDI map, and settings.
- JACK disconnect, missing files, and unsupported formats fail gracefully.
- Core unit tests pass; manual JACK/Blackstar checklist passes.

## 8. Open decisions

These were flagged during planning and should be settled before or during the relevant phase:

1. **JACK routing**: Is auto-connecting the app to system capture/playback enough, or is arbitrary port patching to a DAW/interface needed? If the latter, plan a direct libjack backend.
2. **Playlist size**: Short loop clips or full songs / hour-long sets? V1 preloads into RAM; long sets would benefit from disk streaming.
3. **License**: Pan Loop currently uses **AGPLv3**, because JUCE’s free license is AGPLv3. Confirm this before distributing binaries, or obtain a JUCE commercial license / switch stacks if a permissive license is required.
4. **Stop/Discard semantics**: Confirm that Stop finalizes the take and leaves it selected, with the next Record creating a new channel; and Discard aborts an in-progress take or deletes the current/last layer.
