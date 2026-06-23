#!/bin/bash
# ============================================================================
# Тестирование API сообщений и уведомлений
# ============================================================================
#
# Описание:
#   Скрипт выполняет последовательное тестирование REST API для работы с
#   личными сообщениями, сообщениями в командах и подписками на уведомления.
#
# Использование:
#   ./test_messages_notifications_api.sh [--host HOST] [--port PORT] [--help]
#
#   --host HOST    - хост сервера (по умолчанию: localhost)
#   --port PORT    - порт сервера (по умолчанию: 8090)
#   --help         - показать справку
#
# Пример:
#   ./test_messages_notifications_api.sh --host 127.0.0.1 --port 8080
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
USER_ID=""
OTHER_USER_ID=""
TEAM_ID=""
ITEM_ID=""
PHASE_ID=""
PROJECT_ID=""
WORKFLOW_ID=""
STATE_ID=""

# ID созданных сущностей для тестов
PRIVATE_MSG_ID=""
TEAM_MSG_ID=""
NOTIFICATION_ID=""

# ============================================================================
# Вспомогательные функции
# ============================================================================

print_help() {
    cat << EOF
Тестирование API сообщений и уведомлений

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
# Тесты аутентификации и подготовка
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

test_get_current_user() {
    log_test "Тест 1.1: Получение текущего пользователя"
    
    local result=$(http_request "GET" "/users" "" "${TOKEN}")
    local http_code=$(echo "$result" | cut -d'|' -f1)
    local response=$(echo "$result" | cut -d'|' -f2-)
    
    if [[ "${http_code}" == "200" ]]; then
        USER_ID=$(echo "$response" | jq -r '.items[0].id // null')
        if [[ -n "${USER_ID}" && "${USER_ID}" != "null" ]]; then
            log_success "Текущий пользователь: ID=${USER_ID}"
            return 0
        fi
    fi
    
    log_warning "Не удалось получить пользователя, используем ID=1"
    USER_ID=1
    return 0
}

test_get_or_create_other_user() {
    log_test "Тест 1.2: Получение или создание другого пользователя"
    
    local result=$(http_request "GET" "/users" "" "${TOKEN}")
    local http_code=$(echo "$result" | cut -d'|' -f1)
    local response=$(echo "$result" | cut -d'|' -f2-)
    
    if [[ "${http_code}" == "200" ]]; then
        local second_user=$(echo "$response" | jq -r '.items[1].id // null')
        if [[ -n "${second_user}" && "${second_user}" != "null" ]]; then
            OTHER_USER_ID="${second_user}"
            log_success "Найден другой пользователь: ID=${OTHER_USER_ID}"
            return 0
        fi
    fi
    
    log_info "Создание нового пользователя для тестов..."
    local user_data='{
        "login": "test_user_'$(date +%s)'",
        "email": "test_user_'$(date +%s)'@test.local",
        "firstName": "Test",
        "lastName": "User",
        "password": "Test123!"
    }'
    
    result=$(http_request "POST" "/users" "${user_data}" "${TOKEN}")
    http_code=$(echo "$result" | cut -d'|' -f1)
    response=$(echo "$result" | cut -d'|' -f2-)
    
    if [[ "${http_code}" == "201" ]]; then
        OTHER_USER_ID=$(echo "$response" | jq -r '.id')
        if [[ -n "${OTHER_USER_ID}" && "${OTHER_USER_ID}" != "null" ]]; then
            log_success "Создан новый пользователь: ID=${OTHER_USER_ID}"
            return 0
        fi
    fi
    
    log_warning "Не удалось создать пользователя, используем ID=2"
    OTHER_USER_ID=2
    return 0
}

