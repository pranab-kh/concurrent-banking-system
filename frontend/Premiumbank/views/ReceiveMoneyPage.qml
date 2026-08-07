import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 20

        Button {
            text: "← Back"
            onClicked: mainStack.pop()
        }

        Text {
            text: "Receive Money"
            font.family: Style.mainFont
            font.pixelSize: 22
            font.bold: true
            color: "#1a237e"
            Layout.alignment: Qt.AlignHCenter
        }

        // QR Code Container Box
        Rectangle {
            width: 240
            height: 240
            color: "white"
            radius: 16
            border.color: "#eef0f2"
            border.width: 2
            Layout.alignment: Qt.AlignHCenter

            Image {
                anchors.fill: parent
                anchors.margins: 16
                // Put your QR code image in your project folder!
                source: "../image/qr.png"
                fillMode: Image.PreserveAspectFit
            }
        }

        Text {
            text: "Account: 04810017509872"
            font.family: Style.mainFont
            font.pixelSize: 16
            font.bold: true
            color: "#333333"
            Layout.alignment: Qt.AlignHCenter
        }

        Text {
            text: "Scan this QR code to receive payments directly into your account."
            font.family: Style.mainFont
            font.pixelSize: 11
            color: "#777777"
            horizontalAlignment: Text.AlignHCenter
            Layout.fillWidth: true
        }

        Item { Layout.fillHeight: true }
    }
}