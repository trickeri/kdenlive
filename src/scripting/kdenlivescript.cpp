/*
    SPDX-FileCopyrightText: Nuldrums
    SPDX-License-Identifier: GPL-3.0-or-later
*/
#include "kdenlivescript.h"

#include "bin/bin.h"
#include "bin/projectclip.h"
#include "bin/projectitemmodel.h"
#include "core.h"
#include "doc/kdenlivedoc.h"
#include "effects/effectstack/model/effectstackmodel.hpp"
#include "mainwindow.h"
#include "monitor/monitor.h"
#include "monitor/monitormanager.h"
#include "profiles/profilemodel.hpp"
#include "profiles/profilerepository.hpp"
#include "project/projectmanager.h"
#include "timeline2/model/timelineitemmodel.hpp"
#include "timeline2/view/timelinewidget.h"

#include <KActionCollection>
#include <QAction>
#include <QApplication>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSize>
#include <QTextStream>
#include <QTimer>
#include <QUrl>
#include <algorithm>
#include <cmath>
#include <numeric>

// Nuldrums scripting log: every D-Bus call records args/results/failures here so the
// controlling agent can see exactly what happened (Kdenlive otherwise swallows stderr).
static void nlog(const QString &msg)
{
    QFile f(QStringLiteral("/tmp/kdenlive-scripting.log"));
    if (f.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream(&f) << QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss.zzz")) << "  " << msg << '\n';
        f.close();
    }
}

KdenliveScript::KdenliveScript(QObject *parent)
    : QObject(parent)
{
    nlog(QStringLiteral("=== KdenliveScript constructed (scripting interface registered) ==="));
}

QString KdenliveScript::ping()
{
    const bool hasDoc = pCore && pCore->currentDoc() != nullptr;
    return QStringLiteral("kdenlive-scripting ok, doc=%1").arg(hasDoc ? QStringLiteral("yes") : QStringLiteral("no"));
}

QString KdenliveScript::projectInfo()
{
    QJsonObject o;
    const QSize fs = pCore->getCurrentFrameSize();
    o[QStringLiteral("width")] = fs.width();
    o[QStringLiteral("height")] = fs.height();
    o[QStringLiteral("fps")] = pCore->getCurrentProfile()->fps();
    KdenliveDoc *doc = pCore->currentDoc();
    o[QStringLiteral("docOpen")] = doc != nullptr;
    o[QStringLiteral("docPath")] = doc ? doc->url().toLocalFile() : QString();
    return QString::fromUtf8(QJsonDocument(o).toJson(QJsonDocument::Compact));
}

void KdenliveScript::importClip(const QString &path)
{
    nlog(QStringLiteral("importClip('%1') doc=%2 fileExists=%3")
             .arg(path)
             .arg(pCore && pCore->currentDoc() ? QStringLiteral("yes") : QStringLiteral("no"))
             .arg(QFileInfo::exists(path) ? QStringLiteral("yes") : QStringLiteral("no")));
    if (pCore->window()) {
        pCore->window()->addProjectClip(path);
        const int found = pCore->projectItemModel()->getClipByUrl(QFileInfo(path)).size();
        nlog(QStringLiteral("importClip: addProjectClip returned; getClipByUrl count now = %1").arg(found));
    } else {
        nlog(QStringLiteral("importClip: no window!"));
    }
}

void KdenliveScript::addToTimeline(const QString &path)
{
    if (pCore->window()) {
        pCore->window()->addTimelineClip(path);
    }
}

void KdenliveScript::addEffect(const QString &effectId)
{
    if (pCore->window()) {
        pCore->window()->addEffect(effectId);
    }
}

