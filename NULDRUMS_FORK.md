# Nuldrums fork of Kdenlive

This is a **fork of [KDE/kdenlive](https://github.com/KDE/kdenlive)** that adds:

1. a **D-Bus scripting interface** so external tools can drive a live timeline, and
2. **embedded GUI panels** (Qt) for the companion features — Kdenlive has no
   third-party plugin API, so a CapCut-style in-app experience has to live in the
   fork's own source.

It hosts two companion plugins:

- **[NulEdit](https://github.com/trickeri/nuledit)** (MIT) — voice + agentic
  editing control.
- **[NulCaption](https://github.com/trickeri/nulcaption)** (MIT) — local,
  **Vulkan-only** word-by-word karaoke captioning.

> **Platform: Linux only.** This fork is developed, built, and run on
> **Arch Linux (RTX 4090)**. There is no supported Windows build. (NulCaption's
> captioning *pipeline* happens to run on Windows as a CLI, but the in-Kdenlive
> GUI is Linux.)

## License

This fork inherits Kdenlive's license: **GPL-3.0** (see `COPYING`). It stays
GPL — only the companion plugins are MIT, and they couple to this fork purely
over D-Bus (no GPL code linked into the plugins).

## Branch strategy (minimal diff)

- `master` — tracks upstream `KDE/kdenlive` unchanged (for clean rebases).
- `nuldrums` — our changes only. Kept deliberately tiny.

Rebase onto upstream:

```bash
git fetch upstream
git checkout master && git merge --ff-only upstream/master
git checkout nuldrums && git rebase master
```

## Scope: lean D-Bus core + embedded GUI panels

Two layers of change:

**1. D-Bus scripting core (keep it lean).** Kdenlive's GUI is action-driven
(`KActionCollection` / `QAction`). Most edit ops ("cut", "ripple delete", "close
gap", "undo") already exist as named actions, so the scripting surface is mostly
a generic action trigger plus a handful of state getters. Everything destructive
routes through Kdenlive's existing undo stack, so "undo" works for free. This is
what NulEdit's voice/agent paths call.

**2. Embedded GUI panels (modify Kdenlive as needed).** The product UX is in-app,
so we add real Qt dock/menu UI to the fork — starting with an **"Auto Karaoke
Captions"** panel (source picker, style/preset dropdowns, Apply → runs NulCaption
and drops captions on a subtitle track or burns in). The diff here is allowed to
grow; the rebase strategy below absorbs it.

Intended `Q_SCRIPTABLE` surface on `org.kde.kdenlive.scripting` (or
`org.nuldrums.kdenlive`) — **verify/confirm action names against the pinned
Kdenlive version before implementing (Phase 0):**

```cpp
Q_SCRIPTABLE bool        triggerAction(const QString &actionName); // razor, ripple delete, close gap, undo...
Q_SCRIPTABLE void        seek(int frame);
Q_SCRIPTABLE int         playhead();
Q_SCRIPTABLE void        setPlaying(bool play);
Q_SCRIPTABLE int         activeTrack();
Q_SCRIPTABLE void        setActiveTrack(int trackIndex);
Q_SCRIPTABLE QStringList tracks();
Q_SCRIPTABLE QVariantMap clipAt(int trackIndex, int frame);        // id, in, out, name
Q_SCRIPTABLE void        gotoEdit(int trackIndex, int direction);  // +1 next, -1 prev
Q_SCRIPTABLE void        selectClip(const QString &clipId);
Q_SCRIPTABLE QString     projectStateJson();                       // for the agent to reason over
```

## Building on Arch (do this first)

Phase 1 is a clean upstream build before any fork changes. See
[`nuldrums/BUILD_ARCH.md`](nuldrums/BUILD_ARCH.md) for the full dependency list
and steps; the short version:

```bash
# from this repo root, on Arch:
./nuldrums/build-arch.sh        # installs deps (pacman) + configures + builds
```

Then verify the **native-`\kf` question** (the last open Phase 0 item): hand-author
a tiny `.ass` with `\kf` tags, load it on a subtitle track, scrub + export, and
note whether the karaoke animates in preview and in the render. That decides
whether NulCaption's Path A (native track) is viable or burn-in stays primary.

## Status

- This doc + build scaffolding only — **no Kdenlive source changes yet.**
- Captioning **pipeline** (NulCaption) is built and verified on Windows
  (whisper.cpp Vulkan large-v3 → karaoke → burn-in). The **embedded panel** and
  the **D-Bus interface** are the Linux work that starts here on Arch.
- Build plans:
  [NulEdit](https://github.com/trickeri/nuledit/blob/main/docs/PLAN.md) ·
  [NulCaption](https://github.com/trickeri/nulcaption/blob/main/docs/PLAN.md).