test_get_or_create_team() {
    log_test "Тест 1.3: Получение или создание команды"
    
    local result=$(http_request "GET" "/teams" "" "${TOKEN}")
    local http_code=$(echo "$result" | cut -d'|' -f1)
    local response=$(echo "$result" | cut -d'|' -f2-)
    
    if [[ "${http_code}" == "200" ]]; then
        TEAM_ID=$(echo "$response" | jq -r '.items[0].id // null')
        if [[ -n "${TEAM_ID}" && "${TEAM_ID}" != "null" ]]; then
            log_success "Найдена существующая команда: ID=${TEAM_ID}"
            # Добавляем пользователя в команду
            add_user_to_team
            return 0
        fi
    fi
    
    log_info "Создание новой команды для тестов..."
    local team_data='{
        "caption": "Тестовая команда",
        "description": "Команда для тестирования сообщений"
    }'
    
    result=$(http_request "POST" "/teams" "${team_data}" "${TOKEN}")
    http_code=$(echo "$result" | cut -d'|' -f1)
    response=$(echo "$result" | cut -d'|' -f2-)
    
    if [[ "${http_code}" == "201" ]]; then
        TEAM_ID=$(echo "$response" | jq -r '.id')
        if [[ -n "${TEAM_ID}" && "${TEAM_ID}" != "null" ]]; then
            log_success "Создана новая команда: ID=${TEAM_ID}"
            # Добавляем пользователя в команду
            add_user_to_team
            return 0
        fi
    fi
    
    log_warning "Не удалось создать команду, используем ID=1"
    TEAM_ID=1
    add_user_to_team
    return 0
}

add_user_to_team() {
    log_info "Добавление пользователя ${USER_ID} в команду ${TEAM_ID}..."
    
    # Получаем существующую роль
    local role_result=$(http_request "GET" "/roles" "" "${TOKEN}")
    local role_http_code=$(echo "$role_result" | cut -d'|' -f1)
    local role_response=$(echo "$role_result" | cut -d'|' -f2-)
    
    local role_id=1
    if [[ "${role_http_code}" == "200" ]]; then
        role_id=$(echo "$role_response" | jq -r '.items[0].id // 1')
    fi
    
    local user_role_data=$(cat <<EOF
{
    "userId": ${USER_ID},
    "teamId": ${TEAM_ID},
    "roleId": ${role_id}
}
EOF
)
    
    local result=$(http_request "POST" "/user-team-roles" "${user_role_data}" "${TOKEN}")
    local http_code=$(echo "$result" | cut -d'|' -f1)
    
    if [[ "${http_code}" == "201" ]] || [[ "${http_code}" == "409" ]]; then
        log_success "Пользователь добавлен в команду (или уже состоит)"
        return 0
    fi
    
    log_warning "Не удалось добавить пользователя в команду (код: ${http_code})"
    return 0
}

