/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "generative_chat_model.h"

#include <QLoggingCategory>
Q_LOGGING_CATEGORY(lc_gen_chat, "model.generative_chat")

GenerativeChatRunner::GenerativeChatRunner() :
    egf_process(EGF::EgfProcess::Config {
        .work_dir = FindFirstExistingWorkDir(),
        .prg_name = FindFirstExistingProgram(),
        .prg_args = "eiq_genai_flow.py --capture-device plughw:CARD=btscoaudio --playback-device plughw:CARD=btscoaudio --use-rag -i chat_interface",
        .lockfile = "/tmp/egf_process.lock",
    }),
    egf_connector(EGF::EgfConnector::Config::getDefault()),
    stop_signal(false),
    is_connected(false)
{
    timer.setSingleShot(true);
    timer.setInterval(5000);
    QObject::connect(&timer, &QTimer::timeout, this, &GenerativeChatRunner::onTimerOverflow);
    QObject::connect(this, &GenerativeChatRunner::retryRequest, this, &GenerativeChatRunner::onRetryRequest);
}

GenerativeChatRunner::~GenerativeChatRunner(void)
{
    sendStopSignal();
    wait();
}

bool GenerativeChatRunner::isConnected(void)
{
    return is_connected;
}

void GenerativeChatRunner::setIsConnected(bool b)
{
    if (is_connected == b){
        return;
    }
    is_connected = b;
    emit connectedChanged(is_connected);
}

void GenerativeChatRunner::connectQueue(void)
{
    setIsConnected(false);
    qCDebug(lc_gen_chat) << "chat runner trying to connect to mqueue";

    if (egf_connector.connect() == 0) {
        qCDebug(lc_gen_chat) << "chat runner mqueue connected";
        setIsConnected(true);
    } else {
        qCDebug(lc_gen_chat) << "chat runner failed to connect to mqueue";
    }
}

void GenerativeChatRunner::run(void)
{
    int32_t ret = 0;
    struct timespec mq_timeout;

    auto state_cleaner = [this](GenerativeChatRunner* sp) {
        if (isConnected()) {
            qCDebug(lc_gen_chat) << "chat runner close mq on exit";
            egf_connector.disconnect();
        }
        setIsConnected(false);
        if (!getStopSignal()) {
            emit retryRequest(); // start timer to trigger retry
        }
        stop_signal = false;
    };
    std::unique_ptr<GenerativeChatRunner, decltype(state_cleaner)>
        state_guard(this, state_cleaner);

    connectQueue();
    if (!isConnected()) {
        return;
    }

    egf_connector.registerRawCallback([this](const std::string_view &message) {
        QByteArray msg(message.data(), message.size());
        qCDebug(lc_gen_chat) << "runner message received: " << msg;
        emit messageReceived(msg);
    });

    qCDebug(lc_gen_chat) << "chat runner listening on queue";
    while (!(getStopSignal() && egf_connector.isSendQueueEmpty())) {
        egf_connector.handler();
    }
}

void GenerativeChatRunner::setState(bool b)
{
    if (b) {
        if (b == isRunning() || b == timer.isActive())
            return;
        else
            start();
    } else {
        if (b == isRunning())
            sendStopSignal();
        else if (b == timer.isActive())
            timer.stop();
    }
}

bool GenerativeChatRunner::getStopSignal(void)
{
    stop_mutex.lock();
    bool b = stop_signal;
    stop_mutex.unlock();
    return b;
}

void GenerativeChatRunner::sendStopSignal(void)
{
    stop_mutex.lock();
    stop_signal = true;
    stop_mutex.unlock();
}

std::string GenerativeChatRunner::FindFirstExistingProgram() {
    std::vector<std::string> programs =
    {
        "/usr/share/dcc/.venv/bin/python3",
        "/usr/share/dcc/miniconda3/envs/venv/bin/python3"
    };

    namespace fs = std::filesystem;

    for (const auto& program : programs)
    {
        if (fs::exists(program) && fs::is_regular_file(program))
        {
            return program;
        }
    }

    return "";
}

