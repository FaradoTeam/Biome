// ../theme/Theme.qml (пример)
pragma Singleton
import QtQuick 2.15

QtObject {
    property color bgPrimary: '#bad9ee'           // основной фон страницы
    property color alternativeBgPrimary: '#d2b0dad7' // альтернативный фон страницы
    property color cardBackground: "white"       // фон карточки
    property color cardBorder: '#2c2a2a'         // рамка карточки
    property color textPrimary: "#333333"        // основной текст
    property color textSecondary: "#666666"      // второстепенный текст
    property color textError: "#d32f2f"          // текст ошибки
    property color linkColor: "#2196F3"          // цвет ссылки
    property color inputBackground: '#ffffff'    // фон полей ввода
    property color inputBorder: "#dddddd"        // рамка поля
    property color inputBorderFocus: "#2196F3"   // рамка при фокусе
    property color btnPrimaryBg: "#2196F3"       // фон кнопки
    property color btnPrimaryHovered: "#1976D2"  // фон кнопки при наведении
    property color btnPrimaryDisabled: "#B0BEC5" // фон кнопки (отключена)
    property color btnText: "white"              // текст кнопки
}