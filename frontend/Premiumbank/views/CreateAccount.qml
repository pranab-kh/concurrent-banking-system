import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: createaccount

    // Signal to send the request data to C++ backend or handler
    signal submitAccountCreation(
        string userId,
        string fullName,
        string address,
        string mobile,
        string email,
        string gender,
        string nid,
        string accountType,
        string password
    )

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true

        ColumnLayout {
            anchors.horizontalCenter: parent.horizontalCenter
            width: parent.width * 0.85
            spacing: 12

            // Top Header
            Text {
                text: "Create New Account"
                font.family: Style.mainFont
                font.pixelSize: 20
                font.bold: false
                color: "gray" // Deep Navy Blue
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 20
                Layout.bottomMargin: 10
            }
            // 1. USER ID
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4

                            Text {
                                text: "User ID"
                                font.bold: false
                                font.pixelSize: 13
                                color: "#333333"
                            }
                            TextField {
                                id: userIdField
                                placeholderText: "e.g. NPB1001"
                                Layout.fillWidth: true
                            }
                        }

                        // 2. FULL NAME
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4

                            Text {
                                text: "Full Name"
                                font.bold: false
                                font.pixelSize: 13
                                color: "#333333"
                            }
                            TextField {
                                id: fullNameField
                                placeholderText: "Enter full name"
                                Layout.fillWidth: true
                            }
                        }

                        // 3. ADDRESS
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4

                            Text {
                                text: "Address"
                                font.bold: false
                                font.pixelSize: 13
                                color: "#333333"
                            }
                            TextField {
                                id: addressField
                                placeholderText: "e.g. Kathmandu, Nepal"
                                Layout.fillWidth: true
                            }
                        }

                        // 4. MOBILE NUMBER
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4

                            Text {
                                text: "Mobile Number"
                                font.bold: false
                                font.pixelSize: 13
                                color: "#333333"
                            }
                            TextField {
                                id: mobileField
                                placeholderText: "e.g. 98XXXXXXXX"
                                inputMethodHints: Qt.ImhDigitsOnly
                                Layout.fillWidth: true
                            }
                        }

                        // 5. EMAIL
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4

                            Text {
                                text: "Email Address"
                                font.bold: false
                                font.pixelSize: 13
                                color: "#333333"
                            }
                            TextField {
                                id: emailField
                                placeholderText: "e.g. user@example.com"
                                inputMethodHints: Qt.ImhEmailCharactersOnly
                                Layout.fillWidth: true
                            }
                        }

                        // 6. GENDER SELECTION
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4

                            Text {
                                text: "Gender"
                                font.bold: false
                                font.pixelSize: 13
                                color: "#333333"
                            }
                            ComboBox {
                                id: genderBox
                                Layout.fillWidth: true
                                model: ["Male", "Female", "Other"]
                            }
                        }

                        // 7. NATIONAL ID (NID)
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4

                            Text {
                                text: "National ID (NID)"
                                font.bold: false
                                font.pixelSize: 13
                                color: "#333333"
                            }
                            TextField {
                                id: nidField
                                placeholderText: "Enter NID number"
                                Layout.fillWidth: true
                            }
                        }

                        // 8. ACCOUNT TYPE
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4

                            Text {
                                text: "Account Type"
                                font.bold: false
                                font.pixelSize: 13
                                color: "#333333"
                            }
                            ComboBox {
                                id: accountTypeBox
                                Layout.fillWidth: true
                                // Must match the DB's account_table_account_type_check
                                // constraint exactly: CHECKING / SAVING / BUSINESS.
                                model: ["SAVING", "CHECKING", "BUSINESS"]
                            }
                        }

                        // 9. PASSWORD
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4

                            Text {
                                text: "Password"
                                font.bold: false
                                font.pixelSize: 13
                                color: "#333333"
                            }
                            TextField {
                                id: passwordField
                                placeholderText: "Set temporary password"
                                echoMode: TextInput.Password
                                Layout.fillWidth: true
                            }
                        }

                        // Status message on validation failure
                        Text {
                            id: statusText
                            text: ""
                            color: "red"
                            font.pixelSize: 12
                            Layout.alignment: Qt.AlignHCenter
                            visible: text !== ""
                        }

                        // SUBMIT BUTTON
                        Button {
                            id: submitButton
                            text: "Create Account"
                            Layout.fillWidth: true
                            Layout.preferredHeight: 48
                            Layout.topMargin: 10
                            Layout.bottomMargin: 30

                            background: Rectangle {
                                color: "gray" // gray color
                                radius: 8
                            }

                            contentItem: Text {
                                text: "Create Account"
                                color: "white"
                                font.bold: true
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }

                            onClicked: {
                                if (fullNameField.text === "" || passwordField.text === "") {
                                    statusText.text = "Please fill in all required fields!"
                                    return
                                }

                                statusText.text = "Submitting..."
                                submitButton.enabled = false

                                // Trigger the creation signal (kept for anything else listening)
                                createaccount.submitAccountCreation(
                                    userIdField.text,
                                    fullNameField.text,
                                    addressField.text,
                                    mobileField.text,
                                    emailField.text,
                                    genderBox.currentText,
                                    nidField.text,
                                    accountTypeBox.currentText,
                                    passwordField.text
                                )

                                // Actually send it to the backend.
                                // userIdField left blank -> brand-new user (backend creates a fresh user_id).
                                // userIdField filled -> existing user adding another account (verified by password).
                                backend.createAccount(
                                    userIdField.text,
                                    fullNameField.text,
                                    addressField.text,
                                    mobileField.text,
                                    emailField.text,
                                    genderBox.currentText,
                                    nidField.text,
                                    accountTypeBox.currentText,
                                    passwordField.text
                                )
                            }
                        }

                        Connections {
                            target: backend
                            function onCreateAccountResult(success, message) {
                                submitButton.enabled = true

                                if (success) {
                                    statusText.color = "#009645"
                                    statusText.text = "Account created successfully!"
                                    mainStack.pop()
                                } else {
                                    statusText.color = "red"
                                    statusText.text = message !== "" ? message : "Account creation failed."
                                }
                            }
                        }
                    }
                }
}