std::string GenerativeChatRunner::FindFirstExistingWorkDir() {
    return "/usr/share/dcc/dm-eiq-genai-flow-demonstrator";
}

void GenerativeChatRunner::onRetryRequest(void)
{
    qCDebug(lc_gen_chat) << "chat runner retry request";
    if (!egf_process.isStarted()) {
        qCDebug(lc_gen_chat) << "chat runner retry: starting process";
        startInferenceProcessDetatched();
    } else {
        qCDebug(lc_gen_chat) << "chat runner retry: process already started";
    }
    timer.start();
}

void GenerativeChatRunner::onTimerOverflow(void)
{
    qCDebug(lc_gen_chat) << "LLM queue timer timeout";
    start();
}

void GenerativeChatRunner::enqueueCommand(QByteArray const &cmd)
{
    qCDebug(lc_gen_chat) << "chat runner enqueue command:" << cmd;
    egf_connector.sendCommand(cmd.toStdString());
}

void GenerativeChatRunner::startInferenceProcessDetatched(void)
{
    int ret = egf_process.start();
    if (ret == 0) {
        qCDebug(lc_gen_chat) << "LLM process started";
    } else {
        qCDebug(lc_gen_chat) << "LLM process failed to start";
    }
}

GenerativeChatMessage::GenerativeChatMessage(bool _from_self, const QString &_user_name) :
    QObject(nullptr),
    from_self(_from_self),
    is_typing(false),
    user_name(_user_name),
    time_stamp(QDateTime::currentDateTime()),
    message("")
{

}

void GenerativeChatMessage::setIsTyping(bool b)
{
    if (is_typing == b) {
        return;
    }
    is_typing = b;
    emit isTypingChanged();
}

void GenerativeChatMessage::setTimeStamp(const QDateTime &t)
{
    if (time_stamp == t) {
        return;
    }
    time_stamp = t;
    emit timeStampChanged();
}

void GenerativeChatMessage::editMessage(std::function<void (QString&)> f)
{
    if (lock.tryLock()) {
        f(message);
        lock.unlock();
        emit messageChanged();
    } else {
        qCWarning(lc_gen_chat) <<"message is taken by someone else";
    }
}

void GenerativeChatMessage::setMessage(const QString &s)
{
    editMessage([=](QString &m){
        m = s;
    });
}

void GenerativeChatMessage::appendMessage(const QString &s)
{
    editMessage([=](QString &m){
        m.append(s);
    });
}

GenerativeChatMessage& GenerativeChatMessage::operator << (const QString &s)
{
    appendMessage(s);
    return *this;
}

bool GenerativeChatMessage::isTyping(void)
{
    return is_typing;
}

bool GenerativeChatMessage::isFromSelf(void)
{
    return from_self;
}

ControlBoxPageModel* ControlBoxPageModel::instance()
{
    static std::unique_ptr<ControlBoxPageModel> s_instance = nullptr;
    if(!s_instance)
    {
        s_instance.reset(new ControlBoxPageModel);
    }
    return s_instance.get();
}

void ControlBoxPageModel::setPageCommand(int page)
{
    page_cmd = page;
    emit pageCmdChanged(page_cmd);
    page_cmd_pending = true;
    emit pageCmdPendingChanged(page_cmd_pending);
}

int ControlBoxPageModel::setPageCommandAndWait(int cmd, uint32_t timeout_ms)
{
    setPageCommand(cmd);
    QEventLoop loop;
    int ret = -1;
    QObject::connect(this, &ControlBoxPageModel::pageCmdPendingChanged,
    &loop, [&](bool v) {
        if (!v) {
            // Quit the loop when we get false
            ret = 0;
            loop.quit();
        }
    });

    // Optional timeout for robustness, UINT32_MAX for unlimited
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, &loop, [&]{
        loop.quit();
    });
    if (timeout_ms != UINT32_MAX) {
        timer.start(timeout_ms);
    }
    loop.exec();
    return ret;
}

