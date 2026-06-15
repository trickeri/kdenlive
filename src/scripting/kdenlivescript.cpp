/*
    SPDX-FileCopyrightText: Nuldrums
    SPDX-License-Identifier: GPL-3.0-or-later
*/
#include "kdenlivescript.h"

#include "core.h"
#include "doc/kdenlivedoc.h"
#include "mainwindow.h"
#include "monitor/monitor.h"
#include "monitor/monitormanager.h"
#include "profiles/profilemodel.hpp"
#include "project/projectmanager.h"

#include <QApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSize>
#include <QTimer>
#include <QUrl>

KdenliveScript::KdenliveScript(QObject *parent)
    : QObject(parent)
{
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
    if (pCore->window()) {
        pCore->window()->addProjectClip(path);
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
    // Prompt-free shutdown for automation: exit the event loop directly. main()'s
    // Core::clean() then runs and removes the crash lock, so no recovery dialog next launch.
    // Unsaved changes are intentionally not auto-saved; call save() first if needed.
    QTimer::singleShot(0, qApp, []() { qApp->quit(); });
}
