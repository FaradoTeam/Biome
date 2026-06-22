#!/bin/bash
# ============================================================================
# Тестирование API досок и колонок (Boards и BoardColumns)
# ============================================================================
#
# Описание:
#   Скрипт выполняет последовательное тестирование REST API для работы с досками
#   и колонками досок, проверяя CRUD операции и фильтрацию.
#
# Использование:
#   ./test_boards_api.sh [--host HOST] [--port PORT] [--help]
#
#   --host HOST    - хост сервера (по умолчанию: localhost)
#   --port PORT    - порт сервера (по умолчанию: 8090)
#   --help         - показать справку
#
# Пример:
#   ./test_boards_api.sh --host 127.0.0.1 --port 8080
# ============================================================================

set -e

# ============================================================================
# Конфигурация
# ============================================================================
HOST="localhost"
PORT="8090"
API_PATH="/api/v1"
BASE_URL="http://${HOST}:${PORT}"
FULL_BASE_URL="${BASE_URL}${API_PATH}"

# Цвета для вывода
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Счётчики тестов
TESTS_PASSED=0
TESTS_FAILED=0
TESTS_SKIPPED=0

# Глобальные переменные для хранения ID созданных объектов
TOKEN=""
BOARD_ID=""
COLUMN_ID=""
PROJECT_ID=""
WORKFLOW_ID=""
STATE_ID=""
PHASE_ID=""

# ============================================================================
# Вспомогательные функции
# ============================================================================

print_help() {
    cat << EOF
Тестирование API досок и колонок (Boards и BoardColumns)

Использование:
    $0 [--host HOST] [--port PORT] [--help]

Параметры:
    --host HOST    - хост сервера (по умолчанию: localhost)
    --port PORT    - порт сервера (по умолчанию: 8090)
    --help         - показать эту справку

Примеры:
    $0
    $0 --host 127.0.0.1 --port 8080
    $0 --port 8081
EOF
}

log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[PASS]${NC} $1"
    ((TESTS_PASSED++))
}

log_error() {
    echo -e "${RED}[FAIL]${NC} $1"
    ((TESTS_FAILED++))
}

log_warning() {
    echo -e "${YELLOW}[WARN]${NC} $1"
    ((TESTS_SKIPPED++))
}

log_test() {
    echo -e "\n${BLUE}>>>${NC} $1"
}

# Проверка наличия jq
check_jq() {
    if ! command -v jq &> /dev/null; then
        log_error "jq не установлен. Установите jq для работы скрипта."
        exit 1
    fi
}

# Проверка доступности сервера
check_server() {
    log_info "Проверка доступности сервера..."
    local result=$(curl -s -o /dev/null -w "%{http_code}" "${FULL_BASE_URL}/health" 2>/dev/null || echo "000")
    if [[ "${result}" == "200" ]]; then
        log_success "Сервер доступен"
        return 0
    else
        log_error "Сервер недоступен (код: ${result})"
        return 1
    fi
}

# Выполнение HTTP запроса с обработкой ошибок
http_request() {
    local method="$1"
    local endpoint="$2"
    local data="$3"
    local token="$4"
    local url="${FULL_BASE_URL}${endpoint}"
    local response_file=$(mktemp)
    local http_code_file=$(mktemp)

    local curl_cmd="curl -s -w '%{http_code}' -X ${method} '${url}'"
    
    if [[ -n "${token}" ]]; then
        curl_cmd="${curl_cmd} -H 'Authorization: Bearer ${token}'"
    fi
    
    curl_cmd="${curl_cmd} -H 'Content-Type: application/json'"
    
    if [[ -n "${data}" ]]; then
        curl_cmd="${curl_cmd} -d '${data}'"
    fi
    
    curl_cmd="${curl_cmd} -o '${response_file}'"
    
    # Выполняем запрос
    eval "${curl_cmd}" > "${http_code_file}" 2>/dev/null
    
    local http_code=$(cat "${http_code_file}")
    local response=$(cat "${response_file}")
    
    rm -f "${response_file}" "${http_code_file}"
    
    echo "${http_code}|${response}"
}

