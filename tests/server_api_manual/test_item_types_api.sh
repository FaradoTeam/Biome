#!/bin/bash

API_HOST="http://127.0.0.1:8090"
TOKEN=""
PASS=0
FAIL=0

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

print_pass() { echo -e "${GREEN}[PASS]${NC} $1"; PASS=$((PASS+1)); }
print_fail() { echo -e "${RED}[FAIL]${NC} $1"; FAIL=$((FAIL+1)); }
print_info() { echo -e "${YELLOW}[INFO]${NC} $1"; }
print_header() { echo -e "\n${CYAN}=== $1 ===${NC}"; }

# Функция URL-кодирования
urlencode() {
    local string="$1"
    local strlen=${#string}
    local encoded=""
    local pos c o

    for (( pos=0 ; pos<strlen ; pos++ )); do
        c=${string:$pos:1}
        case "$c" in
            [-_.~a-zA-Z0-9] ) o="${c}" ;;
            * ) printf -v o '%%%02x' "'$c"
        esac
        encoded+="${o}"
    done
    echo "${encoded}"
}

check_status() {
    local expected=$1 actual=$2 message=$3
    if [ "$actual" -eq "$expected" ]; then
        print_pass "$message (HTTP $actual)"
    else
        print_fail "$message (ожидался HTTP $expected, получен HTTP $actual)"
    fi
}

# Аутентификация
print_header "Авторизация"
RESPONSE=$(curl -s -w "\n%{http_code}" \
    -X POST "$API_HOST/api/v1/auth/login" \
    -H "Content-Type: application/json" \
    -d '{"login":"admin","password":"password"}')
HTTP_CODE=$(echo "$RESPONSE" | tail -n1)
BODY=$(echo "$RESPONSE" | sed '$d')
check_status 200 "$HTTP_CODE" "Вход в систему"
TOKEN=$(echo "$BODY" | jq -r '.access_token')
print_info "Токен: ${TOKEN:0:30}..."

# 1. Создание (Create)
print_header "1. СОЗДАНИЕ ТИПОВ ЭЛЕМЕНТОВ"

print_info "Создание 'Задача' (issue)..."
RESPONSE=$(curl -s -w "\n%{http_code}" \
    -X POST "$API_HOST/api/v1/item-types" \
    -H "Authorization: Bearer $TOKEN" \
    -H "Content-Type: application/json" \
    -d '{"caption":"Задача","kind":"issue","workflowId":1,"defaultStateId":1,"defaultContent":"## Описание задачи\n\n"}')
HTTP_CODE=$(echo "$RESPONSE" | tail -n1)
BODY=$(echo "$RESPONSE" | sed '$d')
check_status 201 "$HTTP_CODE" "Создание типа 'Задача'"
ISSUE_TYPE_ID=$(echo "$BODY" | jq -r '.id // empty')
[ -n "$ISSUE_TYPE_ID" ] && print_info "ID: $ISSUE_TYPE_ID"

print_info "Создание 'Папка' (folder)..."
RESPONSE=$(curl -s -w "\n%{http_code}" \
    -X POST "$API_HOST/api/v1/item-types" \
    -H "Authorization: Bearer $TOKEN" \
    -H "Content-Type: application/json" \
    -d '{"caption":"Папка","kind":"folder","workflowId":1,"defaultStateId":1}')
HTTP_CODE=$(echo "$RESPONSE" | tail -n1)
BODY=$(echo "$RESPONSE" | sed '$d')
check_status 201 "$HTTP_CODE" "Создание типа 'Папка'"

print_info "Создание 'Требование' (requirement)..."
RESPONSE=$(curl -s -w "\n%{http_code}" \
    -X POST "$API_HOST/api/v1/item-types" \
    -H "Authorization: Bearer $TOKEN" \
    -H "Content-Type: application/json" \
    -d '{"caption":"Требование","kind":"requirement","workflowId":1,"defaultStateId":1}')
HTTP_CODE=$(echo "$RESPONSE" | tail -n1)
check_status 201 "$HTTP_CODE" "Создание типа 'Требование'"

print_info "Тест: невалидный kind..."
RESPONSE=$(curl -s -w "\n%{http_code}" \
    -X POST "$API_HOST/api/v1/item-types" \
    -H "Authorization: Bearer $TOKEN" \
    -H "Content-Type: application/json" \
    -d '{"caption":"Bad Type","kind":"invalid_kind","workflowId":1,"defaultStateId":1}')
HTTP_CODE=$(echo "$RESPONSE" | tail -n1)
check_status 400 "$HTTP_CODE" "Отказ с невалидным kind"

# 2. Чтение (Read)
print_header "2. ЧТЕНИЕ ТИПОВ ЭЛЕМЕНТОВ"

print_info "Получение списка всех типов..."
RESPONSE=$(curl -s -w "\n%{http_code}" \
    -X GET "$API_HOST/api/v1/item-types" \
    -H "Authorization: Bearer $TOKEN")
HTTP_CODE=$(echo "$RESPONSE" | tail -n1)
BODY=$(echo "$RESPONSE" | sed '$d')
check_status 200 "$HTTP_CODE" "Получение списка"
echo "$BODY" | jq '{total: .totalCount, items: [.items[] | {id, caption, kind}]}'

print_info "Получение по ID ($ISSUE_TYPE_ID)..."
RESPONSE=$(curl -s -w "\n%{http_code}" \
    -X GET "$API_HOST/api/v1/item-types/$ISSUE_TYPE_ID" \
    -H "Authorization: Bearer $TOKEN")
