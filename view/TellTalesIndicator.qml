/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
import QtQuick
import Style 1.0
import TellTalesModel 1.0
import QtQuick.Effects

Item {
    id: indicator

    property alias source: colorIcon.source
    property bool active: false
    property color activeColor: Style.highlighterGreen
    property color inactiveColor: Style.darkBlue
    property alias indicatorOpacity: colorIcon.opacity
    property alias blinking: indicatorBlinkAnimation.running

    ColorIcon {
        id: colorIcon
        anchors.fill: parent
        width: indicator.width
        height: indicator.height
        color: indicator.active ? indicator.activeColor : indicator.inactiveColor

        Behavior on opacity { NumberAnimation {
                easing.type: Easing.InOutQuad
                duration: TellTalesModel.opacityChangeDuration
        }}
    }

    SequentialAnimation {
        id: indicatorBlinkAnimation
        loops: Animation.Infinite
        alwaysRunToEnd: true

        ScriptAction {
            script: colorIcon.color = indicator.activeColor
        }

        PauseAnimation {
            duration: 400
        }

        ScriptAction {
            script: colorIcon.color = indicator.inactiveColor
        }

        PauseAnimation {
            duration: 300
        }

        onStopped: {
            if (indicator.active) { colorIcon.color = indicator.activeColor; }
            else { colorIcon.color = indicator.inactiveColor; }
        }
    }
}
