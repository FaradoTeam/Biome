#!/bin/bash

# Цвета для вывода
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Базовый URL API
BASE_URL="http://localhost:8090/api/v1"
AUTH_URL="http://localhost:8090/api/v1/auth"

# Переменные для хранения ID созданных объектов
TOKEN=""
PROJECT_ID=""
PHASE_ID=""
WORKFLOW_ID=""
STATE_ID=""
ITEM_TYPE_ID=""
ITEM_ID=""
PLAN_ID_ACTIVE=""
PLAN_ID_DRAFT=""
PLAN_ITEM_ID=""

# Счётчики тестов
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# Функция для логирования
log_success() {
    echo -e "${GREEN}✓ $1${NC}"
    ((PASSED_TESTS++))
    ((TOTAL_TESTS++))
}

log_failure() {
    echo -e "${RED}✗ $1${NC}"
    ((FAILED_TESTS++))
    ((TOTAL_TESTS++))
}

log_info() {
    echo -e "${YELLOW}▶ $1${NC}"
}

log_debug() {
    echo "  $1"
}

# Функция для выполнения запроса и проверки кода ответа
check_response() {
    local response="$1"
    local expected_code="$2"
    local test_name="$3"
    local extract_var="$4"
    local extract_key="$5"
    
    local http_code=$(echo "$response" | tail -n1)
    local body=$(echo "$response" | sed '$d')
    
    if [ "$http_code" == "$expected_code" ]; then
        if [ -n "$extract_var" ] && [ -n "$extract_key" ]; then
            # Извлекаем значение из JSON, используя более надёжный способ
            local value=$(echo "$body" | sed -n "s/.*\"$extract_key\":\([0-9]*\).*/\1/p" | head -1)
            if [ -n "$value" ]; then
                eval "$extract_var=$value"
                log_success "$test_name - Код: $http_code, $extract_key: $value"
                return 0
            else
                log_failure "$test_name - Не удалось извлечь $extract_key из ответа"
                log_debug "Ответ: $body"
                return 1
            fi
        else
            log_success "$test_name - Код: $http_code"
            return 0
        fi
    else
        log_failure "$test_name - Ожидаемый код: $expected_code, Получен: $http_code"
        log_debug "Ответ: $body"
        return 1
    fi
}

# Функция для выполнения запроса и проверки кода ответа (без извлечения данных)
check_response_simple() {
    local response="$1"
    local expected_code="$2"
    local test_name="$3"
    
    local http_code=$(echo "$response" | tail -n1)
    local body=$(echo "$response" | sed '$d')
    
    if [ "$http_code" == "$expected_code" ]; then
        log_success "$test_name - Код: $http_code"
        return 0
    else
        log_failure "$test_name - Ожидаемый код: $expected_code, Получен: $http_code"
        log_debug "Ответ: $body"
        return 1
    fi
}

# Проверяем, запущен ли сервер
echo "=================================================="
echo "Тестирование API планов"
echo "=================================================="
echo ""

log_info "Проверка доступности сервера"
HEALTH_RESPONSE=$(curl -s -o /dev/null -w "%{http_code}" "http://localhost:8090/api/v1/health")
if [ "$HEALTH_RESPONSE" != "200" ]; then
    echo -e "${RED}Ошибка: Сервер не доступен. Запустите biome-vault-server сначала.${NC}"
    exit 1
fi
log_success "Сервер доступен"

# ============================================================
# 1. Логин в систему
# ============================================================
echo ""
log_info "Тест: Логин в систему"
LOGIN_RESPONSE=$(curl -s -w "\n%{http_code}" -X POST "$AUTH_URL/login" \
    -H "Content-Type: application/json" \
    -d '{
        "login": "admin",
        "password": "password"
    }' 2>&1)

HTTP_CODE=$(echo "$LOGIN_RESPONSE" | tail -n1)
BODY=$(echo "$LOGIN_RESPONSE" | sed '$d')

if [ "$HTTP_CODE" != "200" ]; then
    log_failure "Ошибка входа в систему - Код: $HTTP_CODE"
    log_debug "Ответ: $BODY"
    exit 1
fi

