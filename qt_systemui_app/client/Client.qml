import QtQuick 2.0
import QtQuick.Window 2.12
import QtQuick.Controls 2.5
import com.alientek.qmlcomponents 1.0
Item {
    id: client
    property string programmerName
    anchors.fill: parent
    property real scaleFactor: client.width / 1024
    SystemUICommonApiClient {
        id: systemUICommonApiClient
        appName: programmerName
        onActionCommand: {
            if (cmd === SystemUICommonApiClient.Show) {
                //systemUICommonApiClient.requestVisibilityChange(SystemUICommonApiClient.Hide)
                systemUICommonApiClient.askSystemUItohideOrShow(SystemUICommonApiClient.Hide)
                window.x = 0
                window.y = 0
                appMainBody.visible = true
                window.show()
            }
            if (cmd === SystemUICommonApiClient.Quit)
                Qt.quit()
        }
    }


    AppMainBody {
        anchors.fill: parent
        id: appMainBody
        visible: true
    }

    Timer {
        repeat: false
        onTriggered: backBtn.enabled = true
        running: true
        interval: 1000
    }
    RoundButton {
        visible: !systemUICommonApiClient.applicationAnimation && appMainBody.visible //&& !controlCenter.opened
        id: backBtn
        x: parent.x + parent.width - 100 * scaleFactor
        y: parent.y + parent.height / 2 - 100 * scaleFactor
        width: 100 * scaleFactor
        height: width
        hoverEnabled: enabled
        enabled: false
        opacity: mouseArea.pressed ? 1.0 : 0.3
        background: Rectangle{
            color: "#88101010"
            radius: parent.width / 2
        }

        Rectangle{
            anchors.centerIn: parent
            width: 90 * scaleFactor
            height: width
            color: "#88ffffff"
            radius: parent.width / 2
        }

        Rectangle{
            anchors.centerIn: parent
            width: 80 * scaleFactor
            height: width
            color: "#aaffffff"
            radius: parent.width / 2
        }

        Rectangle{
            anchors.centerIn: parent
            width: 70 * scaleFactor
            height: width
            color: "#ffffff"
            radius: parent.width / 2
        }
        MouseArea {
            id: mouseArea
            anchors.fill: parent
            drag.target: backBtn
            drag.minimumX: 0
            drag.minimumY: 0
            drag.maximumX: client.width - 100 * scaleFactor
            drag.maximumY: client.height - 100 * scaleFactor
            onClicked: {
                //window.flags = Qt.FramelessWindowHint |  Qt.WindowTransparentForInput
                if (!systemUICommonApiClient.backgroundTask) {
                    Qt.quit()
                } else {
                    /*appMainBody.grabToImage(function(result) {
                        systemUICommonApiClient.sendAppStateImage(result.image)
                    })*/
                    //appMainBody.visible = false
                    window.x = Screen.desktopAvailableWidth
                    window.y = Screen.desktopAvailableHeight
                    appMainBody.visible = false
                    window.hide()
                    systemUICommonApiClient.askSystemUItohideOrShow(SystemUICommonApiClient.Show)
                }
            }
        }
    }
}
