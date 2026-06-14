# Nuldrums fork of Kdenlive

This is a **fork of [KDE/kdenlive](https://github.com/KDE/kdenlive)** that adds a
small **D-Bus scripting interface** so external tools can drive a live timeline.
It exists to host two companion plugins:

- **[NulEdit](https://github.com/trickeri/nuledit)** (MIT) — voice + agentic
  editing control.
- **[NulCaption](https://github.com/trickeri/nulcaption)** (MIT) — local
  word-by-word karaoke captioning.

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

## Why the fork stays small: `triggerAction()`

Kdenlive's GUI is action-driven (`KActionCollection` / `QAction`). Most edit ops
("cut", "ripple delete", "close gap", "undo") already exist as named actions, so
the D-Bus surface is mostly a generic action trigger plus a handful of state
getters. Everything destructive routes through Kdenlive's existing undo stack, so
"undo" works for free.

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

## Status

Scaffold. No source changes yet beyond this document. Phase 0 (research + clean
upstream build) gates the actual interface work — see the build plan in
[NulEdit `docs/PLAN.md`](https://github.com/trickeri/nuledit/blob/main/docs/PLAN.md).

## Platforms

Primary target is **Arch Linux + RTX 4090** (D-Bus + MLT/libass + local ASR).
The plugins themselves target **Windows and Linux**; the running fork and its
D-Bus interface are the Linux-side piece.
