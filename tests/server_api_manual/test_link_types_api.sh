#!/bin/bash

# ============================================================
# Тестирование API типов связей (Link Types)
# ============================================================

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

BASE_URL="http://localhost:8090"
AUTH_TOKEN=""
TESTS_PASSED=0
TESTS_FAILED=0

test_pass() {
    echo -e "${GREEN}✓ $1${NC}"
    ((TESTS_PASSED++))
}

test_fail() {
    echo -e "${RED}✗ $1${NC}"
    echo -e "${RED}  $2${NC}"
    ((TESTS_FAILED++))
}

get_status() {
    local method="$1"
    local endpoint="$2"
    local data="$3"
    local token="$4"
    
    local url="${BASE_URL}${endpoint}"
    local auth_header=""
    
    if [ -n "$token" ]; then
        auth_header="-H \"Authorization: Bearer $token\""
    fi
    
    local curl_cmd="curl -s -o /dev/null -w '%{http_code}' -X $method \"$url\" -H \"Content-Type: application/json\""
    
    if [ -n "$auth_header" ]; then
        curl_cmd="$curl_cmd $auth_header"
    fi
    
    if [ -n "$data" ] && [ "$method" != "GET" ] && [ "$method" != "DELETE" ]; then
        curl_cmd="$curl_cmd -d '$data'"
    fi
    
    eval "$curl_cmd" 2>/dev/null
}

get_body() {
    local method="$1"
    local endpoint="$2"
    local data="$3"
    local token="$4"
    
    local url="${BASE_URL}${endpoint}"
    local auth_header=""
    
    if [ -n "$token" ]; then
        auth_header="-H \"Authorization: Bearer $token\""
    fi
    
    local curl_cmd="curl -s -X $method \"$url\" -H \"Content-Type: application/json\""
    
    if [ -n "$auth_header" ]; then
        curl_cmd="$curl_cmd $auth_header"
    fi
    
    if [ -n "$data" ] && [ "$method" != "GET" ] && [ "$method" != "DELETE" ]; then
        curl_cmd="$curl_cmd -d '$data'"
    fi
    
    eval "$curl_cmd" 2>/dev/null
}

get_auth_token() {
    local data='{"login":"admin","password":"password"}'
    local body
    body=$(get_body "POST" "/api/v1/auth/login" "$data" "")
    echo "$body" | grep -o '"access_token":"[^"]*"' | head -1 | cut -d'"' -f4
}

# ============================================================
# Начало тестов
# ============================================================

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}Тестирование API типов связей (Link Types)${NC}"
echo -e "${BLUE}========================================${NC}\n"

# 1. Аутентификация
echo -e "${YELLOW}1. Аутентификация${NC}"
AUTH_TOKEN=$(get_auth_token)
if [ -n "$AUTH_TOKEN" ]; then
    test_pass "Получение токена"
else
    test_fail "Получение токена" "Токен не получен"
    exit 1
fi
echo ""

# 2. Подготовка данных
echo -e "${YELLOW}2. Подготовка данных${NC}"

# Получаем workflowId
WORKFLOWS=$(get_body "GET" "/api/v1/workflows" "" "$AUTH_TOKEN")
WORKFLOW_ID=$(echo "$WORKFLOWS" | grep -o '"id":[0-9]*' | head -1 | cut -d':' -f2)
if [ -z "$WORKFLOW_ID" ]; then
    WORKFLOW_ID=1
fi
echo -e "${BLUE}Workflow ID: $WORKFLOW_ID${NC}"

# Получаем stateId
STATES=$(get_body "GET" "/api/v1/states?workflowId=$WORKFLOW_ID" "" "$AUTH_TOKEN")
STATE_ID=$(echo "$STATES" | grep -o '"id":[0-9]*' | head -1 | cut -d':' -f2)
if [ -z "$STATE_ID" ]; then
    STATE_ID=1
fi
echo -e "${BLUE}State ID: $STATE_ID${NC}"

# Создаём тестовый тип элемента
CREATE_TYPE_DATA="{\"caption\":\"Тестовый тип\",\"kind\":\"issue\",\"workflowId\":$WORKFLOW_ID,\"defaultStateId\":$STATE_ID}"
CREATE_TYPE_STATUS=$(get_status "POST" "/api/v1/item-types" "$CREATE_TYPE_DATA" "$AUTH_TOKEN")
CREATE_TYPE_BODY=$(get_body "POST" "/api/v1/item-types" "$CREATE_TYPE_DATA" "$AUTH_TOKEN")

if [ "$CREATE_TYPE_STATUS" = "201" ]; then
    ITEM_TYPE_ID=$(echo "$CREATE_TYPE_BODY" | grep -o '"id":[0-9]*' | head -1 | cut -d':' -f2)
    test_pass "Создание типа элемента (ID: $ITEM_TYPE_ID)"
else
    test_fail "Создание типа элемента" "HTTP $CREATE_TYPE_STATUS"
    ITEM_TYPE_ID=1
fi
echo ""

# ============================================================
# Тесты CRUD
# ============================================================

echo -e "${YELLOW}3. CRUD операции${NC}\n"

# 3.1 Создание
echo -e "${BLUE}3.1 Создание типа связи${NC}"
CREATE_DATA="{\"caption\":\"связан с\",\"sourceItemTypeId\":$ITEM_TYPE_ID,\"destinationItemTypeId\":$ITEM_TYPE_ID,\"isBidirectional\":false}"
CREATE_STATUS=$(get_status "POST" "/api/v1/link-types" "$CREATE_DATA" "$AUTH_TOKEN")
CREATE_BODY=$(get_body "POST" "/api/v1/link-types" "$CREATE_DATA" "$AUTH_TOKEN")