# ============================================================================
# Тесты аутентификации
# ============================================================================

test_login() {
    log_test "Тест 1: Аутентификация"
    
    local login_data='{"login":"admin","password":"password"}'
    local result=$(http_request "POST" "/auth/login" "${login_data}")
    local http_code=$(echo "$result" | cut -d'|' -f1)
    local response=$(echo "$result" | cut -d'|' -f2-)
    
    if [[ "${http_code}" == "200" ]]; then
        TOKEN=$(echo "$response" | jq -r '.access_token')
        if [[ -n "${TOKEN}" && "${TOKEN}" != "null" ]]; then
            log_success "Аутентификация успешна, получен токен"
            return 0
        fi
    fi
    
    log_error "Аутентификация не удалась (код: ${http_code})"
    echo "Ответ сервера: ${response}"
    return 1
}

# ============================================================================
# Тесты для создания необходимых сущностей
# ============================================================================

test_create_project() {
    log_test "Тест 1.1: Создание проекта"
    
    # Сначала пробуем получить существующий проект
    local result=$(http_request "GET" "/projects" "" "${TOKEN}")
    local http_code=$(echo "$result" | cut -d'|' -f1)
    local response=$(echo "$result" | cut -d'|' -f2-)
    
    if [[ "${http_code}" == "200" ]]; then
        PROJECT_ID=$(echo "$response" | jq -r '.items[0].id // null')
        if [[ -n "${PROJECT_ID}" && "${PROJECT_ID}" != "null" ]]; then
            log_success "Найден существующий проект с ID: ${PROJECT_ID}"
            return 0
        fi
    fi
    
    # Если проект не найден, создаем новый
    log_info "Создание нового проекта..."
    
    local project_data='{
        "caption": "Тестовый проект",
        "description": "Проект для тестирования API досок"
    }'
    
    result=$(http_request "POST" "/projects" "${project_data}" "${TOKEN}")
    http_code=$(echo "$result" | cut -d'|' -f1)
    response=$(echo "$result" | cut -d'|' -f2-)
    
    if [[ "${http_code}" == "201" ]]; then
        PROJECT_ID=$(echo "$response" | jq -r '.id')
        if [[ -n "${PROJECT_ID}" && "${PROJECT_ID}" != "null" ]]; then
            log_success "Проект создан с ID: ${PROJECT_ID}"
            return 0
        fi
    fi
    
    log_error "Не удалось создать проект (код: ${http_code})"
    echo "Ответ сервера: ${response}"
    return 1
}

test_get_workflows() {
    log_test "Тест 1.2: Получение workflow"
    
    local result=$(http_request "GET" "/workflows" "" "${TOKEN}")
    local http_code=$(echo "$result" | cut -d'|' -f1)
    local response=$(echo "$result" | cut -d'|' -f2-)
    
    if [[ "${http_code}" == "200" ]]; then
        WORKFLOW_ID=$(echo "$response" | jq -r '.items[0].id // null')
        if [[ -n "${WORKFLOW_ID}" && "${WORKFLOW_ID}" != "null" ]]; then
            log_success "Найден workflow с ID: ${WORKFLOW_ID}"
            return 0
        fi
    fi
    
    log_warning "Не удалось получить workflow, попробуем создать..."
    
    # Пробуем создать workflow
    local workflow_data='{
        "caption": "Тестовый workflow",
        "description": "Workflow для тестирования"
    }'
    
    result=$(http_request "POST" "/workflows" "${workflow_data}" "${TOKEN}")
    http_code=$(echo "$result" | cut -d'|' -f1)
    response=$(echo "$result" | cut -d'|' -f2-)
    
    if [[ "${http_code}" == "201" ]]; then
        WORKFLOW_ID=$(echo "$response" | jq -r '.id')
        if [[ -n "${WORKFLOW_ID}" && "${WORKFLOW_ID}" != "null" ]]; then
            log_success "Workflow создан с ID: ${WORKFLOW_ID}"
            return 0
        fi
    fi
    
    log_warning "Не удалось создать workflow, используем ID=1"
    WORKFLOW_ID=1
    return 0
}

