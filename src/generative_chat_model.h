/*
 * Copyright 2024-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef GENERATIVE_CHAT_MODEL
#define GENERATIVE_CHAT_MODEL

#include <QObject>
#include <QtQml/qqmlregistration.h>

#include <QVariant>
#include <QList>
#include <QTimer>
#include <QString>
#include <QByteArray>
#include <QQueue>

#include <QDateTime>
#include <QDebug>
#include <QQuickImageProvider>

#include <QReadWriteLock>
#include <QtConcurrent>
#include <QPromise>
#include <QFuture>
#include <QFutureWatcher>
#include <QMutex>
#include <QMutexLocker>
#include <QTimer>
#include <QProcess>

#include <iostream>
#include <memory>
#include <any>
#include <functional>

#include <fcntl.h>
#include <mqueue.h>
#include <sys/epoll.h>
#include <unistd.h>

#include <egf_api.h>

#include "dms.h"
#include "telltales_model.h"

class GenerativeChatRunner : public QThread
{
    Q_OBJECT

public:
    GenerativeChatRunner();
    ~GenerativeChatRunner();

    void run(void) override;
    void setState(bool b);
    bool isConnected(void);

    // void startInferenceProcess(void);
    void startInferenceProcessDetatched(void);

    void enqueueCommand(QByteArray const &cmd);

signals:
    void connectedChanged(bool b);
    void retryRequest(void);
    void messageReceived(QByteArray message);

private:

    // base control
    QMutex stop_mutex;
    bool stop_signal;

    EGF::EgfConnector egf_connector;

    bool is_connected;
    void setIsConnected(bool b);
    void connectQueue(void);

    bool getStopSignal(void);
    void sendStopSignal(void);

    std::string FindFirstExistingProgram();
    std::string FindFirstExistingWorkDir();

    // llm process
    EGF::EgfProcess egf_process;
    QTimer timer;

private Q_SLOTS:
    void onRetryRequest(void);
    void onTimerOverflow(void);
};

class GenerativeChatMessage: public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool fromSelf MEMBER from_self NOTIFY metadataChanged)
    Q_PROPERTY(bool isTyping MEMBER is_typing NOTIFY isTypingChanged)
    Q_PROPERTY(QString userName MEMBER user_name NOTIFY metadataChanged)
    Q_PROPERTY(QDateTime timeStamp MEMBER time_stamp NOTIFY timeStampChanged)
    Q_PROPERTY(QString message MEMBER message NOTIFY messageChanged)

    QML_ANONYMOUS

public:
    GenerativeChatMessage(bool _from_self, const QString &_user_name);

    GenerativeChatMessage& operator << (const QString &s);

    void setIsTyping(bool b);
    void setTimeStamp(const QDateTime &t);
    void editMessage(std::function<void (QString&)> f);
    void setMessage(const QString &s);
    void appendMessage(const QString &s);
    bool isTyping(void);
    bool isFromSelf(void);

signals:
    void metadataChanged(void);
    void isTypingChanged(void);
    void timeStampChanged(void);
    void messageChanged(void);

private:
    /* forbid detault */
    GenerativeChatMessage(void) = delete;
    /* forbid copy */
    GenerativeChatMessage(const GenerativeChatMessage &s) = delete;

    QMutex lock;

    bool from_self;
    bool is_typing;
    QString user_name;
    QDateTime time_stamp;
    QString message;
};

class ControlBoxPageModel: public QObject
{
Q_OBJECT
    Q_PROPERTY(bool pageCmdPending MEMBER page_cmd_pending NOTIFY pageCmdPendingChanged)
    Q_PROPERTY(int pageCmd MEMBER page_cmd NOTIFY pageCmdChanged)

public:
    ControlBoxPageModel(void) = default;
    ~ControlBoxPageModel(void) = default;

    static ControlBoxPageModel* instance();
    void setPageCommand(int cmd);
    int setPageCommandAndWait(int cmd, uint32_t timeout_ms);

Q_SIGNALS:
    void pageCmdPendingChanged(bool b);
    void pageCmdChanged(int cmd);

private:
    bool page_cmd_pending; // set from cpp, clear from qml
    int page_cmd; // page index to be switched to
};

