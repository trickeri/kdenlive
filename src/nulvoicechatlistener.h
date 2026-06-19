/*
    Nuldrums: voicechat transcript listener.
    SPDX-FileCopyrightText: none
    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include <QByteArray>
#include <QObject>

class QLocalSocket;
class QTimer;

/** @brief Listens on the voicechat dictation daemon's transcript socket and emits a signal
 * for each finished transcript.
 *
 * voicechat (the headless dictation daemon) broadcasts every finished transcript on a Unix
 * domain socket — `$VOICECHAT_SOCKET`, else `$XDG_RUNTIME_DIR/voicechat.sock` — as one JSON
 * line per transcript: `{"text":"…","app":"…","mode":"…","ts":…}`. When Kdenlive is the
 * focused app, voicechat uses its "emit" mode: it does NOT synthesize a paste, expecting us
 * to consume the text here instead.
 *
 * This listener connects to that socket, reconnects automatically if voicechat isn't running
 * yet or restarts, parses the newline-delimited JSON, and emits transcriptReceived() on the
 * GUI thread for each transcript. Wire that signal to whatever should happen with dictated
 * text (see MainWindow::handleVoiceTranscript). It is read-only and best-effort: if the
 * socket is unavailable the listener simply keeps retrying and Kdenlive is unaffected.
 */
class NulVoiceChatListener : public QObject
{
    Q_OBJECT
public:
    explicit NulVoiceChatListener(QObject *parent = nullptr);

    /** @brief The transcript socket path: `$VOICECHAT_SOCKET`, else `$XDG_RUNTIME_DIR/voicechat.sock`. */
    static QString socketPath();

Q_SIGNALS:
    /** @brief Emitted for each transcript. @p mode is voicechat's routing mode (e.g. "emit"). */
    void transcriptReceived(const QString &text, const QString &app, const QString &mode);

private Q_SLOTS:
    void onReadyRead();
    void scheduleReconnect();
    void tryConnect();

private:
    QLocalSocket *m_socket;
    QTimer *m_reconnect;
    QByteArray m_buffer;
};
