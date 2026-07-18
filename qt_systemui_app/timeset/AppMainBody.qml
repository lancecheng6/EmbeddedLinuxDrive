import QtQuick 2.12
import QtQuick.Controls 2.5
import com.alientek.qmlcomponents 1.0

Item {
    id: appMainBody
    anchors.fill: parent
    property real sf: parent.width / 1024

    TimeControl { id: timeControl }

    Rectangle {
        anchors.fill: parent
        color: "#f0f0f0"

        Flickable {
            anchors.fill: parent
            contentHeight: col.height + 40 * sf
            clip: true

            Column {
                id: col
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: 20 * sf
                anchors.rightMargin: 20 * sf
                spacing: 12 * sf
                anchors.top: parent.top
                anchors.topMargin: 20 * sf

                Text {
                    text: "Set System Time"
                    font.pixelSize: 24 * sf
                    font.bold: true
                    color: "#333333"
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                Text {
                    id: currentTimeText
                    text: "Current: " + timeControl.getCurrentTime()
                    font.pixelSize: 16 * sf
                    color: "#666666"
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                Rectangle { height: 1; width: parent.width * 0.9; color: "#cccccc"; anchors.horizontalCenter: parent.horizontalCenter }

                Row {
                    spacing: 6 * sf
                    anchors.horizontalCenter: parent.horizontalCenter
                    Text { text: "Date:"; font.pixelSize: 14 * sf; anchors.verticalCenter: parent.verticalCenter; color: "#333333"; width: 50 * sf }
                    SpinBox {
                        id: yearSpin; from: 2020; to: 2099; value: 2025
                        width: 160 * sf; height: 50 * sf
                        font.pixelSize: 16 * sf
                    }
                    Text { text: "-"; font.pixelSize: 18 * sf; anchors.verticalCenter: parent.verticalCenter }
                    SpinBox {
                        id: monthSpin; from: 1; to: 12; value: 7
                        width: 120 * sf; height: 50 * sf
                        font.pixelSize: 16 * sf
                    }
                    Text { text: "-"; font.pixelSize: 18 * sf; anchors.verticalCenter: parent.verticalCenter }
                    SpinBox {
                        id: daySpin; from: 1; to: 31; value: 18
                        width: 120 * sf; height: 50 * sf
                        font.pixelSize: 16 * sf
                    }
                }

                Row {
                    spacing: 6 * sf
                    anchors.horizontalCenter: parent.horizontalCenter
                    Text { text: "Time:"; font.pixelSize: 14 * sf; anchors.verticalCenter: parent.verticalCenter; color: "#333333"; width: 50 * sf }
                    SpinBox {
                        id: hourSpin; from: 0; to: 23; value: 12
                        width: 120 * sf; height: 50 * sf
                        font.pixelSize: 16 * sf
                    }
                    Text { text: ":"; font.pixelSize: 18 * sf; anchors.verticalCenter: parent.verticalCenter }
                    SpinBox {
                        id: minSpin; from: 0; to: 59; value: 0
                        width: 120 * sf; height: 50 * sf
                        font.pixelSize: 16 * sf
                    }
                    Text { text: ":"; font.pixelSize: 18 * sf; anchors.verticalCenter: parent.verticalCenter }
                    SpinBox {
                        id: secSpin; from: 0; to: 59; value: 0
                        width: 120 * sf; height: 50 * sf
                        font.pixelSize: 16 * sf
                    }
                }

                Button {
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: 240 * sf; height: 50 * sf
                    Text {
                        text: "Set Time"
                        font.pixelSize: 18 * sf; color: "white"
                        anchors.centerIn: parent
                    }
                    background: Rectangle {
                        color: parent.pressed ? "#0066cc" : "#0088ff"
                        radius: 8
                    }
                    onClicked: {
                        var result = timeControl.setSystemTime(
                            yearSpin.value, monthSpin.value, daySpin.value,
                            hourSpin.value, minSpin.value, secSpin.value
                        );
                        currentTimeText.text = "Current: " + result;
                    }
                }
            }
        }
    }
}