class GenerativeChatModel: public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool modelConnection READ getModelConnected NOTIFY modelConnectionChanged)
    Q_PROPERTY(bool modelLoadReady MEMBER model_load_ready NOTIFY modelLoadReadyChanged)
    Q_PROPERTY(bool modelActive MEMBER model_active_state NOTIFY modelActiveChanged)
    Q_PROPERTY(bool userTyping MEMBER user_typing NOTIFY userTypingChanged)
    Q_PROPERTY(bool modelGenerating MEMBER model_generating NOTIFY modelGeneratingChanged)
    Q_PROPERTY(QList<GenerativeChatMessage *> messagesList READ getMessages NOTIFY messagesUpdated)
    Q_PROPERTY(bool dmsConnected READ getDmsConnected NOTIFY dmsConnectedChanged)
    Q_PROPERTY(bool dmsPersonDetected READ getDmsPersonDetected NOTIFY dmsPersonDetectedChanged)
    Q_PROPERTY(bool dmsWarning READ getDmsWarning NOTIFY dmsWaringChanged)

    QML_ELEMENT
    QML_SINGLETON

public:

    struct VoiceCommand {
        QByteArray key;
        std::function<void(QPromise<QByteArray> &promise, QByteArray const &dat)> handler;
    };

    ~GenerativeChatModel(void);
    static GenerativeChatModel* instance();

    void addMessage(GenerativeChatMessage *msg);
    GenerativeChatMessage *latestMessage(void);

    bool getModelConnected(void);
    bool getModelActive(void);

    void startVisionService(void);
    void stoptVisionService(void);

Q_SIGNALS:
    void modelConnectionChanged(bool b);
    void modelLoadReadyChanged(bool b);
    void modelActiveChanged(bool b);
    void userTypingChanged(bool b);
    void modelGeneratingChanged(bool b);
    void messagesUpdated(void);
    void dmsConnectedChanged(bool);
    void dmsPersonDetectedChanged(bool);
    void dmsWaringChanged(bool);

public Q_SLOTS:
    void controlModelActive(bool b);

private:

    explicit GenerativeChatModel(QObject* parent = nullptr);

    QList<GenerativeChatMessage *> getMessages();

    QMutex lock;

    QList<GenerativeChatMessage *> messages;

    // LLM
    struct LlmState {
        std::string name;
        std::function<void(QByteArray const &hdr, QByteArray const &dat)> onEnter;
        std::function<void(QByteArray const &hdr, QByteArray const &dat)> onExit;
        std::function<void(QByteArray const &hdr, QByteArray const &dat)> processMessage;
    };
    LlmState const *llm_chat_prev_status, *llm_chat_status, *llm_chat_next_status;
    QMutex llm_chat_status_mutex;
    void setLlmChatNextStatus(LlmState const *new_status);
    void switchLlmChatStatus(LlmState const *new_status, QByteArray const &hdr, QByteArray const &dat);
    void switchLlmChatStatus(LlmState const *new_status);

    LlmState const state_runner_stopped;
    LlmState const state_runner_started;
    LlmState const state_model_paused;
    LlmState const state_model_req_pause;
    LlmState const state_model_req_resume;
    LlmState const state_ready_idle;
    LlmState const state_user_typing;
    LlmState const state_llm_typing;
    LlmState const state_llm_speaking;
    LlmState const state_command_executing;

    std::unordered_map<QByteArray, VoiceCommand const&> voice_command_map;
    VoiceCommand const *active_command;
    QFuture<QByteArray> fu_command;
    QFutureWatcher<QByteArray> fw_command;

    GenerativeChatRunner llm_runner;
    bool model_load_ready; // is "<ready>" received
    bool model_active_state; // is chat ui visible
    bool user_typing;
    bool model_generating;
    void processNewMessage(QByteArray msg);

    // vision ml task
    QProcess vision_ml_p;

    // vision dms thread
    DMS dms_runner;
    bool getDmsConnected(void);
    bool getDmsPersonDetected(void);
    bool getDmsWarning(void);
};

#endif /** GENERATIVE_CHAT_MODEL */