# Извлекаем токен
TOKEN=$(echo "$BODY" | sed -n 's/.*"access_token":"\([^"]*\)".*/\1/p')
if [ -z "$TOKEN" ]; then
    log_failure "Не удалось извлечь токен из ответа"
    log_debug "Ответ: $BODY"
    exit 1
fi
log_success "Успешный вход в систему"
log_debug "Токен получен: ${TOKEN:0:30}..."

# ============================================================
# 2. GET /api/v1/phases/10/plans (проверка, что фаза 10 не имеет планов)
# ============================================================
echo ""
log_info "Тест: GET /api/v1/phases/10/plans"
RESPONSE=$(curl -s -w "\n%{http_code}" -X GET "$BASE_URL/phases/10/plans" \
    -H "Authorization: Bearer $TOKEN" 2>&1)

check_response_simple "$RESPONSE" "200" "Получение списка планов"

# Извлекаем totalCount
BODY=$(echo "$RESPONSE" | sed '$d')
TOTAL_COUNT=$(echo "$BODY" | sed -n 's/.*"totalCount":\([0-9]*\).*/\1/p')
if [ "$TOTAL_COUNT" == "0" ]; then
    log_success "Проверка: в фазе 10 нет планов (totalCount=0)"
else
    log_failure "В фазе 10 есть планы (totalCount=$TOTAL_COUNT), хотя ожидалось 0"
fi

# ============================================================
# 3. Создание проекта для тестов
# ============================================================
echo ""
log_info "Тест: POST /api/v1/projects (создание проекта)"
RESPONSE=$(curl -s -w "\n%{http_code}" -X POST "$BASE_URL/projects" \
    -H "Authorization: Bearer $TOKEN" \
    -H "Content-Type: application/json" \
    -d '{
        "caption": "Тестовый проект для планов",
        "description": "Проект для тестирования API планов"
    }' 2>&1)

check_response "$RESPONSE" "201" "Создание проекта" "PROJECT_ID" "id"
if [ -z "$PROJECT_ID" ]; then
    log_failure "Не удалось создать проект"
    exit 1
fi

# ============================================================
# 4. Создание фазы для тестов
# ============================================================
echo ""
log_info "Тест: POST /api/v1/phases (создание фазы)"
RESPONSE=$(curl -s -w "\n%{http_code}" -X POST "$BASE_URL/phases" \
    -H "Authorization: Bearer $TOKEN" \
    -H "Content-Type: application/json" \
    -d "{
        \"caption\": \"Тестовая фаза для планов\",
        \"projectId\": $PROJECT_ID,
        \"description\": \"Фаза для тестирования API планов\"
    }" 2>&1)

check_response "$RESPONSE" "201" "Создание фазы" "PHASE_ID" "id"
if [ -z "$PHASE_ID" ]; then
    log_failure "Не удалось создать фазу"
    exit 1
fi

# ============================================================
# 5. Создание workflow для тестов
# ============================================================
echo ""
log_info "Тест: POST /api/v1/workflows (создание workflow)"
RESPONSE=$(curl -s -w "\n%{http_code}" -X POST "$BASE_URL/workflows" \
    -H "Authorization: Bearer $TOKEN" \
    -H "Content-Type: application/json" \
    -d '{
        "caption": "Тестовый workflow",
        "description": "Workflow для тестирования API планов"
    }' 2>&1)

check_response "$RESPONSE" "201" "Создание workflow" "WORKFLOW_ID" "id"
if [ -z "$WORKFLOW_ID" ]; then
    log_failure "Не удалось создать workflow"
    exit 1
fi

# ============================================================
# 6. Создание состояния для тестов
# ============================================================
echo ""
log_info "Тест: POST /api/v1/states (создание состояния)"
RESPONSE=$(curl -s -w "\n%{http_code}" -X POST "$BASE_URL/states" \
    -H "Authorization: Bearer $TOKEN" \
    -H "Content-Type: application/json" \
    -d "{
        \"workflowId\": $WORKFLOW_ID,
        \"caption\": \"Новое состояние\",
        \"description\": \"Состояние для тестирования\",
        \"orderNumber\": 1,
        \"weight\": 0
    }" 2>&1)