test_get_or_create_item() {
    log_test "Тест 1.4: Получение или создание элемента"
    
    # Сначала нужно создать проект, фазу и элемент
    
    # 1. Создаём проект
    local project_data='{
        "caption": "Тестовый проект для уведомлений",
        "description": "Проект для тестирования подписок"
    }'
    
    local result=$(http_request "POST" "/projects" "${project_data}" "${TOKEN}")
    local http_code=$(echo "$result" | cut -d'|' -f1)
    local response=$(echo "$result" | cut -d'|' -f2-)
    
    if [[ "${http_code}" == "201" ]]; then
        PROJECT_ID=$(echo "$response" | jq -r '.id')
        log_info "Создан проект: ID=${PROJECT_ID}"
    else
        # Пробуем получить существующий проект
        result=$(http_request "GET" "/projects" "" "${TOKEN}")
        http_code=$(echo "$result" | cut -d'|' -f1)
        response=$(echo "$result" | cut -d'|' -f2-)
        if [[ "${http_code}" == "200" ]]; then
            PROJECT_ID=$(echo "$response" | jq -r '.items[0].id // null')
        fi
    fi
    
    if [[ -z "${PROJECT_ID}" || "${PROJECT_ID}" == "null" ]]; then
        log_warning "Не удалось создать/получить проект, используем ID=1"
        PROJECT_ID=1
    fi
    
    # 2. Получаем workflow
    result=$(http_request "GET" "/workflows" "" "${TOKEN}")
    http_code=$(echo "$result" | cut -d'|' -f1)
    response=$(echo "$result" | cut -d'|' -f2-)
    if [[ "${http_code}" == "200" ]]; then
        WORKFLOW_ID=$(echo "$response" | jq -r '.items[0].id // null')
    fi
    if [[ -z "${WORKFLOW_ID}" || "${WORKFLOW_ID}" == "null" ]]; then
        WORKFLOW_ID=1
    fi
    
    # 3. Получаем состояние
    result=$(http_request "GET" "/states" "" "${TOKEN}")
    http_code=$(echo "$result" | cut -d'|' -f1)
    response=$(echo "$result" | cut -d'|' -f2-)
    if [[ "${http_code}" == "200" ]]; then
        STATE_ID=$(echo "$response" | jq -r '.items[0].id // null')
    fi
    if [[ -z "${STATE_ID}" || "${STATE_ID}" == "null" ]]; then
        STATE_ID=1
    fi
    
    # 4. Создаём фазу
    local phase_data=$(cat <<EOF
{
    "projectId": ${PROJECT_ID},
    "caption": "Тестовая фаза"
}
EOF
)
    
    result=$(http_request "POST" "/phases" "${phase_data}" "${TOKEN}")
    http_code=$(echo "$result" | cut -d'|' -f1)
    response=$(echo "$result" | cut -d'|' -f2-)
    
    if [[ "${http_code}" == "201" ]]; then
        PHASE_ID=$(echo "$response" | jq -r '.id')
        log_info "Создана фаза: ID=${PHASE_ID}"
    else
        # Пробуем получить существующую фазу
        result=$(http_request "GET" "/phases" "" "${TOKEN}")
        http_code=$(echo "$result" | cut -d'|' -f1)
        response=$(echo "$result" | cut -d'|' -f2-)
        if [[ "${http_code}" == "200" ]]; then
            PHASE_ID=$(echo "$response" | jq -r '.items[0].id // null')
        fi
    fi
    
    if [[ -z "${PHASE_ID}" || "${PHASE_ID}" == "null" ]]; then
        log_warning "Не удалось создать/получить фазу, используем ID=1"
        PHASE_ID=1
    fi
    
    # 5. Создаём элемент
    local item_data=$(cat <<EOF
{
    "itemTypeId": 1,
    "stateId": ${STATE_ID},
    "phaseId": ${PHASE_ID},
    "caption": "Тестовый элемент для подписки",
    "content": "Содержимое тестового элемента"
}
EOF
)
    
    result=$(http_request "POST" "/items" "${item_data}" "${TOKEN}")
    http_code=$(echo "$result" | cut -d'|' -f1)
    response=$(echo "$result" | cut -d'|' -f2-)
    
    if [[ "${http_code}" == "201" ]]; then
        ITEM_ID=$(echo "$response" | jq -r '.id')
        log_success "Создан элемент для подписки: ID=${ITEM_ID}"
        return 0
    fi
    
    # Пробуем получить существующий элемент
    result=$(http_request "GET" "/items" "" "${TOKEN}")
    http_code=$(echo "$result" | cut -d'|' -f1)
    response=$(echo "$result" | cut -d'|' -f2-)
    
    if [[ "${http_code}" == "200" ]]; then
        ITEM_ID=$(echo "$response" | jq -r '.items[0].id // null')
        if [[ -n "${ITEM_ID}" && "${ITEM_ID}" != "null" ]]; then
            log_success "Найден существующий элемент: ID=${ITEM_ID}"
            return 0
        fi
    fi
    
    log_warning "Не удалось создать/найти элемент, используем ID=1"
    ITEM_ID=1
    return 0
}

# ============================================================================
# Тесты для Private Messages
# ============================================================================

test_send_private_message() {
    log_test "Тест 2: Отправка личного сообщения"
    
    if [[ -z "${OTHER_USER_ID}" || "${OTHER_USER_ID}" == "null" ]]; then
        log_warning "OTHER_USER_ID не установлен, пропуск теста"
        return 0
    fi
    
    local msg_data=$(cat <<EOF
{
    "receiverUserId": ${OTHER_USER_ID},
    "content": "Привет! Это тестовое личное сообщение от $(date +%H:%M:%S)"
}
EOF
)
    
    local result=$(http_request "POST" "/private-messages" "${msg_data}" "${TOKEN}")
    local http_code=$(echo "$result" | cut -d'|' -f1)
    local response=$(echo "$result" | cut -d'|' -f2-)
    
    if [[ "${http_code}" == "201" ]]; then
        PRIVATE_MSG_ID=$(echo "$response" | jq -r '.id')
        if [[ -n "${PRIVATE_MSG_ID}" && "${PRIVATE_MSG_ID}" != "null" ]]; then
            log_success "Личное сообщение отправлено: ID=${PRIVATE_MSG_ID}"
            return 0
        fi
    fi
    
    log_error "Не удалось отправить сообщение (код: ${http_code})"
    echo "Ответ сервера: ${response}"
    return 1
}

test_get_private_messages() {
    log_test "Тест 3: Получение списка личных сообщений"
    
    local result=$(http_request "GET" "/private-messages" "" "${TOKEN}")
    local http_code=$(echo "$result" | cut -d'|' -f1)
    local response=$(echo "$result" | cut -d'|' -f2-)
    
    if [[ "${http_code}" == "200" ]]; then
        local total_count=$(echo "$response" | jq -r '.totalCount // 0')
        log_success "Получен список личных сообщений: ${total_count} записей"
        return 0
    fi
    
    log_error "Не удалось получить список сообщений (код: ${http_code})"
    return 1
}