int KdenliveScript::addVideoTrack()
{
    if (!pCore || !pCore->currentDoc()) {
        return -1;
    }
    std::shared_ptr<TimelineItemModel> timeline = pCore->currentDoc()->getTimeline(pCore->currentTimelineId());
    if (!timeline) {
        return -1;
    }
    int id = -1;
    const bool ok = timeline->requestTrackInsertion(-1, id, QString(), false); // pos=-1 (top), video
    nlog(QStringLiteral("addVideoTrack: ok=%1 id=%2").arg(ok).arg(id));
    return ok ? id : -1;
}

int KdenliveScript::videoTrackCount()
{
    if (!pCore || !pCore->currentDoc()) {
        return -1;
    }
    std::shared_ptr<TimelineItemModel> timeline = pCore->currentDoc()->getTimeline(pCore->currentTimelineId());
    if (!timeline) {
        return -1;
    }
    return timeline->getTracksIds(false).size();
}

int KdenliveScript::addClipToTrack(const QString &path, int videoTrackIndex, int position)
{
    nlog(QStringLiteral("addClipToTrack('%1', vtrack=%2, pos=%3)").arg(path).arg(videoTrackIndex).arg(position));
    if (!pCore || !pCore->currentDoc()) {
        nlog(QStringLiteral("addClipToTrack -> -10 (no document)"));
        return -10; // no document
    }
    std::shared_ptr<TimelineItemModel> timeline = pCore->currentDoc()->getTimeline(pCore->currentTimelineId());
    if (!timeline) {
        nlog(QStringLiteral("addClipToTrack -> -11 (no timeline model for uuid %1)").arg(pCore->currentTimelineId().toString()));
        return -11; // no active timeline model
    }
    const QStringList ids = pCore->projectItemModel()->getClipByUrl(QFileInfo(path));
    if (ids.isEmpty()) {
        nlog(QStringLiteral("addClipToTrack -> -12 (clip not in bin; total bin clips=%1)").arg(pCore->projectItemModel()->clipsCount()));
        return -12; // clip not in bin
    }
    const QString binId = ids.constFirst();
    std::shared_ptr<ProjectClip> bc = pCore->projectItemModel()->getClipByBinID(binId);
    if (bc) {
        const QSize fs = bc->frameSize();
        nlog(QStringLiteral("addClipToTrack: bin clip frameSize=%1x%2 status=%3").arg(fs.width()).arg(fs.height()).arg(int(bc->clipStatus())));
    }
    QList<int> vids = timeline->getTracksIds(false);
    nlog(QStringLiteral("addClipToTrack: binId=%1, video tracks=%2").arg(binId).arg(vids.size()));
    if (videoTrackIndex < 1 || videoTrackIndex > vids.size()) {
        nlog(QStringLiteral("addClipToTrack -> -13 (track index out of range)"));
        return -13; // track index out of range
    }
    // Order video tracks bottom-to-top so index 1 == V1 (bottom).
    std::sort(vids.begin(), vids.end(), [&timeline](int a, int b) { return timeline->getTrackPosition(a) < timeline->getTrackPosition(b); });
    const int trackId = vids.at(videoTrackIndex - 1);
    int id = -1;
    const bool ok = timeline->requestClipInsertion(binId, trackId, position, id, true, true, false);
    nlog(QStringLiteral("addClipToTrack: requestClipInsertion(track=%1) ok=%2 id=%3").arg(trackId).arg(ok).arg(id));
    return ok ? id : -14; // -14: requestClipInsertion failed
}

QString KdenliveScript::clipIdsOnTrack(int videoTrackIndex)
{
    if (!pCore || !pCore->currentDoc()) {
        return QString();
    }
    std::shared_ptr<TimelineItemModel> timeline = pCore->currentDoc()->getTimeline(pCore->currentTimelineId());
    if (!timeline) {
        return QString();
    }
    QList<int> vids = timeline->getTracksIds(false);
    if (videoTrackIndex < 1 || videoTrackIndex > vids.size()) {
        return QString();
    }
    std::sort(vids.begin(), vids.end(), [&timeline](int a, int b) { return timeline->getTrackPosition(a) < timeline->getTrackPosition(b); });
    const int trackId = vids.at(videoTrackIndex - 1);
    QStringList ids;
    for (int c : timeline->getItemsInRange(trackId, 0, -1, false)) { // clips only (no compositions)
        ids << QString::number(c);
    }
    nlog(QStringLiteral("clipIdsOnTrack(%1) -> [%2]").arg(videoTrackIndex).arg(ids.join(QLatin1Char(','))));
    return ids.join(QLatin1Char(','));
}

