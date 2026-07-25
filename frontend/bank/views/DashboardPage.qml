import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    Column {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 24

        // Header
        Column {
            width: parent.width
            spacing: 4

            Text {
                text: "Good Evening, Pratik"
                font.family: Style.mainFont
                font.pixelSize: 22
                font.bold: true
                color: "#1a237e"
            }

            Text {
                text: "Account No: 04810017509872"
                font.family: Style.mainFont
                font.pixelSize: 14
                color: "#666666"
            }
        }

        // Total Amount Card
        Rectangle {
            width: parent.width
            height: 120
            color: "#ffffff"
            radius: 16
            border.color: "#eef0f2"
            border.width: 1

            Column {
                anchors.centerIn: parent
                spacing: 8

                Text {
                    text: "TOTAL AMOUNT"
                    font.family: Style.mainFont
                    font.pixelSize: 12
                    font.weight: Font.DemiBold
                    color: "#888888"
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                Text {
                    text: "NPR 2,744.97"
                    font.family: Style.mainFont
                    font.pixelSize: 28
                    font.bold: true
                    color: "#009645"
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }
        }

        // Action Buttons
        RowLayout {
            width: parent.width
            spacing: 16

            // Send Money Button
            Button {
                Layout.fillWidth: true
                Layout.preferredHeight: 55
                
                contentItem: Text {
                    text: "📤 Send Money"
                    font.family: Style.mainFont
                    font.pixelSize: 16
                    font.bold: true
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                
                background: Rectangle {
                    color: "#009645"
                    radius: 12
                }
                
                // Open Send Money Screen
                onClicked: mainStack.push("SendMoneyPage.qml")
            }

            // Receive Money Button
            Button {
                Layout.fillWidth: true
                Layout.preferredHeight: 55
                
                contentItem: Text {
                    text: "📥 Receive Money"
                    font.family: Style.mainFont
                    font.pixelSize: 16
                    font.bold: true
                    color: "#009645"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                
                background: Rectangle {
                    color: "transparent"
                    radius: 12
                    border.color: "#009645"
                    border.width: 2
                }
                
                // Open Receive Money Screen
                onClicked: mainStack.push("ReceiveMoneyPage.qml")
            }
        }
    }
}
