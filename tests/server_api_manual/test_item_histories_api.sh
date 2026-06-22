#!/bin/bash

# Цвета для вывода
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

BASE_URL="http://127.0.0.1:8090"

print_header() { echo -e "${BLUE}[INFO]${NC} $1" >&2; }
print_success() { echo -e "${GREEN}[OK]${NC} $1" >&2; }
print_error() { echo -e "${RED}[FAIL]${NC} $1" >&2; }

# Функции для HTTP запросов
do_post() {
    curl -s -X POST "${BASE_URL}$1" \
        -H "Content-Type: application/json" \
        -H "Authorization: Bearer $2" \
        -d "$3"
}

do_get() {
    curl -s -X GET "${BASE_URL}$1" \
        -H "Authorization: Bearer $2"
}

do_delete() {
    curl -s -X DELETE "${BASE_URL}$1" \
        -H "Authorization: Bearer $2"
}

do_put() {
    curl -s -X PUT "${BASE_URL}$1" \
        -H "Content-Type: application/json" \
        -H "Authorization: Bearer $2" \
        -d "$3"
}

# Аутентификация
authenticate() {
    print_header "Аутентификация пользователя admin..."
    
    local response=$(curl -s -X POST "${BASE_URL}/api/v1/auth/login" \
        -H "Content-Type: application/json" \
        -d '{"login":"admin","password":"password"}')
    
    local token=$(echo "$response" | jq -r '.access_token // empty')
    
    if [ -z "$token" ] || [ "$token" = "null" ]; then
        print_error "Аутентификация не удалась: $response"
        return 1
    fi
    
    print_success "Аутентификация успешна"
    echo "$token"
    return 0
}

# Проверка успешности ответа
is_success() {
    local response="$1"
    if [ -z "$response" ]; then
        return 1
    fi
    if echo "$response" | jq -e '.error' > /dev/null 2>&1; then
        return 1
    fi
    return 0
}

# Создание рабочего процесса
create_workflow() {
    local token="$1"
    local caption="$2"
    
    print_header "Создание рабочего процесса '$caption'..."
    local response=$(do_post "/api/v1/workflows" "$token" "{\"caption\": \"$caption\", \"description\": \"Created by test\"}")
    
    if is_success "$response"; then
        local id=$(echo "$response" | jq -r '.id')
        print_success "Рабочий процесс создан с ID: $id"
        echo "$id"
        return 0
    else
        print_error "Не удалось создать рабочий процесс: $response"
        return 1
    fi
}

# Создание состояния
create_state() {
    local token="$1"
    local workflow_id="$2"
    local caption="$3"
    local order="$4"
    
    local response=$(do_post "/api/v1/states" "$token" "{\"workflowId\": $workflow_id, \"caption\": \"$caption\", \"orderNumber\": $order}")
    
    if is_success "$response"; then
        local id=$(echo "$response" | jq -r '.id')
        echo "$id"
        return 0
    else
        echo ""
        return 1
    fi
}

# Создание типа элемента
create_item_type() {
    local token="$1"
    local workflow_id="$2"
    local default_state_id="$3"
    local caption="$4"
    local kind="$5"
    
    local response=$(do_post "/api/v1/item-types" "$token" "{\"workflowId\": $workflow_id, \"defaultStateId\": $default_state_id, \"caption\": \"$caption\", \"kind\": \"$kind\"}")
    
    if is_success "$response"; then
        local id=$(echo "$response" | jq -r '.id')
        echo "$id"
        return 0
    else
        echo ""
        return 1
    fi
}

# Создание проекта
create_project() {
    local token="$1"
    local caption="$2"
    
    local response=$(do_post "/api/v1/projects" "$token" "{\"caption\": \"$caption\"}")
    
    if is_success "$response"; then
        local id=$(echo "$response" | jq -r '.id')
        echo "$id"
        return 0
    else
        echo ""
        return 1
    fi
}

# Создание фазы
create_phase() {
    local token="$1"
    local project_id="$2"
    local caption="$3"
    
    local response=$(do_post "/api/v1/phases" "$token" "{\"caption\": \"$caption\", \"projectId\": $project_id}")
    
    if is_success "$response"; then
        local id=$(echo "$response" | jq -r '.id')
        echo "$id"
        return 0
    else
        echo ""
        return 1
    fi
}