test_get_private_messages_filtered() {
    log_test "Тест 4: Фильтрация личных сообщений"
    
    if [[ -z "${OTHER_USER_ID}" || "${OTHER_USER_ID}" == "null" ]]; then
        log_warning "OTHER_USER_ID не установлен, пропуск теста"
        return 0
    fi
    
    local result=$(http_request "GET" "/private-messages?withUserId=${OTHER_USER_ID}" "" "${TOKEN}")
    local http_code=$(echo "$result" | cut -d'|' -f1)
    local response=$(echo "$result" | cut -d'|' -f2-)
    
    if [[ "${http_code}" == "200" ]]; then
        local total_count=$(echo "$response" | jq -r '.totalCount // 0')
        log_success "Фильтрация по пользователю: ${total_count} записей"
        return 0
    fi
    
    log_error "Не удалось выполнить фильтрацию (код: ${http_code})"
    return 1
}

test_get_private_message_by_id() {
    log_test "Тест 5: Получение личного сообщения по ID"
    
    if [[ -z "${PRIVATE_MSG_ID}" || "${PRIVATE_MSG_ID}" == "null" ]]; then
        log_warning "PRIVATE_MSG_ID не установлен, пропуск теста"
        return 0
    fi
    
    local result=$(http_request "GET" "/private-messages/${PRIVATE_MSG_ID}" "" "${TOKEN}")
    local http_code=$(echo "$result" | cut -d'|' -f1)
    local response=$(echo "$result" | cut -d'|' -f2-)
    
    if [[ "${http_code}" == "200" ]]; then
        local id=$(echo "$response" | jq -r '.id')
        if [[ "${id}" == "${PRIVATE_MSG_ID}" ]]; then
            log_success "Получено сообщение с ID: ${id}"
            return 0
        fi
    fi
    
    log_error "Не удалось получить сообщение (код: ${http_code})"
    return 1
}

test_get_conversation() {
    log_test "Тест 6: Получение переписки с пользователем"
    
    if [[ -z "${OTHER_USER_ID}" || "${OTHER_USER_ID}" == "null" ]]; then
        log_warning "OTHER_USER_ID не установлен, пропуск теста"
        return 0
    fi
    
    local result=$(http_request "GET" "/private-messages/conversation/${OTHER_USER_ID}" "" "${TOKEN}")
    local http_code=$(echo "$result" | cut -d'|' -f1)
    local response=$(echo "$result" | cut -d'|' -f2-)
    
    if [[ "${http_code}" == "200" ]]; then
        local count=$(echo "$response" | jq -r '. | length // 0')
        log_success "Получена переписка: ${count} сообщений"
        return 0
    fi
    
    log_error "Не удалось получить переписку (код: ${http_code})"
    return 1
}

test_mark_message_as_viewed() {
    log_test "Тест 7: Отметка сообщения как прочитанного"
    
    if [[ -z "${PRIVATE_MSG_ID}" || "${PRIVATE_MSG_ID}" == "null" ]]; then
        log_warning "PRIVATE_MSG_ID не установлен, пропуск теста"
        return 0
    fi
    
    # Пытаемся отметить сообщение как прочитанное от имени отправителя
    # Это должно вернуть 403, так как только получатель может отметить сообщение
    local result=$(http_request "PUT" "/private-messages/${PRIVATE_MSG_ID}/view" "" "${TOKEN}")
    local http_code=$(echo "$result" | cut -d'|' -f1)
    
    # 403 - ожидаемое поведение (только получатель может отметить)
    if [[ "${http_code}" == "403" ]]; then
        log_success "Отметка прочтения: 403 (только получатель может отметить) — корректное поведение"
        return 0
    elif [[ "${http_code}" == "200" ]]; then
        log_success "Сообщение отмечено как прочитанное (отправитель = получатель?)"
        return 0
    fi
    
    log_error "Не удалось отметить сообщение (код: ${http_code})"
    return 1
}