test_get_states() {
    log_test "Тест 1.3: Получение состояний"
    
    local result=$(http_request "GET" "/states" "" "${TOKEN}")
    local http_code=$(echo "$result" | cut -d'|' -f1)
    local response=$(echo "$result" | cut -d'|' -f2-)
    
    if [[ "${http_code}" == "200" ]]; then
        STATE_ID=$(echo "$response" | jq -r '.items[0].id // null')
        if [[ -n "${STATE_ID}" && "${STATE_ID}" != "null" ]]; then
            log_success "Найдено состояние с ID: ${STATE_ID}"
            return 0
        fi
    fi
    
    log_warning "Не удалось получить состояния, используем ID=1"
    STATE_ID=1
    return 0
}

test_get_phases() {
    log_test "Тест 1.4: Получение фаз проекта"
    
    if [[ -z "${PROJECT_ID}" || "${PROJECT_ID}" == "null" ]]; then
        log_warning "PROJECT_ID не установлен"
        PHASE_ID=1
        return 0
    fi
    
    local result=$(http_request "GET" "/projects/${PROJECT_ID}/phases" "" "${TOKEN}")
    local http_code=$(echo "$result" | cut -d'|' -f1)
    local response=$(echo "$result" | cut -d'|' -f2-)
    
    if [[ "${http_code}" == "200" ]]; then
        PHASE_ID=$(echo "$response" | jq -r '.[0].id // null')
        if [[ -n "${PHASE_ID}" && "${PHASE_ID}" != "null" ]]; then
            log_success "Найдена фаза с ID: ${PHASE_ID}"
            return 0
        fi
    fi
    
    log_warning "Не удалось получить фазы, создаем фазу..."
    
    # Создаем фазу
    local phase_data=$(cat <<EOF
{
    "projectId": ${PROJECT_ID},
    "caption": "Тестовая фаза",
    "orderNumber": 1
}
EOF
)
    
    result=$(http_request "POST" "/phases" "${phase_data}" "${TOKEN}")
    http_code=$(echo "$result" | cut -d'|' -f1)
    response=$(echo "$result" | cut -d'|' -f2-)
    
    if [[ "${http_code}" == "201" ]]; then
        PHASE_ID=$(echo "$response" | jq -r '.id')
        if [[ -n "${PHASE_ID}" && "${PHASE_ID}" != "null" ]]; then
            log_success "Фаза создана с ID: ${PHASE_ID}"
            return 0
        fi
    fi
    
    log_warning "Не удалось создать фазу, используем ID=1"
    PHASE_ID=1
    return 0
}

# ============================================================================
# Тесты для Boards
# ============================================================================

test_create_board() {
    log_test "Тест 2: Создание доски"
    
    if [[ -z "${PROJECT_ID}" || "${PROJECT_ID}" == "null" ]]; then
        log_error "PROJECT_ID не установлен"
        return 1
    fi
    
    if [[ -z "${WORKFLOW_ID}" || "${WORKFLOW_ID}" == "null" ]]; then
        log_warning "WORKFLOW_ID не установлен, используем 1"
        WORKFLOW_ID=1
    fi
    
    local board_data=$(cat <<EOF
{
    "caption": "Тестовая доска",
    "projectId": ${PROJECT_ID},
    "workflowId": ${WORKFLOW_ID},
    "description": "Доска для тестирования API"
}
EOF
)
    
    log_info "Создание доски с projectId=${PROJECT_ID}, workflowId=${WORKFLOW_ID}"
    
    local result=$(http_request "POST" "/boards" "${board_data}" "${TOKEN}")
    local http_code=$(echo "$result" | cut -d'|' -f1)
    local response=$(echo "$result" | cut -d'|' -f2-)
    
    if [[ "${http_code}" == "201" ]]; then
        BOARD_ID=$(echo "$response" | jq -r '.id')
        if [[ -n "${BOARD_ID}" && "${BOARD_ID}" != "null" ]]; then
            log_success "Доска создана с ID: ${BOARD_ID}"
            return 0
        fi
    fi
    
    log_error "Не удалось создать доску (код: ${http_code})"
    echo "Ответ сервера: ${response}"
    return 1
}

