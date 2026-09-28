/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
import QtQuick
import QtQuick.Effects
import QtQuick.Controls
import QtQuick.Layouts
import QtQml.Models
import Style 1.0
import TellTalesModel 1.0

Item {
    id: root

    required property var messagesList

    property real globalOpacity: 0.6

    FontLoader {
        id: fontLoader
        source: "qrc:/images/NotoSansSC-Regular.ttf"
    }

    Rectangle {
        id: bgShade
        anchors.fill: parent
        color: "black"
        opacity: root.globalOpacity
        border {
            color: Style.accentBlue
            width: 3
        }
        radius: 16
    }

    Item {
        id: header

        property string headerText: "Generative AI Chat"
        property string hintText: "Say \"Hey NXP\" to start conversation"

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.leftMargin: bgShade.border.width
        anchors.rightMargin: bgShade.border.width
        anchors.topMargin: bgShade.border.width
        height: 60

        Rectangle {
            id: headerBg
            anchors.fill: parent
            color: Style.accentBlue
            opacity: root.globalOpacity
            radius: 12
        }

        Text {
            id: headerStr
            width: parent.width
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: 2
            text: header.headerText
            font.family: fontLoader.name
            font.bold: true
            font.pointSize: 20
            color: Style.lightPeriwinkle
            elide: Text.ElideMiddle
            wrapMode: Text.Wrap
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        Text {
            id: scanStateStr
            width: parent.width
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 2
            text: header.hintText
            font.family: fontLoader.name
            font.pointSize: headerStr.font.pointSize * 0.7
            color: Style.grey
            elide: Text.ElideMiddle
            wrapMode: Text.Wrap
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }

    Item {
        id: scrollArea
        anchors {
            left: parent.left
            leftMargin: bgShade.border.width * 2
            right: parent.right
            rightMargin: bgShade.border.width * 2
            top: header.bottom
            bottom: parent.bottom
            topMargin: bgShade.border.width
            bottomMargin: bgShade.border.width + 4
        }
        clip: true

        ListView {
            id: msgListView

            width: parent.width
            height: parent.height
            // spacing: 4
            clip: true
            verticalLayoutDirection: ListView.BottomToTop

            model: root.messagesList
            delegate: GenerativeChatItem {
                width: msgListView.width
                // height: 60
            }

            ScrollIndicator.vertical: ScrollIndicator {
                active: true
            }
        }
    }

//    Item {
//        id: bottomHandle
//        anchors.left: parent.left
//        anchors.right: parent.right
//        anchors.bottom: parent.bottom
//        height: 20
//
//        Rectangle {
//            anchors.fill: parent
//            anchors.leftMargin: height + 8
//            anchors.rightMargin: height + 8
//            anchors.bottomMargin: height
//            height: 12
//            radius: height / 2
//            color: Style.accentBlue
//            opacity: root.globalOpacity
//        }
//    }
}
