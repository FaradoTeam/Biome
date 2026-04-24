project(farado_tests)

enable_testing()

if(BUILD_DTO_TESTS)
    add_subdirectory(dto)
endif()

if(BUILD_STORAGE_TESTS)
    add_subdirectory(storage)
endif()

if(BUILD_REPO_TESTS)
    add_subdirectory(repo)
endif()

if(BUILD_API_TESTS)
    add_subdirectory(rest_server)
endif()
