import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../theme" as Theme 

Page {
    id: root

    property alias loginText: loginField.text
    property alias passwordText: passwordField.text
    property bool authenticating: false
    property string errorMessage: ""

    signal tryLogin(string login, string password)

    background: Rectangle {
        color: Theme.Theme.bgPrimary
    }

    // Центральная карточка
    Rectangle {
        anchors.centerIn: parent
        width: Math.min(parent.width - 40, 360)
        height: 420
        radius: 20
        color: Theme.Theme.cardBackground
        border.color: Theme.Theme.cardBorder
        border.width: 1

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 24
            spacing: 16

            Text {
                text: "Вход в аккаунт"
                font.pixelSize: 24
                font.bold: true
                color: Theme.Theme.textPrimary
                Layout.alignment: Qt.AlignHCenter
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
            }

            // Поле логина
            TextField {
                id: loginField
                placeholderText: "Логин или email"
                font.pixelSize: 16
                Layout.fillWidth: true
                Layout.preferredHeight: 48
                background: Rectangle {
                    radius: 8
                    color: Theme.Theme.inputBackground
                    border.color: parent.activeFocus ? Theme.Theme.inputBorderFocus : Theme.Theme.inputBorder
                    border.width: 1
                }
            }

            // Поле пароля
            TextField {
                id: passwordField
                placeholderText: "Пароль"
                echoMode: TextField.Password
                font.pixelSize: 16
                Layout.fillWidth: true
                Layout.preferredHeight: 48
                background: Rectangle {
                    radius: 8
                    color: Theme.Theme.inputBackground
                    border.color: parent.activeFocus ? Theme.Theme.inputBorderFocus : Theme.Theme.inputBorder
                    border.width: 1
                }
                onAccepted: loginButton.clicked()
            }

            // Сообщение об ошибке
            Text {
                text: errorMessage
                color: Theme.Theme.textError
                font.pixelSize: 13
                wrapMode: Text.WordWrap
                visible: text !== ""
                Layout.fillWidth: true
            }

            // Кнопка входа
            Button {
                id: loginButton
                text: authenticating ? "Проверка..." : "Войти"
                enabled: !authenticating
                Layout.fillWidth: true
                Layout.preferredHeight: 48
                Layout.topMargin: 8
                contentItem: Text {
                    text: loginButton.text
                    color: Theme.Theme.btnText
                    font.bold: true
                    font.pixelSize: 16
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    radius: 8
                    color: loginButton.enabled
                           ? (loginButton.hovered ? Theme.Theme.btnPrimaryHovered : Theme.Theme.btnPrimaryBg)
                           : Theme.Theme.btnPrimaryDisabled
                }
                onClicked: {
                    errorMessage = ""
                    tryLogin(loginField.text, passwordField.text)
                }
            }

            // Ссылка "Регистрация"
            RowLayout {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 8
                spacing: 4

                Text {
                    text: "Нет аккаунта?"
                    color: Theme.Theme.textSecondary
                }
                Text {
                    text: "Зарегистрироваться"
                    color: Theme.Theme.linkColor
                    font.bold: true
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: console.log("Переход на регистрацию")
                    }
                }
            }
        }
    }
}