# Создание элемента
create_item() {
    local token="$1"
    local item_type_id="$2"
    local state_id="$3"
    local phase_id="$4"
    local caption="$5"
    
    local response=$(do_post "/api/v1/items" "$token" "{\"caption\": \"$caption\", \"itemTypeId\": $item_type_id, \"stateId\": $state_id, \"phaseId\": $phase_id}")
    
    if is_success "$response"; then
        local id=$(echo "$response" | jq -r '.id')
        echo "$id"
        return 0
    else
        echo ""
        return 1
    fi
}

main() {
    echo -e "${BLUE}==========================================${NC}" >&2
    echo -e "${BLUE}Тестирование API истории изменений элементов${NC}" >&2
    echo -e "${BLUE}==========================================${NC}" >&2
    
    TOKEN=$(authenticate)
    [ $? -ne 0 ] && exit 1
    echo "" >&2
    
    # Создаём рабочий процесс
    WORKFLOW_ID=$(create_workflow "$TOKEN" "Test Workflow for ItemHistory")
    [ -z "$WORKFLOW_ID" ] && exit 1
    
    # Создаём состояния
    print_header "Создание состояний..."
    STATE1_ID=$(create_state "$TOKEN" "$WORKFLOW_ID" "Новая" 1)
    STATE2_ID=$(create_state "$TOKEN" "$WORKFLOW_ID" "В работе" 2)
    STATE3_ID=$(create_state "$TOKEN" "$WORKFLOW_ID" "Завершена" 3)
    
    if [ -z "$STATE1_ID" ] || [ -z "$STATE2_ID" ]; then
        print_error "Не удалось создать состояния"
        exit 1
    fi
    print_success "Состояния созданы: $STATE1_ID, $STATE2_ID, $STATE3_ID"
    
    # Создаём тип элемента
    ITEM_TYPE_ID=$(create_item_type "$TOKEN" "$WORKFLOW_ID" "$STATE1_ID" "Тестовая задача" "issue")
    if [ -z "$ITEM_TYPE_ID" ]; then
        print_error "Не удалось создать тип элемента"
        exit 1
    fi
    print_success "Тип элемента создан с ID: $ITEM_TYPE_ID"
    
    # Создаём проект
    PROJECT_ID=$(create_project "$TOKEN" "Test Project for ItemHistory")
    if [ -z "$PROJECT_ID" ]; then
        print_error "Не удалось создать проект"
        exit 1
    fi
    print_success "Проект создан с ID: $PROJECT_ID"
    
    # Создаём фазу
    PHASE_ID=$(create_phase "$TOKEN" "$PROJECT_ID" "Спринт 1")
    if [ -z "$PHASE_ID" ]; then
        print_error "Не удалось создать фазу"
        exit 1
    fi
    print_success "Фаза создана с ID: $PHASE_ID"
    
    # Создаём элемент
    ITEM_ID=$(create_item "$TOKEN" "$ITEM_TYPE_ID" "$STATE1_ID" "$PHASE_ID" "Тестовый элемент")
    if [ -z "$ITEM_ID" ]; then
        print_error "Не удалось создать элемент"
        exit 1
    fi
    print_success "Элемент создан с ID: $ITEM_ID"
    
    # ============================================================
    # Тест 1: Создание записи истории
    # ============================================================
    echo "" >&2
    print_header "Тест 1: Создание записи истории изменений"
    
    HISTORY_DATA='{"diff": "{\"caption\": \"Изменён заголовок\"}"}'
    RESPONSE=$(do_post "/api/v1/items/${ITEM_ID}/histories" "$TOKEN" "$HISTORY_DATA")
    
    if is_success "$RESPONSE"; then
        HISTORY_ID=$(echo "$RESPONSE" | jq -r '.id')
        if [ -n "$HISTORY_ID" ] && [ "$HISTORY_ID" != "null" ]; then
            print_success "Запись истории создана с ID: $HISTORY_ID"
        else
            print_error "Не удалось получить ID истории: $RESPONSE"
            HISTORY_ID=""
        fi
    else
        print_error "Не удалось создать запись истории: $RESPONSE"
        HISTORY_ID=""
    fi
    
    # Тест 2: Получение записи истории
    if [ -n "$HISTORY_ID" ]; then
        echo "" >&2
        print_header "Тест 2: Получение записи истории по ID"
        
        RESPONSE=$(do_get "/api/v1/items/histories/${HISTORY_ID}" "$TOKEN")
        FOUND_ID=$(echo "$RESPONSE" | jq -r '.id // empty')
        
        if [ "$FOUND_ID" = "$HISTORY_ID" ]; then
            print_success "Запись истории получена"
            DIFF=$(echo "$RESPONSE" | jq -r '.diff // empty')
            print_header "Содержимое diff: $DIFF"
        else
            print_error "Не удалось получить запись истории: $RESPONSE"
        fi
        
        # Тест 3: Получение списка записей
        echo "" >&2
        print_header "Тест 3: Получение списка записей истории"
        
        RESPONSE=$(do_get "/api/v1/items/histories?itemId=${ITEM_ID}" "$TOKEN")
        TOTAL_COUNT=$(echo "$RESPONSE" | jq -r '.totalCount // 0')
        
        if [ "$TOTAL_COUNT" -gt 0 ]; then
            print_success "Список записей получен, всего: $TOTAL_COUNT"
        else
            print_error "Не удалось получить список записей: $RESPONSE"
        fi
        
        # Тест 4: Удаление записи истории
        echo "" >&2
        print_header "Тест 4: Удаление записи истории"
        
        do_delete "/api/v1/items/histories/${HISTORY_ID}" "$TOKEN"
        
        RESPONSE=$(do_get "/api/v1/items/histories/${HISTORY_ID}" "$TOKEN")
        CHECK_ID=$(echo "$RESPONSE" | jq -r '.id // empty')
        
        if [ -z "$CHECK_ID" ]; then
            print_success "Запись истории успешно удалена"
        else
            print_error "Не удалось удалить запись истории"
        fi
    fi
    
    # ============================================================
    # Очистка (в обратном порядке)
    # ============================================================
    echo "" >&2
    print_header "Очистка тестовых данных"
    
    # Удаляем элемент
    if [ -n "$ITEM_ID" ]; then
        do_delete "/api/v1/items/${ITEM_ID}" "$TOKEN"
        print_success "Удалён элемент ID: $ITEM_ID"
    fi
    
    # Удаляем фазу
    if [ -n "$PHASE_ID" ]; then
        do_delete "/api/v1/phases/${PHASE_ID}" "$TOKEN"
        print_success "Удалена фаза ID: $PHASE_ID"
    fi
    
    # Удаляем проект
    if [ -n "$PROJECT_ID" ]; then
        do_delete "/api/v1/projects/${PROJECT_ID}" "$TOKEN"
        print_success "Удалён проект ID: $PROJECT_ID"
    fi
    
    # Удаляем тип элемента
    if [ -n "$ITEM_TYPE_ID" ]; then
        do_delete "/api/v1/item-types/${ITEM_TYPE_ID}" "$TOKEN"
        print_success "Удалён тип элемента ID: $ITEM_TYPE_ID"
    fi
    
    # Удаляем состояния
    if [ -n "$STATE1_ID" ]; then
        do_delete "/api/v1/states/${STATE1_ID}" "$TOKEN"
        print_success "Удалено состояние ID: $STATE1_ID"
    fi
    if [ -n "$STATE2_ID" ]; then
        do_delete "/api/v1/states/${STATE2_ID}" "$TOKEN"
        print_success "Удалено состояние ID: $STATE2_ID"
    fi
    if [ -n "$STATE3_ID" ]; then
        do_delete "/api/v1/states/${STATE3_ID}" "$TOKEN"
        print_success "Удалено состояние ID: $STATE3_ID"
    fi
    
    # Удаляем рабочий процесс
    if [ -n "$WORKFLOW_ID" ]; then
        do_delete "/api/v1/workflows/${WORKFLOW_ID}" "$TOKEN"
        print_success "Удалён рабочий процесс ID: $WORKFLOW_ID"
    fi
    
    echo "" >&2
    print_success "Все тесты завершены!"
}

main 2>&1