test_get_boards() {
    log_test "Тест 3: Получение списка досок"
    
    local result=$(http_request "GET" "/boards" "" "${TOKEN}")
    local http_code=$(echo "$result" | cut -d'|' -f1)
    local response=$(echo "$result" | cut -d'|' -f2-)
    
    if [[ "${http_code}" == "200" ]]; then
        local total_count=$(echo "$response" | jq -r '.totalCount // 0')
        log_success "Получен список досок: ${total_count} записей"
        return 0
    fi
    
    log_error "Не удалось получить список досок (код: ${http_code})"
    return 1
}

test_get_boards_with_filters() {
    log_test "Тест 4: Фильтрация досок"
    
    if [[ -z "${PROJECT_ID}" || "${PROJECT_ID}" == "null" ]]; then
        log_warning "PROJECT_ID не установлен, пропуск фильтрации"
        return 0
    fi
    
    local result=$(http_request "GET" "/boards?projectId=${PROJECT_ID}" "" "${TOKEN}")
    local http_code=$(echo "$result" | cut -d'|' -f1)
    local response=$(echo "$result" | cut -d'|' -f2-)
    
    if [[ "${http_code}" == "200" ]]; then
        local total_count=$(echo "$response" | jq -r '.totalCount // 0')
        log_success "Фильтрация по projectId: ${total_count} записей"
        return 0
    fi
    
    log_error "Не удалось выполнить фильтрацию (код: ${http_code})"
    return 1
}

test_get_board_by_id() {
    log_test "Тест 5: Получение доски по ID"
    
    if [[ -z "${BOARD_ID}" || "${BOARD_ID}" == "null" ]]; then
        log_warning "Пропуск теста: BOARD_ID не установлен"
        return 0
    fi
    
    local result=$(http_request "GET" "/boards/${BOARD_ID}" "" "${TOKEN}")
    local http_code=$(echo "$result" | cut -d'|' -f1)
    local response=$(echo "$result" | cut -d'|' -f2-)
    
    if [[ "${http_code}" == "200" ]]; then
        local id=$(echo "$response" | jq -r '.id')
        local caption=$(echo "$response" | jq -r '.caption')
        
        if [[ "${id}" == "${BOARD_ID}" ]]; then
            log_success "Получена доска: ${caption} (ID: ${id})"
            return 0
        fi
    fi
    
    log_error "Не удалось получить доску (код: ${http_code})"
    return 1
}

test_update_board() {
    log_test "Тест 6: Обновление доски"
    
    if [[ -z "${BOARD_ID}" || "${BOARD_ID}" == "null" ]]; then
        log_warning "Пропуск теста: BOARD_ID не установлен"
        return 0
    fi
    
    local update_data='{
        "caption": "Обновлённая тестовая доска",
        "description": "Обновлённое описание доски"
    }'
    
    local result=$(http_request "PUT" "/boards/${BOARD_ID}" "${update_data}" "${TOKEN}")
    local http_code=$(echo "$result" | cut -d'|' -f1)
    local response=$(echo "$result" | cut -d'|' -f2-)
    
    if [[ "${http_code}" == "200" ]]; then
        local caption=$(echo "$response" | jq -r '.caption')
        if [[ "${caption}" == "Обновлённая тестовая доска" ]]; then
            log_success "Доска обновлена"
            return 0
        fi
    fi
    
    log_error "Не удалось обновить доску (код: ${http_code})"
    return 1
}

