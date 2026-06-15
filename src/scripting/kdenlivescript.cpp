/*
    SPDX-FileCopyrightText: Nuldrums
    SPDX-License-Identifier: GPL-3.0-or-later
*/
#include "kdenlivescript.h"

#include "bin/projectclip.h"
#include "bin/projectitemmodel.h"
#include "core.h"
#include "doc/kdenlivedoc.h"
#include "effects/effectstack/model/effectstackmodel.hpp"
#include "mainwindow.h"
#include "monitor/monitor.h"
#include "monitor/monitormanager.h"
#include "profiles/profilemodel.hpp"
#include "project/projectmanager.h"
#include "timeline2/model/timelineitemmodel.hpp"
#include "timeline2/view/timelinewidget.h"

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
    std::shared_ptr<EffectStackModel> stack = timeline->getClipEffectStack(clipId);
    if (!stack) {
        nlog(QStringLiteral("setClipTransform: no effect stack for clip %1").arg(clipId));
        return false;
    }
    stack->setBuiltInTransform(x, y, w, h);
    nlog(QStringLiteral("setClipTransform: applied to clip %1").arg(clipId));
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
