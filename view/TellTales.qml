/*
 * Copyright 2024-2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
import QtQuick
import TellTalesModel 1.0
import Style 1.0
import BluetoothDevice 1.0
import DccFindMyModel 1.0
import DccKeyDistanceModel 1.0
import PhoneCallModel 1.0
import GenerativeChatModel 1.0

Row {
    id: telltales
    spacing: 16

    // TellTalesIndicator
    // {
    //     height: 50
    //     width: 50
    //     source: "qrc:/images/telltales/turn_left.svg"
    //     activeColor: Style.highlighterGreen
    //     indicatorOpacity: TellTalesModel.indicatorOpacity
    //     Binding on active {
    //         when: TellTalesModel.turnLightChanged
    //         value: TellTalesModel.turnLeftActive
    //     }
    //     Binding on blinking {
    //         when: TellTalesModel.turnLightChanged
    //         value: TellTalesModel.turnLeftBlinking
    //     }
    // }
    TellTalesIndicator
    {
        height: 50
        width: 50
        source: "qrc:/images/telltales/phone.svg"
        activeColor: PhoneCallModel.acceptState ? Style.highlighterGreen : Style.brightBlue
        active: TellTalesModel.phoneActive
        indicatorOpacity: TellTalesModel.indicatorOpacity
    }
    TellTalesIndicator
    {
        height: 50
        width: 50
        source: "qrc:/images/telltales/findmy.svg"
        activeColor: Style.highlighterYellow
        active: DccFindMyModel.active
        indicatorOpacity: TellTalesModel.indicatorOpacity

        visible: TellTalesModel.settingsFindMyEnable
    }
    // TellTalesIndicator
    // {
    //     height: 50
    //     width: 50
    //     source: "qrc:/images/telltales/front_light.svg"
    //     activeColor: Style.brightBlue
    //     active: TellTalesModel.beamActive
    //     indicatorOpacity: TellTalesModel.indicatorOpacity
    // }
    TellTalesIndicator
    {
        height: 50
        width: 50
        source: "qrc:/images/telltales/key.svg"
        activeColor: Style.highlighterYellow
        active: DccKeyDistanceModel.state
        indicatorOpacity: TellTalesModel.indicatorOpacity

        visible: TellTalesModel.settingsKeyEnable
    }
    TellTalesIndicator
    {
        height: 50
        width: 50
        source: "qrc:/images/telltales/dms.svg"
        //activeColor: Style.highlighterYellow
        activeColor: GenerativeChatModel.dmsWarning ? Style.highlighterYellow : Style.brightBlue
        active: GenerativeChatModel.dmsPersonDetected
        indicatorOpacity: TellTalesModel.indicatorOpacity
    }
    TellTalesIndicator
    {
        height: 50
        width: 50
        source: "qrc:/images/telltales/helmet.svg"
        activeColor: Style.brightBlue
        active: TellTalesModel.helmetActive0
        indicatorOpacity: TellTalesModel.indicatorOpacity

        Rectangle {
            width: 20
            height: 16
            radius: height / 2
            color: Style.brightBlue
            anchors {
                top: parent.top
                topMargin: -12
                right: parent.right
                rightMargin: 0
            }
            visible: TellTalesModel.helmetActive0

            Text {
                anchors.fill: parent
                text: TellTalesModel.helmetActive1 ? "2" : "1"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                color: Style.lightPeriwinkle
                font.family: "Sarabun"
                font.pixelSize: 16
                font.bold: true
            }
        }
    }
    // TellTalesIndicator
    // {
    //     height: 50
    //     width: 50
    //     source: "qrc:/images/telltales/turn_right.svg"
    //     activeColor: Style.highlighterGreen
    //     indicatorOpacity: TellTalesModel.indicatorOpacity
    //     blinking: TellTalesModel.turnRightBlinking
    //     Binding on active {
    //         when: TellTalesModel.turnLightChanged
    //         value: TellTalesModel.turnRightActive
    //     }
    //     Binding on blinking {
    //         when: TellTalesModel.turnLightChanged
    //         value: TellTalesModel.turnRightBlinking
    //     }
    // }
}
