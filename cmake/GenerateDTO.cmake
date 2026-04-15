# Модуль для генерации DTO из OpenAPI спецификации

function(generate_dto_from_openapi)
    set(options FORCE)
    set(oneValueArgs OPENAPI_FILE OUTPUT_DIR TEMPLATES_DIR PYTHON_SCRIPT)
    set(multiArgs "")
    cmake_parse_arguments(GEN_DTO "${options}" "${oneValueArgs}" "${multiArgs}" ${ARGN})

    if(NOT GEN_DTO_OPENAPI_FILE)
        message(FATAL_ERROR "OPENAPI_FILE is required for generate_dto_from_openapi")
    endif()
    if(NOT GEN_DTO_OUTPUT_DIR)
        message(FATAL_ERROR "OUTPUT_DIR is required for generate_dto_from_openapi")
    endif()
    if(NOT GEN_DTO_TEMPLATES_DIR)
        message(FATAL_ERROR "TEMPLATES_DIR is required for generate_dto_from_openapi")
    endif()
    if(NOT GEN_DTO_PYTHON_SCRIPT)
        message(FATAL_ERROR "PYTHON_SCRIPT is required for generate_dto_from_openapi")
    endif()

    # Создаем список выходных файлов (пока неизвестных)
    set(GENERATED_FILES_VAR "GENERATED_DTO_FILES" PARENT_SCOPE)

    # Команда для генерации
    add_custom_target(generate_dto ALL
        COMMAND ${CMAKE_COMMAND} -E echo "Generating DTO from OpenAPI..."
        COMMAND ${CMAKE_COMMAND} -E make_directory ${GEN_DTO_OUTPUT_DIR}
        COMMAND python3 ${GEN_DTO_PYTHON_SCRIPT}
            --openapi ${GEN_DTO_OPENAPI_FILE}
            --output ${GEN_DTO_OUTPUT_DIR}
            --templates ${GEN_DTO_TEMPLATES_DIR}
        COMMENT "Generating DTO classes from OpenAPI specification"
        VERBATIM
    )

    # Добавляем зависимость от скрипта и openapi файла
    set_property(DIRECTORY APPEND PROPERTY
        ADDITIONAL_CLEAN_FILES ${GEN_DTO_OUTPUT_DIR}
    )

    # Если нужно принудительно генерировать при каждой сборке
    if(GEN_DTO_FORCE)
        set_property(TARGET generate_dto PROPERTY ALWAYS_TRUE TRUE)
    endif()

    # Устанавливаем переменную с выходной директорией
    set(DTO_GENERATED_DIR ${GEN_DTO_OUTPUT_DIR} PARENT_SCOPE)
endfunction()

function(generate_dto_tests_from_openapi)
    set(options FORCE)
    set(oneValueArgs OPENAPI_FILE OUTPUT_DIR TEMPLATES_DIR PYTHON_SCRIPT)
    set(multiArgs "")
    cmake_parse_arguments(GEN_TEST "${options}" "${oneValueArgs}" "${multiArgs}" ${ARGN})

    if(NOT GEN_TEST_OPENAPI_FILE)
        message(FATAL_ERROR "OPENAPI_FILE is required for generate_dto_tests_from_openapi")
    endif()
    if(NOT GEN_TEST_OUTPUT_DIR)
        message(FATAL_ERROR "OUTPUT_DIR is required for generate_dto_tests_from_openapi")
    endif()
    if(NOT GEN_TEST_TEMPLATES_DIR)
        message(FATAL_ERROR "TEMPLATES_DIR is required for generate_dto_tests_from_openapi")
    endif()
    if(NOT GEN_TEST_PYTHON_SCRIPT)
        message(FATAL_ERROR "PYTHON_SCRIPT is required for generate_dto_tests_from_openapi")
    endif()

    add_custom_target(generate_dto_tests ALL
        COMMAND ${CMAKE_COMMAND} -E echo "Generating DTO tests..."
        COMMAND ${CMAKE_COMMAND} -E make_directory ${GEN_TEST_OUTPUT_DIR}
        COMMAND python3 ${GEN_TEST_PYTHON_SCRIPT}
            --openapi ${GEN_TEST_OPENAPI_FILE}
            --output ${GEN_TEST_OUTPUT_DIR}
            --templates ${GEN_TEST_TEMPLATES_DIR}
        COMMENT "Generating DTO tests from OpenAPI specification"
        VERBATIM
    )

    if(GEN_TEST_FORCE)
        set_property(TARGET generate_dto_tests PROPERTY ALWAYS_TRUE TRUE)
    endif()

    set(DTO_TEST_GENERATED_DIR ${GEN_TEST_OUTPUT_DIR} PARENT_SCOPE)
endfunction()

if(GENERATE_DTO)
    set(DTO_OUTPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/common/dto)
    set(OPENAPI_FILE ${CMAKE_CURRENT_SOURCE_DIR}/api/openapi.yaml)
    set(TEMPLATES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tools/code_generators/templates)
    set(DTO_GENERATOR_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/tools/code_generators/common_dto_generator.py)

    if(EXISTS ${OPENAPI_FILE} AND EXISTS ${DTO_GENERATOR_SCRIPT})
        generate_dto_from_openapi(
            OPENAPI_FILE ${OPENAPI_FILE}
            OUTPUT_DIR ${DTO_OUTPUT_DIR}
            TEMPLATES_DIR ${TEMPLATES_DIR}
            PYTHON_SCRIPT ${DTO_GENERATOR_SCRIPT}
            FORCE ${FORCE_REGENERATE_DTO}
        )

        # Добавляем зависимость для целей, которые используют DTO
        set(DTO_GENERATED_DIR ${DTO_OUTPUT_DIR})
        set(DTO_GENERATION_TARGET generate_dto)
    else()
        message(WARNING "Cannot generate DTO: OpenAPI file or generator script not found")
    endif()
endif()

if(GENERATE_DTO_TESTS)
    set(DTO_TEST_OUTPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tests/dto)
    set(OPENAPI_FILE ${CMAKE_CURRENT_SOURCE_DIR}/api/openapi.yaml)
    set(TEMPLATES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tools/code_generators/templates)
    set(DTO_TEST_GENERATOR_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/tools/code_generators/test_dto_generator.py)

    if(EXISTS ${OPENAPI_FILE} AND EXISTS ${DTO_TEST_GENERATOR_SCRIPT})
        generate_dto_tests_from_openapi(
            OPENAPI_FILE ${OPENAPI_FILE}
            OUTPUT_DIR ${DTO_TEST_OUTPUT_DIR}
            TEMPLATES_DIR ${TEMPLATES_DIR}
            PYTHON_SCRIPT ${DTO_TEST_GENERATOR_SCRIPT}
            FORCE ${FORCE_REGENERATE_DTO}
        )
        set(DTO_TEST_GENERATED_DIR ${DTO_TEST_OUTPUT_DIR})
        set(DTO_TEST_GENERATION_TARGET generate_dto_tests)
    endif()
endif()
