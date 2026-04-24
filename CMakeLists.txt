cmake_minimum_required(VERSION 3.15.7)

# Опции сборки проекта
# ---------------------------------------------------------------------------- #
option(BUILD_VAULT_SERVER "Build Farado Vault Server" ON)
option(BUILD_CLIENT "Build Client" ON)
option(BUILD_MIGRATOR "Build database migration tool" ON)
# ---------------------------------------------------------------------------- #

# Опции сборки тестов
# ---------------------------------------------------------------------------- #
option(ENABLE_TESTING "Enable testing" OFF)
option(BUILD_DTO_TESTS "Build tests for data transfer objects" ON)
option(BUILD_STORAGE_TESTS "Build tests for storage" ON)
option(BUILD_REPO_TESTS "Build tests for repository" ON)
option(BUILD_API_TESTS "Build tests for rest server" ON)
# ---------------------------------------------------------------------------- #

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(CMAKE_PROJECT_TOP_LEVEL_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/conan_provider.cmake
)

project(farado)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(BUILD_VAULT_SERVER)
    add_subdirectory(vault-server)
endif()

if(BUILD_CLIENT)
    add_subdirectory(client)
endif()

if(BUILD_MIGRATOR)
    add_subdirectory(tools/migrator)
endif()

if(ENABLE_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()

# Вывод информации о конфигурации
# ---------------------------------------------------------------------------- #
message(STATUS "=== Farado Project Configuration ===")
message(STATUS "Build Vault Server: ${BUILD_VAULT_SERVER}")
message(STATUS "Enable testing: ${ENABLE_TESTING}")
message(STATUS "Generate DTO: ${GENERATE_DTO}")
message(STATUS "Generate DTO Tests: ${GENERATE_DTO_TESTS}")
message(STATUS "====================================")