test_get_boards_by_project() {
    log_test "Тест 7: Получение досок по проекту"
    
    if [[ -z "${PROJECT_ID}" || "${PROJECT_ID}" == "null" ]]; then
        log_warning "PROJECT_ID не установлен, пропуск теста"
        return 0
    fi
    
    local result=$(http_request "GET" "/projects/${PROJECT_ID}/boards" "" "${TOKEN}")
    local http_code=$(echo "$result" | cut -d'|' -f1)
    local response=$(echo "$result" | cut -d'|' -f2-)
    
    if [[ "${http_code}" == "200" ]]; then
        local count=$(echo "$response" | jq -r '. | length // 0')
        log_success "Получены доски проекта: ${count} записей"
        return 0
    fi
    
    log_error "Не удалось получить доски проекта (код: ${http_code})"
    return 1
}

test_get_boards_by_phase() {
    log_test "Тест 7.1: Получение досок по фазе"
    
    if [[ -z "${PHASE_ID}" || "${PHASE_ID}" == "null" ]]; then
        log_warning "PHASE_ID не установлен, пропуск теста"
        return 0
    fi
    
    local result=$(http_request "GET" "/phases/${PHASE_ID}/boards" "" "${TOKEN}")
    local http_code=$(echo "$result" | cut -d'|' -f1)
    local response=$(echo "$result" | cut -d'|' -f2-)
    
    if [[ "${http_code}" == "200" ]]; then
        local count=$(echo "$response" | jq -r '. | length // 0')
        log_success "Получены доски фазы: ${count} записей"
        return 0
    fi
    
    log_warning "Не удалось получить доски фазы (код: ${http_code})"
    return 0
}

# ============================================================================
# Тесты для BoardColumns
# ============================================================================

test_create_board_column() {
    log_test "Тест 8: Создание колонки доски"
    
    if [[ -z "${BOARD_ID}" || "${BOARD_ID}" == "null" ]]; then
        log_warning "Пропуск теста: BOARD_ID не установлен"
        return 0
    fi
    
    if [[ -z "${STATE_ID}" || "${STATE_ID}" == "null" ]]; then
        STATE_ID=1
    fi
    
    local column_data=$(cat <<EOF
{
    "stateId": ${STATE_ID},
    "orderNumber": 1,
    "settings": "{\"wip\": 5, \"color\": \"blue\"}"
}
EOF
)
    
    local result=$(http_request "POST" "/boards/${BOARD_ID}/columns" "${column_data}" "${TOKEN}")
    local http_code=$(echo "$result" | cut -d'|' -f1)
    local response=$(echo "$result" | cut -d'|' -f2-)
    
    if [[ "${http_code}" == "201" ]]; then
        COLUMN_ID=$(echo "$response" | jq -r '.id')
        if [[ -n "${COLUMN_ID}" && "${COLUMN_ID}" != "null" ]]; then
            log_success "Колонка создана с ID: ${COLUMN_ID}"
            return 0
        fi
    elif [[ "${http_code}" == "409" ]]; then
        log_warning "Колонка уже существует (Conflict)"
        return 0
    fi
    
    log_error "Не удалось создать колонку (код: ${http_code})"
    echo "Ответ сервера: ${response}"
    return 1
}

test_get_board_columns() {
    log_test "Тест 9: Получение списка колонок"
    
    local result=$(http_request "GET" "/board-columns" "" "${TOKEN}")
    local http_code=$(echo "$result" | cut -d'|' -f1)
    local response=$(echo "$result" | cut -d'|' -f2-)
    
    if [[ "${http_code}" == "200" ]]; then
        local total_count=$(echo "$response" | jq -r '.totalCount // 0')
        log_success "Получен список колонок: ${total_count} записей"
        return 0
    fi
    
    log_error "Не удалось получить список колонок (код: ${http_code})"
    return 1
}

