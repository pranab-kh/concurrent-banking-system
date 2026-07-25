import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

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
            text: "Send Money"
            font.family: Style.mainFont
            font.pixelSize: 22
            font.bold: true
            color: "#1a237e"
        }

        TextField {
            id: recipientName
            placeholderText: "Recipient's Name"
            Layout.fillWidth: true
        }

        TextField {
            id: recipientAccount
            placeholderText: "Account Number"
            Layout.fillWidth: true
        }

        TextField {
            id: amountField
            placeholderText: "Amount (NPR)"
            inputMethodHints: Qt.ImhFormattedNumbersOnly
            Layout.fillWidth: true
        }

        TextField {
            id: verifyPassword
            placeholderText: "Enter Login Password to Confirm"
            echoMode: TextInput.Password
            Layout.fillWidth: true
            font.family: Style.mainFont
        }

        Text {
            id: errorMsg
            text: ""
            color: "red"
            visible: text !== ""
        }

        Button {
            text: "Confirm Transfer"
            Layout.fillWidth: true
            Layout.preferredHeight: 50

            background: Rectangle {
                color: "#009645"
                radius: 10
            }

            contentItem: Text {
                text: "Confirm Transfer"
                color: "white"
                font.bold: true
                font.family: Style.mainFont
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            onClicked: {
                // Verify if the password matches the login password
                if (verifyPassword.text === userSessionPassword) {
                    // Trigger backend transaction
                    transactionBackend.sendMoney(parseFloat(amountField.text), recipientAccount.text)

                    // Return to dashboard
                    mainStack.pop()
                } else {
                    errorMsg.text = "Incorrect password! Try again."
                }
            }
        }

        Item { Layout.fillHeight: true } // Spacer
    }
}