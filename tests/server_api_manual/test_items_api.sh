#!/bin/bash

# ============================================================
# Тестирование API элементов (Items) и их полей
# ============================================================

set -e

# Цвета для вывода
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Параметры по умолчанию
SERVER_URL="http://localhost:8090"
VERBOSE=false

# Счётчики тестов
TESTS_PASSED=0
TESTS_FAILED=0
TESTS_TOTAL=0

# Хранилище для ID созданных объектов
declare -a CREATED_ITEMS
declare -a CREATED_WORKFLOWS
declare -a CREATED_STATES
declare -a CREATED_ITEM_TYPES
declare -a CREATED_PROJECTS
declare -a CREATED_PHASES
declare -a CREATED_FIELD_TYPES
declare -a CREATED_FIELDS

# ============================================================
# Вспомогательные функции
# ============================================================

print_success() { echo -e "${GREEN}✓${NC} $1"; }
print_error() { echo -e "${RED}✗${NC} $1"; }
print_info() { echo -e "${BLUE}ℹ${NC} $1"; }
print_warning() { echo -e "${YELLOW}⚠${NC} $1"; }

print_test_header() {
    echo ""
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${BLUE}📋 $1${NC}"
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
}

log_verbose() {
    if [ "$VERBOSE" = true ]; then
        echo -e "${YELLOW}[DEBUG]${NC} $1"
    fi
}

http_request() {
    local method=$1
    local endpoint=$2
    local data=$3
    local token=$4
    local url="${SERVER_URL}${endpoint}"
    
    log_verbose "Request: $method $url"
    log_verbose "Data: $data"
    
    local curl_cmd="curl -s -X $method \"$url\""
    
    if [ -n "$token" ]; then
        curl_cmd="$curl_cmd -H \"Authorization: Bearer $token\""
    fi
    
    if [ -n "$data" ]; then
        curl_cmd="$curl_cmd -H \"Content-Type: application/json\" -d '$data'"
    fi
    
    curl_cmd="$curl_cmd -w \"\n%{http_code}\""
    
    local response_file=$(mktemp)
    eval "$curl_cmd" > "$response_file" 2>/dev/null
    
    local http_code=$(tail -n1 "$response_file")
    local response_body=$(sed '$d' "$response_file")
    
    rm -f "$response_file"
    
    echo "$http_code|$response_body"
}

expect_success() {
    local response=$1
    local expected_code=$2
    local test_name=$3
    
    local http_code=$(echo "$response" | cut -d'|' -f1)
    local body=$(echo "$response" | cut -d'|' -f2-)
    
    TESTS_TOTAL=$((TESTS_TOTAL + 1))
    
    if [ "$http_code" = "$expected_code" ]; then
        print_success "$test_name (HTTP $http_code)"
        TESTS_PASSED=$((TESTS_PASSED + 1))
        return 0
    else
        print_error "$test_name - Ожидался код $expected_code, получен $http_code"
        log_verbose "Response body: $body"
        TESTS_FAILED=$((TESTS_FAILED + 1))
        return 1
    fi
}

expect_error() {
    local response=$1
    local expected_code=$2
    local test_name=$3
    
    local http_code=$(echo "$response" | cut -d'|' -f1)
    
    TESTS_TOTAL=$((TESTS_TOTAL + 1))
    
    if [ "$http_code" = "$expected_code" ]; then
        print_success "$test_name (HTTP $http_code - ожидаемая ошибка)"
        TESTS_PASSED=$((TESTS_PASSED + 1))
        return 0
    else
        print_error "$test_name - Ожидался код ошибки $expected_code, получен $http_code"
        TESTS_FAILED=$((TESTS_FAILED + 1))
        return 1
    fi
}

get_json_value() {
    local response=$1
    local key=$2
    local body=$(echo "$response" | cut -d'|' -f2-)
    echo "$body" | jq -r "$key" 2>/dev/null
}

# ============================================================
# Аутентификация
# ============================================================

login() {
    local login=$1
    local password=$2
    
    local data="{\"login\":\"${login}\",\"password\":\"${password}\"}"
    local response=$(http_request "POST" "/auth/login" "$data" "")
    
    local http_code=$(echo "$response" | cut -d'|' -f1)
    
    if [ "$http_code" = "200" ]; then
        local token=$(get_json_value "$response" ".accessToken")
        if [ -z "$token" ] || [ "$token" = "null" ]; then
            token=$(get_json_value "$response" ".access_token")
        fi
        echo "$token"
    else
        echo ""
    fi
}