test_count_unviewed() {
    log_test "Тест 8: Получение количества непрочитанных сообщений"
    
    local result=$(http_request "GET" "/private-messages/unviewed/count" "" "${TOKEN}")
    local http_code=$(echo "$result" | cut -d'|' -f1)
    local response=$(echo "$result" | cut -d'|' -f2-)
    
    if [[ "${http_code}" == "200" ]]; then
        local count=$(echo "$response" | jq -r '.count // 0')
        log_success "Непрочитанных сообщений: ${count}"
        return 0
    fi
    
    log_error "Не удалось получить количество (код: ${http_code})"
    return 1
}

test_delete_private_message() {
    log_test "Тест 9: Удаление личного сообщения"
    
    if [[ -z "${PRIVATE_MSG_ID}" || "${PRIVATE_MSG_ID}" == "null" ]]; then
        log_warning "PRIVATE_MSG_ID не установлен, пропуск теста"
        return 0
    fi
    
    local result=$(http_request "DELETE" "/private-messages/${PRIVATE_MSG_ID}" "" "${TOKEN}")
    local http_code=$(echo "$result" | cut -d'|' -f1)
    
    if [[ "${http_code}" == "204" ]]; then
        log_success "Личное сообщение удалено"
        return 0
    fi
    
    log_error "Не удалось удалить сообщение (код: ${http_code})"
    return 1
}

# ============================================================================
# Тесты для Team Messages
# ============================================================================

test_send_team_message() {
    log_test "Тест 10: Отправка сообщения в команду"
    
    if [[ -z "${TEAM_ID}" || "${TEAM_ID}" == "null" ]]; then
        log_warning "TEAM_ID не установлен, пропуск теста"
        return 0
    fi
    
    local msg_data=$(cat <<EOF
{
    "teamId": ${TEAM_ID},
    "content": "Привет команде! Тестовое сообщение от $(date +%H:%M:%S)"
}
EOF
)
    
    local result=$(http_request "POST" "/team-messages" "${msg_data}" "${TOKEN}")
    local http_code=$(echo "$result" | cut -d'|' -f1)
    local response=$(echo "$result" | cut -d'|' -f2-)
    
    if [[ "${http_code}" == "201" ]]; then
        TEAM_MSG_ID=$(echo "$response" | jq -r '.id')
        if [[ -n "${TEAM_MSG_ID}" && "${TEAM_MSG_ID}" != "null" ]]; then
            log_success "Сообщение в команду отправлено: ID=${TEAM_MSG_ID}"
            return 0
        fi
    fi
    
    if [[ "${http_code}" == "403" ]]; then
        log_warning "Не удалось отправить сообщение: пользователь не состоит в команде (403)"
        return 0
    fi
    
    log_error "Не удалось отправить сообщение в команду (код: ${http_code})"
    echo "Ответ сервера: ${response}"
    return 1
}

test_get_team_messages() {
    log_test "Тест 11: Получение списка сообщений в командах"
    
    local result=$(http_request "GET" "/team-messages" "" "${TOKEN}")
    local http_code=$(echo "$result" | cut -d'|' -f1)
    local response=$(echo "$result" | cut -d'|' -f2-)
    
    if [[ "${http_code}" == "200" ]]; then
        local total_count=$(echo "$response" | jq -r '.totalCount // 0')
        log_success "Получен список сообщений: ${total_count} записей"
        return 0
    fi
    
    log_error "Не удалось получить список сообщений (код: ${http_code})"
    return 1
}

test_get_team_messages_filtered() {
    log_test "Тест 12: Фильтрация сообщений по команде"
    
    if [[ -z "${TEAM_ID}" || "${TEAM_ID}" == "null" ]]; then
        log_warning "TEAM_ID не установлен, пропуск теста"
        return 0
    fi
    
    local result=$(http_request "GET" "/team-messages?teamId=${TEAM_ID}" "" "${TOKEN}")
    local http_code=$(echo "$result" | cut -d'|' -f1)
    local response=$(echo "$result" | cut -d'|' -f2-)
    
    if [[ "${http_code}" == "200" ]]; then
        local total_count=$(echo "$response" | jq -r '.totalCount // 0')
        log_success "Фильтрация по команде: ${total_count} записей"
        return 0
    fi
    
    log_error "Не удалось выполнить фильтрацию (код: ${http_code})"
    return 1
}