GenerativeChatModel::VoiceCommand const voice_command_media_control = {
    .key = QByteArray("SW_MUSIC"),
    .handler = [](QPromise<QByteArray> &promise, QByteArray const &dat) {
        auto control = ControlBoxPageModel::instance();
        int ret = control->setPageCommandAndWait(0, 500);
        if (ret == 0) {
            promise.addResult(QByteArray("<ok>"));
        } else {
            promise.addResult(QByteArray("<err>"));
        }
    },
};
GenerativeChatModel::VoiceCommand const voice_command_interconnect = {
    .key = QByteArray("SW_HELMET"),
    .handler = [](QPromise<QByteArray> &promise, QByteArray const &dat) {
        auto control = ControlBoxPageModel::instance();
        int ret = control->setPageCommandAndWait(1, 500);
        if (ret == 0) {
            promise.addResult(QByteArray("<ok>"));
        } else {
            promise.addResult(QByteArray("<err>"));
        }
    },
};

GenerativeChatModel::GenerativeChatModel(QObject* parent) :
    QObject(parent),
    messages(0),
    state_runner_stopped({
        .name = "stopped",
        .onEnter = [this](QByteArray const &hdr, QByteArray const &dat) {
            model_load_ready = false;
            emit modelLoadReadyChanged(model_load_ready);
            user_typing = false;
            emit userTypingChanged(user_typing);
            model_generating = false;
            emit modelGeneratingChanged(model_generating);
        },
        .onExit = [this](QByteArray const &hdr, QByteArray const &dat) {},
        .processMessage = [this](QByteArray const &hdr, QByteArray const &dat) {},
    }),
    state_runner_started({
        .name = "model runner started",
        .onEnter = [this](QByteArray const &hdr, QByteArray const &dat) {
            qCDebug(lc_gen_chat) << "waiting for llm model to load";
            model_load_ready = false;
            emit modelLoadReadyChanged(model_load_ready);
            llm_runner.enqueueCommand("CTL:STATE?");
        },
        .onExit = [this](QByteArray const &hdr, QByteArray const &dat) {
            qCDebug(lc_gen_chat) << "llm model loading finished";
            model_load_ready = true;
            emit modelLoadReadyChanged(model_load_ready);
        },
        .processMessage = [this](QByteArray const &hdr, QByteArray const &dat) {
            if ((hdr == QByteArray("CTL"))) {
                if (dat == QByteArray("<ready>")) {
                    qCDebug(lc_gen_chat) << "llm model ready";
                    if (!model_active_state) {
                        setLlmChatNextStatus(&state_model_req_pause);
                    } else {
                        setLlmChatNextStatus(&state_ready_idle);
                    }
                } else if (dat == QByteArray("<paused>")) {
                    qCDebug(lc_gen_chat) << "llm model paused";
                    setLlmChatNextStatus(&state_model_paused);
                    if (model_active_state) {
                        setLlmChatNextStatus(&state_model_req_resume);
                    } else {
                        setLlmChatNextStatus(&state_model_paused);
                    }
                } else if (dat == QByteArray("<loading>")) {
                    qCDebug(lc_gen_chat) << "llm model is loading, waiting for <ready> or <paused>";
                } else {
                    llm_runner.enqueueCommand("CTL:STATE?");
                }
            } else {
                qCDebug(lc_gen_chat) << "loading: unknown message";
            }
        }
    }),
    state_model_paused({
        .name = "model paused",
        .onEnter = [this](QByteArray const &hdr, QByteArray const &dat) {},
        .onExit = [this](QByteArray const &hdr, QByteArray const &dat) {},
        .processMessage = [this](QByteArray const &hdr, QByteArray const &dat) {},
    }),
    state_model_req_pause({
        .name = "model requesting pause",
        .onEnter = [this](QByteArray const &hdr, QByteArray const &dat) {
            qCDebug(lc_gen_chat) << "requesting llm model pause";
            llm_runner.enqueueCommand("CTL:PAUSE");
        },
        .onExit = [this](QByteArray const &hdr, QByteArray const &dat) {},
        .processMessage = [this](QByteArray const &hdr, QByteArray const &dat) {
            if (hdr == QByteArray("CTL") && dat == QByteArray("<ok>")) {
                qCDebug(lc_gen_chat) << "llm model paused";
                setLlmChatNextStatus(&state_model_paused);
            } else {
                qCDebug(lc_gen_chat) << "Unknown message received, retry";
                qCDebug(lc_gen_chat) << hdr << ":" << dat;
                llm_runner.enqueueCommand("CTL:PAUSE");
            }
        },
    }),
    state_model_req_resume({
        .name = "model requesting resume",
        .onEnter = [this](QByteArray const &hdr, QByteArray const &dat) {
            qCDebug(lc_gen_chat) << "requesting llm model resume";
            llm_runner.enqueueCommand("CTL:RESUME");
        },
        .onExit = [this](QByteArray const &hdr, QByteArray const &dat) {},
        .processMessage = [this](QByteArray const &hdr, QByteArray const &dat) {
            if (hdr == QByteArray("CTL") && dat == QByteArray("<ok>")) {
                qCDebug(lc_gen_chat) << "llm model resumed";
                setLlmChatNextStatus(&state_ready_idle);
            } else {
                qCDebug(lc_gen_chat) << "Unknown message received, retry";
                llm_runner.enqueueCommand("CTL:RESUME");
            }
        },
    }),
    state_ready_idle({
        .name = "model ready idle",
        .onEnter = [this](QByteArray const &hdr, QByteArray const &dat) {
            if (user_typing) {
                user_typing = false;
                emit userTypingChanged(user_typing);
            }
            if (model_generating) {
                model_generating = false;
                emit modelGeneratingChanged(model_generating);
            }
        },
        .onExit = [this](QByteArray const &hdr, QByteArray const &dat) {
        },
        .processMessage = [this](QByteArray const &hdr, QByteArray const &dat) {
            if (hdr == QByteArray(EGF::Header::WAKEWORD)) {
                qCDebug(lc_gen_chat) << "llm model user typing";
                setLlmChatNextStatus(&state_user_typing);
            // TODO: add this to handle start up message
            //} else if (hdr == QByteArray(EGF::Header::RESPONSE) || hdr == QByteArray(EGF::Header::COMMAND)) {
            //    qCDebug(lc_gen_chat) << "llm model llm typing";
            //    llm_chat_next_status = &state_llm_typing;
            //    return;
            } else {
                qCDebug(lc_gen_chat) << "idle: unknown message";
            }
        },
    }),
    state_user_typing({
        .name = "user typing",
        .onEnter = [this](QByteArray const &hdr, QByteArray const &dat) {
            addMessage(new GenerativeChatMessage(true, "You"));
            latestMessage()->setIsTyping(true);
            user_typing = true;
            emit userTypingChanged(user_typing);
        },
        .onExit = [this](QByteArray const &hdr, QByteArray const &dat) {
            if (latestMessage()->isFromSelf()) {
                latestMessage()->setIsTyping(false);
                latestMessage()->setTimeStamp(QDateTime::currentDateTime());
            }
            user_typing = false;
            emit userTypingChanged(user_typing);
        },
        .processMessage = [this](QByteArray const &hdr, QByteArray const &dat) {
            if (hdr == QByteArray(EGF::Header::QUESTION) && dat == QByteArray(EGF::Signal::END)) {
                qCDebug(lc_gen_chat) << "user typing end detected";
                // assume llm will always respond, move to llm_typing
                setLlmChatNextStatus(&state_llm_typing);
            } else if (hdr == QByteArray(EGF::Header::QUESTION)) {
                latestMessage()->appendMessage(dat);
            } else {
                qCDebug(lc_gen_chat) << "user_typing: unknown message";
            }
        },
    }),
    state_llm_typing({
        .name = "llm typing",
        .onEnter = [this](QByteArray const &hdr, QByteArray const &dat) {
            if (llm_chat_next_status == &state_command_executing) {
                // Reuse message from voice command
                latestMessage()->setMessage("");
                return;
            }

            addMessage(new GenerativeChatMessage(false, "AI Assistant"));
            latestMessage()->setIsTyping(true);
            model_generating = true;
            emit modelGeneratingChanged(model_generating);
        },
        .onExit = [this](QByteArray const &hdr, QByteArray const &dat) {
            if (llm_chat_next_status == &state_command_executing) {
                // Leave message open for voice command
                return;
            }

            if (!latestMessage()->isFromSelf()) {
                latestMessage()->setIsTyping(false);
                latestMessage()->setTimeStamp(QDateTime::currentDateTime());
            }
        },
        .processMessage = [this](QByteArray const &hdr, QByteArray const &dat) {
            if (hdr == QByteArray(EGF::Header::RESPONSE) && dat == QByteArray(EGF::Signal::END)) {
                qCDebug(lc_gen_chat) << "llm typing end detected";
                setLlmChatNextStatus(&state_llm_speaking);
            } else if (hdr == QByteArray(EGF::Header::RESPONSE)) {
                latestMessage()->appendMessage(dat);
            } else if (hdr == QByteArray(EGF::Header::COMMAND)) {
                qCDebug(lc_gen_chat) << "llm command detected";
                setLlmChatNextStatus(&state_command_executing);
            } else if (hdr == QByteArray(EGF::Header::TTS_HAS_FINISH)) {
                qCDebug(lc_gen_chat) << "llm speaking end detected";
                setLlmChatNextStatus(&state_ready_idle);
            } else {
                qCDebug(lc_gen_chat) << "llm_typing: unknown message";
            }
        },
    }),
    state_llm_speaking({
        .name = "llm_speaking",
        .onEnter = [this](QByteArray const &hdr, QByteArray const &dat) {
        },
        .onExit = [this](QByteArray const &hdr, QByteArray const &dat) {
            model_generating = false;
            emit modelGeneratingChanged(model_generating);
        },
        .processMessage = [this](QByteArray const &hdr, QByteArray const &dat) {
            if (hdr == QByteArray(EGF::Header::TTS_HAS_FINISH)) {
                qCDebug(lc_gen_chat) << "llm speaking end detected";
                setLlmChatNextStatus(&state_ready_idle);
            } else {
                qCDebug(lc_gen_chat) << "llm_speaking: unknown message";
            }
        },
    }),
    state_command_executing({
        .name = "command_executing",
        .onEnter = [this](QByteArray const &hdr, QByteArray const &dat) {
            // Process voice command
            auto it = voice_command_map.find(dat);
            if (it == voice_command_map.end()) {
                qCWarning(lc_gen_chat) << "voice command" << dat << "not found";
                latestMessage()->appendMessage("Command not found.");
                llm_runner.enqueueCommand(QByteArray(EGF::Header::COMMAND) + QByteArray(":") + QByteArray("<err>"));
                setLlmChatNextStatus(&state_ready_idle);
                return;
            }

            if (llm_chat_prev_status == &state_llm_typing) {
                latestMessage()->setMessage("Executing command...");
            }

            active_command = &(it->second);
            fu_command = QtConcurrent::task([=](QPromise<QByteArray> &promise) {
                active_command->handler(promise, dat);
            }).spawn();
            fw_command.setFuture(fu_command);
        },
        .onExit = [this](QByteArray const &hdr, QByteArray const &dat) {
            // Close message that is left open by state_llm_typing
            if (!latestMessage()->isFromSelf()) {
                latestMessage()->setIsTyping(false);
                latestMessage()->setTimeStamp(QDateTime::currentDateTime());
            }
        },
        .processMessage = [this](QByteArray const &hdr, QByteArray const &dat) {
        },
      }),
    llm_chat_prev_status(&state_runner_stopped),
    llm_chat_next_status(&state_runner_stopped),
    llm_chat_status(&state_runner_stopped),
    voice_command_map({
        { voice_command_media_control.key, voice_command_media_control },
        { voice_command_interconnect.key, voice_command_interconnect },
    }),
    model_load_ready(false),
    model_active_state(false),
    user_typing(false),
    model_generating(false)
{
    connect(&vision_ml_p, &QProcess::finished, [=](int exitCode, QProcess::ExitStatus exitStatus) {
        qCDebug(lc_gen_chat) << "vision_ml process exit with" << exitCode;
    });

    startVisionService();

    connect(&llm_runner, &GenerativeChatRunner::connectedChanged, this, &GenerativeChatModel::modelConnectionChanged);
    connect(&llm_runner, &GenerativeChatRunner::connectedChanged, [=](bool b) {
        if (b) {
            qCDebug(lc_gen_chat) << "llm model connection established, querying status";
            QMutexLocker locker(&llm_chat_status_mutex);
            switchLlmChatStatus(&state_runner_started);
        } else {
            qCDebug(lc_gen_chat) << "llm model connection lost";
            switchLlmChatStatus(&state_runner_stopped);
        }
    });
    connect(&llm_runner, &GenerativeChatRunner::messageReceived,
            this, &GenerativeChatModel::processNewMessage, Qt::QueuedConnection);

    if (TellTalesModel::instance()->settingsLlmEnable()) {
        llm_runner.setState(true);
    }

    connect(&dms_runner, &DMS::connectedChanged, this, &GenerativeChatModel::dmsConnectedChanged);
    connect(&dms_runner, &DMS::personDetectedChanged, this, &GenerativeChatModel::dmsPersonDetectedChanged);
    connect(&dms_runner, &DMS::dmsWarningChanged, this, &GenerativeChatModel::dmsWaringChanged);

    connect(&fw_command, &QFutureWatcher<QByteArray>::finished, [=](void) {
        QByteArray result;

        if (llm_chat_status != &state_command_executing) {
            qCWarning(lc_gen_chat) << "voice command task finished unexpectedly";
            return;
        }

        active_command = nullptr;
        switchLlmChatStatus(&state_llm_typing);
        if(fu_command.resultCount() == 0U) {
            qCWarning(lc_gen_chat) << "voice command task finished without result";
            result = QByteArray("<err>");
        }else if(fu_command.resultCount() > 1U) {
            qCWarning(lc_gen_chat) << "voice command task finished with multiple result";
            result = QByteArray("<err>");
        } else {
            result = fw_command.result();
        }
        llm_runner.enqueueCommand(QByteArray(EGF::Header::COMMAND) + QByteArray(":") + result);
        qCDebug(lc_gen_chat) << "voice command task finished";
    });
}

