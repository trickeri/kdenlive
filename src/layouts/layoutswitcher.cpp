/*
SPDX-FileCopyrightText: 2012 Jean-Baptiste Mardelle <jb@kdenlive.org>
SPDX-FileCopyrightText: 2014 Till Theato <root@ttill.de>
SPDX-FileCopyrightText: 2020 Julius Künzel <julius.kuenzel@kde.org>
SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "layouts/layoutswitcher.h"
#include <QHBoxLayout>
#include <QSignalBlocker>

LayoutSwitcher::LayoutSwitcher(QWidget *parent)
    : QWidget(parent)
    , m_combo(new QComboBox(this))
{
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_combo);

    m_combo->setFocusPolicy(Qt::NoFocus);
    // Styled like the timeline mode dropdown, but with a rounded purple border.
    m_combo->setStyleSheet(QStringLiteral("QComboBox{border:1px solid #a100ff;border-radius:6px;padding:2px 6px 2px 10px;color:#00f8fc;background:#171719;}"
                                          "QComboBox:hover{border-color:#c04dff;}"
                                          "QComboBox::drop-down{border:none;width:22px;subcontrol-origin:padding;subcontrol-position:center right;}"
                                          "QComboBox QAbstractItemView{border:1px solid #a100ff;background:#171719;color:#00f8fc;"
                                          "selection-background-color:#a100ff;selection-color:#ffffff;outline:none;}"));

    connect(m_combo, QOverload<int>::of(&QComboBox::activated), this, [this](int index) {
        if (index < 0) {
            return;
        }
        m_currentLayoutId = m_combo->itemData(index).toString();
        Q_EMIT layoutSelected(m_currentLayoutId);
    });
}

void LayoutSwitcher::setLayouts(const QList<QPair<QString, QString>> &layouts, const QString &currentLayout)
{
    QSignalBlocker blocker(m_combo);
    m_combo->clear();
    for (const auto &pair : layouts) {
        // userData holds the internal layout id, display text is the label
        m_combo->addItem(pair.second, pair.first);
    }
    if (!currentLayout.isEmpty()) {
        const int idx = m_combo->findData(currentLayout);
        if (idx >= 0) {
            m_combo->setCurrentIndex(idx);
            m_currentLayoutId = currentLayout;
        }
    }
}

QString LayoutSwitcher::currentLayout() const
{
    return m_combo->currentData().toString();
}

void LayoutSwitcher::setCurrentLayout(const QString &layoutId)
{
    QSignalBlocker blocker(m_combo);
    int idx = m_combo->findData(layoutId);
    if (idx < 0) {
        // Not a preset (empty/unknown id) => fall back to the "Custom" entry if present.
        idx = m_combo->findData(QStringLiteral("__custom__"));
    }
    if (idx >= 0) {
        m_combo->setCurrentIndex(idx);
        m_currentLayoutId = m_combo->itemData(idx).toString();
    } else {
        m_combo->setCurrentIndex(-1);
        m_currentLayoutId.clear();
    }
}
