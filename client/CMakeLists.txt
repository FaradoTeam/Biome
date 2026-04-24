project(biome-client)

set(CMAKE_AUTOMOC ON)   # Включаем Meta-Object Compiler для сигналов/слотов
set(CMAKE_AUTORCC ON)   # Включаем автоматическую компиляцию .qrc ресурсов

find_package(Qt6 REQUIRED)

qt_add_executable(${PROJECT_NAME}
    main.cpp
)

# 4. Подключаем QML модуль к проекту
#    Этот вызов автоматически создаст и свяжет модуль 'MyAppModule',
#    сгенерирует qmldir и qmltypes, и настроит qmlcachegen.
qt_add_resources(${PROJECT_NAME} "qml_resources"
    PREFIX "/"
    FILES
        qml/pages/main.qml
        qml/pages/AuthPage.qml
        qml/components/GearButton.qml
        qml/components/ButtonGroup.qml
        qml/theme/Theme.qml
        qml/theme/qmldir
)

# 5. Явно указываем, что исполняемый файл должен быть связан с нашим QML модулем
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        qt::qt
)

# 6. Для статической сборки: импортируем все необходимые QML плагины
qt_import_qml_plugins(${PROJECT_NAME})

set_target_properties(
    ${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/client
)