bool KdenliveScript::setClipTransform(int clipId, int x, int y, int w, int h)
{
    nlog(QStringLiteral("setClipTransform(clip=%1, %2 %3 %4 %5)").arg(clipId).arg(x).arg(y).arg(w).arg(h));
    if (!pCore || !pCore->currentDoc()) {
        nlog(QStringLiteral("setClipTransform: no document"));
        return false;
    }
    std::shared_ptr<TimelineItemModel> timeline = pCore->currentDoc()->getTimeline(pCore->currentTimelineId());
    if (!timeline) {
        nlog(QStringLiteral("setClipTransform: no timeline model"));
        return false;
    }
    if (!timeline->isClip(clipId)) {
        nlog(QStringLiteral("setClipTransform: %1 is not a valid clip id (likely stale after reload)").arg(clipId));
        return false;
    }
    std::shared_ptr<EffectStackModel> stack = timeline->getClipEffectStack(clipId);
    if (!stack) {
        nlog(QStringLiteral("setClipTransform: no effect stack for clip %1").arg(clipId));
        return false;
    }
    stack->setBuiltInTransform(x, y, w, h);
    nlog(QStringLiteral("setClipTransform: applied to clip %1").arg(clipId));
    return true;
}

bool KdenliveScript::resizeClip(int clipId, int durationFrames)
{
    nlog(QStringLiteral("resizeClip(clip=%1, frames=%2)").arg(clipId).arg(durationFrames));
    if (!pCore || !pCore->currentDoc()) {
        return false;
    }
    std::shared_ptr<TimelineItemModel> timeline = pCore->currentDoc()->getTimeline(pCore->currentTimelineId());
    if (!timeline || !timeline->isClip(clipId)) {
        nlog(QStringLiteral("resizeClip: invalid clip id %1").arg(clipId));
        return false;
    }
    // Resize the right edge to the requested duration (snapDistance=-1 disables snapping).
    const int result = timeline->requestItemResize(clipId, durationFrames, true, true, -1, true);
    nlog(QStringLiteral("resizeClip: requestItemResize -> %1").arg(result));
    return result > -1;
}

bool KdenliveScript::setClipFade(int clipId, int x, int y, int w, int h, int fadeInFrames, int fadeOutFrames)
{
    nlog(QStringLiteral("setClipFade(clip=%1, rect=%2 %3 %4 %5, in=%6, out=%7)").arg(clipId).arg(x).arg(y).arg(w).arg(h).arg(fadeInFrames).arg(fadeOutFrames));
    if (!pCore || !pCore->currentDoc()) {
        return false;
    }
    std::shared_ptr<TimelineItemModel> timeline = pCore->currentDoc()->getTimeline(pCore->currentTimelineId());
    if (!timeline || !timeline->isClip(clipId)) {
        nlog(QStringLiteral("setClipFade: invalid clip id %1").arg(clipId));
        return false;
    }
    const int dur = timeline->getClipPlaytime(clipId);
    if (dur < 2) {
        nlog(QStringLiteral("setClipFade: clip too short (dur=%1)").arg(dur));
        return false;
    }
    // Build a keyframed animatedrect: position/size constant, opacity ramped at the edges.
    QString kf;
    auto add = [&](int frame, double opacity) {
        if (!kf.isEmpty()) {
            kf += QLatin1Char(';');
        }
        kf += QStringLiteral("%1=%2 %3 %4 %5 %6").arg(frame).arg(x).arg(y).arg(w).arg(h).arg(opacity, 0, 'f', 3);
    };
    const int fin = qBound(0, fadeInFrames, dur - 1);
    const int fout = qBound(0, fadeOutFrames, dur - 1);
    if (fin > 0) {
        add(0, 0.0);
        add(fin, 1.0);
    } else {
        add(0, 1.0);
    }
    if (fout > 0) {
        add(qMax(fin, dur - 1 - fout), 1.0);
        add(dur - 1, 0.0);
    }
    nlog(QStringLiteral("setClipFade: rect keyframes = %1").arg(kf));
    std::shared_ptr<EffectStackModel> stack = timeline->getClipEffectStack(clipId);
    if (!stack) {
        nlog(QStringLiteral("setClipFade: no effect stack for clip %1").arg(clipId));
        return false;
    }
    stack->setBuiltInRect(kf);
    return true;
}

