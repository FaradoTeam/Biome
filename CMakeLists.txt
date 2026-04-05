cmake_minimum_required(VERSION 3.15.7)
project(farado)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Опции сборки проекта
# ---------------------------------------------------------------------------- #
option(BUILD_VAULT_SERVER "Build Farado Vault Server" ON)
option(BUILD_TESTS "Build tests" OFF)
option(BUILD_ALL "Build all" OFF)
# ---------------------------------------------------------------------------- #

if(BUILD_VAULT_SERVER OR BUILD_ALL)
    add_subdirectory(vault-server)
endif()

if(BUILD_TESTS OR BUILD_ALL)
    enable_testing()
    add_subdirectory(tests)
endif()
