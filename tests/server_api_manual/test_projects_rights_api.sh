#!/bin/bash

API_HOST="${API_HOST:-127.0.0.1}"
API_PORT="${API_PORT:-8090}"
API_URL="http://${API_HOST}:${API_PORT}"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

TESTS_PASSED=0
TESTS_FAILED=0
declare -A TOKENS
ADMIN_PROJECT_ID=""

print_success() { echo -e "${GREEN}✓${NC} $1"; ((TESTS_PASSED++)); }
print_error() { echo -e "${RED}✗${NC} $1"; ((TESTS_FAILED++)); }
print_info() { echo -e "${BLUE}ℹ${NC} $1"; }
print_warning() { echo -e "${YELLOW}⚠${NC} $1"; }
print_test_header() {
    echo "";
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}";
    echo -e "${BLUE}$1${NC}";
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}";
}

make_request() {
    local method="$1" endpoint="$2" token="$3" data="$4" expected_code="$5" test_name="$6"
    local curl_cmd="curl -s -w '\n%{http_code}'"
    [ -n "$token" ] && [ "$token" != "null" ] && curl_cmd="${curl_cmd} -H 'Authorization: Bearer ${token}'"
    [ -n "$data" ] && curl_cmd="${curl_cmd} -H 'Content-Type: application/json' -d '${data}'"
    curl_cmd="${curl_cmd} -X ${method} '${API_URL}${endpoint}'"

    local response=$(eval "$curl_cmd" 2>&1)
    local http_code=$(echo "$response" | tail -n1)
    local response_body=$(echo "$response" | sed '$d')

    if [ -n "$expected_code" ]; then
        if [ "$http_code" = "$expected_code" ]; then
            print_success "$test_name"
            return 0
        else
            print_error "$test_name - ожидался $expected_code, получен $http_code"
            [ -n "$response_body" ] && echo "  Ответ: $(echo "$response_body" | head -c 200)"
            return 1
        fi
    fi
    echo "$response_body"
}

login() {
    local login="$1" password="$2"
    local data="{\"login\":\"${login}\",\"password\":\"${password}\"}"
    local response=$(curl -s -X POST "${API_URL}/api/v1/auth/login" -H "Content-Type: application/json" -d "$data")
    echo "$response" | grep -o '"access_token":"[^"]*"' | head -1 | cut -d'"' -f4
}

create_project() {
    local token="$1" name="$2"
    local data="{\"caption\":\"${name}\"}"
    local response=$(make_request "POST" "/api/v1/projects" "$token" "$data" 201 "Создание ${name}" 2>/dev/null)
    echo "$response" | grep -o '"id":[0-9]*' | head -1 | cut -d':' -f2
}

# =============================================================================
# Настройка
# =============================================================================

setup() {
    print_info "Настройка тестовых пользователей..."

    TOKENS["admin"]=$(login "admin" "password")
    if [ -z "${TOKENS["admin"]}" ]; then
        print_error "Не удалось получить токен администратора"
        return 1
    fi
    print_success "Токен администратора получен"

    for login in project_manager developer viewer; do
        local user_data="{\"login\":\"${login}\",\"email\":\"${login}@test.com\",\"password\":\"Pass123456\",\"firstName\":\"${login}\",\"lastName\":\"Test\"}"
        make_request "POST" "/api/v1/users" "${TOKENS["admin"]}" "$user_data" 201 "Создание $login" > /dev/null 2>&1
        sleep 0.5
        TOKENS["$login"]=$(login "$login" "Pass123456")
        if [ -n "${TOKENS["$login"]}" ]; then
            print_success "  Токен для $login получен"
        fi
    done

    # Создаем тестовый проект
    local data="{\"caption\":\"Тестовый проект админа\"}"
    local response=$(curl -s -X POST "${API_URL}/api/v1/projects" \
        -H "Authorization: Bearer ${TOKENS["admin"]}" \
        -H "Content-Type: application/json" \
        -d "$data")

    ADMIN_PROJECT_ID=$(echo "$response" | grep -o '"id":[0-9]*' | head -1 | cut -d':' -f2)
    if [ -n "$ADMIN_PROJECT_ID" ] && [ "$ADMIN_PROJECT_ID" != "null" ] && [ "$ADMIN_PROJECT_ID" != "" ]; then
        print_success "Тестовый проект создан с ID: $ADMIN_PROJECT_ID"
    else
        print_warning "Не удалось создать тестовый проект"
    fi
}

