import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import bank

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

            // Hidden admin entry point: long-press the logo. No real admin
            // authentication exists on the backend yet, so this is
            // deliberately not a visible/labeled button — just a way for
            // testing to reach AdminPanel.qml without exposing it to
            // regular users. Replace with a proper admin login flow once
            // the backend has one.
            MouseArea {
                anchors.fill: parent
                onPressAndHold: mainStack.push("AdminPanel.qml")
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
            placeholderText: "User ID"
            inputMethodHints: Qt.ImhDigitsOnly
            validator: IntValidator { bottom: 0 }
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
            id: loginButton
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
                    statusText.text = "Please enter both User ID and password."
                    return
                }

                statusText.text = "Logging in..."
                loginButton.enabled = false

                // Stash the password so DashboardPage / SendMoneyPage can
                // re-verify it locally (matches existing userSessionPassword
                // pattern used elsewhere in the app).
                rootwindow.userSessionPassword = passwordField.text
                rootwindow.currentUsername = usernameField.text

                backend.login(parseInt(usernameField.text), passwordField.text)
            }
        }

        Connections {
            target: backend
            function onLoginResult(success, message) {
                loginButton.enabled = true

                if (success) {
                    statusText.text = ""
                    usernameField.text = ""
                    passwordField.text = ""
                    mainStack.push("DashboardPage.qml")
                } else {
                    statusText.text = (message && message !== "") ? message : "Login failed. Please try again."
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