import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import bank

Item {
    id: adminDashboardPage

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // TOP HEADER BAR
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            color: "#001F3F" // Deep Navy Blue

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 20
                anchors.rightMargin: 20

                Text {
                    text: "Admin Control Center"
                    color: "white"
                    font.family: Style.mainFont
                    font.pixelSize: 18
                    font.bold: true
                }

                Item { Layout.fillWidth: true } // Spacer

                Button {
                    text: "Logout"
                    onClicked: mainStack.pop()

                    background: Rectangle {
                        color: "transparent"
                        border.color: "white"
                        border.width: 1
                        radius: 4
                    }
                    contentItem: Text {
                        text: "Logout"
                        color: "white"
                        font.pixelSize: 12
                    }
                }
            }
        }

        // DEPOSIT / WITHDRAW
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 24
            spacing: 16

            Text {
                text: "Fund or Debit an Account"
                font.pixelSize: 16
                font.bold: true
                color: "#333333"
            }

            Text {
                text: "Enter the target account number, choose Deposit or Withdraw, and confirm. This moves real money via the backend — same transaction engine as Send Money."
                font.pixelSize: 12
                color: "#666666"
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                Layout.preferredWidth: 480
            }

            ColumnLayout {
                Layout.preferredWidth: 420
                spacing: 12

                TextField {
                    id: targetAccountField
                    placeholderText: "Target Account Number"
                    inputMethodHints: Qt.ImhDigitsOnly
                    validator: IntValidator { bottom: 1 }
                    Layout.fillWidth: true
                }

                TextField {
                    id: adminAmountField
                    placeholderText: "Amount (NPR)"
                    inputMethodHints: Qt.ImhFormattedNumbersOnly
                    validator: DoubleValidator { bottom: 0.01 }
                    Layout.fillWidth: true
                }

                TextField {
                    id: adminRemarksField
                    placeholderText: "Remarks (optional)"
                    Layout.fillWidth: true
                }

                RowLayout {
                    spacing: 12
                    Layout.fillWidth: true

                    Button {
                        id: adminDepositButton
                        text: "Deposit"
                        Layout.fillWidth: true
                        Layout.preferredHeight: 46

                        background: Rectangle {
                            color: "#009645"
                            radius: 8
                        }
                        contentItem: Text {
                            text: "➕ Deposit"
                            color: "white"
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }

                        onClicked: {
                            if (targetAccountField.text === "" || adminAmountField.text === "") {
                                adminStatusText.color = "red"
                                adminStatusText.text = "Enter an account number and amount."
                                return
                            }

                            adminStatusText.color = "#666666"
                            adminStatusText.text = "Processing deposit..."
                            adminDepositButton.enabled = false
                            adminWithdrawButton.enabled = false

                            var amountCents = Math.round(parseFloat(adminAmountField.text) * 100)
                            backend.deposit(parseInt(targetAccountField.text), amountCents, adminRemarksField.text)
                        }
                    }

                    Button {
                        id: adminWithdrawButton
                        text: "Withdraw"
                        Layout.fillWidth: true
                        Layout.preferredHeight: 46

                        background: Rectangle {
                            color: "#B00020"
                            radius: 8
                        }
                        contentItem: Text {
                            text: "➖ Withdraw"
                            color: "white"
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }

                        onClicked: {
                            if (targetAccountField.text === "" || adminAmountField.text === "") {
                                adminStatusText.color = "red"
                                adminStatusText.text = "Enter an account number and amount."
                                return
                            }

                            adminStatusText.color = "#666666"
                            adminStatusText.text = "Processing withdrawal..."
                            adminDepositButton.enabled = false
                            adminWithdrawButton.enabled = false

                            var amountCents = Math.round(parseFloat(adminAmountField.text) * 100)
                            backend.withdraw(parseInt(targetAccountField.text), amountCents, adminRemarksField.text)
                        }
                    }
                }

                Text {
                    id: adminStatusText
                    text: ""
                    font.pixelSize: 13
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
            }

            Item { Layout.fillHeight: true } // Spacer
        }
    }

    Connections {
        target: backend
        function onTransactionResult(success, message) {
            adminDepositButton.enabled = true
            adminWithdrawButton.enabled = true

            if (success) {
                // Parse the target account's new balance directly from
                // this response rather than reading backend.accountBalanceCents
                // — that property reflects whichever account this message
                // was about, so using it here would silently overwrite the
                // *admin's own* cached balance with the target account's balance.
                var newBalanceText = ""
                try {
                    var parsed = JSON.parse(message)
                    if (parsed && typeof parsed.balance_cents === "number") {
                        newBalanceText = " New balance: NPR " +
                            (parsed.balance_cents / 100).toLocaleString(Qt.locale(), 'f', 2)
                    }
                } catch (e) {
                    // message wasn't JSON — just skip the balance detail
                }

                adminStatusText.color = "#009645"
                adminStatusText.text = "Success." + newBalanceText
                targetAccountField.text = ""
                adminAmountField.text = ""
                adminRemarksField.text = ""
            } else {
                adminStatusText.color = "red"
                adminStatusText.text = (message && message !== "") ? message : "Transaction failed."
            }
        }
    }
}