# =============================================================================
# Тесты
# =============================================================================

test_unauthorized() {
    print_test_header "Тест 1: Доступ без авторизации"
    make_request "GET" "/api/v1/projects" "" "" 401 "GET без токена"
    make_request "POST" "/api/v1/projects" "" '{"caption":"Test"}' 401 "POST без токена"
    make_request "PUT" "/api/v1/projects/1" "" '{"caption":"Test"}' 401 "PUT без токена"
    make_request "DELETE" "/api/v1/projects/1" "" "" 401 "DELETE без токена"
}

test_admin_access() {
    print_test_header "Тест 2: Супер-администратор"

    # Создаем тестовый проект
    local pid=$(create_project "${TOKENS["admin"]}" "Админ проект")
    if [ -n "$pid" ] && [ "$pid" != "null" ] && [ "$pid" != "" ]; then
        print_success "Админ создал проект $pid"
        make_request "GET" "/api/v1/projects/${pid}" "${TOKENS["admin"]}" "" 200 "GET проект"
        make_request "PUT" "/api/v1/projects/${pid}" "${TOKENS["admin"]}" '{"caption":"Updated"}' 200 "PUT проект"
        make_request "DELETE" "/api/v1/projects/${pid}" "${TOKENS["admin"]}" "" 204 "DELETE проект"
    else
        print_warning "Не удалось создать тестовый проект для админа (возможно, уже существует)"
    fi
    make_request "GET" "/api/v1/projects" "${TOKENS["admin"]}" "" 200 "GET список"
}

test_manager_permissions() {
    print_test_header "Тест 3: Project Manager"

    if [ -z "${TOKENS["project_manager"]}" ]; then
        print_warning "Пропуск: project_manager не авторизован"
        return
    fi

    if [ -z "$ADMIN_PROJECT_ID" ] || [ "$ADMIN_PROJECT_ID" = "null" ]; then
        print_warning "Пропуск: нет тестового проекта"
        return
    fi

    make_request "POST" "/api/v1/projects" "${TOKENS["project_manager"]}" '{"caption":"Test"}' 403 "Создание корневого проекта"
    make_request "GET" "/api/v1/projects/${ADMIN_PROJECT_ID}" "${TOKENS["project_manager"]}" "" 404 "Чтение чужого проекта"
    make_request "PUT" "/api/v1/projects/${ADMIN_PROJECT_ID}" "${TOKENS["project_manager"]}" '{"caption":"Hack"}' 404 "Обновление чужого проекта"

    local data="{\"caption\":\"Sub\",\"parentId\":${ADMIN_PROJECT_ID}}"
    make_request "POST" "/api/v1/projects" "${TOKENS["project_manager"]}" "$data" 403 "Создание подпроекта в чужом проекте"
}

test_developer_permissions() {
    print_test_header "Тест 4: Developer"

    if [ -z "${TOKENS["developer"]}" ]; then
        print_warning "Пропуск: developer не авторизован"
        return
    fi

    if [ -z "$ADMIN_PROJECT_ID" ] || [ "$ADMIN_PROJECT_ID" = "null" ]; then
        print_warning "Пропуск: нет тестового проекта"
        return
    fi

    make_request "POST" "/api/v1/projects" "${TOKENS["developer"]}" '{"caption":"Test"}' 403 "Создание корневого проекта"
    make_request "GET" "/api/v1/projects/${ADMIN_PROJECT_ID}" "${TOKENS["developer"]}" "" 404 "Чтение чужого проекта"
}

test_viewer_permissions() {
    print_test_header "Тест 5: Viewer"

    if [ -z "${TOKENS["viewer"]}" ]; then
        print_warning "Пропуск: viewer не авторизован"
        return
    fi

    if [ -z "$ADMIN_PROJECT_ID" ] || [ "$ADMIN_PROJECT_ID" = "null" ]; then
        print_warning "Пропуск: нет тестового проекта"
        return
    fi

    make_request "POST" "/api/v1/projects" "${TOKENS["viewer"]}" '{"caption":"Test"}' 403 "Создание корневого проекта"
    make_request "GET" "/api/v1/projects/${ADMIN_PROJECT_ID}" "${TOKENS["viewer"]}" "" 404 "Чтение чужого проекта"
}

