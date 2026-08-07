import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: rootwindow
    visible: true
    width: 360
    height: 640
    title: "Nepal Premium Bank"
    color: "#f7f9fa"

    // Global properties accessible on all screens
    property string currentUsername:"pratik"
    property string userSessionPassword: "thapa"

    // Master Navigation Controller
    StackView {
        id: mainStack
        anchors.fill: parent
        initialItem: "views/LoginPage.qml" // Starts on the login page
    }
}