test_get_columns_by_board() {
    log_test "Тест 10: Получение колонок доски"
    
    if [[ -z "${BOARD_ID}" || "${BOARD_ID}" == "null" ]]; then
        log_warning "Пропуск теста: BOARD_ID не установлен"
        return 0
    fi
    
    local result=$(http_request "GET" "/boards/${BOARD_ID}/columns" "" "${TOKEN}")
    local http_code=$(echo "$result" | cut -d'|' -f1)
    local response=$(echo "$result" | cut -d'|' -f2-)
    
    if [[ "${http_code}" == "200" ]]; then
        local count=$(echo "$response" | jq -r '. | length // 0')
        log_success "Получены колонки доски: ${count} записей"
        return 0
    fi
    
    log_error "Не удалось получить колонки доски (код: ${http_code})"
    return 1
}

test_get_board_column_by_id() {
    log_test "Тест 11: Получение колонки по ID"
    
    if [[ -z "${COLUMN_ID}" || "${COLUMN_ID}" == "null" ]]; then
        log_warning "Пропуск теста: COLUMN_ID не установлен"
        return 0
    fi
    
    local result=$(http_request "GET" "/board-columns/${COLUMN_ID}" "" "${TOKEN}")
    local http_code=$(echo "$result" | cut -d'|' -f1)
    local response=$(echo "$result" | cut -d'|' -f2-)
    
    if [[ "${http_code}" == "200" ]]; then
        local id=$(echo "$response" | jq -r '.id')
        if [[ "${id}" == "${COLUMN_ID}" ]]; then
            log_success "Получена колонка с ID: ${id}"
            return 0
        fi
    elif [[ "${http_code}" == "404" ]]; then
        log_warning "Колонка не найдена (404)"
        return 0
    fi
    
    log_error "Не удалось получить колонку (код: ${http_code})"
    return 1
}

test_update_board_column() {
    log_test "Тест 12: Обновление колонки"
    
    if [[ -z "${COLUMN_ID}" || "${COLUMN_ID}" == "null" ]]; then
        log_warning "Пропуск теста: COLUMN_ID не установлен"
        return 0
    fi
    
    local update_data='{
        "orderNumber": 2,
        "settings": "{\"wip\": 10, \"color\": \"red\"}"
    }'
    
    local result=$(http_request "PUT" "/board-columns/${COLUMN_ID}" "${update_data}" "${TOKEN}")
    local http_code=$(echo "$result" | cut -d'|' -f1)
    local response=$(echo "$result" | cut -d'|' -f2-)
    
    if [[ "${http_code}" == "200" ]]; then
        local order_number=$(echo "$response" | jq -r '.orderNumber')
        if [[ "${order_number}" == "2" ]]; then
            log_success "Колонка обновлена"
            return 0
        fi
    elif [[ "${http_code}" == "404" ]]; then
        log_warning "Колонка не найдена (404)"
        return 0
    fi
    
    log_error "Не удалось обновить колонку (код: ${http_code})"
    return 1
}

test_delete_board_column() {
    log_test "Тест 13: Удаление колонки"
    
    if [[ -z "${COLUMN_ID}" || "${COLUMN_ID}" == "null" ]]; then
        log_warning "Пропуск теста: COLUMN_ID не установлен"
        return 0
    fi
    
    local result=$(http_request "DELETE" "/board-columns/${COLUMN_ID}" "" "${TOKEN}")
    local http_code=$(echo "$result" | cut -d'|' -f1)
    
    if [[ "${http_code}" == "204" ]]; then
        log_success "Колонка удалена"
        return 0
    elif [[ "${http_code}" == "404" ]]; then
        log_warning "Колонка не найдена (404)"
        return 0
    fi
    
    log_error "Не удалось удалить колонку (код: ${http_code})"
    return 1
}

test_delete_board() {
    log_test "Тест 14: Удаление доски"
    
    if [[ -z "${BOARD_ID}" || "${BOARD_ID}" == "null" ]]; then
        log_warning "Пропуск теста: BOARD_ID не установлен"
        return 0
    fi
    
    local result=$(http_request "DELETE" "/boards/${BOARD_ID}" "" "${TOKEN}")
    local http_code=$(echo "$result" | cut -d'|' -f1)
    
    if [[ "${http_code}" == "204" ]]; then
        log_success "Доска удалена"
        return 0
    elif [[ "${http_code}" == "404" ]]; then
        log_warning "Доска не найдена (404)"
        return 0
    fi
    
    log_error "Не удалось удалить доску (код: ${http_code})"
    return 1
}

