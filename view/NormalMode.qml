/*
 * Copyright 2024-2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
import QtQuick
import QtQuick3D
import QtQuick.Effects
import QtQuick.Controls
import Style 1.0
import DriveSimulation 1.0
import TellTalesModel 1.0
import BluetoothDevice 1.0
import DccKeyDistanceModel 1.0
import MediaControlModel 1.0
import PhoneCallModel 1.0
import KillMeModel 1.0
import ControlBoxPageModel 1.0
import GenerativeChatModel 1.0
import MotionModel 1.0
import Nxp_motorcycle 1.0

Item {
    id: root
    property real scale: 1.0

    FontLoader {
        id: fontLoader
        source: "qrc:/images/NotoSansSC-Regular.ttf"
    }

    // Gauge {
    //     id: leftGauge
    //     x: gaugeEdgeMargin
    //     y: 100
    //     leftOrientation: true
    //     value: DriveSimulation.bikeSpeed
    //     maxValue: 120
    //     textLabel: "km/h"

    //     transform: [
    //         Scale {
    //             origin.x: leftGauge.width / 2
    //             origin.y: leftGauge.height / 2
    //             xScale: 1.25
    //             yScale: 1.25
    //         }
    //     ]
    // }

    // Gauge {
    //     id: rightGauge
    //     x: root.width - rightGauge.width - gaugeEdgeMargin
    //     y: 100
    //     leftOrientation: false
    //     value: DriveSimulation.engineRpm / 100
    //     valueText: {
    //         if (DriveSimulation.bikeSpeed === 0) {
    //             return 'P';
    //         } else if (DriveSimulation.gearLevel === -1) {
    //             return 'D';
    //         } else {
    //             return "G" + (DriveSimulation.gearLevel + 1);
    //         }
    //     }
    //     maxValue: 100
    //     maxAngle: 180
    //     textLabel: "x100"

    //     transform: [
    //         Scale {
    //             origin.x: rightGauge.width / 2
    //             origin.y: rightGauge.height / 2
    //             xScale: 1.25
    //             yScale: 1.25
    //         }
    //     ]
    // }

    Item {
        id: motorCycle
        anchors {
            bottom: parent.bottom
            horizontalCenter: parent.horizontalCenter
        }

        width: 400
        height: 250

        visible: true

        Rectangle {
            id: motorCycleArea
            anchors.fill: parent
            color: "black"

            View3D {
                id: view3D
                anchors.fill: parent
                anchors.rightMargin: 0
                anchors.bottomMargin: 0
                anchors.leftMargin: 0
                anchors.topMargin: 0

                environment: sceneEnvironment

                SceneEnvironment {
                    id: sceneEnvironment
                    antialiasingMode: SceneEnvironment.MSAA
                    antialiasingQuality: SceneEnvironment.High
                }

                Node {
                    id: scene
                    DirectionalLight {
                        id: directionalLight00
                        x: -45
                        y: -85
                        eulerRotation.z: 0
                        eulerRotation.y: -30
                        eulerRotation.x: 0
                        brightness: 0.9
                        z: 13.76266
                    }

                    DirectionalLight {
                        id: directionalLight01
                        x: 300
                        y: -20
                        z: 15
                        eulerRotation.z: 0
                        eulerRotation.y: 0
                        eulerRotation.x: -30
                        brightness: 3
                    }

                    PerspectiveCamera {
                        id: sceneCamera
                        x: 0
                        y: 125
                        z: 200
                        eulerRotation.z: 0
                        eulerRotation.y: 0
                        eulerRotation.x: -30
                    }

                    Nxp_motorcycle {
                        id: nxp_motorcycle_imu
                        pivot.y: 20
                        pivot.x: 0
                        visible: TellTalesModel.settingsImuEnable
                        rotation: MotionModel.attitude
                    }

                    Nxp_motorcycle {
                        id: nxp_motorcycle
                        pivot.y: 20
                        pivot.x: 0
                        eulerRotation.z: 0
                        eulerRotation.x: 0

                        visible: !TellTalesModel.settingsImuEnable

                        NumberAnimation on eulerRotation.y {
                            from: 0
                            to: 359
                            duration: 30000
                            running: true
                            loops: Animation.Infinite
                        }
                    }
                }
            }

            // MouseArea {
            //     id: mouseArea
            //     hoverEnabled: true
            //     property real startX: 0
            //     property real startRotation: 0
            //     property bool mousePressed: false
            //     anchors.fill: parent

            //     Connections {
            //         target: mouseArea
            //         function onPressed(mouse) {
            //             onRelesased: mouseArea.mousePressed = true
            //             mouseArea.startRotation = motorcycle_BlendSwap3.eulerRotation.y
            //             mouseArea.startX = mouse.x
            //         }
            //     }

            //     Connections {
            //         target: mouseArea
            //         function onReleased() { mouseArea.mousePressed = false }
            //     }

            //     Connections {
            //         target: mouseArea
            //         function onpositionChanged(mouse) {
            //             if (mouseArea.mousePressed) {
            //                 var delax = (mouse.x - mouseArea.startX) / 5
            //                 motorcycle_BlendSwap3.eulerRotation.y = mouseArea.startRotation + delax
            //             }
            //         }
            //     }

            // }
        }
    }

    Item {
        id: generativeChatArea

        anchors {
            horizontalCenter: parent.horizontalCenter
        }
        y: 120

        width: 800
        height: 360

        visible: GenerativeChatModel.modelActive

        GenerativeChat {
            id: genChatPage
            anchors.fill: parent
            messagesList: GenerativeChatModel.messagesList
        }
    }

    Item {
        id: btSelDrawer
        x: 860
        y: 0
        width: 360
        height: 480
        clip: true

        property bool _retracted: true
        property real _velocityY
        property real _oldMouseY
        property real _slideAnimDuration: 500

        signal extended()
        signal retracted()

        MouseArea {
            id: btSwipeToExpand
            width: parent.width
            height: 60
            visible: btSelDrawer._retracted

            drag.target: btSlider
            drag.minimumY: 0
            drag.maximumY: 1000
            drag.axis: Qt.Vertical
            // drag.onActiveChanged: {
            //     console.log("active:", drag.active);
            // }
            onMouseXChanged: {
                btSelDrawer._velocityY = (mouseY - btSelDrawer._oldMouseY);
                btSelDrawer._oldMouseY = mouseY;
                // console.log("velocity:", btSelDrawer._velocityY);
            }
            onPressed: {
                btSlideAnim.enabled = false;
            }
            onReleased: {
                // console.log("released");
                if (btSelDrawer._retracted) {
                    if ((btSelDrawer._velocityY > 0 && btSlider.y > 50) ||
                            btSelDrawer._velocityY > 10.0) {
                        btSelDrawer._slideAnimDuration = (btSlider.height - btSlider.y) / btSelDrawer._velocityY * 20;
                        var durationUpperLimit = 500 * (btSlider.height - btSlider.y) / btSlider.height;
                        btSelDrawer._slideAnimDuration = btSelDrawer._slideAnimDuration > durationUpperLimit ?
                            durationUpperLimit : btSelDrawer._slideAnimDuration;
                        btSlideAnim.enabled = true;
                        btSlider.y = btSlider.height
                        btSelDrawer._retracted = false;
                        btSelDrawer.extended();
                        console.log("drawer extend: velocity", btSelDrawer._velocityY, "duration", btSelDrawer._slideAnimDuration);
                    } else {
                        btSelDrawer._slideAnimDuration = 500 * btSlider.y / btSlider.height;;
                        btSlideAnim.enabled = true;
                        btSlider.y = 0;
                        btSelDrawer._retracted = true;
                        console.log("drawer not extend: retract duration", btSelDrawer._slideAnimDuration);
                    }
                }
            }
        }

        MouseArea {
            id: btClickToRetract
            anchors.fill: btSwipeToExpand
            visible: !btSelDrawer._retracted

            onClicked: {
                console.log("retract");
                btSelDrawer._slideAnimDuration = 500;
                btSlider.y = 0;
                btSelDrawer._retracted = true;
                btSelDrawer.retracted();
            }
        }

        Item {
            id: btSlider
            width: parent.width
            height: parent.height
            Behavior on y {
                id: btSlideAnim
                NumberAnimation {
                    duration: btSelDrawer._slideAnimDuration
                    easing.type: Easing.OutCubic
                }
            }

            BluetoothSelection {
                id: btSelPage
                y: -height
                width: parent.width
                height: parent.height
                deviceList: BluetoothDevice.devicesList
                btDeviceModel: BluetoothDevice // TODO: not required anymore?
            }
            Connections {
                target: btSelDrawer
                function onExtended() { BluetoothDevice.guiState = true; }
            }
            Connections {
                target: btSelDrawer
                function onRetracted() { BluetoothDevice.guiState = false; }
            }
        }
    }

    Item {
        id: locBubble
        x: 60
        y: 520
        visible: DccKeyDistanceModel.state
        width: 320
        height: 64
        LocatorBubble {
            locConnected: DccKeyDistanceModel.state
            locDistance: DccKeyDistanceModel.distance
            locAccuracy: DccKeyDistanceModel.accuracy
        }
    }

    Item {
        id: killMeSwitch

        x: 80
        y: 600

        visible: !TellTalesModel.showGauge

        KillMeSwitch {
            anchors {
                horizontalCenter: parent.horizontalCenter
                verticalCenter: parent.verticalCenter
            }

            onClicked: {
                console.log("kill-me triggered!");
                KillMeModel.killMe();
            }
        }
    }

    Item {
        id: phoneCallArea
        x: 60
        width: 360
        height: 90
        clip: true
        visible: true

        Rectangle {
            id: phoneCallBg
            anchors.fill: parent
            color: "black"
            opacity: 0.6
            radius: 16
            border {
                color: "transparent"
                width: 3
            }
        }

        y: PhoneCallModel.callState ? 4 : (- height - 4)
        Behavior on y {
            NumberAnimation {
                duration: 300
                alwaysRunToEnd: true
                easing.type: Easing.OutCubic
            }
        }

        PhoneCall {
            id: phoneCallCallout
            callState: PhoneCallModel.callState
            acceptState: PhoneCallModel.acceptState
            contactNameStr: PhoneCallModel.contactName
            phoneNumberStr: PhoneCallModel.phoneNumber

            Connections {
                target: phoneCallCallout
                function onCallAccept() { PhoneCallModel.accept(); }
            }
            Connections {
                target: phoneCallCallout
                function onCallReject() { PhoneCallModel.reject(); }
            }
        }
    }

    Item {
        id: controlRect
        width: 420
        height: 175
        anchors {
            bottom: parent.bottom
            right: parent.right
            bottomMargin: 30
            rightMargin: 35
        }
        clip: true

        Rectangle {
            id: controlAreaBg
            anchors.fill: parent
            color: "black"
            opacity: 0.6
            radius: 16
            border {
                color: Style.accentBlue
                width: 3
            }
        }

        SwipeView {
            id: mediaSwiper
            orientation: Qt.Vertical
            currentIndex: 0
            spacing: 8
            anchors {
                top: parent.top
                topMargin: controlAreaBg.border.width
                bottom: parent.bottom
                bottomMargin: controlAreaBg.border.width
                left: parent.left
                leftMargin: controlAreaBg.border.width
                right: parent.right
                rightMargin: controlAreaBg.border.width
            }
            clip: true

            BluetoothMediaController {
                id: mediaController

                control_enable: (TellTalesModel.phoneActive &&
                                !(MediaControlModel.androidAutoState
                                || PhoneCallModel.callState
                                || MediaControlModel.helmetInterconState
                                || GenerativeChatModel.modelActive))
                state: MediaControlModel.playerState
                cover_art_id: "image://media_cover_art/cover_art"
                Binding on title {
                    when: MediaControlModel.trackChanged
                    value: MediaControlModel.title
                }
                Binding on artist {
                    when: MediaControlModel.trackChanged
                    value: MediaControlModel.artist
                }
                Binding on track_length {
                    when: MediaControlModel.trackChanged
                    value: MediaControlModel.track_length
                }
                Binding on playing_time {
                    when: MediaControlModel.playingTimeChanged
                    value: MediaControlModel.playing_time
                }

                Connections {
                    target: mediaController
                    function onPlayClicked() { MediaControlModel.play(); }
                }
                Connections {
                    target: mediaController
                    function onPauseClicked() { MediaControlModel.pause(); }
                }
                Connections {
                    target: mediaController
                    function onPrevClicked() { MediaControlModel.prevTrack(); }
                }
                Connections {
                    target: mediaController
                    function onNextClicked() { MediaControlModel.nextTrack(); }
                }
            }

            HelmetInterconController {
                id: helmetInterconController

                helmetState0: TellTalesModel.helmetActive0
                helmetState1: TellTalesModel.helmetActive1
                linkState: PhoneCallModel.helmetInterconState
                commandState: PhoneCallModel.helmetInterconCommandState

                Connections {
                    target: helmetInterconController
                    function onHelmetClicked() {
                        console.log("helmet clicked", !PhoneCallModel.helmetInterconState);
                        PhoneCallModel.controlHelmetInterconState(!PhoneCallModel.helmetInterconState);
                    }
                }
            }

            Component {
                id: aapSettingWrapper
                AndroidAutoController {
                    id: androidAutoController

                    androidState: MediaControlModel.androidAutoState
                    commandState: MediaControlModel.androidAutoCommandState

                    Connections {
                        target: androidAutoController
                        function onAndroidClicked() {
                            MediaControlModel.setAndroidAutoState(!MediaControlModel.androidAutoState);
                        }
                    }
                }
            }

            Component {
                id: llmSettingWrapper
                ChatController {
                    id: chatController

                    buttonEnabled: TellTalesModel.helmetActive0 && GenerativeChatModel.modelLoadReady && !(GenerativeChatModel.userTyping || GenerativeChatModel.modelGenerating)
                    hintEnabled: !TellTalesModel.helmetActive0 || !GenerativeChatModel.modelLoadReady
                    // blinking: (GenerativeChatModel.modelConnection && !GenerativeChatModel.modelLoadReady) || GenerativeChatModel.userTyping
                    // blinkActiveColor: GenerativeChatModel.modelLoadReady ? Style.highlighterGreen : Style.orange
                    // blinkInactiveColor: GenerativeChatModel.modelLoadReady ? Style.darkBlue : Style.darkRed
                    staticColor: GenerativeChatModel.userTyping ? Style.highlighterGreen :
                        (GenerativeChatModel.modelGenerating ? Style.orange :
                            ((TellTalesModel.helmetActive0 && GenerativeChatModel.modelLoadReady) ? Style.darkBlue : Style.darkRed)
                        )
                    hintText: (GenerativeChatModel.userTyping || GenerativeChatModel.modelGenerating) ?
                        "LLM model is running" : (!GenerativeChatModel.modelLoadReady ?
                        "LLM model is loading..." : (!TellTalesModel.helmetActive0 ?
                        "Please connect a helmet" : ""))

                    Connections {
                        target: chatController
                        function onChatClicked() {
                            console.log("chat clicked");
                            PhoneCallModel.controlChatState(!GenerativeChatModel.modelActive);
                        }
                    }
                    Connections {
                        target: GenerativeChatModel
                        function onUserTypingChanged() {
			    console.log("chat clicked");
                        }
                    }
                    Connections {
                        target: ControlBoxPageModel
                        function onPageCmdPendingChanged() {
                            if (ControlBoxPageModel.pageCmdPending === true) {
                                console.log("execute page cmd");
                                mediaSwiper.setCurrentIndex(ControlBoxPageModel.pageCmd);
                                ControlBoxPageModel.pageCmdPending = false;
                            }
                        }
                    }
                }
            }

            Component.onCompleted: {
                // add optional components base on settings
                if (TellTalesModel.settingsAapEnable) {
                    mediaSwiper.addItem(aapSettingWrapper.createObject(mediaSwiper));
                }
                if (TellTalesModel.settingsLlmEnable) {
                    mediaSwiper.addItem(llmSettingWrapper.createObject(mediaSwiper));
                }
            }
        }

        PageIndicator {
            id: mediaPageIndictor
            count: mediaSwiper.count
            currentIndex: mediaSwiper.currentIndex
            anchors {
                horizontalCenter: parent.right
                horizontalCenterOffset: -24
                verticalCenter: parent.verticalCenter
            }
            rotation: 90
            spacing: 8

            delegate: Rectangle {
                width: 10
                height: 10
                radius: width / 2
                color: Style.lightPeriwinkle
                opacity: (index === mediaPageIndictor.currentIndex) ? 1.0 : 0.4
                Behavior on opacity {
                    OpacityAnimator {
                        duration: 200
                    }
                }
            }
        }
    }
}