void KdenliveScript::renderFrame(const QString &path)
{
    nlog(QStringLiteral("renderFrame('%1')").arg(path));
    if (pCore && pCore->monitorManager() && pCore->monitorManager()->projectMonitor()) {
        pCore->monitorManager()->projectMonitor()->extractFrame(path);
        nlog(QStringLiteral("renderFrame: extractFrame called (async)"));
    } else {
        nlog(QStringLiteral("renderFrame: no project monitor"));
    }
}

void KdenliveScript::playPause()
{
    if (pCore->monitorManager()) {
        pCore->monitorManager()->slotPlay();
    }
}

void KdenliveScript::seek(int position)
{
    if (pCore->monitorManager() && pCore->monitorManager()->projectMonitor()) {
        pCore->monitorManager()->projectMonitor()->requestSeek(position);
    }
}

bool KdenliveScript::newProject(const QString &profilePath, const QString &savePath)
{
    nlog(QStringLiteral("newProject(profile='%1', save='%2')").arg(profilePath, savePath));
    if (!pCore || !pCore->projectManager()) {
        nlog(QStringLiteral("newProject: no projectManager"));
        return false;
    }
    // Create a fresh project with the requested profile, skipping the settings dialog
    // (false == don't show ProjectSettings). Equivalent to File > New + picking the preset.
    pCore->projectManager()->newFile(profilePath, false);
    if (!pCore->currentDoc()) {
        nlog(QStringLiteral("newProject: no document after newFile"));
        return false;
    }
    // Persist it to the requested path (save over any existing file, not a copy).
    const bool ok = pCore->projectManager()->saveFileAs(savePath, true, false);
    nlog(QStringLiteral("newProject: saveFileAs('%1') -> %2").arg(savePath).arg(ok));
    return ok;
}

bool KdenliveScript::save()
{
    if (pCore && pCore->projectManager()) {
        return pCore->projectManager()->saveFile();
    }
    return false;
}

void KdenliveScript::quit()
{
    // Prompt-free shutdown for automation: mark the doc unmodified so Core::clean()'s
    // close does not raise a "save changes?" dialog, then exit the event loop. main()'s
    // Core::clean() removes the crash lock, so no recovery dialog next launch.
    // Unsaved changes are intentionally discarded; call save() first if needed.
    QTimer::singleShot(0, qApp, []() {
        if (pCore && pCore->currentDoc()) {
            pCore->currentDoc()->setModified(false);
        }
        qApp->quit();
    });
}

bool KdenliveScript::triggerAction(const QString &name)
{
    if (!pCore || !pCore->window()) {
        nlog(QStringLiteral("triggerAction('%1'): no window").arg(name));
        return false;
    }
    QAction *a = pCore->window()->actionCollection()->action(name);
    if (!a) {
        nlog(QStringLiteral("triggerAction('%1'): no such action").arg(name));
        return false;
    }
    a->trigger();
    nlog(QStringLiteral("triggerAction('%1'): triggered").arg(name));
    return true;
}

