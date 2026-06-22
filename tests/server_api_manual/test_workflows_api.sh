#!/bin/bash

# Конфигурация
API_HOST="http://127.0.0.1:8090"
CONTENT_TYPE="Content-Type: application/json"

# Цвета для вывода
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Счетчики тестов
PASSED=0
FAILED=0
TOTAL=0

print_header() {
    echo -e "\n${BLUE}============================================================${NC}"
    echo -e "${BLUE}$1${NC}"
    echo -e "${BLUE}============================================================${NC}"
}

print_test() {
    TOTAL=$((TOTAL + 1))
    echo -ne "${YELLOW}[TEST $TOTAL]${NC} $1... "
}

print_pass() {
    echo -e "${GREEN}PASSED${NC}"
    PASSED=$((PASSED + 1))
}

print_fail() {
    echo -e "${RED}FAILED${NC}"
    echo -e "${RED}  Expected: $1${NC}"
    echo -e "${RED}  Got: $2${NC}"
    FAILED=$((FAILED + 1))
}

get_token() {
    local response=$(curl -s -X POST "${API_HOST}/api/v1/auth/login" \
        -H "${CONTENT_TYPE}" \
        -d '{"login":"admin","password":"password"}')
    
    echo "$response" | jq -r '.access_token // empty'
}

# Проверка утилит
if ! command -v curl &> /dev/null; then
    echo -e "${RED}curl не установлен. Установите curl для запуска тестов.${NC}"
    exit 1
fi

if ! command -v jq &> /dev/null; then
    echo -e "${RED}jq не установлен. Установите jq для запуска тестов.${NC}"
    exit 1
fi

echo -e "${YELLOW}Получение токена авторизации...${NC}"
TOKEN=$(get_token)

if [ -z "$TOKEN" ]; then
    echo -e "${RED}Не удалось получить токен. Проверьте, что сервер запущен на ${API_HOST}${NC}"
    exit 1
fi

echo -e "${GREEN}Токен получен успешно${NC}"
AUTH_HEADER="Authorization: Bearer ${TOKEN}"

# ============================================================
# Workflows
# ============================================================
print_header "ТЕСТЫ РАБОЧИХ ПРОЦЕССОВ"

print_test "Создание рабочего процесса"
CREATE_WF_RESPONSE=$(curl -s -X POST "${API_HOST}/api/v1/workflows" \
    -H "${CONTENT_TYPE}" \
    -H "${AUTH_HEADER}" \
    -d '{"caption":"Scrum Process","description":"Scrum workflow for testing"}')

WF_ID=$(echo "$CREATE_WF_RESPONSE" | jq -r '.id // empty')
if [ -n "$WF_ID" ] && [ "$WF_ID" -gt 0 ] 2>/dev/null; then
    print_pass
else
    print_fail "Valid ID" "$WF_ID"
fi

print_test "Получение списка рабочих процессов"
WF_LIST=$(curl -s -X GET "${API_HOST}/api/v1/workflows" \
    -H "${AUTH_HEADER}")

TOTAL_COUNT=$(echo "$WF_LIST" | jq -r '.totalCount // 0')
if [ "$TOTAL_COUNT" -gt 0 ]; then
    print_pass
else
    print_fail "totalCount > 0" "$TOTAL_COUNT"
fi

print_test "Получение рабочего процесса по ID"
WF_BY_ID=$(curl -s -X GET "${API_HOST}/api/v1/workflows/${WF_ID}" \
    -H "${AUTH_HEADER}")

WF_CAPTION=$(echo "$WF_BY_ID" | jq -r '.caption // empty')
if [ "$WF_CAPTION" == "Scrum Process" ]; then
    print_pass
else
    print_fail "Scrum Process" "$WF_CAPTION"
fi

print_test "Обновление рабочего процесса"
UPDATE_WF_RESPONSE=$(curl -s -X PUT "${API_HOST}/api/v1/workflows/${WF_ID}" \
    -H "${CONTENT_TYPE}" \
    -H "${AUTH_HEADER}" \
    -d '{"caption":"Updated Scrum Process","description":"Updated description"}')

UPDATED_CAPTION=$(echo "$UPDATE_WF_RESPONSE" | jq -r '.caption // empty')
if [ "$UPDATED_CAPTION" == "Updated Scrum Process" ]; then
    print_pass
