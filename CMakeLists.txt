cmake_minimum_required(VERSION 3.15.7)
project(farado)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Опции сборки проекта
# ---------------------------------------------------------------------------- #
option(BUILD_VAULT_SERVER "Build Farado Vault Server" ON)
option(BUILD_ALL "Build all" OFF)
# ---------------------------------------------------------------------------- #

# Опции сборки тестов
# ---------------------------------------------------------------------------- #
option(ENABLE_TESTING "Enable testing" OFF)
option(BUILD_DTO_TESTS "Build tests for data transfer objects" ON)
option(BUILD_STORAGE_TESTS "Build tests for storage" ON)
option(BUILD_API_TESTS "Build tests for rest server" ON)
# ---------------------------------------------------------------------------- #

# Опции для генерации кода
# ---------------------------------------------------------------------------- #
option(GENERATE_DTO "Generate DTO classes from OpenAPI" ON)
option(GENERATE_DTO_TESTS "Generate DTO tests from OpenAPI" ON)
option(FORCE_REGENERATE_DTO "Force regeneration of DTO even if files exist" OFF)
# ---------------------------------------------------------------------------- #

# Подключаем вспомогательные модули
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(GenerateDTO)

if(BUILD_VAULT_SERVER OR BUILD_ALL)
    add_subdirectory(vault-server)
endif()

if(ENABLE_TESTING OR BUILD_ALL)
    enable_testing()
    add_subdirectory(tests)
endif()

# Вывод информации о конфигурации
# ---------------------------------------------------------------------------- #
message(STATUS "=== Farado Project Configuration ===")
message(STATUS "Build Vault Server: ${BUILD_VAULT_SERVER}")
message(STATUS "Build Tools: ${BUILD_TOOLS}")
message(STATUS "Build Tests: ${BUILD_TESTS}")
message(STATUS "Generate DTO: ${GENERATE_DTO}")
message(STATUS "Generate DTO Tests: ${GENERATE_DTO_TESTS}")
message(STATUS "====================================")
