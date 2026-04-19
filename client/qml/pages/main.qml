import QtQuick 2.15
import QtQuick.Controls 2.15
import "../components" as Components
import "."  

ApplicationWindow {
    visible: true
    width: 480
    height: 800
    title: "KabanClient"

    property bool loggedIn: false

    StackView {
        id: stackView
        anchors.fill: parent
        initialItem: loggedIn ? mainPage : loginPage
    }

    Component {
        id: loginPage
        AuthPage { 
            onTryLogin: (login, pwd) => {
                if (login === "user" && pwd === "123") {
                    loggedIn = true
                    stackView.replace(mainPage)
                } else {
                    errorMessage = "Неверный логин или пароль"
                }
            }
        }
    }

    Components.ButtonGroup {
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.leftMargin: 10
        anchors.bottomMargin: parent.height * 0.03 
        width: 200 
        z: 1
    
        Components.GearButton {
            width: Math.max(30, parent.width * 0.12)
            height: width
            checkable: true
            checked: true
        }
    }

    Component {
        id: mainPage
        Page {
            Text {
                anchors.centerIn: parent
                text: "Добро пожаловать!"
                font.pixelSize: 24
            }
            Button {
                text: "Выйти"
                anchors.bottom: parent.bottom
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.margins: 20
                onClicked: {
                    loggedIn = false
                    stackView.replace(loginPage)
                }
            }
        }
    }
}