else
    print_fail "Updated Scrum Process" "$UPDATED_CAPTION"
fi

print_test "Создание дубликата рабочего процесса"
DUP_WF_RESPONSE=$(curl -s -X POST "${API_HOST}/api/v1/workflows" \
    -H "${CONTENT_TYPE}" \
    -H "${AUTH_HEADER}" \
    -d '{"caption":"Updated Scrum Process"}')

DUP_STATUS=$(echo "$DUP_WF_RESPONSE" | jq -r '.code // 200')
if [ "$DUP_STATUS" == "409" ] || [ "$DUP_STATUS" == "400" ]; then
    print_pass
else
    print_fail "409 or 400" "$DUP_STATUS"
fi

# ============================================================
# States
# ============================================================
print_header "ТЕСТЫ СОСТОЯНИЙ"

print_test "Создание состояния 'To Do'"
CREATE_STATE1_RESPONSE=$(curl -s -X POST "${API_HOST}/api/v1/states" \
    -H "${CONTENT_TYPE}" \
    -H "${AUTH_HEADER}" \
    -d "{\"workflowId\":${WF_ID},\"caption\":\"To Do\",\"orderNumber\":1,\"weight\":0}")

TODO_STATE_ID=$(echo "$CREATE_STATE1_RESPONSE" | jq -r '.id // empty')
if [ -n "$TODO_STATE_ID" ] && [ "$TODO_STATE_ID" -gt 0 ] 2>/dev/null; then
    print_pass
else
    print_fail "Valid ID" "$TODO_STATE_ID"
fi

print_test "Создание состояния 'In Progress'"
CREATE_STATE2_RESPONSE=$(curl -s -X POST "${API_HOST}/api/v1/states" \
    -H "${CONTENT_TYPE}" \
    -H "${AUTH_HEADER}" \
    -d "{\"workflowId\":${WF_ID},\"caption\":\"In Progress\",\"orderNumber\":2,\"weight\":50}")

INPROGRESS_STATE_ID=$(echo "$CREATE_STATE2_RESPONSE" | jq -r '.id // empty')
if [ -n "$INPROGRESS_STATE_ID" ] && [ "$INPROGRESS_STATE_ID" -gt 0 ] 2>/dev/null; then
    print_pass
else
    print_fail "Valid ID" "$INPROGRESS_STATE_ID"
fi

print_test "Создание состояния 'Done'"
CREATE_STATE3_RESPONSE=$(curl -s -X POST "${API_HOST}/api/v1/states" \
    -H "${CONTENT_TYPE}" \
    -H "${AUTH_HEADER}" \
    -d "{\"workflowId\":${WF_ID},\"caption\":\"Done\",\"orderNumber\":3,\"weight\":100}")

DONE_STATE_ID=$(echo "$CREATE_STATE3_RESPONSE" | jq -r '.id // empty')
if [ -n "$DONE_STATE_ID" ] && [ "$DONE_STATE_ID" -gt 0 ] 2>/dev/null; then
    print_pass
else
    print_fail "Valid ID" "$DONE_STATE_ID"
fi

print_test "Получение состояний по workflowId"
STATES_LIST=$(curl -s -X GET "${API_HOST}/api/v1/states?workflowId=${WF_ID}" \
    -H "${AUTH_HEADER}")

STATES_COUNT=$(echo "$STATES_LIST" | jq -r '.totalCount // 0')
if [ "$STATES_COUNT" -eq 3 ]; then
    print_pass
else
    print_fail "3 states" "$STATES_COUNT states"
fi

print_test "Обновление состояния 'In Progress'"
UPDATE_STATE_RESPONSE=$(curl -s -X PUT "${API_HOST}/api/v1/states/${INPROGRESS_STATE_ID}" \
    -H "${CONTENT_TYPE}" \
    -H "${AUTH_HEADER}" \
    -d '{"weight":60,"isQueue":true}')

UPDATED_WEIGHT=$(echo "$UPDATE_STATE_RESPONSE" | jq -r '.weight // 0')
UPDATED_QUEUE=$(echo "$UPDATE_STATE_RESPONSE" | jq -r '.isQueue // false')
if [ "$UPDATED_WEIGHT" == "60" ] && [ "$UPDATED_QUEUE" == "true" ]; then
    print_pass
