import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: window
    visible: true
    width: 360
    height: 640
    title: "Simple Banking"
    color: "#f7f9fa"

    // Global properties accessible on all screens
    property string userSessionPassword: ""

    // Master Navigation Controller
    StackView {
        id: mainStack
        anchors.fill: parent
        initialItem: "views/LoginPage.qml" // Starts on the login page
    }
}