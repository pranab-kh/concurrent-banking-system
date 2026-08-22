import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import bank

Item {
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 16

        // Back Button
        Button {
            text: "← Back"
            onClicked: mainStack.pop()
        }

        Text {
            text: "Deposit Money"
            font.family: Style.mainFont
            font.pixelSize: 22
            font.bold: true
            color: "#1a237e"
        }

        Text {
            text: "Depositing into Account " + rootwindow.currentUsername
            font.family: Style.mainFont
            font.pixelSize: 13
            color: "#666666"
        }

        TextField {
            id: amountField
            placeholderText: "Amount (NPR)"
            inputMethodHints: Qt.ImhFormattedNumbersOnly
            Layout.fillWidth: true
        }

        TextField {
            id: remarksField
            placeholderText: "Remarks (optional)"
            Layout.fillWidth: true
        }

        Text {
            id: errorMsg
            text: ""
            color: "red"
            visible: text !== ""
        }

        Button {
            id: confirmButton
            text: "Confirm Deposit"
            Layout.fillWidth: true
            Layout.preferredHeight: 50

            background: Rectangle {
                color: "#009645"
                radius: 10
            }

            contentItem: Text {
                text: "Confirm Deposit"
                color: "white"
                font.bold: true
                font.family: Style.mainFont
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            onClicked: {
                if (amountField.text === "") {
                    errorMsg.text = "Please enter an amount."
                    return
                }

                var amountCents = Math.round(parseFloat(amountField.text) * 100)
                if (isNaN(amountCents) || amountCents <= 0) {
                    errorMsg.text = "Please enter a valid amount."
                    return
                }

                if (!backend.accountInfoAvailable || backend.accountId <= 0) {
                    errorMsg.text = "No valid account is loaded. Please log in again."
                    return
                }

                errorMsg.text = ""
                confirmButton.enabled = false

                backend.deposit(
                    backend.accountId,
                    amountCents,
                    remarksField.text
                )
            }
        }

        Connections {
            target: backend
            function onTransactionResult(success, message) {
                confirmButton.enabled = true

                if (success) {
                    mainStack.pop()
                } else {
                    errorMsg.text = message !== "" ? message : "Deposit failed."
                }
            }
        }

        Item { Layout.fillHeight: true } // Spacer
    }
}