test_invalid_token() {
    print_test_header "Тест 6: Невалидный токен"
    local invalid="invalid.token.here"
    make_request "GET" "/api/v1/projects" "$invalid" "" 401 "GET с невалидным токеном"
    make_request "POST" "/api/v1/projects" "$invalid" '{"caption":"Test"}' 401 "POST с невалидным токеном"
}

test_pagination() {
    print_test_header "Тест 7: Пагинация"

    # Создаем тестовые проекты для пагинации
    for i in 1 2 3; do
        create_project "${TOKENS["admin"]}" "Page $i" > /dev/null 2>&1
    done

    make_request "GET" "/api/v1/projects?page=1&pageSize=2" "${TOKENS["admin"]}" "" 200 "page=1&pageSize=2"
    make_request "GET" "/api/v1/projects?page=2&pageSize=2" "${TOKENS["admin"]}" "" 200 "page=2&pageSize=2"
    make_request "GET" "/api/v1/projects?page=999&pageSize=10" "${TOKENS["admin"]}" "" 200 "page за пределами"
    make_request "GET" "/api/v1/projects?parentId=0" "${TOKENS["admin"]}" "" 200 "parentId=0 (корневые)"
}

test_search() {
    print_test_header "Тест 8: Поиск"

    create_project "${TOKENS["admin"]}" "Alpha Project" > /dev/null 2>&1
    create_project "${TOKENS["admin"]}" "Beta Project" > /dev/null 2>&1
    create_project "${TOKENS["admin"]}" "Gamma Version" > /dev/null 2>&1

    make_request "GET" "/api/v1/projects?searchCaption=Project" "${TOKENS["admin"]}" "" 200 "Поиск 'Project'"
    make_request "GET" "/api/v1/projects?searchCaption=Alpha" "${TOKENS["admin"]}" "" 200 "Поиск 'Alpha'"
    make_request "GET" "/api/v1/projects?searchCaption=NonExistent" "${TOKENS["admin"]}" "" 200 "Поиск несуществующего"
}

cleanup() {
    print_test_header "Очистка"
    if [ -n "$ADMIN_PROJECT_ID" ] && [ "$ADMIN_PROJECT_ID" != "null" ] && [ "$ADMIN_PROJECT_ID" != "" ]; then
        curl -s -X DELETE "${API_URL}/api/v1/projects/${ADMIN_PROJECT_ID}" \
            -H "Authorization: Bearer ${TOKENS["admin"]}" > /dev/null 2>&1
        print_success "Тестовый проект удален"
    fi
    print_success "Очистка завершена"
}

# =============================================================================
# Запуск
# =============================================================================

main() {
    echo ""
    echo "═══════════════════════════════════════════════════════════════════════"
    echo "              ТЕСТИРОВАНИЕ API ПРОЕКТОВ"
    echo "═══════════════════════════════════════════════════════════════════════"
    echo "API URL: ${API_URL}"
    echo ""

    if ! curl -s "${API_URL}/health" > /dev/null 2>&1; then
        print_error "Сервер недоступен"
        exit 1
    fi

    setup || exit 1

    test_unauthorized
    test_admin_access
    test_manager_permissions
    test_developer_permissions
    test_viewer_permissions
    test_invalid_token
    test_pagination
    test_search

    cleanup

    echo ""
    echo "═══════════════════════════════════════════════════════════════════════"
    echo "                          РЕЗУЛЬТАТЫ"
    echo "═══════════════════════════════════════════════════════════════════════"
    echo -e "${GREEN}Пройдено: ${TESTS_PASSED}${NC}"
    echo -e "${RED}Провалено: ${TESTS_FAILED}${NC}"
    echo ""

    if [ $TESTS_FAILED -eq 0 ]; then
        echo -e "${GREEN} ВСЕ ТЕСТЫ ПРОЙДЕНЫ!${NC}"
        exit 0
    else
        echo -e "${RED} ЕСТЬ ПРОВАЛЕННЫЕ ТЕСТЫ!${NC}"
        exit 1
    fi
}

main "$@"