if [ "$CREATE_STATUS" = "201" ]; then
    LINK_TYPE_ID=$(echo "$CREATE_BODY" | grep -o '"id":[0-9]*' | head -1 | cut -d':' -f2)
    test_pass "Создание типа связи (ID: $LINK_TYPE_ID)"
else
    test_fail "Создание типа связи" "HTTP $CREATE_STATUS"
fi
echo ""

# 3.2 Получение по ID
if [ -n "$LINK_TYPE_ID" ]; then
    echo -e "${BLUE}3.2 Получение по ID${NC}"
    GET_STATUS=$(get_status "GET" "/api/v1/link-types/$LINK_TYPE_ID" "" "$AUTH_TOKEN")
    GET_BODY=$(get_body "GET" "/api/v1/link-types/$LINK_TYPE_ID" "" "$AUTH_TOKEN")
    
    if [ "$GET_STATUS" = "200" ]; then
        CAPTION=$(echo "$GET_BODY" | grep -o '"caption":"[^"]*"' | head -1 | cut -d'"' -f4)
        test_pass "Получение по ID (caption: $CAPTION)"
    else
        test_fail "Получение по ID" "HTTP $GET_STATUS"
    fi
    echo ""
fi

# 3.3 Получение несуществующего
echo -e "${BLUE}3.3 Получение несуществующего${NC}"
GET_STATUS=$(get_status "GET" "/api/v1/link-types/99999" "" "$AUTH_TOKEN")
if [ "$GET_STATUS" = "404" ]; then
    test_pass "404 для несуществующего"
else
    test_fail "404 для несуществующего" "HTTP $GET_STATUS"
fi
echo ""

# 3.4 Обновление
if [ -n "$LINK_TYPE_ID" ]; then
    echo -e "${BLUE}3.4 Обновление типа связи${NC}"
    UPDATE_DATA='{"caption":"обновлённая связь"}'
    UPDATE_STATUS=$(get_status "PUT" "/api/v1/link-types/$LINK_TYPE_ID" "$UPDATE_DATA" "$AUTH_TOKEN")
    
    if [ "$UPDATE_STATUS" = "200" ]; then
        test_pass "Обновление типа связи"
    else
        test_fail "Обновление типа связи" "HTTP $UPDATE_STATUS"
    fi
    echo ""
fi

# 3.5 Получение списка
echo -e "${BLUE}3.5 Получение списка${NC}"
LIST_STATUS=$(get_status "GET" "/api/v1/link-types" "" "$AUTH_TOKEN")
LIST_BODY=$(get_body "GET" "/api/v1/link-types" "" "$AUTH_TOKEN")

if [ "$LIST_STATUS" = "200" ]; then
    TOTAL_COUNT=$(echo "$LIST_BODY" | grep -o '"totalCount":[0-9]*' | head -1 | cut -d':' -f2)
    test_pass "Получение списка (totalCount: ${TOTAL_COUNT:-0})"
else
    test_fail "Получение списка" "HTTP $LIST_STATUS"
fi
echo ""

# 3.6 Удаление
if [ -n "$LINK_TYPE_ID" ]; then
    echo -e "${BLUE}3.6 Удаление типа связи${NC}"
    DELETE_STATUS=$(get_status "DELETE" "/api/v1/link-types/$LINK_TYPE_ID" "" "$AUTH_TOKEN")
    if [ "$DELETE_STATUS" = "204" ]; then
        test_pass "Удаление типа связи"
    else
        test_fail "Удаление типа связи" "HTTP $DELETE_STATUS"
    fi
    echo ""
fi

# 3.7 Проверка удаления
if [ -n "$LINK_TYPE_ID" ]; then
    echo -e "${BLUE}3.7 Проверка удаления${NC}"
    GET_STATUS=$(get_status "GET" "/api/v1/link-types/$LINK_TYPE_ID" "" "$AUTH_TOKEN")
    if [ "$GET_STATUS" = "404" ]; then
        test_pass "Тип связи удалён"
    else
        test_fail "Тип связи удалён" "HTTP $GET_STATUS"
    fi
    echo ""
fi

# ============================================================
# Тесты безопасности
# ============================================================

echo -e "${YELLOW}4. Тесты безопасности${NC}\n"

echo -e "${BLUE}4.1 Доступ без токена${NC}"
NO_TOKEN_STATUS=$(get_status "GET" "/api/v1/link-types" "" "")
if [ "$NO_TOKEN_STATUS" = "401" ]; then
    test_pass "Без токена - 401"
else
    test_fail "Без токена - 401" "HTTP $NO_TOKEN_STATUS"
fi
echo ""

echo -e "${BLUE}4.2 Невалидный токен${NC}"
INVALID_STATUS=$(get_status "GET" "/api/v1/link-types" "" "invalid_token")
if [ "$INVALID_STATUS" = "401" ]; then
    test_pass "Невалидный токен - 401"
else
    test_fail "Невалидный токен - 401" "HTTP $INVALID_STATUS"
fi
echo ""

# ============================================================
# Итоги
# ============================================================

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}Итоги${NC}"
echo -e "${BLUE}========================================${NC}"
echo -e "${GREEN}Пройдено: $TESTS_PASSED${NC}"
echo -e "${RED}Провалено: $TESTS_FAILED${NC}"

if [ $TESTS_FAILED -eq 0 ]; then
    echo -e "\n${GREEN}✓ Все тесты типов связей API пройдены успешно!${NC}"
    exit 0
else
    echo -e "\n${RED}✗ Есть ошибки.${NC}"
    exit 1
fi