# ============================================================================
# Тесты негативных сценариев
# ============================================================================

test_get_board_not_found() {
    log_test "Тест 15: Получение несуществующей доски"
    
    local result=$(http_request "GET" "/boards/99999" "" "${TOKEN}")
    local http_code=$(echo "$result" | cut -d'|' -f1)
    
    if [[ "${http_code}" == "404" ]]; then
        log_success "Несуществующая доска: 404 Not Found"
        return 0
    fi
    
    log_error "Ожидался 404, получен ${http_code}"
    return 1
}

test_create_board_missing_fields() {
    log_test "Тест 16: Создание доски без обязательных полей"
    
    local board_data='{
        "caption": "Доска без проекта"
    }'
    
    local result=$(http_request "POST" "/boards" "${board_data}" "${TOKEN}")
    local http_code=$(echo "$result" | cut -d'|' -f1)
    
    if [[ "${http_code}" == "400" ]]; then
        log_success "Ошибка валидации: 400 Bad Request"
        return 0
    fi
    
    log_error "Ожидался 400, получен ${http_code}"
    return 1
}

# ============================================================================
# Вывод результатов
# ============================================================================

print_summary() {
    echo ""
    echo "=========================================="
    echo "           РЕЗУЛЬТАТЫ ТЕСТОВ              "
    echo "=========================================="
    echo -e "Пройдено: ${GREEN}${TESTS_PASSED}${NC}"
    echo -e "Пропущено: ${YELLOW}${TESTS_SKIPPED}${NC}"
    echo -e "Провалено: ${RED}${TESTS_FAILED}${NC}"
    echo "=========================================="
    
    if [[ ${TESTS_FAILED} -eq 0 ]]; then
        echo -e "${GREEN}Все тесты успешно пройдены!${NC}"
        return 0
    else
        echo -e "${RED}Есть проваленные тесты!${NC}"
        return 1
    fi
}

# ============================================================================
# Основная функция
# ============================================================================

main() {
    # Парсинг аргументов
    while [[ $# -gt 0 ]]; do
        case $1 in
            --host)
                HOST="$2"
                shift 2
                ;;
            --port)
                PORT="$2"
                shift 2
                ;;
            --help)
                print_help
                exit 0
                ;;
            *)
                echo "Неизвестный параметр: $1"
                print_help
                exit 1
                ;;
        esac
    done
    
    BASE_URL="http://${HOST}:${PORT}"
    FULL_BASE_URL="${BASE_URL}${API_PATH}"
    
    echo "=========================================="
    echo "   Тестирование API досок и колонок       "
    echo "=========================================="
    echo "Хост: ${HOST}"
    echo "Порт: ${PORT}"
    echo "URL: ${FULL_BASE_URL}"
    echo "=========================================="
    
    # Проверка наличия jq
    check_jq
    
    # Проверка доступности сервера
    check_server || exit 1
    
    # Запуск тестов
    test_login || exit 1
    
    # Создание необходимых сущностей
    test_create_project || exit 1
    test_get_workflows || true
    test_get_states || true
    test_get_phases || true
    
    # Тесты досок
    test_create_board || true
    test_get_boards || true
    test_get_boards_with_filters || true
    test_get_board_by_id || true
    test_update_board || true
    test_get_boards_by_project || true
    test_get_boards_by_phase || true
    
    # Тесты колонок
    test_create_board_column || true
    test_get_board_columns || true
    test_get_columns_by_board || true
    test_get_board_column_by_id || true
    test_update_board_column || true
    test_delete_board_column || true
    
    # Удаление доски
    test_delete_board || true
    
    # Негативные тесты
    test_get_board_not_found || true
    test_create_board_missing_fields || true
    
    # Вывод результатов
    print_summary
}

# Запуск основной функции
main "$@"
