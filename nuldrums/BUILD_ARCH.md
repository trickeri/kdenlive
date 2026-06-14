# Building the Nuldrums Kdenlive fork on Arch Linux

Phase 1 of the build plan: get a **clean build of this fork** running before
touching the D-Bus interface or the captioning panel. Target box: Arch Linux,
RTX 4090.

> Not yet run on hardware — verify each step on the Arch box and update this doc
> inline with anything that drifts (KDE Frameworks / Qt / MLT versions move).

## 1. Dependencies

Kdenlive is Qt6 + KDE Frameworks 6 + MLT. On Arch most of this is packaged:

```bash
sudo pacman -S --needed \
  base-devel git cmake ninja extra-cmake-modules \
  qt6-base qt6-multimedia qt6-svg qt6-declarative qt6-tools \
  kcoreaddons kguiaddons ki18n kxmlgui kconfig kconfigwidgets \
  kwidgetsaddons knewstuff knotifications knotifyconfig kio \
  karchive kbookmarks kcrash kdbusaddons kiconthemes ktextwidgets \
  kdeclarative kfilemetadata purpose \
  mlt ffmpeg \
  vulkan-icd-loader vulkan-headers shaderc glslang   # for NulCaption's Vulkan whisper.cpp
```

(If a `k*` package name has drifted, search with `pacman -Ss <name>`. Kdenlive is
itself in `extra` — installing it pulls the exact runtime deps as a reference:
`pacman -Si kdenlive | grep Depends`.)

## 2. Configure + build

```bash
# from the repo root (this fork), on branch `nuldrums`
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_INSTALL_PREFIX="$PWD/install"
cmake --build build -j
```

Run it in-place without installing:

```bash
./build/bin/kdenlive
```

## 3. Phase 0 verification (on first successful build)

1. **Action names.** Confirm the real `KActionCollection` names for: razor/split
   at playhead, lift delete, ripple delete, remove space, undo/redo, transport,
   seek, track activation, next/prev edit. These back the D-Bus surface.
2. **Native `\kf` rendering.** Load a hand-authored `\kf` ASS on a subtitle track;
   scrub + export; record whether karaoke animates in preview and render. Decides
   NulCaption Path A vs. burn-in.
3. **MLT subtitle filter.** Note the exact MLT `avfilter.subtitles` / ffmpeg `ass`
   filter name + params in the bundled MLT (the burn-in fallback uses it).

## 4. Branch / rebase

`master` tracks upstream untouched; all changes live on `nuldrums`.

```bash
git fetch upstream
git checkout master && git merge --ff-only upstream/master
git checkout nuldrums && git rebase master
```