else
    print_fail "weight=60, isQueue=true" "weight=$UPDATED_WEIGHT, isQueue=$UPDATED_QUEUE"
fi

# ============================================================
# Edges
# ============================================================
print_header "ТЕСТЫ ПЕРЕХОДОВ"

print_test "Создание перехода To Do -> In Progress"
CREATE_EDGE1_RESPONSE=$(curl -s -X POST "${API_HOST}/api/v1/edges" \
    -H "${CONTENT_TYPE}" \
    -H "${AUTH_HEADER}" \
    -d "{\"beginStateId\":${TODO_STATE_ID},\"endStateId\":${INPROGRESS_STATE_ID}}")

EDGE1_ID=$(echo "$CREATE_EDGE1_RESPONSE" | jq -r '.id // empty')
if [ -n "$EDGE1_ID" ] && [ "$EDGE1_ID" -gt 0 ] 2>/dev/null; then
    print_pass
else
    print_fail "Valid ID" "$EDGE1_ID"
fi

print_test "Создание перехода In Progress -> Done"
CREATE_EDGE2_RESPONSE=$(curl -s -X POST "${API_HOST}/api/v1/edges" \
    -H "${CONTENT_TYPE}" \
    -H "${AUTH_HEADER}" \
    -d "{\"beginStateId\":${INPROGRESS_STATE_ID},\"endStateId\":${DONE_STATE_ID}}")

EDGE2_ID=$(echo "$CREATE_EDGE2_RESPONSE" | jq -r '.id // empty')
if [ -n "$EDGE2_ID" ] && [ "$EDGE2_ID" -gt 0 ] 2>/dev/null; then
    print_pass
else
    print_fail "Valid ID" "$EDGE2_ID"
fi

print_test "Создание дубликата перехода"
DUP_EDGE_RESPONSE=$(curl -s -X POST "${API_HOST}/api/v1/edges" \
    -H "${CONTENT_TYPE}" \
    -H "${AUTH_HEADER}" \
    -d "{\"beginStateId\":${TODO_STATE_ID},\"endStateId\":${INPROGRESS_STATE_ID}}")

DUP_EDGE_STATUS=$(echo "$DUP_EDGE_RESPONSE" | jq -r '.code // 201')
if [ "$DUP_EDGE_STATUS" == "409" ] || [ "$DUP_EDGE_STATUS" == "400" ]; then
    print_pass
else
    print_fail "409 or 400" "$DUP_EDGE_STATUS"
fi

print_test "Получение переходов по workflowId"
WORKFLOW_EDGES=$(curl -s -X GET "${API_HOST}/api/v1/workflows/${WF_ID}/edges" \
    -H "${AUTH_HEADER}")

EDGES_COUNT=$(echo "$WORKFLOW_EDGES" | jq -r '.totalCount // 0')
if [ "$EDGES_COUNT" -eq 2 ]; then
    print_pass
else
    print_fail "2 edges" "$EDGES_COUNT edges"
fi

print_test "Получение переходов с фильтром по beginStateId"
FILTERED_EDGES=$(curl -s -X GET "${API_HOST}/api/v1/edges?beginStateId=${TODO_STATE_ID}" \
    -H "${AUTH_HEADER}")

FILTERED_COUNT=$(echo "$FILTERED_EDGES" | jq -r '.totalCount // 0')
if [ "$FILTERED_COUNT" -eq 1 ]; then
    print_pass
else
    print_fail "1 edge" "$FILTERED_COUNT edges"
fi

# ============================================================
# Delete tests
# ============================================================
print_header "ТЕСТЫ УДАЛЕНИЯ"

print_test "Удаление перехода To Do -> In Progress"
DELETE_EDGE1_STATUS=$(curl -s -o /dev/null -w "%{http_code}" -X DELETE \
    "${API_HOST}/api/v1/edges/${EDGE1_ID}" \
    -H "${AUTH_HEADER}")

if [ "$DELETE_EDGE1_STATUS" == "204" ]; then
    print_pass
else
    print_fail "204" "$DELETE_EDGE1_STATUS"
fi

print_test "Проверка удаления первого перехода"
CHECK_EDGE1=$(curl -s -X GET "${API_HOST}/api/v1/edges/${EDGE1_ID}" \
    -H "${AUTH_HEADER}")