GenerativeChatModel::~GenerativeChatModel(void)
{
    llm_runner.quit();
    for (auto&& msg : messages) {
        delete msg;
    }
}

GenerativeChatModel* GenerativeChatModel::instance()
{
    static std::unique_ptr<GenerativeChatModel> s_instance = nullptr;
    if(!s_instance)
    {
        s_instance.reset(new GenerativeChatModel);
    }
    return s_instance.get();
}

void GenerativeChatModel::addMessage(GenerativeChatMessage *msg)
{
    if (msg == nullptr) {
        return;
    }

    msg->moveToThread(QCoreApplication::instance()->thread());
    msg->setParent(this);
    messages.prepend(msg);

    // limit memory usage by removing oldest msg
    // TODO: 16 is for testing, change to a larger value before release
    if (messages.size() > 16) {
        qCDebug(lc_gen_chat) << "generative chat: removing oldest message";
        GenerativeChatMessage *msg_to_rm = messages.takeLast();
        delete msg_to_rm;
    }

    emit messagesUpdated();
}

GenerativeChatMessage *GenerativeChatModel::latestMessage(void)
{
    if (messages.isEmpty()) {
        return nullptr;
    } else {
        return messages.first();
    }
}

bool GenerativeChatModel::getModelConnected(void)
{
    return llm_runner.isConnected();
}