HTTP_CODE=$(echo "$RESPONSE" | tail -n1)
check_status 200 "$HTTP_CODE" "Получение по ID"

print_info "Фильтр по kind=issue..."
RESPONSE=$(curl -s -w "\n%{http_code}" \
    -X GET "$API_HOST/api/v1/item-types?kind=issue" \
    -H "Authorization: Bearer $TOKEN")
HTTP_CODE=$(echo "$RESPONSE" | tail -n1)
BODY=$(echo "$RESPONSE" | sed '$d')
check_status 200 "$HTTP_CODE" "Фильтр по kind=issue"
echo "$BODY" | jq '{total: .totalCount}'

# Исправлено: URL-кодирование кириллицы
print_info "Поиск по названию (Задача)..."
ENCODED_SEARCH=$(urlencode "Задача")
RESPONSE=$(curl -s -w "\n%{http_code}" \
    -X GET "$API_HOST/api/v1/item-types?searchCaption=$ENCODED_SEARCH" \
    -H "Authorization: Bearer $TOKEN")
HTTP_CODE=$(echo "$RESPONSE" | tail -n1)
BODY=$(echo "$RESPONSE" | sed '$d')
check_status 200 "$HTTP_CODE" "Поиск по названию"
echo "$BODY" | jq '{total: .totalCount, items: [.items[] | {id, caption}]}'

# Альтернативный вариант с --data-urlencode
print_info "Поиск по названию через GET с --data-urlencode..."
RESPONSE=$(curl -s -w "\n%{http_code}" \
    -G "$API_HOST/api/v1/item-types" \
    -H "Authorization: Bearer $TOKEN" \
    --data-urlencode "searchCaption=Задача")
HTTP_CODE=$(echo "$RESPONSE" | tail -n1)
BODY=$(echo "$RESPONSE" | sed '$d')
check_status 200 "$HTTP_CODE" "Поиск по названию (curl --data-urlencode)"
echo "$BODY" | jq '{total: .totalCount, items: [.items[] | {id, caption}]}'

# 3. Обновление (Update)
print_header "3. ОБНОВЛЕНИЕ ТИПОВ ЭЛЕМЕНТОВ"

print_info "Обновление названия..."
RESPONSE=$(curl -s -w "\n%{http_code}" \
    -X PUT "$API_HOST/api/v1/item-types/$ISSUE_TYPE_ID" \
    -H "Authorization: Bearer $TOKEN" \
    -H "Content-Type: application/json" \
    -d '{"caption":"Задача v2"}')
HTTP_CODE=$(echo "$RESPONSE" | tail -n1)
BODY=$(echo "$RESPONSE" | sed '$d')
check_status 200 "$HTTP_CODE" "Обновление типа"
echo "$BODY" | jq '{id, caption, kind}'

# 4. Удаление (Delete)
print_header "4. УДАЛЕНИЕ ТИПОВ ЭЛЕМЕНТОВ"

# Создаем временный для удаления
RESPONSE=$(curl -s -w "\n%{http_code}" \
    -X POST "$API_HOST/api/v1/item-types" \
    -H "Authorization: Bearer $TOKEN" \
    -H "Content-Type: application/json" \
    -d '{"caption":"TempDelete","kind":"test-case","workflowId":1,"defaultStateId":1}')
HTTP_CODE=$(echo "$RESPONSE" | tail -n1)
TEMP_ID=$(echo "$RESPONSE" | sed '$d' | jq -r '.id // empty')

if [ -n "$TEMP_ID" ]; then
    print_info "Удаление временного типа (ID=$TEMP_ID)..."
    RESPONSE=$(curl -s -w "\n%{http_code}" \
        -X DELETE "$API_HOST/api/v1/item-types/$TEMP_ID" \
        -H "Authorization: Bearer $TOKEN")
    HTTP_CODE=$(echo "$RESPONSE" | tail -n1)
    check_status 204 "$HTTP_CODE" "Удаление временного типа"
fi

# 5. Пагинация
print_header "5. ПАГИНАЦИЯ"

RESPONSE=$(curl -s -w "\n%{http_code}" \
    -X GET "$API_HOST/api/v1/item-types?page=1&pageSize=2" \
    -H "Authorization: Bearer $TOKEN")
HTTP_CODE=$(echo "$RESPONSE" | tail -n1)
BODY=$(echo "$RESPONSE" | sed '$d')
check_status 200 "$HTTP_CODE" "Пагинация (page=1, pageSize=2)"
echo "$BODY" | jq '{total: .totalCount, page: .page, pageSize: .pageSize, count: (.items | length)}'

# 6. Ошибки
print_header "6. ПРОВЕРКА ОШИБОК"

RESPONSE=$(curl -s -w "\n%{http_code}" -X GET "$API_HOST/api/v1/item-types/99999" -H "Authorization: Bearer $TOKEN")
check_status 404 "$(echo "$RESPONSE" | tail -n1)" "Несуществующий ID (404)"

RESPONSE=$(curl -s -w "\n%{http_code}" -X GET "$API_HOST/api/v1/item-types")
check_status 401 "$(echo "$RESPONSE" | tail -n1)" "Без авторизации (401)"

# Итоги
print_header "РЕЗУЛЬТАТЫ"
echo -e "Всего: $((PASS+FAIL)) | ${GREEN}Пройдено: $PASS${NC} | ${RED}Провалено: $FAIL${NC}"
[ $FAIL -eq 0 ] && echo -e "${GREEN}✓ Все тесты пройдены!${NC}" || echo -e "${RED}✗ Есть ошибки!${NC}"
exit $FAIL