DELETED_EDGE1_STATUS=$(echo "$CHECK_EDGE1" | jq -r '.code // 200')
if [ "$DELETED_EDGE1_STATUS" == "404" ]; then
    print_pass
else
    print_fail "404" "$DELETED_EDGE1_STATUS"
fi

print_test "Удаление перехода In Progress -> Done"
DELETE_EDGE2_STATUS=$(curl -s -o /dev/null -w "%{http_code}" -X DELETE \
    "${API_HOST}/api/v1/edges/${EDGE2_ID}" \
    -H "${AUTH_HEADER}")

if [ "$DELETE_EDGE2_STATUS" == "204" ]; then
    print_pass
else
    print_fail "204" "$DELETE_EDGE2_STATUS"
fi

print_test "Проверка удаления второго перехода"
CHECK_EDGE2=$(curl -s -X GET "${API_HOST}/api/v1/edges/${EDGE2_ID}" \
    -H "${AUTH_HEADER}")

DELETED_EDGE2_STATUS=$(echo "$CHECK_EDGE2" | jq -r '.code // 200')
if [ "$DELETED_EDGE2_STATUS" == "404" ]; then
    print_pass
else
    print_fail "404" "$DELETED_EDGE2_STATUS"
fi

print_test "Удаление состояния 'In Progress'"
DELETE_STATE_STATUS=$(curl -s -o /dev/null -w "%{http_code}" -X DELETE \
    "${API_HOST}/api/v1/states/${INPROGRESS_STATE_ID}" \
    -H "${AUTH_HEADER}")

if [ "$DELETE_STATE_STATUS" == "204" ]; then
    print_pass
else
    print_fail "204" "$DELETE_STATE_STATUS"
fi

print_test "Проверка удаления состояния 'In Progress'"
CHECK_STATE=$(curl -s -X GET "${API_HOST}/api/v1/states/${INPROGRESS_STATE_ID}" \
    -H "${AUTH_HEADER}")

DELETED_STATE_STATUS=$(echo "$CHECK_STATE" | jq -r '.code // 200')
if [ "$DELETED_STATE_STATUS" == "404" ]; then
    print_pass
else
    print_fail "404" "$DELETED_STATE_STATUS"
fi

print_test "Удаление рабочего процесса"
DELETE_WF_STATUS=$(curl -s -o /dev/null -w "%{http_code}" -X DELETE \
    "${API_HOST}/api/v1/workflows/${WF_ID}" \
    -H "${AUTH_HEADER}")

if [ "$DELETE_WF_STATUS" == "204" ]; then
    print_pass
else
    print_fail "204" "$DELETE_WF_STATUS"
fi

print_test "Проверка удаления рабочего процесса"
CHECK_WF=$(curl -s -X GET "${API_HOST}/api/v1/workflows/${WF_ID}" \
    -H "${AUTH_HEADER}")

DELETED_WF_STATUS=$(echo "$CHECK_WF" | jq -r '.code // 200')
if [ "$DELETED_WF_STATUS" == "404" ]; then
    print_pass
else
    print_fail "404" "$DELETED_WF_STATUS"
fi

# Проверка каскадного удаления состояний
print_test "Проверка каскадного удаления состояний"
CHECK_STATES=$(curl -s -X GET "${API_HOST}/api/v1/states?workflowId=${WF_ID}" \
    -H "${AUTH_HEADER}")

REMAINING_STATES=$(echo "$CHECK_STATES" | jq -r '.totalCount // 0')
if [ "$REMAINING_STATES" -eq 0 ]; then
    print_pass
else
    print_fail "0 states (cascade delete)" "$REMAINING_STATES states (foreign_keys not enabled)"
fi

# ============================================================
# Summary
# ============================================================
print_header "РЕЗУЛЬТАТЫ ТЕСТИРОВАНИЯ"
echo -e "Всего тестов: ${TOTAL}"
echo -e "${GREEN}Пройдено: ${PASSED}${NC}"
if [ "$FAILED" -gt 0 ]; then
    echo -e "${RED}Провалено: ${FAILED}${NC}"
fi

if [ "$FAILED" -eq 0 ]; then
    echo -e "\n${GREEN}✓ Все тесты пройдены успешно!${NC}"
    exit 0
else
    echo -e "\n${RED}✗ Некоторые тесты не пройдены${NC}"
    exit 1
fi