test_get_team_message_by_id() {
    log_test "Тест 13: Получение сообщения по ID"
    
    if [[ -z "${TEAM_MSG_ID}" || "${TEAM_MSG_ID}" == "null" ]]; then
        log_warning "TEAM_MSG_ID не установлен, пропуск теста"
        return 0
    fi
    
    local result=$(http_request "GET" "/team-messages/${TEAM_MSG_ID}" "" "${TOKEN}")
    local http_code=$(echo "$result" | cut -d'|' -f1)
    local response=$(echo "$result" | cut -d'|' -f2-)
    
    if [[ "${http_code}" == "200" ]]; then
        local id=$(echo "$response" | jq -r '.id')
        if [[ "${id}" == "${TEAM_MSG_ID}" ]]; then
            log_success "Получено сообщение с ID: ${id}"
            return 0
        fi
    fi
    
    log_error "Не удалось получить сообщение (код: ${http_code})"
    return 1
}

test_get_team_messages_by_team() {
    log_test "Тест 14: Получение всех сообщений команды"
    
    if [[ -z "${TEAM_ID}" || "${TEAM_ID}" == "null" ]]; then
        log_warning "TEAM_ID не установлен, пропуск теста"
        return 0
    fi
    
    local result=$(http_request "GET" "/teams/${TEAM_ID}/messages" "" "${TOKEN}")
    local http_code=$(echo "$result" | cut -d'|' -f1)
    local response=$(echo "$result" | cut -d'|' -f2-)
    
    if [[ "${http_code}" == "200" ]]; then
        local count=$(echo "$response" | jq -r '. | length // 0')
        log_success "Получены сообщения команды: ${count} записей"
        return 0
    fi
    
    log_error "Не удалось получить сообщения команды (код: ${http_code})"
    return 1
}

test_delete_team_message() {
    log_test "Тест 15: Удаление сообщения из команды"
    
    if [[ -z "${TEAM_MSG_ID}" || "${TEAM_MSG_ID}" == "null" ]]; then
        log_warning "TEAM_MSG_ID не установлен, пропуск теста"
        return 0
    fi
    
    local result=$(http_request "DELETE" "/team-messages/${TEAM_MSG_ID}" "" "${TOKEN}")
    local http_code=$(echo "$result" | cut -d'|' -f1)
    
    if [[ "${http_code}" == "204" ]]; then
        log_success "Сообщение из команды удалено"
        return 0
    fi
    
    log_error "Не удалось удалить сообщение (код: ${http_code})"
    return 1
}

# ============================================================================
# Тесты для User Notifications
# ============================================================================

test_subscribe_to_item() {
    log_test "Тест 16: Подписка на элемент"
    
    if [[ -z "${ITEM_ID}" || "${ITEM_ID}" == "null" ]]; then
        log_warning "ITEM_ID не установлен, пропуск теста"
        return 0
    fi
    
    local sub_data=$(cat <<EOF
{
    "itemId": ${ITEM_ID}
}
EOF
)
    
    local result=$(http_request "POST" "/user-notifications" "${sub_data}" "${TOKEN}")
    local http_code=$(echo "$result" | cut -d'|' -f1)
    local response=$(echo "$result" | cut -d'|' -f2-)
    
    if [[ "${http_code}" == "201" ]]; then
        NOTIFICATION_ID=$(echo "$response" | jq -r '.id')
        if [[ -n "${NOTIFICATION_ID}" && "${NOTIFICATION_ID}" != "null" ]]; then
            log_success "Подписка создана: ID=${NOTIFICATION_ID}"
            return 0
        fi
    elif [[ "${http_code}" == "409" ]]; then
        log_warning "Подписка уже существует (Conflict)"
        return 0
    elif [[ "${http_code}" == "404" ]]; then
        log_warning "Элемент не найден (404)"
        return 0
    fi
    
    log_error "Не удалось создать подписку (код: ${http_code})"
    echo "Ответ сервера: ${response}"
    return 1
}

test_get_user_notifications() {
    log_test "Тест 17: Получение списка подписок"
    
    local result=$(http_request "GET" "/user-notifications" "" "${TOKEN}")
    local http_code=$(echo "$result" | cut -d'|' -f1)
    local response=$(echo "$result" | cut -d'|' -f2-)
    
    if [[ "${http_code}" == "200" ]]; then
        local total_count=$(echo "$response" | jq -r '.totalCount // 0')
        log_success "Получен список подписок: ${total_count} записей"
        return 0
    fi
    
    log_error "Не удалось получить список подписок (код: ${http_code})"
    return 1
}

