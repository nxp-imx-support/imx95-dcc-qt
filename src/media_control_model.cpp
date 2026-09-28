/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "media_control_model.h"

#include <QLoggingCategory>
Q_LOGGING_CATEGORY(lc_media, "model.media_control")

MediaCoverArtProvider::MediaCoverArtProvider(void)
    : QQuickImageProvider(QQuickImageProvider::Image)
{

}

MediaCoverArtProvider::~MediaCoverArtProvider(void)
{

}

void MediaCoverArtProvider::setImage(QImage _image)
{
    //qCDebug(lc_media) << "setImage" << _image.size();
    image = QImage(_image);
}

QImage MediaCoverArtProvider::requestImage(
    const QString &id, QSize *size, const QSize &requestedSize)
{
    //qCDebug(lc_media) << "requestImage" << id;
    if (id == QString("cover_art")) {
        return image;
    }
    else {
        return QImage();
    }
}

MediaControlModel::MediaControlModel(QObject* parent) : QObject(parent)
{
    bt = BluetoothDevice::instance();
    connect(bt, &BluetoothDevice::bluetoothAgentReady, [=]{
        bt->controller->get_player_info();
        qCDebug(lc_media) << "Qt MediaControlModel: send get player info request !";
    });

    connect(&bt->BtInfoReceiver, &BtInfoRxThread::ForwardMediaInfo, this, &MediaControlModel::onMediaInfoUpdate, Qt::QueuedConnection);

    setState(false);
    setTrackInfo(
        QString("No Media"),
        QString("..."),
        0,
        QImageReader(":/images/media/compact-disc.svg", "svg").read()
    );
    playing_time = QTime::fromMSecsSinceStartOfDay(0);

    connect(bt, &BluetoothDevice::phoneDeviceConnectionChanged, [=](bool b){
        /* reset media controller on phone disconnect  */
        if (!b) {
            setState(false);
            setTrackInfo(
                QString("No Media"),
                QString("..."),
                0,
                QImageReader(":/images/media/compact-disc.svg", "svg").read()
            );
        }
    });

    android_auto_state_request = android_auto_state = false;
    connect(&command_process, &QProcess::stateChanged, [=](QProcess::ProcessState newState) {
        if (newState == QProcess::Starting) {
            if (android_auto_state != android_auto_state_request) {
                qCDebug(lc_media) << "aap command start";
                emit androidAutoCommandStateChanged(true);
            }
        }
    });
    connect(&command_process, &QProcess::finished, [=](int exitCode, QProcess::ExitStatus exitStatus) {
        if (android_auto_state != android_auto_state_request) {
            qCDebug(lc_media) << "aap command stop";
            emit androidAutoCommandStateChanged(false);
        }
        if (exitCode == 0) {
            if (android_auto_state != android_auto_state_request) {
                android_auto_state = android_auto_state_request;
                qCDebug(lc_media) << "aap state changed to" << android_auto_state;
                emit androidAutoStateChanged(android_auto_state);
            }
        } else {
            // reset request to unchanged state
            android_auto_state_request = android_auto_state;
            qCDebug(lc_media) << "media control command return error: " << exitCode;
        }
    });
}

MediaControlModel::~MediaControlModel()
{

}

MediaControlModel* MediaControlModel::instance()
{
    static std::unique_ptr<MediaControlModel> s_instance = nullptr;
    if(!s_instance)
    {
        s_instance.reset(new MediaControlModel);
    }
    return s_instance.get();
}

bool MediaControlModel::getAndroidAutoCommandState(void)
{
    if (command_process.state() == QProcess::NotRunning) {
        return false;
    } else {
        return (android_auto_state_request != android_auto_state);
    }
}

bool MediaControlModel::getPlayerState(void)
{
    return player_state;
}

void MediaControlModel::play(void)
{
    enqueueCommand(MEDIA_PLAY);
}
void MediaControlModel::pause(void)
{
    enqueueCommand(MEDIA_PAUSE);
}
void MediaControlModel::nextTrack(void)
{
    enqueueCommand(MEDIA_NEXT_TRACK);
}
void MediaControlModel::prevTrack(void)
{
    enqueueCommand(MEDIA_PREV_TRACK);
}

void MediaControlModel::setState(bool b)
{
    qCDebug(lc_media) << "player_state:" << player_state << "input state:"<< b;
    if (player_state == b) {
        return;
    }
    player_state = b;
    emit playerStateChanged(player_state);
}

void MediaControlModel::setAndroidAutoState(bool b)
{
    const char *aap_start_sh = "/usr/share/dcc/aap_start.sh";
    const char *aap_stop_sh = "/usr/share/dcc/aap_stop.sh";
    QString cmd;
    BluetoothDeviceInfo *phone_device;

    if (!TellTalesModel::instance()->settingsAapEnable()) {
        qCDebug(lc_media) << "android auto state change fail: not enabled in settings";
        return;
    }

    if (android_auto_state == b) {
        return;
    }
    if (command_process.state() != QProcess::NotRunning) {
        qCDebug(lc_media) << "media control command busy";
        return;
    }

    cmd = QString("/bin/bash -c \"");
    if (b) {
        cmd += QString::fromUtf8(aap_start_sh);
        phone_device = BluetoothDevice::instance()->getConnectedPhoneDevice();
        if (phone_device) {
            cmd += (QString(" ") + phone_device->getAddress());
        }
    } else {
        cmd = QString::fromUtf8(aap_stop_sh);
    }
    cmd += QString("\"");
    qCDebug(lc_media) << "android auto command:" << cmd;
    android_auto_state_request = b;
    command_process.startCommand(cmd);
}

void MediaControlModel::setTrackInfo(QString _title, QString _artist,
        int _track_lenth_ms, QImage _cover_art)
{
    title = _title;
    artist = _artist;
    track_length = QTime::fromMSecsSinceStartOfDay(_track_lenth_ms);
    playing_time = QTime::fromMSecsSinceStartOfDay(0);
    cover_art_provider.setImage(_cover_art);
    emit trackChanged();
    emit playingTimeChanged();
}

QQuickImageProvider* MediaControlModel::getCoverArtProvider(void)
{
    return &cover_art_provider;
}

void MediaControlModel::testSignal(bool b)
{
    qCDebug(lc_media) << "media state signal emit:" << b;
}

void MediaControlModel::enqueueCommand(enum player_cmd cmd)
{
    switch(cmd) {
        case MEDIA_PLAY:
            if(!player_state)
                bt->controller->play();
            break;
        case MEDIA_PAUSE:
            if(player_state)
                bt->controller->pause();
            break;
        case MEDIA_NEXT_TRACK:
            bt->controller->next();
            break;
        case MEDIA_PREV_TRACK:
            bt->controller->previous();
            break;
        default:
            qCDebug(lc_media) << "err UNKNOWN bt media cmd" << cmd;
            break;
    }

    qCDebug(lc_media) << "command enqueue" << int(cmd);
}

void MediaControlModel::onMediaInfoUpdate(enum player_cmd idx, player_info media)
{
    switch (idx) {
        case PLAYER_STATUS:
            setState(media.player_status);
            break;
        case PLAYER_PROPERTY:
            setTrackInfo(
                QString::fromUtf8(media.title),
                QString::fromUtf8(media.artist),
                media.duration,
                QImageReader(":/images/media/compact-disc.svg", "svg").read()
            );
            break;
        default:
            qCDebug(lc_media) << "MediaControlModel->onMediaInfoUpdate: unsupport media info " << idx;
            break;
    }

}