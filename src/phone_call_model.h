/*
 * Copyright 2024-2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef PHONE_CALL_MODEL_H
#define PHONE_CALL_MODEL_H

#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QVariant>
#include <QtCore/QTimer>
#include <QString>
#include <QQueue>
#include <QTime>
#include <QQuickImageProvider>
#include <QReadWriteLock>
#include <QtConcurrent>
#include <QPromise>
#include <QFuture>
#include <QFutureWatcher>
#include <QMutex>
#include <QTimer>
#include <QProcess>
#include <QtQml/qqmlregistration.h>

#include <QDebug>
#include <iostream>
#include <memory>
#include <any>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "bluetoothctl/btrfcomm.h"
#include "bluetooth_device.h"
#include "media_control_model.h"
#include "generative_chat_model.h"

class VitCtlThread: public QThread
{
    Q_OBJECT

public:
    VitCtlThread(void);
    ~VitCtlThread(void);
    void run(void) override;

Q_SIGNALS:
void sendVitStatus(bool vr, bool wake, enum player_cmd cmd_idx);

private:
    // bool active;
};

class PhoneCallModel: public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool callState MEMBER call_state NOTIFY callStateChanged)
    Q_PROPERTY(bool acceptState MEMBER accept_state NOTIFY acceptStateChanged)
    Q_PROPERTY(QString contactName MEMBER contact_name NOTIFY callStateChanged)
    Q_PROPERTY(QString phoneNumber MEMBER phone_number NOTIFY callStateChanged)

    Q_PROPERTY(bool helmetInterconState MEMBER helmet_intercon_state NOTIFY helmetInterconStateChanged)
    Q_PROPERTY(bool helmetInterconCommandState MEMBER helmet_intercon_state_change NOTIFY helmetInterconCommandStateChanged)

    Q_PROPERTY(bool voiceState MEMBER voice_state NOTIFY voiceStateChanged)
    Q_PROPERTY(bool voiceCommandState MEMBER voice_state_change NOTIFY voiceCommandStateChanged)
    Q_PROPERTY(bool busyState MEMBER voice_wake NOTIFY voiceWakeChanged)

    //Q_PROPERTY(bool chatState MEMBER chat_state NOTIFY chatStateChanged)
    Q_PROPERTY(bool chatCommandState MEMBER chat_state_change NOTIFY chatCommandStateChanged)

    QML_ELEMENT
    QML_SINGLETON

public:
    static PhoneCallModel* instance(void);
    ~PhoneCallModel(void);

    Q_INVOKABLE void accept(void);
    Q_INVOKABLE void reject(void);

    Rfcomm* src_rfcomm;

Q_SIGNALS:
    void callStateChanged();
    void acceptStateChanged(bool b);
    void helmetInterconStateChanged(bool b);
    void helmetInterconCommandStateChanged(bool b);
    void voiceStateChanged(bool b);
    void voiceCommandStateChanged(bool b);
    void voiceWakeChanged(bool b);
    //void chatStateChanged(bool b);
    void chatCommandStateChanged(bool b);

public Q_SLOTS:
    void setCallState(bool b);
    void setAcceptState(bool b);

    // FIXME: this is not setter
    void setVitState(bool vr, bool wake, enum player_cmd cmd_idx);

    // qml control
    void controlHelmetInterconState(bool b);
    void controlVoiceState(bool b);
    void controlChatState(bool b);

    // debug slot
    void testSignal(bool b);

private:
    explicit PhoneCallModel(QObject* parent = nullptr);

    // phone call
    bool phonecall_mode;
    bool call_state;
    bool accept_state;
    QString contact_name, phone_number;

    // intercom, signal must be emitted manually when changed
    bool helmet_intercon_state;
    bool helmet_intercon_state_change;

    // voice recognition, signal must be emitted manually when changed
    bool voice_state;
    bool voice_state_change;
    bool voice_wake;
    VitCtlThread vit_handler;
    enum player_cmd media_cmd;

    // bool chat_state; // use "GenerativeChatModel::instance()->getModelActive()" instead
    bool chat_state_change;

    QFuture<std::any> fu_notify;
    QFutureWatcher<std::any> fw_notify;
    void notifyHandlerStart(void);

    void intoPhonecallMode(void);
    void exitPhonecallMode(void);
    void intoInterconMode(void);
    void exitInterconMode(void);
    void setPipewireSourceId(void);
    void setPipewireSinkId(void);
    void startVR(void);
    void exitVR(void);
    void media_execute(enum player_cmd cmd);

    bool phoneCallModeCheck(void);
    void phoneCallModeSet(void);
    QString stringfilter(QString str);

    enum call_cmd call_command;
    QQueue<enum call_cmd> command_q;
    QProcess command_process,vit_process;
    void enqueueCommand(enum call_cmd cmd);
    void sendCommand(enum call_cmd cmd);

};

#endif // PHONE_CALL_MODEL_H
