import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: adminDashboardPage

    // Signals to connect with your C++ backend controller
    signal verifyUser(string userId)
    signal suspendUser(string userId)
    signal viewUserDetails(string userId)

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // 1. TOP HEADER BAR
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

        // 2. TAB NAVIGATION
        TabBar {
            id: adminTabBar
            Layout.fillWidth: true

            TabButton { text: "User Management & Verification" }
            TabButton { text: "Transaction Ledger" }
        }

        // 3. TAB CONTENT VIEWS
        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: adminTabBar.currentIndex

            // ==========================================
            // TAB 1: USER MANAGEMENT & VERIFICATION
            // ==========================================
            Item {
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 12

                    Text {
                        text: "Account Holders Overview"
                        font.pixelSize: 16
                        font.bold: true
                        color: "#333333"
                    }

                    // User Accounts List
                    ListView {
                        id: userListView
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        spacing: 8

                        // Sample Model (Replace with your C++ QAbstractListModel or JSON backend)
                        model: ListModel {
                            ListElement { userId: "NPB1001"; fullName: "Pranab Kharel"; mobile: "9841234567"; accountType: "Savings"; status: "Pending" }
                            ListElement { userId: "NPB1002"; fullName: "Sakar Baby Dollakoti"; mobile: "9812345678"; accountType: "Current"; status: "Active" }
                            ListElement { userId: "NPB1003"; fullName: "Pratik Singh Thapa"; mobile: "9801122334"; accountType: "Savings"; status: "Active" }
                        }

                        delegate: Rectangle {
                            width: userListView.width
                            height: 70
                            color: "#F9F9F9"
                            border.color: "#E0E0E0"
                            radius: 6

                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 12
                                spacing: 10

                                ColumnLayout {
                                    spacing: 2
                                    Text { text: model.fullName + " (" + model.userId + ")"; font.bold: true; font.pixelSize: 14; color: "#001F3F" }
                                    Text { text: model.accountType + " • " + model.mobile; font.pixelSize: 12; color: "#666666" }
                                }

                                Item { Layout.fillWidth: true } // Spacer

                                // Status Badge
                                Rectangle {
                                    Layout.preferredWidth: 70
                                    Layout.preferredHeight: 24
                                    radius: 12
                                    color: model.status === "Active" ? "#E8F5E9" : (model.status === "Pending" ? "#FFF3E0" : "#FFEBEE")

                                    Text {
                                        anchors.centerIn: parent
                                        text: model.status
                                        font.pixelSize: 11
                                        font.bold: true
                                        color: model.status === "Active" ? "#2E7D32" : (model.status === "Pending" ? "#E65100" : "#C62828")
                                    }
                                }

                                // Action Buttons
                                Button {
                                    text: model.status === "Pending" ? "Verify" : "Suspend"
                                    Layout.preferredHeight: 32
                                    onClicked: {
                                        if (model.status === "Pending") {
                                            adminDashboardPage.verifyUser(model.userId)
                                            model.status = "Active" // Immediate UI update
                                        } else {
                                            adminDashboardPage.suspendUser(model.userId)
                                            model.status = "Suspended"
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // ==========================================
            // TAB 2: TRANSACTION MONITORING
            // ==========================================
            Item {
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 12

                    RowLayout {
                        Layout.fillWidth: true
                        Text {
                            text: "Audit Live Transactions"
                            font.pixelSize: 16
                            font.bold: true
                            color: "#333333"
                        }
                        Item { Layout.fillWidth: true }
                        TextField {
                            placeholderText: "Search User or Txn ID..."
                            Layout.preferredWidth: 200
                        }
                    }

                    // Transactions List
                    ListView {
                        id: txnListView
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        spacing: 8

                        model: ListModel {
                            ListElement { txnId: "TXN99201"; sender: "NPB1002"; receiver: "NPB1003"; amount: "NPR 15,000"; time: "10:42 AM"; status: "Completed" }
                            ListElement { txnId: "TXN99202"; sender: "NPB1001"; receiver: "NPB1002"; amount: "NPR 2,500"; time: "11:15 AM"; status: "Flagged" }
                            ListElement { txnId: "TXN99203"; sender: "NPB1003"; receiver: "NPB1001"; amount: "NPR 50,000"; time: "11:30 AM"; status: "Completed" }
                        }

                        delegate: Rectangle {
                            width: txnListView.width
                            height: 65
                            color: "#FFFFFF"
                            border.color: "#E0E0E0"
                            radius: 6

                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 12

                                ColumnLayout {
                                    spacing: 2
                                    Text { text: model.txnId + " • " + model.time; font.bold: true; font.pixelSize: 13; color: "#333333" }
                                    Text { text: "From: " + model.sender + " ➔ To: " + model.receiver; font.pixelSize: 12; color: "#666666" }
                                }

                                Item { Layout.fillWidth: true }

                                Text {
                                    text: model.amount
                                    font.bold: true
                                    font.pixelSize: 14
                                    color: "#001F3F"
                                    Layout.rightMargin: 10
                                }

                                Text {
                                    text: model.status
                                    font.bold: true
                                    font.pixelSize: 11
                                    color: model.status === "Flagged" ? "red" : "green"
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}