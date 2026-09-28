/*
 * Copyright 2024-2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdlib.h>
#include "media_control_model.h"
#include "phone_call_model.h"

#include <QLoggingCategory>
Q_LOGGING_CATEGORY(lc_phone_call, "model.phone_call")

// for VIT
#define MSG_SIZE 512
typedef struct {
    long mtype;
    int  idx;
} message_buf;

enum vit_message {
    HEY_NXP = 1,
    HEY_TV,

    OTHERS,
    MUTE,
    NEXT,
    SKIP,
    PAIR_DEVICE,
    PAUSE,
    STOP,
    POWER_OFF,
    POWER_ON,
    PLAY_MUSIC,
    PLAY_GAME,
    WATCH_CARTOON,
    WATCH_MOVIE,
};

PhoneCallModel::PhoneCallModel(QObject* parent) : QObject(parent)
{
    BluetoothDevice *bt = BluetoothDevice::instance();
    setCallState(false);
    setAcceptState(false);
    call_command = NOTHING;

    phonecall_mode = false;

    helmet_intercon_state = false;
    helmet_intercon_state_change = false;

    media_cmd = EMPTY;
    voice_state = false;
    voice_state_change = false;
    voice_wake = false;

    //chat_state = false;
    chat_state_change = false;

    src_rfcomm = nullptr;

    QObject::connect(&vit_handler, &VitCtlThread::sendVitStatus, this, &PhoneCallModel::setVitState, Qt::QueuedConnection);
    vit_handler.start();

    // Check and enable phone call on device connection changed
    QObject::connect(bt, &BluetoothDevice::phoneDeviceConnectionChanged, [=](bool new_state){
        if(new_state == true) {
            setPipewireSourceId();
        }
        if(phoneCallModeCheck()) {
            phoneCallModeSet();
        }
    });
    QObject::connect(bt, &BluetoothDevice::headsetDeviceConnectionChanged, [=](bool sink0_state, bool sink1_state){
        if(sink0_state == true) {
            setPipewireSinkId();
        }
        if(phoneCallModeCheck()) {
            phoneCallModeSet();
        }
    });

    connect(&fw_notify, &QFutureWatcher<std::any>::finished, [=](void) {
        notifyHandlerStart();
    });
    connect(&fw_notify, &QFutureWatcher<std::any>::resultReadyAt, [=](int index) {
        std::any result = fw_notify.resultAt(index);
        qCDebug(lc_phone_call) << "phonecall notify task return index"<< index;
        auto phone_info = std::any_cast<call_info&>(result);
        qCDebug(lc_phone_call) << "phonecall info: ring" << phone_info.ring;

        QString cmd_str;
        if (phone_info.ring) {
            // set contact_name, phone_number here
            contact_name = QString::fromUtf8(phone_info.phone_number);
            phone_number = QString::fromUtf8(" ");
            qCDebug(lc_phone_call) << "phonecall info: call id:" << contact_name;
        } else {
            setAcceptState(false);
            switch (phone_info.call_type) {
                case TERMINATE:
                    qCDebug(lc_phone_call) << "----->phone call is TERMINATE";
                    enqueueCommand(TERMINATE);
                    break;
                case REJECT:
                    qCDebug(lc_phone_call) << "----->phone call is REJECT";
                    enqueueCommand(REJECT);
                    break;
                default:
                    qCDebug(lc_phone_call) << "unexpected phone call " << phone_info.call_type;
                    break;
            }
        }

        setCallState(phone_info.ring);
    });
    notifyHandlerStart();

    connect(&command_process, &QProcess::finished, [=](int exitCode, QProcess::ExitStatus exitStatus) {
        QString command_result;
        BluetoothDeviceInfo *phoneDevice, *helmetDevice0, *helmetDevice1;
        phoneDevice = BluetoothDevice::instance()->getConnectedPhoneDevice();
        helmetDevice0 = BluetoothDevice::instance()->getConnectedHelmetDevice0();
        helmetDevice1 = BluetoothDevice::instance()->getConnectedHelmetDevice1();
        int ret;

        if (exitCode == 0) {
            switch (call_command) {
            case ACCEPT:
                setAcceptState(true);
                break;
            case TERMINATE:
                setAcceptState(false); // do not break
            case REJECT:
                setCallState(false);
                break;
            case PHONE_CALL_INTO:
                if (phoneDevice == nullptr) break;     // phone disconnected.
                qCDebug(lc_phone_call) << "phonecall mode is start";
                src_rfcomm = new Rfcomm(phoneDevice->getAddress().toStdString());
                if(src_rfcomm->rfcomm_run()) {
                    // enable show phone number
                    src_rfcomm->send_cmd("AT+CLIP=1");
                } else {
                    ret = system("killall bluealsa");
                    BluetoothDevice::instance()->controller->
                        disconnect(phoneDevice->getAddress().toStdString().c_str());
                    ret = system("/usr/share/dcc/bt_bridge.sh check");
                    if(ret)
                        qCDebug(lc_phone_call) << "bluealsa restart";
                }
                break;
            case INTERCON_INTO:
                helmet_intercon_state_change = false;
                helmet_intercon_state = true;
                qCDebug(lc_phone_call) << "helmet state changed to" << helmet_intercon_state;
                emit helmetInterconCommandStateChanged(helmet_intercon_state_change);
                emit helmetInterconStateChanged(helmet_intercon_state);
                break;
            case INTERCON_EXIT:
                helmet_intercon_state = false;
                helmet_intercon_state_change = false;
                qCDebug(lc_phone_call) << "helmet state changed to" << helmet_intercon_state;
                emit helmetInterconCommandStateChanged(helmet_intercon_state_change);
                emit helmetInterconStateChanged(helmet_intercon_state);
                phoneCallModeCheck();
                break;
            case BT_VOICE_START:
                if (chat_state_change) {
                    chat_state_change = false;
                    emit chatCommandStateChanged(chat_state_change);
                }

                if(voice_state_change && voice_state == false) {
                    startVR();
                    voice_state_change = false;
                    emit voiceCommandStateChanged(voice_state_change);
                    voice_state = true;
                    emit voiceStateChanged(voice_state);
                }
                break;
            case BT_VOICE_STOP:
                if (chat_state_change) {
                    chat_state_change = false;
                    emit chatCommandStateChanged(chat_state_change);
                }

                qCDebug(lc_phone_call) << "voice_state_change: " << voice_state_change << "voice_state: " << voice_state;
                if(voice_state_change) {
                    exitVR();
                    voice_state_change = false;
                    emit voiceCommandStateChanged(voice_state_change);
                    voice_state = false;
                    emit voiceStateChanged(voice_state);
                    media_execute(media_cmd);
                }
                break;
            default:
                qCDebug(lc_phone_call) << "phone call command" << (int)call_command << "finish w/o handling";
                break;
            }
        } else {
            if (helmet_intercon_state_change) {
                // reset state
                helmet_intercon_state_change = false;
                qCDebug(lc_phone_call) << "helmet intercon command return error: " << exitCode;
                emit helmetInterconCommandStateChanged(false);
            } else {
                // reset state
                qCDebug(lc_phone_call) << "phone call command return error: " << exitCode;
                QByteArray procOutput = command_process.readAll();
                qCDebug(lc_phone_call)<<"error log:"<< procOutput.data();
                qCDebug(lc_phone_call)<<"-----------------------------------------------";
            }
        }
        if(command_q.isEmpty()) {
            call_command = NOTHING;
            qCDebug(lc_phone_call) << "phone call command queue empty";
        } else {
            // auto send next command from queue
            enum call_cmd next_cmd = command_q.dequeue();
            sendCommand(next_cmd);
            qCDebug(lc_phone_call) << "phone call command deque" << (int)next_cmd;
        }
    });

    connect(&vit_process, &QProcess::finished, [=](int exitCode, QProcess::ExitStatus exitStatus) {
        qCDebug(lc_phone_call) << "vit_dcc process exit with" << exitCode;

        if(voice_state_change) {
            voice_state_change = false;
            emit voiceCommandStateChanged(false);
        }

        if(voice_state) {
            voice_state = false;
            emit voiceStateChanged(voice_state);
        }

        qCDebug(lc_phone_call)<<QString::fromLocal8Bit(vit_process.readAllStandardError());
        qCDebug(lc_phone_call)<<QString::fromLocal8Bit(vit_process.readAllStandardOutput());
    });
}

PhoneCallModel::~PhoneCallModel()
{
    if(src_rfcomm != nullptr)
        delete src_rfcomm;
}

PhoneCallModel* PhoneCallModel::instance()
{
    static std::unique_ptr<PhoneCallModel> s_instance = nullptr;
    if(!s_instance)
    {
        s_instance.reset(new PhoneCallModel);
    }
    return s_instance.get();
}

void PhoneCallModel::accept(void)
{
    // accept call
    src_rfcomm->send_cmd("ATA");
    enqueueCommand(ACCEPT);
}

void PhoneCallModel::reject(void)
{
    src_rfcomm->send_cmd("AT+CHUP");
}

void PhoneCallModel::setCallState(bool b)
{
    qCDebug(lc_phone_call) << "call_state:" << call_state << "input state:"<< b;
    if (call_state == b) {
        return;
    }
    call_state = b;
    emit callStateChanged();
}

void PhoneCallModel::setAcceptState(bool b)
{
    qCDebug(lc_phone_call) << "accept_state:" << accept_state << "input state:"<< b;
    if (accept_state == b) {
        return;
    }
    accept_state = b;
    emit acceptStateChanged(accept_state);
}

void PhoneCallModel::controlHelmetInterconState(bool b)
{
    BluetoothDeviceInfo *helmetDevice0, *helmetDevice1;
    helmetDevice0 = BluetoothDevice::instance()->getConnectedHelmetDevice0();
    helmetDevice1 = BluetoothDevice::instance()->getConnectedHelmetDevice1();

    if (helmet_intercon_state == b || helmet_intercon_state_change == true) {
        return;
    }

    if (!(helmetDevice0 && helmetDevice0)) {
        qCDebug(lc_phone_call) << "cannot start helmet interconnect without 2 connected helmet device";
        return;
    }

    qCDebug(lc_phone_call) << "helmet state is" << helmet_intercon_state << "and new request is" << b;

    helmet_intercon_state_change = true;
    emit helmetInterconCommandStateChanged(true);

    if(b) {
        if (phonecall_mode) {
            phonecall_mode = false;
            // exitPhonecallMode();
        }
        intoInterconMode();
    } else {
        exitInterconMode();
    }
}

void PhoneCallModel::controlVoiceState(bool b)
{
    QString cmd_str;
    BluetoothDeviceInfo *helmetDevice0;
    helmetDevice0 = BluetoothDevice::instance()->getConnectedHelmetDevice0();
    BluetoothDevice* bt = BluetoothDevice::instance();

    if (voice_state == b || voice_state_change == true) {
        return;
    }

    if (helmetDevice0 == nullptr ) {
        qCDebug(lc_phone_call) << "headset have not cononected";
        return;
    }

    if (accept_state == true || helmet_intercon_state_change == true ) {
        qCDebug(lc_phone_call) << "hfp is working so can't configure bt voice";
        qCDebug(lc_phone_call) << "phonecall accept state:" << accept_state;
        qCDebug(lc_phone_call) << "inter call state:" << accept_state;
        return;
    }

    voice_state_change = true;
    emit voiceCommandStateChanged(voice_state_change);

    if(b) {
        bt->controller->pause();
        enqueueCommand(BT_VOICE_START);
    } else {
        enqueueCommand(BT_VOICE_STOP);
    }

}

void PhoneCallModel::controlChatState(bool b)
{
    QString cmd_str;
    BluetoothDeviceInfo *helmetDevice0;
    helmetDevice0 = BluetoothDevice::instance()->getConnectedHelmetDevice0();

    bool chat_state = GenerativeChatModel::instance()->getModelActive();

    if (chat_state == b || chat_state_change == true) {
        return;
    }

    if (helmetDevice0 == nullptr ) {
        qCDebug(lc_phone_call) << "headset have not cononected";
        return;
    }

    if (accept_state == true || helmet_intercon_state_change == true ) {
        qCDebug(lc_phone_call) << "hfp is working so can't configure bt voice";
        qCDebug(lc_phone_call) << "phonecall accept state:" << accept_state;
        qCDebug(lc_phone_call) << "inter call state:" << accept_state;
        return;
    }

    chat_state_change = true;
    emit chatCommandStateChanged(chat_state_change);

    if(b) {
        qCDebug(lc_phone_call) << "chat mode enable" << chat_state << chat_state_change;
        enqueueCommand(BT_VOICE_START);
        GenerativeChatModel::instance()->controlModelActive(true);
    } else {
        qCDebug(lc_phone_call) << "chat mode disable" << chat_state << chat_state_change;
        enqueueCommand(BT_VOICE_STOP);
        GenerativeChatModel::instance()->controlModelActive(false);
    }
}

void PhoneCallModel::testSignal(bool b)
{
    qCDebug(lc_phone_call) << "call state signal emit:" << b;
}

void PhoneCallModel::notifyHandlerStart(void)
{
    fu_notify = QtConcurrent::task([=] (QPromise<std::any> &promise) {
        extern call_info call_msg;

        pthread_mutex_lock(&call_msg.msg_mutex);
        while(call_msg.handle_mask == false)
            pthread_cond_wait(&call_msg.msg_cond, &call_msg.msg_mutex);

        qCDebug(lc_phone_call) << "--->call info call_type updated" << call_msg.call_type;
        qCDebug(lc_phone_call) << "call info updated" << promise.addResult(std::make_any<call_info>(call_msg), 0);
        call_msg.call_type = NOTHING;
        call_msg.handle_mask = false;
        pthread_mutex_unlock(&call_msg.msg_mutex);
    }).spawn();
    fw_notify.setFuture(fu_notify);
}

void PhoneCallModel::enqueueCommand(enum call_cmd cmd)
{
    if (call_command == NOTHING) {
        sendCommand(cmd);
        return;
    }
    command_q.enqueue(cmd);
    qCDebug(lc_phone_call) << "---->phone call command" << (int)call_command << " is running, enqueue command" << int(cmd);
}

void PhoneCallModel::sendCommand(enum call_cmd cmd)
{
    QString cmd_str;
    QString src_name, sink_name, sco_handle;
    BluetoothDeviceInfo *phoneDevice, *helmetDevice0, *helmetDevice1;

    if (command_process.state() != QProcess::NotRunning) {
        qCDebug(lc_phone_call) << "phone call command busy";
        return;
    }

    phoneDevice = BluetoothDevice::instance()->getConnectedPhoneDevice();
    helmetDevice0 = BluetoothDevice::instance()->getConnectedHelmetDevice0();
    helmetDevice1 = BluetoothDevice::instance()->getConnectedHelmetDevice1();

    switch (cmd) {
    case ACCEPT:
        if (voice_state) {
            if(system("killall vit_dcc") < 0)
                qCDebug(lc_phone_call) << "vit stop failed";
            emit voiceStateChanged(false);
            sco_handle = "8";
        } else {
            sco_handle = "9";
        }
        cmd_str = "/usr/share/dcc/hci_cmd.sh  accept " + phoneDevice->getAddress() + " " + helmetDevice0->getAddress() + " " + sco_handle;
        break;
    case REJECT:
        cmd_str = "/usr/share/dcc/hci_cmd.sh  reject " + phoneDevice->getAddress() + " " + helmetDevice0->getAddress();
        break;
    case TERMINATE:
        if(voice_state) {
            cmd_str = "/usr/share/dcc/hci_cmd.sh  end "    + phoneDevice->getAddress() + " " + helmetDevice0->getAddress()+ " " + "end_VR";
            voice_state = false;
        }
        else
            cmd_str = "/usr/share/dcc/hci_cmd.sh  end "    + phoneDevice->getAddress() + " " + helmetDevice0->getAddress();
        break;
    case PHONE_CALL_INTO:
        if (phoneDevice == nullptr) break;
        cmd_str = "/usr/share/dcc/bt_bridge.sh check";
        break;
    case INTERCON_INTO:
        helmet_intercon_state_change = true;
        emit helmetInterconCommandStateChanged(helmet_intercon_state_change);
        cmd_str = "/usr/share/dcc/hci_cmd.sh intercom_start " + helmetDevice0->getAddress() + " " + helmetDevice1->getAddress();
        break;
    case INTERCON_EXIT:
        helmet_intercon_state_change = true;
        emit helmetInterconCommandStateChanged(helmet_intercon_state_change);
        cmd_str = "/usr/share/dcc/hci_cmd.sh intercom_end "   + helmetDevice0->getAddress() + " " + helmetDevice1->getAddress();
        break;
    case SET_SRC:
        if (phoneDevice == nullptr) break;
        src_name = phoneDevice->getName();
        cmd_str = "/usr/share/dcc/hci_cmd.sh wpctl_src_config "  + stringfilter(src_name);
        break;
    case SET_SINK:
        if (helmetDevice0 == nullptr) break;
        sink_name = helmetDevice0->getName();
        cmd_str = "/usr/share/dcc/hci_cmd.sh wpctl_sink_config " + stringfilter(sink_name);
        break;
    case BT_VOICE_START:
        cmd_str = "/usr/share/dcc/btnxp_voice_recognition.sh start " + helmetDevice0->getAddress();
        break;
    case BT_VOICE_STOP:
        cmd_str = "/usr/share/dcc/btnxp_voice_recognition.sh stop "  + helmetDevice0->getAddress();
        break;
    default:
        qCDebug(lc_phone_call) << "phone call command index" << (int)cmd << "invalid";
        return;
    }

    qCDebug(lc_phone_call) << "phone call command index:" << (int)cmd << "command" << cmd_str;
    call_command = cmd;
    command_process.startCommand(cmd_str);
}

bool PhoneCallModel::phoneCallModeCheck()
{
    bool old_phonecall_mode = phonecall_mode;
    bool intercon_state_changed;
    BluetoothDeviceInfo *phoneDevice, *helmetDevice0,*helmetDevice1;

    phoneDevice = BluetoothDevice::instance()->getConnectedPhoneDevice();
    helmetDevice0 = BluetoothDevice::instance()->getConnectedHelmetDevice0();
    helmetDevice1 = BluetoothDevice::instance()->getConnectedHelmetDevice1();

    if (helmet_intercon_state_change) {
        qCDebug(lc_phone_call) << "phone call mode check called when intercon is changing";
        intercon_state_changed = !helmet_intercon_state;
    } else {
        intercon_state_changed = helmet_intercon_state;
    }

    if(phoneDevice && helmetDevice0 && (intercon_state_changed == false)) {
        phonecall_mode = true;
        if(old_phonecall_mode == phonecall_mode)
            return false;
    } else {
        phonecall_mode = false;
        if(old_phonecall_mode == phonecall_mode)
            return false;
    }

    return true;
}

void PhoneCallModel::phoneCallModeSet()
{
    if ( phonecall_mode )
        intoPhonecallMode();
    else
        exitPhonecallMode();
}

void PhoneCallModel::intoPhonecallMode()
{
    qCDebug(lc_phone_call) << "----->intoPhonecallMode";
    enqueueCommand(PHONE_CALL_INTO);
}

void PhoneCallModel::exitPhonecallMode()
{
    qCDebug(lc_phone_call) << "----->exitPhonecallMode";

    if ( src_rfcomm != nullptr) {
        delete src_rfcomm;
        src_rfcomm = nullptr;
    }
}

void PhoneCallModel::intoInterconMode()
{
    qCDebug(lc_phone_call) << "----->intoInterconMode";
    enqueueCommand(INTERCON_INTO);
}

void PhoneCallModel::exitInterconMode()
{
    qCDebug(lc_phone_call) << "----->exitInterconMode";
    enqueueCommand(INTERCON_EXIT);
}

void PhoneCallModel::setPipewireSourceId()
{
    enqueueCommand(SET_SRC);
}

void PhoneCallModel::setPipewireSinkId()
{
    enqueueCommand(SET_SINK);
}

void PhoneCallModel::startVR()
{
    vit_process.startCommand("/usr/share/dcc/vit_dcc -i IMX9XA55 -d hw:btscoaudio -l ENGLISH");
}

void PhoneCallModel::exitVR()
{
    vit_process.kill();
}

void PhoneCallModel::setVitState(bool vr,  bool wake, enum player_cmd cmd_idx)
{
    media_cmd = cmd_idx;
    controlVoiceState(vr);

    // voice_state = vr;
    voice_wake = wake;
    emit voiceWakeChanged(wake);
}

QString PhoneCallModel::stringfilter(QString str)
{
    char spec_array[]={'(', ')'};
    int length = sizeof (spec_array)/sizeof (char);

    for(int i=0;i<length;i++)
    {
        QString tmp = QString(spec_array[i]);
        if(str.contains(tmp))
        {
            str=str.replace(tmp,"");
        }
    }

    return str;
}

void PhoneCallModel::media_execute(enum player_cmd cmd)
{
    MediaControlModel *player = MediaControlModel::instance();
    switch (cmd) {
            case MEDIA_PLAY:
                qCDebug(lc_phone_call) << "VIT RUN: PLAY MUSIC";
                player->play();
                cmd = EMPTY;
                break;
            case MEDIA_NEXT_TRACK:
                qCDebug(lc_phone_call) << "VIT RUN: PLAY NEXT MUSIC";
                player->nextTrack();
                cmd = EMPTY;
                break;
            case MEDIA_PAUSE:
                qCDebug(lc_phone_call) << "VIT RUN: PAUSE MUSIC";
                player->pause();
                cmd = EMPTY;
                break;
            default:
                qCDebug(lc_phone_call) << "VIT RUN: unsupport cmds: " << cmd;
                break;
        }
}

VitCtlThread::VitCtlThread(void)
{
    qCDebug(lc_phone_call) << "start recevice vit cmd";
}

VitCtlThread::~VitCtlThread(void)
{
    qCDebug(lc_phone_call) << "end recevice vit cmd";
}

void VitCtlThread::run(void)
{
    int msqid;
    key_t key;
    message_buf recvbuf;
    enum vit_message vit_cmd;
    enum player_cmd media_cmd;

    // Generate the same key as sending end
    if ((key = ftok(".", 'v')) == -1) {
        perror("ftok");
        return;
    }
    // Get message queues
    if ((msqid = msgget(key, 0666 | IPC_CREAT)) == -1) {
        perror("msgget");
        return;
    }

    while (1) {
        if (msgrcv(msqid, &recvbuf, sizeof(recvbuf.idx), 1, 0) == -1) {
            perror("msgrcv");
            return;
        }

        vit_cmd = (enum vit_message) recvbuf.idx;

        switch (vit_cmd) {
            case HEY_NXP:
            case HEY_TV:
                qCDebug(lc_phone_call) << "VR Wakeword detected";
                media_cmd = EMPTY;
                emit sendVitStatus(true, true, media_cmd);
                break;
            case PLAY_MUSIC:
                qCDebug(lc_phone_call) << "VR PLAY MUSIC";
                media_cmd = MEDIA_PLAY;
                emit sendVitStatus(false, false, media_cmd);
                break;
            case NEXT:
                qCDebug(lc_phone_call) << "VR PLAY NEXT MUSIC";
                media_cmd = MEDIA_NEXT_TRACK;
                emit sendVitStatus(false, false, media_cmd);
                break;
            case PAUSE:
                qCDebug(lc_phone_call) << "VR PAUSE MUSIC";
                media_cmd = MEDIA_PAUSE;
                emit sendVitStatus(true, false, media_cmd);
                break;
            default:
                qCDebug(lc_phone_call) << "VR unsupport cmds: " << vit_cmd;
                media_cmd = EMPTY;
                emit sendVitStatus(true, false, media_cmd);
                break;
        }
    }
}