check_response "$RESPONSE" "201" "Создание состояния" "STATE_ID" "id"
if [ -z "$STATE_ID" ]; then
    log_failure "Не удалось создать состояние"
    exit 1
fi

# ============================================================
# 7. Создание типа элемента для тестов
# ============================================================
echo ""
log_info "Тест: POST /api/v1/item-types (создание типа элемента)"
RESPONSE=$(curl -s -w "\n%{http_code}" -X POST "$BASE_URL/item-types" \
    -H "Authorization: Bearer $TOKEN" \
    -H "Content-Type: application/json" \
    -d "{
        \"workflowId\": $WORKFLOW_ID,
        \"defaultStateId\": $STATE_ID,
        \"caption\": \"Тестовый тип элемента\",
        \"kind\": \"issue\",
        \"defaultContent\": \"Содержимое по умолчанию\"
    }" 2>&1)

check_response "$RESPONSE" "201" "Создание типа элемента" "ITEM_TYPE_ID" "id"
if [ -z "$ITEM_TYPE_ID" ]; then
    log_failure "Не удалось создать тип элемента"
    exit 1
fi

# ============================================================
# 8. Создание элемента для тестов
# ============================================================
echo ""
log_info "Тест: POST /api/v1/items (создание элемента)"
RESPONSE=$(curl -s -w "\n%{http_code}" -X POST "$BASE_URL/items" \
    -H "Authorization: Bearer $TOKEN" \
    -H "Content-Type: application/json" \
    -d "{
        \"itemTypeId\": $ITEM_TYPE_ID,
        \"stateId\": $STATE_ID,
        \"phaseId\": $PHASE_ID,
        \"caption\": \"Тестовый элемент для плана\",
        \"content\": \"Содержимое тестового элемента\"
    }" 2>&1)

check_response "$RESPONSE" "201" "Создание элемента" "ITEM_ID" "id"
if [ -z "$ITEM_ID" ]; then
    log_failure "Не удалось создать элемент"
    exit 1
fi

# ============================================================
# 9. POST /api/v1/phases/{phaseId}/plans (создание активного плана)
# ============================================================
echo ""
log_info "Тест: POST /api/v1/phases/$PHASE_ID/plans (создание активного плана)"
RESPONSE=$(curl -s -w "\n%{http_code}" -X POST "$BASE_URL/phases/$PHASE_ID/plans" \
    -H "Authorization: Bearer $TOKEN" \
    -H "Content-Type: application/json" \
    -d '{
        "caption": "Активный план",
        "description": "Первый план в фазе, будет активным"
    }' 2>&1)

check_response "$RESPONSE" "201" "Создание активного плана" "PLAN_ID_ACTIVE" "id"
if [ -z "$PLAN_ID_ACTIVE" ]; then
    log_failure "Не удалось создать активный план"
    exit 1
fi

# ============================================================
# 10. POST /api/v1/phases/{phaseId}/plans (дубликат - должен вернуть 403)
# ============================================================
echo ""
log_info "Тест: POST /api/v1/phases/$PHASE_ID/plans (дубликат)"
RESPONSE=$(curl -s -w "\n%{http_code}" -X POST "$BASE_URL/phases/$PHASE_ID/plans" \
    -H "Authorization: Bearer $TOKEN" \
    -H "Content-Type: application/json" \
    -d '{
        "caption": "Дублирующий план",
        "description": "Попытка создать второй план в фазе"
    }' 2>&1)

check_response_simple "$RESPONSE" "403" "Создание дублирующего плана"

# ============================================================
# 11. GET /api/v1/plans/{id}
# ============================================================
echo ""
log_info "Тест: GET /api/v1/plans/$PLAN_ID_ACTIVE"
RESPONSE=$(curl -s -w "\n%{http_code}" -X GET "$BASE_URL/plans/$PLAN_ID_ACTIVE" \
    -H "Authorization: Bearer $TOKEN" 2>&1)

check_response "$RESPONSE" "200" "Получение плана по ID" "" ""