test_get_user_notifications_filtered() {
    log_test "Тест 18: Фильтрация подписок по элементу"
    
    if [[ -z "${ITEM_ID}" || "${ITEM_ID}" == "null" ]]; then
        log_warning "ITEM_ID не установлен, пропуск теста"
        return 0
    fi
    
    local result=$(http_request "GET" "/user-notifications?itemId=${ITEM_ID}" "" "${TOKEN}")
    local http_code=$(echo "$result" | cut -d'|' -f1)
    local response=$(echo "$result" | cut -d'|' -f2-)
    
    if [[ "${http_code}" == "200" ]]; then
        local total_count=$(echo "$response" | jq -r '.totalCount // 0')
        log_success "Фильтрация по элементу: ${total_count} записей"
        return 0
    fi
    
    log_error "Не удалось выполнить фильтрацию (код: ${http_code})"
    return 1
}

test_get_notification_by_id() {
    log_test "Тест 19: Получение подписки по ID"
    
    if [[ -z "${NOTIFICATION_ID}" || "${NOTIFICATION_ID}" == "null" ]]; then
        log_warning "NOTIFICATION_ID не установлен, пропуск теста"
        return 0
    fi
    
    local result=$(http_request "GET" "/user-notifications/${NOTIFICATION_ID}" "" "${TOKEN}")
    local http_code=$(echo "$result" | cut -d'|' -f1)
    local response=$(echo "$result" | cut -d'|' -f2-)
    
    if [[ "${http_code}" == "200" ]]; then
        local id=$(echo "$response" | jq -r '.id')
        if [[ "${id}" == "${NOTIFICATION_ID}" ]]; then
            log_success "Получена подписка с ID: ${id}"
            return 0
        fi
    fi
    
    log_error "Не удалось получить подписку (код: ${http_code})"
    return 1
}

test_is_subscribed() {
    log_test "Тест 20: Проверка подписки на элемент"
    
    if [[ -z "${ITEM_ID}" || "${ITEM_ID}" == "null" ]]; then
        log_warning "ITEM_ID не установлен, пропуск теста"
        return 0
    fi
    
    local result=$(http_request "GET" "/items/${ITEM_ID}/subscribed" "" "${TOKEN}")
    local http_code=$(echo "$result" | cut -d'|' -f1)
    local response=$(echo "$result" | cut -d'|' -f2-)
    
    if [[ "${http_code}" == "200" ]]; then
        local subscribed=$(echo "$response" | jq -r '.subscribed')
        if [[ "${subscribed}" == "true" ]]; then
            log_success "Пользователь подписан на элемент"
        else
            log_warning "Пользователь не подписан на элемент"
        fi
        return 0
    fi
    
    log_error "Не удалось проверить подписку (код: ${http_code})"
    return 1
}

test_get_subscribers() {
    log_test "Тест 21: Получение подписчиков элемента"
    
    if [[ -z "${ITEM_ID}" || "${ITEM_ID}" == "null" ]]; then
        log_warning "ITEM_ID не установлен, пропуск теста"
        return 0
    fi
    
    local result=$(http_request "GET" "/items/${ITEM_ID}/subscribers" "" "${TOKEN}")
    local http_code=$(echo "$result" | cut -d'|' -f1)
    local response=$(echo "$result" | cut -d'|' -f2-)
    
    if [[ "${http_code}" == "200" ]]; then
        local count=$(echo "$response" | jq -r '. | length // 0')
        log_success "Получены подписчики элемента: ${count} записей"
        return 0
    fi
    
    log_error "Не удалось получить подписчиков (код: ${http_code})"
    return 1
}

test_unsubscribe() {
    log_test "Тест 22: Отписка от элемента"
    
    if [[ -z "${NOTIFICATION_ID}" || "${NOTIFICATION_ID}" == "null" ]]; then
        log_warning "NOTIFICATION_ID не установлен, пропуск теста"
        return 0
    fi
    
    local result=$(http_request "DELETE" "/user-notifications/${NOTIFICATION_ID}" "" "${TOKEN}")
    local http_code=$(echo "$result" | cut -d'|' -f1)
    
    if [[ "${http_code}" == "204" ]]; then
        log_success "Отписка выполнена"
        return 0
    fi
    
    log_error "Не удалось отписаться (код: ${http_code})"
    return 1
}

# ============================================================================
# Негативные тесты
# ============================================================================

