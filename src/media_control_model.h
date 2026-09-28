/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef MUSIC_PLAYER_MODEL_H
#define MUSIC_PLAYER_MODEL_H

#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QVariant>
#include <QtCore/QTimer>
#include <QString>
#include <QQueue>
#include <QPixmap>
#include <QImage>
#include <QImageReader>
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

#include "bluetooth_device.h"
#include "telltales_model.h"
// #include "bluetoothctl/bluetoothctl.h"

class MediaCoverArtProvider: public QQuickImageProvider
{
public:
    MediaCoverArtProvider(void);
    ~MediaCoverArtProvider(void);

    QImage image;
    void setImage(QImage _image);
    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;
};

class MediaControlModel: public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool playerState MEMBER player_state NOTIFY playerStateChanged)

    Q_PROPERTY(QString title MEMBER title NOTIFY trackChanged)
    Q_PROPERTY(QString artist MEMBER artist NOTIFY trackChanged)
    Q_PROPERTY(QTime track_length MEMBER track_length NOTIFY trackChanged)

    //Q_PROPERTY(QString lyrics MEMBER lyrics NOTIFY lyricsChanged)
    Q_PROPERTY(QTime playing_time MEMBER playing_time NOTIFY playingTimeChanged)

    Q_PROPERTY(bool androidAutoState MEMBER android_auto_state NOTIFY androidAutoStateChanged)
    Q_PROPERTY(bool androidAutoCommandState READ getAndroidAutoCommandState NOTIFY androidAutoCommandStateChanged)

    QML_ELEMENT
    QML_SINGLETON

public:
    static MediaControlModel* instance(void);
    ~MediaControlModel(void);

    void setTrackInfo(QString _title, QString _artist,
        int _track_lenth_ms, QImage _cover_art);
    QQuickImageProvider* getCoverArtProvider(void);
    bool getAndroidAutoCommandState(void);

    bool getPlayerState(void);

    Q_INVOKABLE void play(void);
    Q_INVOKABLE void pause(void);
    Q_INVOKABLE void nextTrack(void);
    Q_INVOKABLE void prevTrack(void);

Q_SIGNALS:
    void playerStateChanged(bool b);
    void trackChanged(void);
    //void lyricsChanged(QString l);
    void playingTimeChanged(void);
    void androidAutoStateChanged(bool b);
    void androidAutoCommandStateChanged(bool b);

public Q_SLOTS:
    void setState(bool b);
    void onMediaInfoUpdate(enum player_cmd idx, player_info media);

    // FIXME: To be honest these are not setters, but rather qml invokable signal handlers.
    // FIXME: this should be renamed.
    void setAndroidAutoState(bool b);
    // debug slot
    void testSignal(bool b);

private:
    explicit MediaControlModel(QObject* parent = nullptr);

    bool player_state;
    QTimer playing_timer;
    QString title, artist, lyrics;
    QTime track_length, playing_time;
    MediaCoverArtProvider cover_art_provider;
    BluetoothDevice* bt;

    QFuture<bool> fu_command;
    // QFutureWatcher<bool> fw_command;
    // QQueue<enum player_cmd> command_q;
    QReadWriteLock command_q_rwlock;
    void enqueueCommand(enum player_cmd cmd);
    // void commandHandlerStart(void);

    // QFuture<std::any> fu_notify;
    // QFutureWatcher<std::any> fw_notify;
    // void notifyHandlerStart(void);

    bool android_auto_state, android_auto_state_request;
    QProcess command_process;
};

#endif // MUSIC_PLAYER_MODEL_H