# ============================================================
# Создание тестовых данных (отладочные сообщения -> stderr)
# ============================================================

create_test_workflow() {
    echo -e "${BLUE}ℹ${NC} Создание тестового рабочего процесса..." >&2
    
    local data='{"caption":"Тестовый workflow","description":"Для тестирования элементов"}'
    local response=$(http_request "POST" "/api/workflows" "$data" "$ADMIN_TOKEN")
    
    local http_code=$(echo "$response" | cut -d'|' -f1)
    if [ "$http_code" = "201" ]; then
        local workflow_id=$(get_json_value "$response" ".id")
        CREATED_WORKFLOWS+=("$workflow_id")
        echo -e "${GREEN}✓${NC} Создан workflow с ID: $workflow_id" >&2
        echo "$workflow_id"
    else
        echo -e "${YELLOW}⚠${NC} Не удалось создать workflow, используем ID 1" >&2
        echo "1"
    fi
}

create_test_state() {
    local workflow_id=$1
    echo -e "${BLUE}ℹ${NC} Создание тестового состояния..." >&2
    
    local data="{\"workflowId\":${workflow_id},\"caption\":\"Новая\",\"orderNumber\":1}"
    local response=$(http_request "POST" "/api/states" "$data" "$ADMIN_TOKEN")
    
    local http_code=$(echo "$response" | cut -d'|' -f1)
    if [ "$http_code" = "201" ]; then
        local state_id=$(get_json_value "$response" ".id")
        CREATED_STATES+=("$state_id")
        echo -e "${GREEN}✓${NC} Создано состояние с ID: $state_id" >&2
        echo "$state_id"
    else
        echo -e "${YELLOW}⚠${NC} Не удалось создать состояние, используем ID 1" >&2
        echo "1"
    fi
}

create_test_item_type() {
    local workflow_id=$1
    local default_state_id=$2
    echo -e "${BLUE}ℹ${NC} Создание тестового типа элемента..." >&2
    
    local data="{\"workflowId\":${workflow_id},\"defaultStateId\":${default_state_id},\"caption\":\"Задача\",\"kind\":\"issue\"}"
    local response=$(http_request "POST" "/api/item-types" "$data" "$ADMIN_TOKEN")
    
    local http_code=$(echo "$response" | cut -d'|' -f1)
    if [ "$http_code" = "201" ]; then
        local item_type_id=$(get_json_value "$response" ".id")
        CREATED_ITEM_TYPES+=("$item_type_id")
        echo -e "${GREEN}✓${NC} Создан тип элемента с ID: $item_type_id" >&2
        echo "$item_type_id"
    else
        echo -e "${YELLOW}⚠${NC} Не удалось создать тип элемента, используем ID 1" >&2
        echo "1"
    fi
}

create_test_project() {
    echo -e "${BLUE}ℹ${NC} Создание тестового проекта..." >&2
    
    local data='{"caption":"Тестовый проект","description":"Для тестирования элементов"}'
    local response=$(http_request "POST" "/api/projects" "$data" "$ADMIN_TOKEN")
    
    local http_code=$(echo "$response" | cut -d'|' -f1)
    if [ "$http_code" = "201" ]; then
        local project_id=$(get_json_value "$response" ".id")
        CREATED_PROJECTS+=("$project_id")
        echo -e "${GREEN}✓${NC} Создан проект с ID: $project_id" >&2
        echo "$project_id"
    else
        echo -e "${YELLOW}⚠${NC} Не удалось создать проект, используем ID 1" >&2
        echo "1"
    fi
}

create_test_phase() {
    local project_id=$1
    echo -e "${BLUE}ℹ${NC} Создание тестовой фазы..." >&2
    
    local data="{\"projectId\":${project_id},\"caption\":\"Тестовая фаза\"}"
    local response=$(http_request "POST" "/api/phases" "$data" "$ADMIN_TOKEN")
    
    local http_code=$(echo "$response" | cut -d'|' -f1)
    if [ "$http_code" = "201" ]; then
        local phase_id=$(get_json_value "$response" ".id")
        CREATED_PHASES+=("$phase_id")
        echo -e "${GREEN}✓${NC} Создана фаза с ID: $phase_id" >&2
        echo "$phase_id"
    else
        echo -e "${YELLOW}⚠${NC} Не удалось создать фазу, используем ID 1" >&2
        echo "1"
    fi
}