bool KdenliveScript::newSequence(int videoTracks, int audioTracks)
{
    if (!pCore || !pCore->currentDoc() || !pCore->bin()) {
        nlog(QStringLiteral("newSequence: no document/bin"));
        return false;
    }
    // buildSequenceClip(aTracks, vTracks); -1 == Kdenlive's default track counts.
    pCore->bin()->buildSequenceClip(audioTracks, videoTracks);
    nlog(QStringLiteral("newSequence(v=%1, a=%2): created").arg(videoTracks).arg(audioTracks));
    return true;
}

QString KdenliveScript::openNewProjectWithProfile(const QString &profilePath)
{
    if (!pCore || !pCore->projectManager()) {
        return QStringLiteral("error");
    }
    KdenliveDoc *doc = pCore->currentDoc();
    if (doc && doc->isModified()) {
        // Never let newFile() pop a modal "save changes?" dialog (it blocks the
        // bridge). Auto-save when the project already has a path; refuse an
        // untitled+modified one so we never discard unsaved work silently.
        if (doc->url().isEmpty()) {
            nlog(QStringLiteral("openNewProjectWithProfile: current project is untitled+modified -> unsaved"));
            return QStringLiteral("unsaved");
        }
        if (!pCore->projectManager()->saveFile()) {
            nlog(QStringLiteral("openNewProjectWithProfile: auto-save of current project failed"));
            return QStringLiteral("savefailed");
        }
    }
    pCore->projectManager()->newFile(profilePath, false);
    const bool ok = pCore->currentDoc() != nullptr;
    return ok ? QStringLiteral("ok") : QStringLiteral("error");
}

QString KdenliveScript::newProjectProfile(const QString &profilePath)
{
    nlog(QStringLiteral("newProjectProfile('%1')").arg(profilePath));
    const QString res = openNewProjectWithProfile(profilePath);
    nlog(QStringLiteral("newProjectProfile -> %1").arg(res));
    return res;
}

QString KdenliveScript::newProjectFormat(int width, int height, double fps)
{
    nlog(QStringLiteral("newProjectFormat(%1x%2 @ %3fps)").arg(width).arg(height).arg(fps));
    if (width <= 0 || height <= 0 || fps <= 0) {
        return QStringLiteral("error");
    }
    // Derive frame_rate_num/den, handling common NTSC fractional rates (29.97, 23.976, 59.94).
    int fpsNum;
    int fpsDen;
    const int rounded = static_cast<int>(std::lround(fps));
    if (std::fabs(fps - rounded) < 0.005) {
        fpsNum = rounded;
        fpsDen = 1;
    } else if (std::fabs(fps - (rounded * 1000.0 / 1001.0)) < 0.02) {
        fpsNum = rounded * 1000;
        fpsDen = 1001;
    } else {
        fpsNum = static_cast<int>(std::lround(fps * 1000.0));
        fpsDen = 1000;
    }
    // Square pixels (SAR 1:1); display aspect = width:height reduced.
    const int g = std::max(1, std::gcd(width, height));
    ProfileParam param(width, height, fpsNum, fpsDen, width / g, height / g, 1, 1, 709, false);
    param.m_description = QStringLiteral("%1x%2 %3fps").arg(width).arg(height).arg(fps, 0, 'g', 6);
    // Reuse an identical existing MLT profile if there is one, else save a custom profile.
    QString path = ProfileRepository::get()->findMatchingProfile(&param);
    if (path.isEmpty()) {
        path = ProfileRepository::get()->saveProfile(&param);
    }
    if (path.isEmpty()) {
        nlog(QStringLiteral("newProjectFormat: could not resolve or save a matching profile"));
        return QStringLiteral("error");
    }
    nlog(QStringLiteral("newProjectFormat: using profile '%1'").arg(path));
    const QString res = openNewProjectWithProfile(path);
    nlog(QStringLiteral("newProjectFormat -> %1").arg(res));
    return res;
}
