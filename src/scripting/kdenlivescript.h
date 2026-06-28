/*
    SPDX-FileCopyrightText: Nuldrums
    SPDX-License-Identifier: GPL-3.0-or-later

    Nuldrums scripting interface, exposed on the session bus as
    org.kde.kdenlive.scripting for AI / voice-driven editor control.
*/
#pragma once

#include <QObject>
#include <QString>

class KdenliveScript : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdenlive.scripting")
public:
    explicit KdenliveScript(QObject *parent = nullptr);

public Q_SLOTS:
    /** @brief Liveness check; returns a short status string. */
    Q_SCRIPTABLE QString ping();
    /** @brief JSON describing the current project (profile, doc path). */
    Q_SCRIPTABLE QString projectInfo();
    /** @brief Import a media file into the project bin. */
    Q_SCRIPTABLE void importClip(const QString &path);
    /** @brief Insert a bin clip (by source path) onto the active timeline track at the playhead. */
    Q_SCRIPTABLE void addToTimeline(const QString &path);
    /** @brief Add an effect (by effect id) to the current selection. */
    Q_SCRIPTABLE void addEffect(const QString &effectId);
    /** @brief Insert a new video track at the top. Returns the new track id, or -1. */
    Q_SCRIPTABLE int addVideoTrack();
    /** @brief Insert a new audio track. Returns the new track id, or -1. */
    Q_SCRIPTABLE int addAudioTrack();
    /** @brief Number of video tracks in the active timeline. */
    Q_SCRIPTABLE int videoTrackCount();
    /** @brief Number of audio tracks in the active timeline. */
    Q_SCRIPTABLE int audioTrackCount();
    /** @brief Insert a bin clip (by source path) onto a video track (1-based from bottom)
     *  at a frame position. Returns the new timeline clip id, or -1 on failure. */
    Q_SCRIPTABLE int addClipToTrack(const QString &path, int videoTrackIndex, int position);
    /** @brief Like addClipToTrack but with a drop mode: "" normal (A/V), "video" video-only,
     *  "audio" audio-only. Video-only avoids inserting (and prompting for) audio tracks —
     *  used for the upper reframe overlay so audio isn't duplicated. */
    Q_SCRIPTABLE int addClipToTrackEx(const QString &path, int videoTrackIndex, int position, const QString &mode);
    /** @brief Restrict a bin clip's active audio streams to a ';'-separated list of stream
     *  indexes (e.g. "1;3" = keep streams 1 and 3, drop the rest). Set this before inserting
     *  a multi-stream clip so only those streams need audio tracks (no "missing tracks" modal). */
    Q_SCRIPTABLE bool setClipActiveStreams(const QString &path, const QString &streams);
    /** @brief Comma-separated clip ids on a video track (1-based from bottom). Always query this
     *  for LIVE clip ids — they are reassigned when a project is reloaded. */
    Q_SCRIPTABLE QString clipIdsOnTrack(int videoTrackIndex);
    /** @brief Set a timeline clip's transform rect (x, y, w, h in project pixels). */
    Q_SCRIPTABLE bool setClipTransform(int clipId, int x, int y, int w, int h);
    /** @brief Make clipId the timeline selection (so selection-driven actions like
     *  generate_karaoke_captions resolve it). Returns false on an invalid id. */
    Q_SCRIPTABLE bool selectTimelineClip(int clipId);
    /** @brief Resize a timeline clip to durationFrames (extends/trims its right edge).
     *  Used to stretch a short banner still to a chosen on-screen duration. */
    Q_SCRIPTABLE bool resizeClip(int clipId, int durationFrames);
    /** @brief Set a banner clip's rect + an opacity fade via its built-in qtblend.
     *  fadeInFrames ramps opacity 0->1 at the clip start; fadeOutFrames ramps 1->0 at
     *  the clip end (0 disables either). Position/size stay fixed at x,y,w,h. */
    Q_SCRIPTABLE bool setClipFade(int clipId, int x, int y, int w, int h, int fadeInFrames, int fadeOutFrames);
    /** @brief Save the current project-monitor frame (composited) to a PNG path, for visual verification. */
    Q_SCRIPTABLE void renderFrame(const QString &path);
    /** @brief Toggle play/pause on the active monitor. */
    Q_SCRIPTABLE void playPause();
    /** @brief Seek the project monitor to an absolute frame position. */
    Q_SCRIPTABLE void seek(int position);
    /** @brief Create a new project from scratch with the given MLT profile path
     *  (e.g. /usr/share/mlt-7/profiles/vertical_hd_30 for Vertical HD 30fps) and
     *  save it to savePath. No dialogs. Returns true on success. */
    Q_SCRIPTABLE bool newProject(const QString &profilePath, const QString &savePath);
    /** @brief Save the current project. Returns true on success. */
    Q_SCRIPTABLE bool save();
    /** @brief Save the current project to an explicit path (no dialog), titling an
     *  untitled project. Used to auto-save a freshly-built vertical short. */
    Q_SCRIPTABLE bool saveAs(const QString &path);
    /** @brief Cleanly shut down Kdenlive (prompt-free; removes the crash lock). Call save() first if needed. */
    Q_SCRIPTABLE void quit();
    /** @brief Trigger a Kdenlive menu/toolbar action by its action-collection name
     *  (e.g. "cut_timeline_all_clips" = razor all tracks at the playhead, "undo").
     *  Returns true if the action exists and was triggered. Lets voice control fire
     *  any editor action without a dedicated D-Bus method per action. */
    Q_SCRIPTABLE bool triggerAction(const QString &name);
    /** @brief Add a new (blank) sequence tab to the current project. videoTracks /
     *  audioTracks default to -1 (Kdenlive's default counts). The sequence uses the
     *  PROJECT profile — a sequence can't have its own resolution. Returns true. */
    Q_SCRIPTABLE bool newSequence(int videoTracks = -1, int audioTracks = -1);
    /** @brief Create a new project using the MLT profile at @p profilePath
     *  (untitled; no save dialog). To avoid a modal save prompt that would block
     *  the bridge, a modified current project is auto-saved first when it already
     *  has a path; an untitled+modified one is refused. Returns "ok" | "unsaved"
     *  (save the current project first) | "savefailed" | "error". */
    Q_SCRIPTABLE QString newProjectProfile(const QString &profilePath);
    /** @brief Create a new project at an explicit pixel size and frame rate (e.g.
     *  newProjectFormat(1080, 1920, 30) for vertical 1080p @ 30fps). Square pixels,
     *  display aspect derived from width:height, progressive. Reuses a matching MLT
     *  profile if one exists, otherwise saves a custom one — so the agent never needs
     *  to know a profile file path. Same save-guard/return values as newProjectProfile:
     *  "ok" | "unsaved" | "savefailed" | "error". */
    Q_SCRIPTABLE QString newProjectFormat(int width, int height, double fps);

private:
    /** @brief Shared tail for newProjectProfile/newProjectFormat: guard a modified
     *  current project against a modal save prompt, then open an untitled project on
     *  @p profilePath. Returns "ok" | "unsaved" | "savefailed" | "error". */
    QString openNewProjectWithProfile(const QString &profilePath);
};