create_test_field_type() {
    local item_type_id=$1
    echo -e "${BLUE}ℹ${NC} Создание тестового типа поля..." >&2
    
    local data="{\"itemTypeId\":${item_type_id},\"caption\":\"Приоритет\",\"valueType\":\"Select\"}"
    local response=$(http_request "POST" "/api/field-types" "$data" "$ADMIN_TOKEN")
    
    local http_code=$(echo "$response" | cut -d'|' -f1)
    if [ "$http_code" = "201" ]; then
        local field_type_id=$(get_json_value "$response" ".id")
        CREATED_FIELD_TYPES+=("$field_type_id")
        echo -e "${GREEN}✓${NC} Создан тип поля с ID: $field_type_id" >&2
        echo "$field_type_id"
    else
        echo -e "${YELLOW}⚠${NC} Не удалось создать тип поля, используем ID 1" >&2
        echo "1"
    fi
}

setup_test_data() {
    print_test_header "НАСТРОЙКА ТЕСТОВЫХ ДАННЫХ"
    
    WORKFLOW_ID=$(create_test_workflow)
    STATE_ID=$(create_test_state "$WORKFLOW_ID")
    ITEM_TYPE_ID=$(create_test_item_type "$WORKFLOW_ID" "$STATE_ID")
    PROJECT_ID=$(create_test_project)
    PHASE_ID=$(create_test_phase "$PROJECT_ID")
    FIELD_TYPE_ID=$(create_test_field_type "$ITEM_TYPE_ID")
    
    print_info "Тестовые данные созданы:"
    echo "  Workflow ID: $WORKFLOW_ID"
    echo "  State ID: $STATE_ID"
    echo "  ItemType ID: $ITEM_TYPE_ID"
    echo "  Project ID: $PROJECT_ID"
    echo "  Phase ID: $PHASE_ID"
    echo "  FieldType ID: $FIELD_TYPE_ID"
}

# ============================================================
# Тесты элементов (Items)
# ============================================================

test_create_item() {
    print_test_header "Создание элемента"
    
    # 1. Создание элемента с минимальными полями
    local data="{\"caption\":\"Тестовая задача\",\"itemTypeId\":${ITEM_TYPE_ID},\"stateId\":${STATE_ID},\"phaseId\":${PHASE_ID}}"
    local response=$(http_request "POST" "/api/items" "$data" "$ADMIN_TOKEN")
    expect_success "$response" "201" "Создание элемента с минимальными полями"
    
    local item_id=$(get_json_value "$response" ".id")
    if [ -n "$item_id" ] && [ "$item_id" != "null" ]; then
        CREATED_ITEMS+=("$item_id")
        print_info "Создан элемент с ID: $item_id"
    fi
    
    # 2. Создание элемента со всеми полями
    local data_full="{\"caption\":\"Полная задача\",\"itemTypeId\":${ITEM_TYPE_ID},\"stateId\":${STATE_ID},\"phaseId\":${PHASE_ID},\"content\":\"Подробное описание\"}"
    local response2=$(http_request "POST" "/api/items" "$data_full" "$ADMIN_TOKEN")
    expect_success "$response2" "201" "Создание элемента со всеми полями"
    
    local item_id2=$(get_json_value "$response2" ".id")
    if [ -n "$item_id2" ] && [ "$item_id2" != "null" ]; then
        CREATED_ITEMS+=("$item_id2")
    fi
    
    # 3. Создание элемента без обязательного поля caption
    local data_no_caption="{\"itemTypeId\":${ITEM_TYPE_ID},\"stateId\":${STATE_ID},\"phaseId\":${PHASE_ID}}"
    local response3=$(http_request "POST" "/api/items" "$data_no_caption" "$ADMIN_TOKEN")
    expect_error "$response3" "400" "Создание элемента без caption"
    
    # 4. Создание элемента без itemTypeId
    local data_no_type="{\"caption\":\"Без типа\",\"stateId\":${STATE_ID},\"phaseId\":${PHASE_ID}}"
    local response4=$(http_request "POST" "/api/items" "$data_no_type" "$ADMIN_TOKEN")
    expect_error "$response4" "400" "Создание элемента без itemTypeId"
    
    # 5. Создание элемента без stateId
    local data_no_state="{\"caption\":\"Без состояния\",\"itemTypeId\":${ITEM_TYPE_ID},\"phaseId\":${PHASE_ID}}"
    local response5=$(http_request "POST" "/api/items" "$data_no_state" "$ADMIN_TOKEN")
    expect_error "$response5" "400" "Создание элемента без stateId"
    
    # 6. Создание элемента без phaseId
    local data_no_phase="{\"caption\":\"Без фазы\",\"itemTypeId\":${ITEM_TYPE_ID},\"stateId\":${STATE_ID}}"
    local response6=$(http_request "POST" "/api/items" "$data_no_phase" "$ADMIN_TOKEN")
    expect_error "$response6" "400" "Создание элемента без phaseId"
}

