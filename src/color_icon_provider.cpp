/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "color_icon_provider.h"

ColorIconModel::ColorIconModel(void)
{
    m_is_valid = false;
    m_width = m_height = 0;
    m_key = ColorIconProvider::instance()->generateKey(this);
    ColorIconProvider::instance()->addModel(this);
    emit keyChanged();

    connect(this, &ColorIconModel::sourceChanged, [=](void){
        //qDebug() << "ColorIcon source changed to" << m_source;
        if (!m_source.toString().startsWith("qrc:")) {
            qDebug() << "color icon source must in qrc";
            m_is_valid = false;
            return;
        }
        if (!m_source.toString().endsWith(".svg")) {
            qDebug() << "color icon source must use svg file";
            m_is_valid = false;
            return;
        }

        QFile svg_file(m_source.toString().replace("qrc", ""));
        if (!svg_file.open(QIODevice::ReadOnly)) {
            qDebug() << "color icon source file" << m_source.toString() << "open failed";
            m_is_valid = false;
            return;
        }
        m_svg_string = QString::fromUtf8(svg_file.readAll());
        svg_file.close();
        QSvgRenderer renderer(m_svg_string.toLocal8Bit());
        if (renderer.isValid()) {
            m_is_valid = true;
        }
    });
}

ColorIconModel::~ColorIconModel(void)
{
    ColorIconProvider::instance()->removeModel(this);
}

size_t ColorIconModel::key(void)
{
    return m_key;
}

QString ColorIconModel::keyString(void)
{
    return QString::number(m_key, 16);
}

QUrl ColorIconModel::source(void)
{
    return m_source;
}

void ColorIconModel::setColor(const QString &color)
{
    const int length = QString("#FFFFFF").length();
    QRegularExpression const regex_fill(QString("fill=\"#.{6}\""));

    if (!m_is_valid) {
        return;
    }

    if (color.length() != length) {
        qDebug() << "color length" << color.length() << "invalid";
        return;
    }

    auto iter = regex_fill.globalMatch(m_svg_string);
    while (iter.hasNext()) {
        QRegularExpressionMatch match = iter.next();
        QString matched_str = match.captured(0);
        m_svg_string.replace(match.capturedStart(), match.capturedLength(), QString("fill=\"") + color + "\"");
    }
}

QImage ColorIconModel::image(void)
{
    if (!m_is_valid) {
        return QImage();
    }

    // create QImage and fill to transparent
    QImage result(m_width, m_height, QImage::Format_ARGB32);
    result.fill(QColor(0, 0, 0, 0));

    // load SVG xml contents
    QSvgRenderer renderer(m_svg_string.toLocal8Bit());

    // paint SVG to QImage
    QPainter painter;
    painter.begin(&result);
    renderer.render(&painter);
    painter.end();

    //qDebug() << "get image success" << m_key << m_source;
    return result;
}

ColorIconProvider::ColorIconProvider(void)
    : QQuickImageProvider(QQuickImageProvider::Image)
{
    m_hash_seed = rand();
}

ColorIconProvider* ColorIconProvider::instance()
{
    static std::unique_ptr<ColorIconProvider> s_instance = nullptr;
    if(!s_instance)
    {
        s_instance.reset(new ColorIconProvider);
    }
    return s_instance.get();
}

ColorIconProvider::~ColorIconProvider(void)
{

}

size_t ColorIconProvider::generateKey(ColorIconModel *const model)
{
    return qHash(size_t(model), m_hash_seed);
}

void ColorIconProvider::addModel(ColorIconModel *const model)
{
    m_model_ht.insert(model->key(), model);
}
void ColorIconProvider::removeModel(ColorIconModel *const model)
{
    m_model_ht.remove(model->key());
}

QImage ColorIconProvider::requestImage(
    const QString &id, QSize *size, const QSize &requestedSize)
{
    QStringList token_list = id.split(u'/');
    //qDebug() << "requestImage tokens:" << token_list;
    if (token_list.size() != 2) {
        qDebug() << "requestImage token format error" << id;
        return QImage();
    }
    QString &color = token_list[0];
    QString &path = token_list[1];
    bool is_num = false;
    size_t key = path.toULongLong(&is_num, 16);
    if (!is_num) {
        qDebug() << "requestImage cannot get key from string" << path;
        return QImage();
    }
    //qDebug() << "requestImage color" << color << " from key" << QString::number(key, 16);

    if (!m_model_ht.contains(key)) {
        qDebug() << "color icon provider could not find model" << path << key;
        return QImage();
    }
    ColorIconModel *model = m_model_ht.value(key);
    model->setColor(color);

    // return correct image here
    return model->image();
}