bool GenerativeChatModel::getModelActive(void)
{
    return model_active_state;
}

void GenerativeChatModel::startVisionService(void)
{
    if (vision_ml_p.state() != QProcess::NotRunning) {
        qCDebug(lc_gen_chat) << "vision_ml command busy";
        return;
    }
    vision_ml_p.startCommand("/bin/bash -c \"systemctl start dccvision.service\"");
    qCDebug(lc_gen_chat) << "vision_ml command started: start vision service";
    dms_runner.setState(true);
}

void GenerativeChatModel::stoptVisionService(void)
{
    if (vision_ml_p.state() != QProcess::NotRunning) {
        qCDebug(lc_gen_chat) << "vision_ml command busy";
        return;
    }
    vision_ml_p.startCommand("/bin/bash -c \"systemctl stop dccvision.service\"");
    qCDebug(lc_gen_chat) << "vision_ml command started: stop vision service";
    dms_runner.setState(false);
}

bool GenerativeChatModel::getDmsConnected(void)
{
    return dms_runner.isConnected();
}

bool GenerativeChatModel::getDmsPersonDetected(void)
{
    return dms_runner.isPersonDetected();
}

bool GenerativeChatModel::getDmsWarning(void)
{
    qCDebug(lc_gen_chat) << "dms warning changed to" << dms_runner.isDmsWarning();
    return dms_runner.isDmsWarning();
}

