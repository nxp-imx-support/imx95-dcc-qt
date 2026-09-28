/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
import QtQuick
import QtQuick.Controls
import Style 1.0

Item {
    id: root

    property bool callState: false
    property bool acceptState: false
    property alias contactNameStr: contactName.text
    property alias phoneNumberStr: phoneNumber.text

    property int phoneCallIconSpacing: 16

    signal callAccept()
    signal callReject()

    anchors.fill: parent

    onAcceptStateChanged: {
        swipeBox.updateBackground();
    }

    FontLoader {
        id: fontLoader
        source: "qrc:/images/NotoSansSC-Regular.ttf"
    }

    Rectangle {
        id: bgBox
        anchors.fill: parent
        anchors.topMargin: 3
        anchors.bottomMargin: 3
        anchors.leftMargin: 3
        anchors.rightMargin: 3

        color: "transparent"
        opacity: 0.0
        radius: 12
    }

    Item {
        id: infoBox
        anchors.fill: bgBox
        anchors.leftMargin: acceptCall.width
        anchors.rightMargin: rejectCall.width
        clip: true

        Text {
            id: contactName
            anchors.top: parent.top
            anchors.topMargin: 8
            anchors.horizontalCenter: parent.horizontalCenter
            font.family: fontLoader.name
            font.pointSize: 20
            color: Style.lightPeriwinkle
            horizontalAlignment: Text.AlignHCenter
            elide: Text.ElideMiddle
            width: parent.width
            wrapMode: Text.NoWrap
        }

        Text {
            id: phoneNumber
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 8
            anchors.horizontalCenter: parent.horizontalCenter
            font.family: fontLoader.name
            font.pointSize: contactName.font.pointSize * 0.7
            color: Style.grey
            horizontalAlignment: Text.AlignHCenter
            elide: Text.ElideMiddle
            width: parent.width
            wrapMode: Text.NoWrap
        }
    }

    SwipeDelegate {
        id: swipeBox

        property bool enableLeftSwipe: false
        property bool disableLeftSwipe: false

        anchors {
            left: parent.left
            leftMargin: 3
            right: parent.right
            rightMargin: 3
            top: parent.top
            topMargin: 3
            bottom: parent.bottom
            bottomMargin: 3
        }
        clip: true

        swipe.onPositionChanged: {
            swipeBox.updateBackground();
        }
        //swipe.onCompleted: {
        //    console.log("phone call", (swipe.position ? ""));
        //}
        swipe.onOpened: {
            delayTimer.start();
            if (swipe.position === -1.0) {
                enableLeftSwipe = true;
                callReject();
                console.log("phone call reject");
            } else if (swipe.position === 1.0) {
                if (!acceptState) {
                    disableLeftSwipe = true;
                    callAccept();
                    console.log("phone call accept");
                }
            }
        }

        function getColorOpacity() : real {
            var raw_opacity = 0.75;
            if (swipeBox.swipe.position === 0.0) {
                return 0.75;
            } else if (swipeBox.swipe.position < 0.0) {
                raw_opacity = (-swipeBox.swipe.position);
            } else if (swipeBox.swipe.position > 0.0) {
                raw_opacity = swipeBox.swipe.position;
            }
            return 0.1 + (raw_opacity) * 0.4;
        }

        function updateBackground() {
            var scaleChange = 0.0;
            if (swipeBox.swipe.position === 0.0) {
                if (acceptState === true) {
                    bgBox.color = Style.highlighterGreen;
                    bgBox.opacity = 0.5;
                } else {
                    bgBox.opacity = 0.0;
                    bgBox.color = "transparent";
                }
            } else if (swipeBox.swipe.position < 0.0) {
                bgBox.opacity = swipeBox.getColorOpacity();
                bgBox.color = Style.highlighterRed;
                if (swipeBox.swipe.position < -0.75) {
                    scaleChange = ((-0.75) - swipeBox.swipe.position) * ((-0.75) - swipeBox.swipe.position);
                }
                rejectCallScale.xScale = 1.0 + scaleChange * 3.2;
                rejectCallScale.yScale = 1.0 + scaleChange * 3.2;
            } else if (swipeBox.swipe.position > 0.0) {
                if (acceptState === true) {
                    bgBox.color = Style.highlighterGreen;
                    bgBox.opacity = 0.5;
                } else {
                    bgBox.opacity = swipeBox.getColorOpacity();
                    bgBox.color = Style.highlighterGreen;
                    if (swipeBox.swipe.position > 0.75) {
                        scaleChange = (swipeBox.swipe.position - 0.75) * (swipeBox.swipe.position - 0.75);
                    }
                }
                acceptCallScale.xScale = 1.0 + scaleChange * 3.2;
                acceptCallScale.yScale = 1.0 + scaleChange * 3.2;
            }
        }

        Timer {
            id: delayTimer
            interval: 200
            repeat: false
            running: false

            onTriggered: {
                swipeBox.swipe.close();
                console.log("phone call timer triggered");
            }
        }

        background: Item {
            id: backgroundBox
            height: parent.height
            width: parent.width
            opacity: { swipeBox.getColorOpacity(); }
        }

        contentItem: Item {
            Image {
                id: acceptCall
                anchors.left: parent.left
                anchors.leftMargin: phoneCallIconSpacing
                anchors.verticalCenter: parent.verticalCenter

                source: "qrc:/images/telltales/phone_accept.png"
                transform: Scale {
                    id: acceptCallScale
                    origin.x: acceptCall.implicitWidth / 2
                    origin.y: acceptCall.implicitHeight / 2
                    xScale: 1.0
                    yScale: 1.0
                }
            }

            Image {
                id: rejectCall
                anchors.right: parent.right
                anchors.rightMargin: phoneCallIconSpacing
                anchors.verticalCenter: parent.verticalCenter

                source: "qrc:/images/telltales/phone_deny.png"
                transform: Scale {
                    id: rejectCallScale
                    origin.x: rejectCall.implicitWidth / 2
                    origin.y: rejectCall.implicitHeight / 2
                    xScale: 1.0
                    yScale: 1.0
                }
            }
        }

        swipe.left: Item {
            anchors.right: backgroundBox.left
            height: parent.height
            width: parent.width - 75

            Connections {
                target: swipeBox.swipe
                function onClosed() {
                    if (swipeBox.disableLeftSwipe) {
                        console.log("phone call left swipe disabled");
                        width = 0;
                        swipeBox.disableLeftSwipe = false;
                    } else if (swipeBox.enableLeftSwipe) {
                        console.log("phone call left swipe enabled");
                        width = parent.width - 75;
                        swipeBox.enableLeftSwipe = false;
                    }
                }
            }
        }
        swipe.right: Item {
            anchors.left: backgroundBox.right
            height: parent.height
            width: parent.width - 75
        }

        swipe.transition: Transition {
            SmoothedAnimation { velocity: 1; easing.type: Easing.OutCubic }
        }
    }
}

