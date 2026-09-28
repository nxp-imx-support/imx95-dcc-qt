/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef KILL_ME_MODEL
#define KILL_ME_MODEL

#include <QList>
#include <QObject>
#include <QVariant>
#include <QProcess>
#include <QtEnvironmentVariables>

#include <QtQml/qqmlregistration.h>

#include <QDebug>
#include <iostream>

class KillMeModel: public QObject
{
    Q_OBJECT

    QML_ELEMENT
    QML_SINGLETON

public:
    static KillMeModel* instance(void);
    ~KillMeModel(void);

    Q_INVOKABLE void killMe(void);

private:
    explicit KillMeModel(QObject* parent = nullptr);

    QProcess runner;
};

#endif // KILL_ME_MODEL
