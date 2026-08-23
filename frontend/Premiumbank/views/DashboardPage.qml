import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import bank

Item {
    Column {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 24

        // Header
        Row {
            width: parent.width

            Column {
                width: parent.width - refreshLogoutRow.width
                spacing: 4

                Text {
                    text: backend.accountInfoAvailable
                          ? "Good Evening, " + backend.accountName
                          : "Good Evening, " + rootwindow.currentUsername
                    font.family: Style.mainFont
                    font.pixelSize: 22
                    font.bold: true
                    color: "#1a237e"
                }

                Text {
                    text: backend.accountInfoAvailable
                          ? "Account No: " + backend.accountId
                          : "Account No: " + rootwindow.currentUsername
                    font.family: Style.mainFont
                    font.pixelSize: 14
                    color: "#666666"
                }
            }

            Row {
                id: refreshLogoutRow
                spacing: 8
                anchors.verticalCenter: parent.verticalCenter

                // Refresh: re-fetches this account's current balance/info
                // from the DB, bypassing the server's in-memory cache, so
                // this picks up changes made from other sessions too.
                Button {
                    id: refreshButton
                    text: "⟳"
                    width: 40
                    height: 40

                    background: Rectangle {
                        color: "transparent"
                        radius: 20
                        border.color: "#009645"
                        border.width: 1
                    }

                    contentItem: Text {
                        text: refreshButton.text
                        color: "#009645"
                        font.pixelSize: 18
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    onClicked: {
                        // refresh() takes a user_id (matching the backend's
                        // /login "refresh" request), not an account_id —
                        // rootwindow.currentUsername holds the user_id
                        // typed at login and is the only place it's kept.
                        refreshButton.enabled = false
                        backend.refresh(parseInt(rootwindow.currentUsername))
                    }

                    Connections {
                        target: backend
                        function onLoginResult(success, message) {
                            refreshButton.enabled = true
                        }
                    }
                }

                // Logout: local-only, clears cached session state and
                // returns to the login screen. Does not notify the
                // backend (no logout protocol message exists yet).
                Button {
                    text: "Logout"

                    background: Rectangle {
                        color: "#e0e0e0"
                        radius: 8
                    }

                    contentItem: Text {
                        text: "Logout"
                        color: "#444444"
                        font.pixelSize: 13
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    onClicked: {
                        backend.logout()
                        rootwindow.currentUsername = ""
                        rootwindow.userSessionPassword = ""
                        mainStack.pop(null)
                    }
                }
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
                    text: backend.accountInfoAvailable
                          ? "NPR " + (backend.accountBalanceCents / 100).toLocaleString(Qt.locale(), 'f', 2)
                          : "Balance unavailable"
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