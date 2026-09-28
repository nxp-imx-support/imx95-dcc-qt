/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef COLOR_ICON_PROVIDER_H
#define COLOR_ICON_PROVIDER_H

#include <QHash>
#include <QObject>
#include <QVariant>
#include <QColor>
#include <QPixmap>
#include <QImage>
#include <QImageReader>
#include <QQuickImageProvider>
#include <QUrl>
#include <QFile>
#include <QString>
#include <QStringMatcher>
#include <QRegularExpression>
#include <QByteArray>
#include <QPainter>
#include <QSvgRenderer>

#include <QtQml/qqmlregistration.h>

#include <QDebug>
#include <iostream>
#include <memory>
#include <random>

class ColorIconModel: public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString key READ keyString NOTIFY keyChanged)
    Q_PROPERTY(QUrl source MEMBER m_source NOTIFY sourceChanged)
    Q_PROPERTY(uint32_t width MEMBER m_width)
    Q_PROPERTY(uint32_t height MEMBER m_height)

    QML_ANONYMOUS

public:
    ColorIconModel(void);
    ~ColorIconModel(void);
    size_t key(void);
    QString keyString(void);
    QUrl source(void);

    void setColor(const QString &color);
    QImage image(void);

Q_SIGNALS:
    void keyChanged(void);
    void sourceChanged(void);

private:
    bool m_is_valid;
    size_t m_key;
    uint32_t m_width, m_height;
    QUrl m_source;
    QString m_svg_string;
};

class ColorIconProvider: public QQuickImageProvider
{
public:
    static ColorIconProvider* instance(void);
    ~ColorIconProvider(void);

    size_t generateKey(ColorIconModel *const model);
    void addModel(ColorIconModel *const model);
    void removeModel(ColorIconModel *const model);

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;

private:
    size_t m_hash_seed;
    QHash<size_t, ColorIconModel*> m_model_ht;

    explicit ColorIconProvider(void);
};

#endif /* COLOR_ICON_PROVIDER_H */