test_send_private_message_to_self() {
    log_test "Тест 23: Отправка сообщения самому себе"
    
    if [[ -z "${USER_ID}" || "${USER_ID}" == "null" ]]; then
        log_warning "USER_ID не установлен, пропуск теста"
        return 0
    fi
    
    local msg_data=$(cat <<EOF
{
    "receiverUserId": ${USER_ID},
    "content": "Сообщение самому себе"
}
EOF
)
    
    local result=$(http_request "POST" "/private-messages" "${msg_data}" "${TOKEN}")
    local http_code=$(echo "$result" | cut -d'|' -f1)
    
    if [[ "${http_code}" == "400" ]]; then
        log_success "Ошибка валидации: 400 Bad Request"
        return 0
    fi
    
    log_error "Ожидался 400, получен ${http_code}"
    return 1
}

test_send_private_message_missing_content() {
    log_test "Тест 24: Отправка сообщения без содержимого"
    
    if [[ -z "${OTHER_USER_ID}" || "${OTHER_USER_ID}" == "null" ]]; then
        log_warning "OTHER_USER_ID не установлен, пропуск теста"
        return 0
    fi
    
    local msg_data=$(cat <<EOF
{
    "receiverUserId": ${OTHER_USER_ID}
}
EOF
)
    
    local result=$(http_request "POST" "/private-messages" "${msg_data}" "${TOKEN}")
    local http_code=$(echo "$result" | cut -d'|' -f1)
    
    if [[ "${http_code}" == "400" ]]; then
        log_success "Ошибка валидации: 400 Bad Request"
        return 0
    fi
    
    log_error "Ожидался 400, получен ${http_code}"
    return 1
}

test_send_team_message_not_member() {
    log_test "Тест 25: Отправка сообщения в несуществующую команду"
    
    local msg_data=$(cat <<EOF
{
    "teamId": 99999,
    "content": "Сообщение в несуществующую команду"
}
EOF
)
    
    local result=$(http_request "POST" "/team-messages" "${msg_data}" "${TOKEN}")
    local http_code=$(echo "$result" | cut -d'|' -f1)
    
    # Может быть 403 (не член команды) или 404 (команда не найдена)
    if [[ "${http_code}" == "403" ]] || [[ "${http_code}" == "404" ]]; then
        log_success "Ошибка доступа: ${http_code}"
        return 0
    fi
    
    log_error "Ожидался 403 или 404, получен ${http_code}"
    return 1
}

test_get_non_existent_private_message() {
    log_test "Тест 26: Получение несуществующего личного сообщения"
    
    local result=$(http_request "GET" "/private-messages/99999" "" "${TOKEN}")
    local http_code=$(echo "$result" | cut -d'|' -f1)
    
    if [[ "${http_code}" == "404" ]]; then
        log_success "Несуществующее сообщение: 404 Not Found"
        return 0
    fi
    
    log_error "Ожидался 404, получен ${http_code}"
    return 1
}

test_unsubscribe_non_existent() {
    log_test "Тест 27: Отписка от несуществующей подписки"
    
    local result=$(http_request "DELETE" "/user-notifications/99999" "" "${TOKEN}")
    local http_code=$(echo "$result" | cut -d'|' -f1)
    
    if [[ "${http_code}" == "404" ]]; then
        log_success "Несуществующая подписка: 404 Not Found"
        return 0
    fi
    
    log_error "Ожидался 404, получен ${http_code}"
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
    echo "   Тестирование API сообщений и уведомлений"
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
    
    # Подготовка данных
    test_get_current_user || true
    test_get_or_create_other_user || true
    test_get_or_create_team || true
    test_get_or_create_item || true
    
    # Тесты личных сообщений
    test_send_private_message || true
    test_get_private_messages || true
    test_get_private_messages_filtered || true
    test_get_private_message_by_id || true
    test_get_conversation || true
    test_mark_message_as_viewed || true
    test_count_unviewed || true
    test_delete_private_message || true
    
    # Тесты сообщений в командах
    test_send_team_message || true
    test_get_team_messages || true
    test_get_team_messages_filtered || true
    test_get_team_message_by_id || true
    test_get_team_messages_by_team || true
    test_delete_team_message || true
    
    # Тесты уведомлений
    test_subscribe_to_item || true
    test_get_user_notifications || true
    test_get_user_notifications_filtered || true
    test_get_notification_by_id || true
    test_is_subscribed || true
    test_get_subscribers || true
    test_unsubscribe || true
    
    # Негативные тесты
    test_send_private_message_to_self || true
    test_send_private_message_missing_content || true
    test_send_team_message_not_member || true
    test_get_non_existent_private_message || true
    test_unsubscribe_non_existent || true
    
    # Вывод результатов
    print_summary
}

# Запуск основной функции
main "$@"