QList<GenerativeChatMessage *> GenerativeChatModel::getMessages()
{
    auto ret = messages;
    return ret;
}

void GenerativeChatModel::controlModelActive(bool b)
{
    if (!TellTalesModel::instance()->settingsLlmEnable()) {
        qCDebug(lc_gen_chat) << "control llm model active failed: llm not enabled in settings";
        return;
    }
    if (model_active_state == b) {
        return;
    }
    if (vision_ml_p.state() != QProcess::NotRunning) {
        qCDebug(lc_gen_chat) << "Failed to control vision and llm: vision_ml command busy";
        return;
    }
    if (b) {
        if (llm_chat_status != &state_model_paused) {
            qCDebug(lc_gen_chat) << "Failed to set llm model resume: model is not paused";
            return;
        }
        qCDebug(lc_gen_chat) << "set llm model resume";
        stoptVisionService();
        switchLlmChatStatus(&state_model_req_resume);
        model_active_state = true;
        emit modelActiveChanged(model_active_state);
    }
    else {
        if (llm_chat_status != &state_ready_idle) {
            qCDebug(lc_gen_chat) << "Failed to set llm model pause: model is not ready and idle";
            return;
        }
        qCDebug(lc_gen_chat) << "set llm model inactive";
        switchLlmChatStatus(&state_model_req_pause);
        model_active_state = false;
        emit modelActiveChanged(model_active_state);
        startVisionService();
        return;
    }
}

