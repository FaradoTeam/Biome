cmake_minimum_required(VERSION 3.15.7)

# Опции сборки проекта
# ---------------------------------------------------------------------------- #
option(BUILD_VAULT_SERVER "Build Biome Vault Server" ON)
option(BUILD_CLIENT "Build Biome Client" ON)
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

project(biome)

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
message(STATUS "=== Biome Project Configuration ===")
message(STATUS "Build Biome Vault Server: ${BUILD_VAULT_SERVER}")
message(STATUS "Build Biome Client: ${BUILD_CLIENT}")
message(STATUS "Build database migration tool: ${BUILD_MIGRATOR}")
message(STATUS "Enable testing: ${ENABLE_TESTING}")
message(STATUS "Build tests for data transfer objects: ${BUILD_DTO_TESTS}")
message(STATUS "Build tests for storage: ${BUILD_STORAGE_TESTS}")
message(STATUS "Build tests for repository: ${BUILD_REPO_TESTS}")
message(STATUS "Build tests for rest server: ${BUILD_API_TESTS}")
message(STATUS "==================================")
