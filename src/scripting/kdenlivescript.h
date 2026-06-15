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
    /** @brief Number of video tracks in the active timeline. */
    Q_SCRIPTABLE int videoTrackCount();
    /** @brief Insert a bin clip (by source path) onto a video track (1-based from bottom)
     *  at a frame position. Returns the new timeline clip id, or -1 on failure. */
    Q_SCRIPTABLE int addClipToTrack(const QString &path, int videoTrackIndex, int position);
    /** @brief Comma-separated clip ids on a video track (1-based from bottom). Always query this
     *  for LIVE clip ids — they are reassigned when a project is reloaded. */
    Q_SCRIPTABLE QString clipIdsOnTrack(int videoTrackIndex);
    /** @brief Set a timeline clip's transform rect (x, y, w, h in project pixels). */
    Q_SCRIPTABLE bool setClipTransform(int clipId, int x, int y, int w, int h);
    /** @brief Save the current project-monitor frame (composited) to a PNG path, for visual verification. */
    Q_SCRIPTABLE void renderFrame(const QString &path);
    /** @brief Toggle play/pause on the active monitor. */
    Q_SCRIPTABLE void playPause();
    /** @brief Seek the project monitor to an absolute frame position. */
    Q_SCRIPTABLE void seek(int position);
    /** @brief Save the current project. Returns true on success. */
    Q_SCRIPTABLE bool save();
    /** @brief Cleanly shut down Kdenlive (prompt-free; removes the crash lock). Call save() first if needed. */
    Q_SCRIPTABLE void quit();
};