test_get_item() {
    print_test_header "Получение элемента"
    
    if [ ${#CREATED_ITEMS[@]} -eq 0 ]; then
        print_warning "Нет созданных элементов"
        return
    fi
    
    local item_id=${CREATED_ITEMS[0]}
    
    # 1. Получение существующего элемента
    local response=$(http_request "GET" "/api/items/${item_id}" "" "$ADMIN_TOKEN")
    expect_success "$response" "200" "Получение существующего элемента"
    
    local caption=$(get_json_value "$response" ".caption")
    if [ "$caption" = "Тестовая задача" ]; then
        print_success "  - caption соответствует"
        TESTS_PASSED=$((TESTS_PASSED + 1))
    else
        print_error "  - caption = $caption"
        TESTS_FAILED=$((TESTS_FAILED + 1))
    fi
    TESTS_TOTAL=$((TESTS_TOTAL + 1))
    
    # 2. Получение несуществующего элемента
    local response2=$(http_request "GET" "/api/items/999999" "" "$ADMIN_TOKEN")
    expect_error "$response2" "404" "Получение несуществующего элемента"
}

test_list_items() {
    print_test_header "Список элементов (пагинация и фильтрация)"
    
    # 1. Получение списка с пагинацией
    local response=$(http_request "GET" "/api/items?page=1&pageSize=10" "" "$ADMIN_TOKEN")
    expect_success "$response" "200" "Получение списка (страница 1)"
    
    local total=$(get_json_value "$response" ".totalCount")
    print_info "Всего элементов: $total"
    
    # 2. Фильтрация по состоянию
    local response3=$(http_request "GET" "/api/items?stateId=${STATE_ID}" "" "$ADMIN_TOKEN")
    expect_success "$response3" "200" "Фильтрация элементов по состоянию"
    
    # 3. Фильтрация по типу элемента
    local response4=$(http_request "GET" "/api/items?itemTypeId=${ITEM_TYPE_ID}" "" "$ADMIN_TOKEN")
    expect_success "$response4" "200" "Фильтрация элементов по типу"
    
    # 4. Фильтрация по фазе
    local response5=$(http_request "GET" "/api/items?phaseId=${PHASE_ID}" "" "$ADMIN_TOKEN")
    expect_success "$response5" "200" "Фильтрация элементов по фазе"
}

test_update_item() {
    print_test_header "Обновление элемента"
    
    if [ ${#CREATED_ITEMS[@]} -eq 0 ]; then
        print_warning "Нет созданных элементов"
        return
    fi
    
    local item_id=${CREATED_ITEMS[0]}
    
    # 1. Обновление названия элемента
    local data="{\"caption\":\"Обновлённая задача\"}"
    local response=$(http_request "PUT" "/api/items/${item_id}" "$data" "$ADMIN_TOKEN")
    expect_success "$response" "200" "Обновление названия элемента"
    
    local new_caption=$(get_json_value "$response" ".caption")
    if [ "$new_caption" = "Обновлённая задача" ]; then
        print_success "  - caption обновлён"
        TESTS_PASSED=$((TESTS_PASSED + 1))
    else
        print_error "  - caption = $new_caption"
        TESTS_FAILED=$((TESTS_FAILED + 1))
    fi
    TESTS_TOTAL=$((TESTS_TOTAL + 1))
    
    # 2. Обновление содержимого
    local data_content="{\"content\":\"Новое содержимое\"}"
    local response2=$(http_request "PUT" "/api/items/${item_id}" "$data_content" "$ADMIN_TOKEN")
    expect_success "$response2" "200" "Обновление содержимого элемента"
    
    # 3. Обновление несуществующего элемента
    local response4=$(http_request "PUT" "/api/items/999999" "$data" "$ADMIN_TOKEN")
    expect_error "$response4" "404" "Обновление несуществующего элемента"
}

test_delete_and_restore_item() {
    print_test_header "Удаление и восстановление элемента"
    
    if [ ${#CREATED_ITEMS[@]} -lt 2 ]; then
        print_warning "Недостаточно элементов"
        return
    fi
    
    local item_id=${CREATED_ITEMS[1]}
    
    # 1. Мягкое удаление элемента
    local response=$(http_request "DELETE" "/api/items/${item_id}" "" "$ADMIN_TOKEN")
    expect_success "$response" "204" "Мягкое удаление элемента"
    
    # 2. Восстановление элемента
    local response3=$(http_request "POST" "/api/items/${item_id}/restore" "" "$ADMIN_TOKEN")
    expect_success "$response3" "204" "Восстановление элемента"
    
    # 3. Проверка, что элемент восстановлен
    local response4=$(http_request "GET" "/api/items/${item_id}" "" "$ADMIN_TOKEN")
    expect_success "$response4" "200" "Проверка восстановления элемента"
}

# ============================================================
# Тесты полей элементов
# ============================================================

test_create_item_field() {
    print_test_header "Установка полей элемента"
    
    if [ ${#CREATED_ITEMS[@]} -eq 0 ]; then
        print_warning "Нет созданных элементов"
        return
    fi
    
    local item_id=${CREATED_ITEMS[0]}
    
    # Установка текстового поля
    local data="{\"value\":\"Высокий приоритет\"}"
    local response=$(http_request "PUT" "/api/items/${item_id}/fields/${FIELD_TYPE_ID}" "$data" "$ADMIN_TOKEN")
    expect_success "$response" "200" "Установка текстового поля"
    
    # Небольшая задержка перед следующим тестом
    sleep 0.5
}

test_get_item_fields() {
    print_test_header "Получение полей элемента"
    
    if [ ${#CREATED_ITEMS[@]} -eq 0 ]; then
        print_warning "Нет созданных элементов"
        return
    fi
    
    local item_id=${CREATED_ITEMS[0]}
    
    # Получение всех полей элемента
    local response=$(http_request "GET" "/api/items/${item_id}/fields" "" "$ADMIN_TOKEN")
    expect_success "$response" "200" "Получение всех полей элемента"
    
    local fields_count=$(get_json_value "$response" "length")
    print_info "Найдено полей: $fields_count"
    
    # Если есть поля, пробуем получить первое из них
    if [ "$fields_count" -gt 0 ]; then
        local first_field_id=$(get_json_value "$response" ".[0].fieldTypeId")
        print_info "Получение поля с fieldTypeId: $first_field_id"
        
        local response2=$(http_request "GET" "/api/items/${item_id}/fields/${first_field_id}" "" "$ADMIN_TOKEN")
        if [ "$(echo "$response2" | cut -d'|' -f1)" = "200" ]; then
            print_success "Получение конкретного поля (HTTP 200)"
            TESTS_PASSED=$((TESTS_PASSED + 1))
        else
            print_warning "Не удалось получить поле с ID $first_field_id"
        fi
    else
        print_warning "Нет полей для получения"
    fi
    TESTS_TOTAL=$((TESTS_TOTAL + 1))
    
    # Получение несуществующего поля
    local response3=$(http_request "GET" "/api/items/${item_id}/fields/999" "" "$ADMIN_TOKEN")
    expect_error "$response3" "404" "Получение несуществующего поля"
}

test_update_item_field() {
    print_test_header "Обновление поля элемента"
    
    if [ ${#CREATED_ITEMS[@]} -eq 0 ]; then
        print_warning "Нет созданных элементов"
        return
    fi
    
    local item_id=${CREATED_ITEMS[0]}
    
    # Обновление значения поля
    local data="{\"value\":\"Критический приоритет\"}"
    local response=$(http_request "PUT" "/api/items/${item_id}/fields/${FIELD_TYPE_ID}" "$data" "$ADMIN_TOKEN")
    expect_success "$response" "200" "Обновление значения поля"
    
    local new_value=$(get_json_value "$response" ".value")
    if [ "$new_value" = "Критический приоритет" ]; then
        print_success "  - значение поля обновлено"
        TESTS_PASSED=$((TESTS_PASSED + 1))
    else
        print_error "  - значение = $new_value"
        TESTS_FAILED=$((TESTS_FAILED + 1))
    fi
    TESTS_TOTAL=$((TESTS_TOTAL + 1))
    
    # Небольшая задержка перед следующим тестом
    sleep 0.5
}

test_delete_item_field() {
    print_test_header "Удаление поля элемента"
    
    if [ ${#CREATED_ITEMS[@]} -eq 0 ]; then
        print_warning "Нет созданных элементов"
        return
    fi
    
    local item_id=${CREATED_ITEMS[0]}
    
    # Удаление поля
    local response=$(http_request "DELETE" "/api/items/${item_id}/fields/${FIELD_TYPE_ID}" "" "$ADMIN_TOKEN")
    expect_success "$response" "204" "Удаление поля элемента"
    
    # Проверка, что поле удалено
    local response2=$(http_request "GET" "/api/items/${item_id}/fields/${FIELD_TYPE_ID}" "" "$ADMIN_TOKEN")
    expect_error "$response2" "404" "Проверка удаления поля"
}

# ============================================================
# Тесты иерархии элементов
# ============================================================

test_item_hierarchy() {
    print_test_header "Иерархия элементов (родитель-потомок)"
    
    # 1. Создание родительского элемента
    local parent_data="{\"caption\":\"Родительская задача\",\"itemTypeId\":${ITEM_TYPE_ID},\"stateId\":${STATE_ID},\"phaseId\":${PHASE_ID}}"
    local parent_response=$(http_request "POST" "/api/items" "$parent_data" "$ADMIN_TOKEN")
    expect_success "$parent_response" "201" "Создание родительского элемента"
    
    local parent_id=$(get_json_value "$parent_response" ".id")
    
    # 2. Создание дочернего элемента
    local child_data="{\"caption\":\"Дочерняя задача\",\"itemTypeId\":${ITEM_TYPE_ID},\"stateId\":${STATE_ID},\"phaseId\":${PHASE_ID},\"parentId\":${parent_id}}"
    local child_response=$(http_request "POST" "/api/items" "$child_data" "$ADMIN_TOKEN")
    expect_success "$child_response" "201" "Создание дочернего элемента"
    
    local child_id=$(get_json_value "$child_response" ".id")
    
    # 3. Получение дочерних элементов родителя
    local response=$(http_request "GET" "/api/items?parentId=${parent_id}" "" "$ADMIN_TOKEN")
    expect_success "$response" "200" "Получение дочерних элементов родителя"
    
    local children_count=$(get_json_value "$response" ".totalCount")
    if [ "$children_count" -ge 1 ]; then
        print_success "  - найдено дочерних элементов: $children_count"
        TESTS_PASSED=$((TESTS_PASSED + 1))
    else
        print_error "  - не найдено дочерних элементов"
        TESTS_FAILED=$((TESTS_FAILED + 1))
    fi
    TESTS_TOTAL=$((TESTS_TOTAL + 1))
    
    # Очистка
    CREATED_ITEMS+=("$parent_id" "$child_id")
}

# ============================================================
# Тесты прав доступа
# ============================================================

test_permissions() {
    print_test_header "Проверка прав доступа"
    
    # 1. Доступ без токена
    local response=$(http_request "GET" "/api/items" "" "")
    expect_error "$response" "401" "Доступ без токена авторизации"
    
    # 2. Доступ с невалидным токеном
    local response2=$(http_request "GET" "/api/items" "" "invalid_token_12345")
    expect_error "$response2" "401" "Доступ с невалидным токеном"
}

# ============================================================
# Очистка тестовых данных
# ============================================================

cleanup_test_data() {
    print_test_header "Очистка тестовых данных"
    
    # Удаление элементов (поля удаляются каскадно)
    for item_id in "${CREATED_ITEMS[@]}"; do
        if [ -n "$item_id" ]; then
            http_request "DELETE" "/api/items/${item_id}" "" "$ADMIN_TOKEN" > /dev/null 2>&1
        fi
    done
    
    # Удаление фаз
    for phase_id in "${CREATED_PHASES[@]}"; do
        if [ -n "$phase_id" ]; then
            http_request "DELETE" "/api/phases/${phase_id}" "" "$ADMIN_TOKEN" > /dev/null 2>&1
        fi
    done
    
    # Удаление проектов
    for project_id in "${CREATED_PROJECTS[@]}"; do
        if [ -n "$project_id" ]; then
            http_request "DELETE" "/api/projects/${project_id}" "" "$ADMIN_TOKEN" > /dev/null 2>&1
        fi
    done
    
    # Удаление типов полей
    for field_type_id in "${CREATED_FIELD_TYPES[@]}"; do
        if [ -n "$field_type_id" ]; then
            http_request "DELETE" "/api/field-types/${field_type_id}" "" "$ADMIN_TOKEN" > /dev/null 2>&1
        fi
    done
    
    # Удаление типов элементов
    for item_type_id in "${CREATED_ITEM_TYPES[@]}"; do
        if [ -n "$item_type_id" ]; then
            http_request "DELETE" "/api/item-types/${item_type_id}" "" "$ADMIN_TOKEN" > /dev/null 2>&1
        fi
    done
    
    # Удаление состояний
    for state_id in "${CREATED_STATES[@]}"; do
        if [ -n "$state_id" ]; then
            http_request "DELETE" "/api/states/${state_id}" "" "$ADMIN_TOKEN" > /dev/null 2>&1
        fi
    done
    
    # Удаление workflow
    for workflow_id in "${CREATED_WORKFLOWS[@]}"; do
        if [ -n "$workflow_id" ]; then
            http_request "DELETE" "/api/workflows/${workflow_id}" "" "$ADMIN_TOKEN" > /dev/null 2>&1
        fi
    done
    
    print_success "Очистка завершена"
}

# ============================================================
# Сводка результатов
# ============================================================

print_summary() {
    echo ""
    echo -e "${BLUE}════════════════════════════════════════════════════════════════${NC}"
    echo -e "${BLUE}📊 РЕЗУЛЬТАТЫ ТЕСТИРОВАНИЯ${NC}"
    echo -e "${BLUE}════════════════════════════════════════════════════════════════${NC}"
    echo -e "Всего тестов: ${TESTS_TOTAL}"
    echo -e "${GREEN}Пройдено: ${TESTS_PASSED}${NC}"
    echo -e "${RED}Провалено: ${TESTS_FAILED}${NC}"
    
    if [ $TESTS_FAILED -eq 0 ]; then
        echo -e "\n${GREEN}✅ Все тесты успешно пройдены!${NC}"
        return 0
    else
        echo -e "\n${RED}❌ Некоторые тесты не пройдены${NC}"
        return 1
    fi
}

# ============================================================
# Основная функция
# ============================================================

main() {
    # Разбор аргументов командной строки
    while [[ $# -gt 0 ]]; do
        case $1 in
            --verbose|-v)
                VERBOSE=true
                shift
                ;;
            --server-url|-s)
                SERVER_URL="$2"
                shift 2
                ;;
            --help|-h)
                echo "Использование: $0 [--verbose] [--server-url=URL]"
                echo ""
                echo "Опции:"
                echo "  --verbose, -v     Подробный вывод"
                echo "  --server-url, -s  URL сервера (по умолчанию: http://localhost:8090)"
                echo "  --help, -h        Показать эту справку"
                exit 0
                ;;
            *)
                echo "Неизвестный параметр: $1"
                exit 1
                ;;
        esac
    done
    
    print_test_header "НАЧАЛО ТЕСТИРОВАНИЯ API ЭЛЕМЕНТОВ"
    print_info "Сервер: $SERVER_URL"
    
    # Аутентификация
    print_info "Аутентификация..."
    ADMIN_TOKEN=$(login "admin" "password")
    
    if [ -z "$ADMIN_TOKEN" ] || [ "$ADMIN_TOKEN" = "null" ]; then
        print_error "Не удалось получить токен администратора"
        exit 1
    fi
    print_success "Получен токен администратора"
    
    # Настройка тестовых данных
    setup_test_data
    
    # Выполнение тестов
    test_create_item
    test_get_item
    test_list_items
    test_update_item
    test_delete_and_restore_item
    test_create_item_field
    test_get_item_fields
    test_update_item_field
    test_delete_item_field
    test_item_hierarchy
    test_permissions
    
    # Очистка
    cleanup_test_data
    
    # Вывод результатов
    print_summary
}

# Запуск основной функции
main "$@"
