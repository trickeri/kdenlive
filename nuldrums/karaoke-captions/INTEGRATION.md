# Right-click → Generate Karaoke Captions — IMPLEMENTED

Status: **wired into the fork (`nuldrums` branch).** Right-click an audio/AV clip
in the Project Bin → **Generate Karaoke Captions**, or use *Subtitles → Generate
Karaoke Captions*. It runs the NulCaption pipeline on the clip's source file and
drops word-by-word karaoke captions onto the project's subtitle track (Path A),
styled with the **per-word highlight** look (the spoken word switches to the
highlight colour) and the Nuldrums style.

It reuses Kdenlive's own subtitle machinery (same as built-in speech-to-text in
`src/dialogs/speechdialog.cpp`): a background `QProcess` produces a Kdenlive-native
karaoke `.ass`, then `SubtitleModel::importSubtitle()` lands it on the track. The
engine is the MIT **`nulcaption`** CLI invoked as a subprocess — no GPL/MIT
linking, matching the fork's plugin-coupling rule.

## Where it lives (fork changes)

- `src/mainwindow.h` — declares `slotGenerateKaraokeCaptions()`.
- `src/mainwindow.cpp` — registers the `generate_karaoke_captions` action (next to
  the other subtitle actions) and implements the slot. It resolves the clip's
  source from **either** the selected timeline clip
  (`controller()->getMainSelectedClip()` → `model()->getClipBinId()` →
  `bin()->getBinClip()->url()`) **or** the selected Project Bin clip
  (`getFirstSelectedClip()`), then `nulcaption caption <url> --native --preset
  pop --ass <tmp>` via `QProcess` → ensure track via `slotEditSubtitle()` →
  `getSubtitleModel()->importSubtitle()`.
- `src/timeline2/view/timelinetabs.cpp` — adds the action to the **timeline clip**
  right-click menu (`TimelineTabs::buildClipMenu`).
- `src/bin/bin.cpp` — adds the action to the **Project Bin** right-click menu
  (`Bin::setupGeneratorMenu`).
- `src/kdenliveui.rc` — adds it to the *Subtitles* menu; GUI `version` bumped
  250 → 251.

So it's reachable three ways: right-click a timeline clip, right-click a bin clip,
or the menubar *Subtitles → Generate Karaoke Captions*.

## Runtime prerequisite

`nulcaption` must be on `PATH` and provisioned once:

```bash
pipx install /path/to/nulcaption      # or: pip install -e nulcaption
nulcaption-setup                      # builds whisper.cpp (Vulkan) + large-v3-turbo
```

If it isn't installed/provisioned the action shows a clear error toast (the CLI
exits non-zero; `QProcess::waitForStarted` also catches a missing binary).

## Build

```bash
cmake --build build --target kdenlive -j      # Ninja; or ./nuldrums/build-arch.sh
```

## Notes / follow-ups

- **v1 places captions at timeline 0** (a bin clip has no single timeline
  position). Offsetting to a placed instance is a follow-up (pass the instance's
  start frame as the `importSubtitle` offset).
- **Style/preset fixed to Nuldrums + `pop`** (per-word highlight). A style/look
  picker belongs in the planned "Auto Karaoke Captions" dock (PLAN Phase 4).
- The action is always enabled; non-media/empty selection yields a graceful error
  rather than being greyed out. Type-aware enable/disable is a possible polish.
- Same `nulcaption caption … --native` contract is what an AI prompt or the D-Bus
  bridge calls headlessly.