# Проверяем, что план активен
BODY=$(echo "$RESPONSE" | sed '$d')
IS_ACTIVE=$(echo "$BODY" | sed -n 's/.*"isActive":\([^,}]*\).*/\1/p')
if [ "$IS_ACTIVE" == "true" ]; then
    log_success "План активен (isActive=true)"
else
    log_failure "План не активен (isActive=$IS_ACTIVE), хотя должен быть активным"
fi

# ============================================================
# 12. POST /api/v1/plans/{id}/fork (создание черновика)
# ============================================================
echo ""
log_info "Тест: POST /api/v1/plans/$PLAN_ID_ACTIVE/fork (создание черновика)"
RESPONSE=$(curl -s -w "\n%{http_code}" -X POST "$BASE_URL/plans/$PLAN_ID_ACTIVE/fork" \
    -H "Authorization: Bearer $TOKEN" \
    -H "Content-Type: application/json" \
    -d '{
        "caption": "Черновик плана",
        "description": "Черновик, созданный из активного плана"
    }' 2>&1)

check_response "$RESPONSE" "201" "Создание черновика" "PLAN_ID_DRAFT" "id"
if [ -z "$PLAN_ID_DRAFT" ]; then
    log_failure "Не удалось создать черновик"
    exit 1
fi

# ============================================================
# 13. POST /api/v1/plans/{planId}/items (добавление элемента в черновик)
# ============================================================
echo ""
log_info "Тест: POST /api/v1/plans/$PLAN_ID_DRAFT/items"
# Используем текущее время + 7 дней для дат
START_DATE=$(date +%s)
END_DATE=$((START_DATE + 7*24*60*60))

RESPONSE=$(curl -s -w "\n%{http_code}" -X POST "$BASE_URL/plans/$PLAN_ID_DRAFT/items" \
    -H "Authorization: Bearer $TOKEN" \
    -H "Content-Type: application/json" \
    -d "{
        \"itemId\": $ITEM_ID,
        \"userId\": 1,
        \"startDate\": $START_DATE,
        \"endDate\": $END_DATE
    }" 2>&1)

check_response "$RESPONSE" "201" "Добавление элемента в черновик" "PLAN_ITEM_ID" "id"
if [ -z "$PLAN_ITEM_ID" ]; then
    log_failure "Не удалось добавить элемент в план"
    exit 1
fi

# ============================================================
# 14. GET /api/v1/plans/{planId}/items
# ============================================================
echo ""
log_info "Тест: GET /api/v1/plans/$PLAN_ID_DRAFT/items"
RESPONSE=$(curl -s -w "\n%{http_code}" -X GET "$BASE_URL/plans/$PLAN_ID_DRAFT/items" \
    -H "Authorization: Bearer $TOKEN" 2>&1)

check_response_simple "$RESPONSE" "200" "Получение элементов плана"

# Проверяем, что есть хотя бы один элемент
BODY=$(echo "$RESPONSE" | sed '$d')
TOTAL_COUNT=$(echo "$BODY" | sed -n 's/.*"totalCount":\([0-9]*\).*/\1/p')
if [ "$TOTAL_COUNT" -ge "1" ]; then
    log_success "Проверка: в плане есть элементы (totalCount=$TOTAL_COUNT)"
else
    log_failure "В плане нет элементов (totalCount=$TOTAL_COUNT)"
fi

# ============================================================
# 15. PUT /api/v1/plan-items/{id} (обновление элемента плана)
# ============================================================
echo ""
log_info "Тест: PUT /api/v1/plan-items/$PLAN_ITEM_ID"
# Используем те же даты
RESPONSE=$(curl -s -w "\n%{http_code}" -X PUT "$BASE_URL/plan-items/$PLAN_ITEM_ID" \
    -H "Authorization: Bearer $TOKEN" \
    -H "Content-Type: application/json" \
    -d "{
        \"planId\": $PLAN_ID_DRAFT,
        \"itemId\": $ITEM_ID,
        \"userId\": 1,
        \"startDate\": $START_DATE,
        \"endDate\": $END_DATE
    }" 2>&1)

check_response_simple "$RESPONSE" "200" "Обновление элемента плана"

