import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: loginPage

    ColumnLayout {
        anchors.top: parent.top
        anchors.topMargin: 50
        anchors.horizontalCenter: parent.horizontalCenter
        width: parent.width * 0.85
        spacing: 16

        // 1. LOGO EMBLEM
        Rectangle {
            Layout.preferredWidth: 150
            Layout.preferredHeight: 150
            Layout.alignment: Qt.AlignHCenter
            color: "transparent"
            radius: width / 2
            border.width: 3
            border.color: "transparent"
            clip: true

            Image {
                anchors.fill: parent
                Layout.alignment: Qt.AlignHCenter
                source: "../image/logoo.png"
                fillMode: Image.PreserveAspectCrop
            }
        }

        // 2. BANK TITLE
        Text {
            text: "Nepal Premium Bank"
            font.family: Style.mainFont
            font.pixelSize: 30
            font.bold: true
            color: "#002B49"
            Layout.alignment: Qt.AlignHCenter
            Layout.bottomMargin: 15
        }

        // Safe display check for currentUserName
        Text {
            text: "Account Holder: " + (typeof currentUserName !== "undefined" ? currentUserName : "Guest")
            font.family: Style.mainFont
            font.pixelSize: 14
            font.bold: true
            color: "#555555"
            Layout.alignment: Qt.AlignHCenter
        }

        // 3. INPUT FIELDS
        TextField {
            id: usernameField
            placeholderText: "Username"
            Layout.fillWidth: true
        }

        TextField {
            id: passwordField
            placeholderText: "Password"
            echoMode: TextInput.Password
            Layout.fillWidth: true
        }

        // Status / Error message display
        Text {
            id: statusText
            text: ""
            color: "red"
            font.pixelSize: 12
            Layout.alignment: Qt.AlignHCenter
            visible: text !== ""
        }

        // 4. LOGIN BUTTON
        Button {
            text: "Login"
            Layout.fillWidth: true
            Layout.preferredHeight: 50

            background: Rectangle {
                color: "#009645"
                radius: 10
            }

            contentItem: Text {
                text: "Login"
                color: "white"
                font.bold: true
                font.family: Style.mainFont
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            onClicked: {
                if (usernameField.text === "" || passwordField.text === "") {
                    statusText.text = "Please enter both username and password."
                    return
                }

                statusText.text = ""

                // Check Admin Credentials
                if (usernameField.text.trim() === "admin" && passwordField.text === "admin123") {
                    usernameField.text = ""
                    passwordField.text = ""
                    mainStack.push("AdminPanel.qml")
                }
                // Customer Login
                else {
                    // Safe property assignments
                    if (typeof rootWindow !== "undefined") {
                        rootWindow.userSessionPassword = passwordField.text
                        rootWindow.currentUserName = usernameField.text
                    }

                    usernameField.text = ""
                    passwordField.text = ""
                    mainStack.push("DashboardPage.qml")
                }
            }
        }

        // 5. REGISTRATION LINK
        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 15
            spacing: 5

            Text {
                text: "Don't have an account?"
                font.pixelSize: 14
                color: "#555555"
            }

            Text {
                text: "Apply Now"
                font.pixelSize: 14
                font.bold: true
                color: "#001F3F"

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        mainStack.push("CreateAccount.qml")
                    }
                }
            }
        }
    }
}