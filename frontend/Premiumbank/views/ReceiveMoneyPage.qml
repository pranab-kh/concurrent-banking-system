import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import bank

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
                id: qrImage
                anchors.fill: parent
                anchors.margins: 16
                fillMode: Image.PreserveAspectFit
                smooth: false // keep QR edges crisp, no blur on the modules
                source: backend.accountInfoAvailable && backend.accountId > 0
                        ? qrCodeHelper.accountQrDataUrl(backend.accountId)
                        : ""
                visible: source !== ""
            }

            // Shown until a real account_id is available (e.g. before the
            // backend's login reply includes it, or if login hasn't
            // completed yet) instead of silently showing a blank box.
            Text {
                anchors.centerIn: parent
                visible: !qrImage.visible
                text: "QR code will appear\nonce your account loads."
                horizontalAlignment: Text.AlignHCenter
                color: "#999999"
                font.family: Style.mainFont
                font.pixelSize: 12
            }
        }

        Text {
            text: backend.accountInfoAvailable
                  ? "Account: " + backend.accountId
                  : "Account: " + rootwindow.currentUsername
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