void GenerativeChatModel::setLlmChatNextStatus(LlmState const *new_status)
{
    assert(new_status);
    llm_chat_next_status = new_status;
}

void GenerativeChatModel::switchLlmChatStatus(LlmState const *new_status, QByteArray const &hdr, QByteArray const &dat)
{
    assert(new_status);
    qCDebug(lc_gen_chat) << "status: switch from " << llm_chat_status->name << " to " << new_status->name;
    llm_chat_prev_status = llm_chat_status;
    llm_chat_status->onExit(hdr, dat);
    llm_chat_status = new_status;
    llm_chat_status->onEnter(hdr, dat);
}

void GenerativeChatModel::switchLlmChatStatus(LlmState const *new_status)
{
    switchLlmChatStatus(new_status, "", "");
}

void GenerativeChatModel::processNewMessage(QByteArray msg) {
    if (msg.size() < 4) {
        qCWarning(lc_gen_chat) << "invalid message size";
        return;
    }

    if (msg[3] != ':') {
        qCWarning(lc_gen_chat) << "invalid message format";
        return;
    }

    // TODO: replace with QByteArrayView in the future to avoid copy
    QByteArray hdr = msg.sliced(0, 3);
    QByteArray dat = msg.size() > 4 ? msg.sliced(4) : QByteArray("");

    QMutexLocker locker(&llm_chat_status_mutex);

    assert(llm_chat_status);

    llm_chat_next_status = llm_chat_status;
    llm_chat_status->processMessage(hdr, dat);

    while (llm_chat_next_status != llm_chat_status) {
        switchLlmChatStatus(llm_chat_next_status, hdr, dat);
    }
}
