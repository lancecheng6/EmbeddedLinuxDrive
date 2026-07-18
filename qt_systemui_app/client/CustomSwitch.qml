import QtQuick 2.2
import QtQuick.Controls 2.5

Switch {
    id: customSwitch
    property color checkedColor: "#0ACF97"
    property bool switchEnable: true

    indicator: Rectangle {
        width: parent.width
        height: parent.height
        radius: height / 2
        color: customSwitch.checked ? checkedColor : "#f0f0f0"
        border.width: 2
        border.color: customSwitch.checked ? checkedColor : "#E5E5E5"
        enabled: switchEnable

        Image {
            id: handle_cion
            x: customSwitch.checked ? parent.width - width - 2 : 2
            anchors.verticalCenter: parent.verticalCenter
            height: parent.height
            width: height
            LayoutMirroring.enabled: true
            LayoutMirroring.childrenInherit: true
            source:customSwitch.checked ? "qrc:/icons/slider_handle_style.png" : "qrc:/icons/slider_handle_style_pressed.png"
            Behavior on x {
                NumberAnimation { duration: 50 }
            }
        }
    }
}
