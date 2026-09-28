/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
import QtQuick
import ColorIconModel 1.0

Item {
    id: root
    required property url source
    required property color color

    ColorIconModel {
        id: model
        width: root.width
        height: root.height
        source: root.source
    }

    Image {
        id: image
        width: root.width
        height: root.height
        anchors.fill: parent
        smooth: true
        opacity: root.color.a
        source: {
            // set alpha to 1.0 to only format #RRGGBB instead of #AARRGGBB to string
            var color_str = Qt.rgba(root.color.r, root.color.g, root.color.b, 1.0).toString();

            // construct string that encodes "image://color_icon_provider/<color-hex>/<key-str>
            var path = "image://color_icon_provider/" + color_str + "/" + model.key;

            //console.log("ColorIcon path:", path);
            return path;
        }
    }

    // Component.onCompleted: {
    //     console.log("ColorIcon_nogpu loaded");
    // }
}
