// LanguageSelector.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import wikipedia_qt

ComboBox {
    id: languageSelector
    objectName: "languageSelector"
    
    // Available languages with their display names and codes
    model: [
        {"code": "en", "name": "English"},
        {"code": "fr", "name": "Français"},
        {"code": "de", "name": "Deutsch"},
        {"code": "es", "name": "Español"},
        {"code": "it", "name": "Italiano"},
        {"code": "pt", "name": "Português"},
        {"code": "ru", "name": "Русский"},
        {"code": "ja", "name": "日本語"},
        {"code": "zh", "name": "中文"},
        {"code": "ar", "name": "العربية"}
    ]
    
    // Display the language name in the dropdown
    textRole: "name"
    valueRole: "code"
    
    // Default to English
    currentIndex: 0
    
    // Make it compact
    implicitWidth: 120
    implicitHeight: 30
    
    // Connect to SettingsState language property
    onActivated: {
        SettingsState.setLanguage(currentValue)
    }
    
    // Keep the header in sync when history restores another edition.
    function syncLanguage() {
        currentIndex = -1
        for (var i = 0; i < model.length; i++) {
            if (model[i].code === SettingsState.language) {
                currentIndex = i
                break
            }
        }
    }
    Component.onCompleted: syncLanguage()
    Connections {
        target: SettingsState
        function onLanguageChanged() { languageSelector.syncLanguage() }
    }
}
