/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
import QtQuick
import Style 1.0
import TellTalesModel 1.0

Item {
    id: root

    required property bool control_enable
    required property bool state
    required property string title
    required property string artist
    required property date track_length //TODO: not used yet
    required property date playing_time // TODO: not used yet
    required property var cover_art_id

    property color titleColor: Qt.darker(Style.lightPeriwinkle, 1.35)

    signal playClicked()
    signal pauseClicked()
    signal prevClicked()
    signal nextClicked()

    FontLoader {
        id: fontLoader
        source: "qrc:/images/NotoSansSC-Regular.ttf"
    }

    Image {
        id: coverImg

        anchors {
            top: parent.top
            topMargin: 24
            bottom: parent.bottom
            bottomMargin: 24
            left: parent.left
            leftMargin: 24
        }
        width: height
        source: cover_art_id
    }

    Column {
        id: header

        anchors {
            left: coverImg.right
            leftMargin: 16
            right: parent.right
            rightMargin: 16
            top: parent.top
            topMargin: 16
            bottom: parent.bottom
            bottomMargin: 16
        }
        spacing: 8

        Flickable {
            id: titelFlick
            width: parent.width
            height: 36
            anchors{
                horizontalCenter: parent.horizontalCenter
            }
            flickableDirection: Flickable.HorizontalFlick
            boundsBehavior: Flickable.StopAtBounds//Flickable.DragAndOvershootBounds
            clip: true
            interactive: false
            contentWidth: titleStr.width
            contentHeight: titleStr.height

            Text {
                id: titleStr
                width: titelFlick.width
                height: 36
                text: title
                font.family: fontLoader.name
                font.bold: true
                font.pointSize: 20
                color: root.titleColor
                elide: Text.ElideNone
                wrapMode: Text.NoWrap
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter

                onTextChanged: {
                    titleAnim.stop();
                    if (titleStr.contentWidth > titelFlick.width) {
                        titleStr.width = titleStr.contentWidth;
                        titleAnim.from = 0
                        titleAnim.to = titleStr.contentWidth - titelFlick.width;
                        titleAnim.duration = titleAnim.to * 24;
                        titleAnim.start();
                    } else {
                        titleStr.width = titelFlick.width;
                        titelFlick.contentX = 0;
                    }
                }
            }

            NumberAnimation on contentX {
                id: titleAnim
            }

            Component.onCompleted: {
                titleStr.width = titelFlick.width;
                titelFlick.contentX = 0;
            }
        }

        Text {
            id: artistStr
            width: parent.width
            height: 20
            anchors{
                horizontalCenter: parent.horizontalCenter
            }
            text: artist
            font.family: fontLoader.name
            font.pointSize: titleStr.font.pointSize * 0.7
            color: Style.grey
            elide: Text.ElideRight
            wrapMode: Text.NoWrap
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        Row {
            id: mediaControl;
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 8
            height: 80
            enabled: root.control_enable

            Behavior on opacity {
                NumberAnimation {
                    duration: 300
                    easing.type: Easing.OutCubic
                }
            }

            MediaControlButton {
                text: "Prev"
                image: "qrc:/images/media/play-previous.svg"
                color: root.titleColor
                animEnable: true
                onClicked: root.prevClicked()
            }
            MediaControlButton {
                image: root.state ? "qrc:/images/media/pause.svg" : "qrc:/images/media/play.svg"
                color: root.titleColor
                animEnable: true
                onPressed: {
                    if (root.state) {
                        text = "Pause";
                    } else {
                        text = "Play";
                    }
                }
                onClicked: {
                    if (root.state) {
                        root.pauseClicked();
                    } else {
                        root.playClicked();
                    }
                }
            }
            MediaControlButton {
                text: "Next"
                image: "qrc:/images/media/play-next.svg"
                color: root.titleColor
                animEnable: true
                onClicked: root.nextClicked()
            }
        }
    }
}