# ============================================================
# 16. POST /api/v1/plans/{id}/activate (активация черновика)
# ============================================================
echo ""
log_info "Тест: POST /api/v1/plans/$PLAN_ID_DRAFT/activate"
RESPONSE=$(curl -s -w "\n%{http_code}" -X POST "$BASE_URL/plans/$PLAN_ID_DRAFT/activate" \
    -H "Authorization: Bearer $TOKEN" \
    -H "Content-Type: application/json" \
    -d "{
        \"activatedByUserId\": 1
    }" 2>&1)

check_response_simple "$RESPONSE" "200" "Активация черновика"

# ============================================================
# 17. POST /api/v1/plans/{planId}/items (активный план) - должен вернуть 403
# ============================================================
echo ""
log_info "Тест: POST /api/v1/plans/$PLAN_ID_DRAFT/items (активный план)"
RESPONSE=$(curl -s -w "\n%{http_code}" -X POST "$BASE_URL/plans/$PLAN_ID_DRAFT/items" \
    -H "Authorization: Bearer $TOKEN" \
    -H "Content-Type: application/json" \
    -d "{
        \"itemId\": $ITEM_ID,
        \"userId\": 1,
        \"startDate\": $START_DATE,
        \"endDate\": $END_DATE
    }" 2>&1)

check_response_simple "$RESPONSE" "403" "Добавление элемента в активный план"

# ============================================================
# 18. POST /api/v1/plans/{id}/fork (создание нового черновика)
# ============================================================
echo ""
log_info "Тест: POST /api/v1/plans/$PLAN_ID_DRAFT/fork (создание нового черновика)"
RESPONSE=$(curl -s -w "\n%{http_code}" -X POST "$BASE_URL/plans/$PLAN_ID_DRAFT/fork" \
    -H "Authorization: Bearer $TOKEN" \
    -H "Content-Type: application/json" \
    -d '{
        "caption": "Новый черновик",
        "description": "Черновик, созданный из активного плана"
    }' 2>&1)

check_response "$RESPONSE" "201" "Создание нового черновика" "PLAN_ID_DRAFT" "id"
if [ -z "$PLAN_ID_DRAFT" ]; then
    log_failure "Не удалось создать новый черновик"
    exit 1
fi

# ============================================================
# 19. POST /api/v1/plans/{planId}/items - должен вернуть 403, так как элемент уже в плане
# ============================================================
echo ""
log_info "Тест: POST /api/v1/plans/$PLAN_ID_DRAFT/items (элемент уже в плане)"
RESPONSE=$(curl -s -w "\n%{http_code}" -X POST "$BASE_URL/plans/$PLAN_ID_DRAFT/items" \
    -H "Authorization: Bearer $TOKEN" \
    -H "Content-Type: application/json" \
    -d "{
        \"itemId\": $ITEM_ID,
        \"userId\": 1,
        \"startDate\": $START_DATE,
        \"endDate\": $END_DATE
    }" 2>&1)

# Ожидаем 403, так как элемент уже добавлен в план при создании форка
check_response_simple "$RESPONSE" "403" "Добавление элемента в новый черновик (дубликат)"

# ============================================================
# 20. GET /api/v1/plans/{planId}/items (проверка, что элемент скопировался)
# ============================================================
echo ""
log_info "Тест: GET /api/v1/plans/$PLAN_ID_DRAFT/items"
RESPONSE=$(curl -s -w "\n%{http_code}" -X GET "$BASE_URL/plans/$PLAN_ID_DRAFT/items" \
    -H "Authorization: Bearer $TOKEN" 2>&1)

check_response_simple "$RESPONSE" "200" "Получение элементов нового черновика"

# Проверяем, что есть хотя бы один элемент
BODY=$(echo "$RESPONSE" | sed '$d')
TOTAL_COUNT=$(echo "$BODY" | sed -n 's/.*"totalCount":\([0-9]*\).*/\1/p')
if [ "$TOTAL_COUNT" -ge "1" ]; then
    log_success "Проверка: в новом черновике есть элементы (totalCount=$TOTAL_COUNT)"
else
    log_failure "В новом черновике нет элементов (totalCount=$TOTAL_COUNT)"
