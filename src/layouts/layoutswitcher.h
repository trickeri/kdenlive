/*
SPDX-FileCopyrightText: 2012 Jean-Baptiste Mardelle <jb@kdenlive.org>
SPDX-FileCopyrightText: 2014 Till Theato <root@ttill.de>
SPDX-FileCopyrightText: 2020 Julius Künzel <julius.kuenzel@kde.org>
SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include <QComboBox>
#include <QList>
#include <QPair>
#include <QString>
#include <QWidget>

/** @brief A compact dropdown for switching between workspace layouts.
 *  (Replaces the former row of push-buttons with a single styled combo box.) */
class LayoutSwitcher : public QWidget
{
    Q_OBJECT
public:
    explicit LayoutSwitcher(QWidget *parent = nullptr);
    void setLayouts(const QList<QPair<QString, QString>> &layouts, const QString &currentLayout = QString());
    QString currentLayout() const;
    void setCurrentLayout(const QString &layoutId);

Q_SIGNALS:
    void layoutSelected(const QString &layoutName);

private:
    QComboBox *m_combo;
    QString m_currentLayoutId;
};
