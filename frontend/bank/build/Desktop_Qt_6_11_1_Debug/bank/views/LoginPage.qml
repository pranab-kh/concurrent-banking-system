import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: loginPage

    ColumnLayout {
        anchors.centerIn: parent
        width: parent.width * 0.85
        spacing: 16

        Text {
            text: "Welcome Back, Pratik"
            font.family: Style.mainFont
            font.pixelSize: 30
            font.bold: true
            color: "orange"
            Layout.alignment: Qt.AlignHCenter
        }
        Text {
            text: "Account Holder: " + currentUserName
            font.family: Style.mainFont
            font.pixelSize: 14
            font.bold: true
            color: "#555555"
            Layout.alignment: Qt.AlignHCenter
        }
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
                //validation in C++ check
                if (usernameField.text !== "" && passwordField.text !== "") {
                    // Save password globally to check later during transfers
                    userSessionPassword = passwordField.text 
                    
                    // Push the Dashboard screen onto the view
                    mainStack.push("DashboardPage.qml")
                }
            }
        }
    }
}