fi

# Извлекаем ID элемента плана для удаления
PLAN_ITEM_ID_FOR_DELETE=$(echo "$BODY" | sed -n 's/.*"id":\([0-9]*\).*/\1/p' | head -1)
if [ -z "$PLAN_ITEM_ID_FOR_DELETE" ]; then
    log_failure "Не удалось извлечь ID элемента плана для удаления"
else
    log_debug "ID элемента плана для удаления: $PLAN_ITEM_ID_FOR_DELETE"
fi

# ============================================================
# 21. DELETE /api/v1/plan-items/{id} (удаление элемента из черновика)
# ============================================================
echo ""
log_info "Тест: DELETE /api/v1/plan-items/$PLAN_ITEM_ID_FOR_DELETE"
RESPONSE=$(curl -s -w "\n%{http_code}" -X DELETE "$BASE_URL/plan-items/$PLAN_ITEM_ID_FOR_DELETE" \
    -H "Authorization: Bearer $TOKEN" 2>&1)

check_response_simple "$RESPONSE" "204" "Удаление элемента из черновика"

# ============================================================
# 22. GET /api/v1/plans/{planId}/items (после удаления)
# ============================================================
echo ""
log_info "Тест: GET /api/v1/plans/$PLAN_ID_DRAFT/items (после удаления)"
RESPONSE=$(curl -s -w "\n%{http_code}" -X GET "$BASE_URL/plans/$PLAN_ID_DRAFT/items" \
    -H "Authorization: Bearer $TOKEN" 2>&1)

check_response_simple "$RESPONSE" "200" "Получение элементов черновика после удаления"

# Проверяем, что элементов больше нет
BODY=$(echo "$RESPONSE" | sed '$d')
TOTAL_COUNT=$(echo "$BODY" | sed -n 's/.*"totalCount":\([0-9]*\).*/\1/p')
if [ "$TOTAL_COUNT" == "0" ]; then
    log_success "Проверка: элементы удалены (totalCount=0)"
else
    log_failure "Элементы не удалены (totalCount=$TOTAL_COUNT)"
fi

# ============================================================
# 23. DELETE /api/v1/plans/{id}
# ============================================================
echo ""
log_info "Тест: DELETE /api/v1/plans/$PLAN_ID_DRAFT"
RESPONSE=$(curl -s -w "\n%{http_code}" -X DELETE "$BASE_URL/plans/$PLAN_ID_DRAFT" \
    -H "Authorization: Bearer $TOKEN" 2>&1)

check_response_simple "$RESPONSE" "204" "Удаление неактивного плана"

# ============================================================
# 24. GET /api/v1/phases/{phaseId}/plans (без токена)
# ============================================================
echo ""
log_info "Тест: GET /api/v1/phases/$PHASE_ID/plans (без токена)"
RESPONSE=$(curl -s -w "\n%{http_code}" -X GET "$BASE_URL/phases/$PHASE_ID/plans" 2>&1)

check_response_simple "$RESPONSE" "401" "Доступ без токена"

# ============================================================
# 25. DELETE /api/v1/projects/{id} (очистка)
# ============================================================
echo ""
log_info "Тест: DELETE /api/v1/projects/$PROJECT_ID"
RESPONSE=$(curl -s -w "\n%{http_code}" -X DELETE "$BASE_URL/projects/$PROJECT_ID" \
    -H "Authorization: Bearer $TOKEN" 2>&1)

check_response_simple "$RESPONSE" "204" "Удаление тестового проекта"

# ============================================================
# Итоги
# ============================================================
echo ""
echo "=================================================="
echo "Итоги тестирования"
echo "=================================================="
echo "Всего тестов: $TOTAL_TESTS"
echo -e "${GREEN}Пройдено: $PASSED_TESTS${NC}"
echo -e "${RED}Провалено: $FAILED_TESTS${NC}"

if [ $FAILED_TESTS -eq 0 ]; then
    echo -e "${GREEN}✅ Все тесты пройдены успешно!${NC}"
    exit 0
else
    echo -e "${RED}❌ Некоторые тесты провалены.${NC}"
    exit 1
fi
