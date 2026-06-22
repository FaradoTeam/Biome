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

# Получаем ID первого доступного ItemType
RESPONSE=$(curl -s -w "\n%{http_code}" \
    -X GET "$API_HOST/api/v1/item-types?page=1&pageSize=1" \
    -H "Authorization: Bearer $TOKEN")
ITEM_TYPE_ID=$(echo "$RESPONSE" | sed '$d' | jq -r '.items[0].id // empty')

if [ -z "$ITEM_TYPE_ID" ] || [ "$ITEM_TYPE_ID" = "null" ]; then
    print_fail "Нет доступных ItemType. Запустите test_item_types_api.sh сначала."
    exit 1
fi
print_info "ItemType ID: $ITEM_TYPE_ID"

# 1. Создание (Create)
print_header "1. СОЗДАНИЕ ТИПОВ ПОЛЕЙ"

print_info "Создание поля 'Приоритет' (Select)..."
RESPONSE=$(curl -s -w "\n%{http_code}" \
    -X POST "$API_HOST/api/v1/field-types" \
    -H "Authorization: Bearer $TOKEN" \
    -H "Content-Type: application/json" \
    -d "{\"caption\":\"Приоритет\",\"valueType\":\"Select\",\"itemTypeId\":$ITEM_TYPE_ID,\"isBoardVisible\":true}")
HTTP_CODE=$(echo "$RESPONSE" | tail -n1)
BODY=$(echo "$RESPONSE" | sed '$d')
check_status 201 "$HTTP_CODE" "Создание поля 'Приоритет'"
SELECT_FIELD_ID=$(echo "$BODY" | jq -r '.id // empty')
[ -n "$SELECT_FIELD_ID" ] && print_info "ID: $SELECT_FIELD_ID"

print_info "Создание поля 'Описание' (MarkdownText)..."
RESPONSE=$(curl -s -w "\n%{http_code}" \
    -X POST "$API_HOST/api/v1/field-types" \
    -H "Authorization: Bearer $TOKEN" \
    -H "Content-Type: application/json" \
    -d "{\"caption\":\"Описание\",\"valueType\":\"MarkdownText\",\"itemTypeId\":$ITEM_TYPE_ID,\"isBoardVisible\":false}")
HTTP_CODE=$(echo "$RESPONSE" | tail -n1)
check_status 201 "$HTTP_CODE" "Создание поля 'Описание'"

print_info "Создание поля 'Сложность' (Integer)..."
RESPONSE=$(curl -s -w "\n%{http_code}" \
    -X POST "$API_HOST/api/v1/field-types" \
    -H "Authorization: Bearer $TOKEN" \
    -H "Content-Type: application/json" \
    -d "{\"caption\":\"Сложность\",\"valueType\":\"Integer\",\"itemTypeId\":$ITEM_TYPE_ID,\"isBoardVisible\":true}")
HTTP_CODE=$(echo "$RESPONSE" | tail -n1)
check_status 201 "$HTTP_CODE" "Создание поля 'Сложность'"

print_info "Тест: невалидный valueType..."
RESPONSE=$(curl -s -w "\n%{http_code}" \
    -X POST "$API_HOST/api/v1/field-types" \
    -H "Authorization: Bearer $TOKEN" \
    -H "Content-Type: application/json" \
    -d "{\"caption\":\"Bad\",\"valueType\":\"BadType\",\"itemTypeId\":$ITEM_TYPE_ID}")
HTTP_CODE=$(echo "$RESPONSE" | tail -n1)
check_status 400 "$HTTP_CODE" "Отказ с невалидным valueType"

# 2. Чтение (Read)
print_header "2. ЧТЕНИЕ ТИПОВ ПОЛЕЙ"

print_info "Получение списка всех типов полей..."
RESPONSE=$(curl -s -w "\n%{http_code}" \
    -X GET "$API_HOST/api/v1/field-types" \
    -H "Authorization: Bearer $TOKEN")
HTTP_CODE=$(echo "$RESPONSE" | tail -n1)
BODY=$(echo "$RESPONSE" | sed '$d')
check_status 200 "$HTTP_CODE" "Получение списка"
echo "$BODY" | jq '{total: .totalCount, items: [.items[] | {id, caption, valueType, isBoardVisible}]}'

print_info "Получение по ID ($SELECT_FIELD_ID)..."
RESPONSE=$(curl -s -w "\n%{http_code}" \
    -X GET "$API_HOST/api/v1/field-types/$SELECT_FIELD_ID" \
    -H "Authorization: Bearer $TOKEN")
HTTP_CODE=$(echo "$RESPONSE" | tail -n1)
check_status 200 "$HTTP_CODE" "Получение по ID"

print_info "Фильтр по valueType=Select..."
RESPONSE=$(curl -s -w "\n%{http_code}" \
    -X GET "$API_HOST/api/v1/field-types?valueType=Select" \
    -H "Authorization: Bearer $TOKEN")
HTTP_CODE=$(echo "$RESPONSE" | tail -n1)
BODY=$(echo "$RESPONSE" | sed '$d')
check_status 200 "$HTTP_CODE" "Фильтр по valueType=Select"
echo "$BODY" | jq '{total: .totalCount}'

# Исправлено: используем --data-urlencode для кириллицы
print_info "Поиск по названию (Приоритет) через --data-urlencode..."
RESPONSE=$(curl -s -w "\n%{http_code}" \
    -G "$API_HOST/api/v1/field-types" \
    -H "Authorization: Bearer $TOKEN" \
    --data-urlencode "searchCaption=Приоритет")
HTTP_CODE=$(echo "$RESPONSE" | tail -n1)
BODY=$(echo "$RESPONSE" | sed '$d')
check_status 200 "$HTTP_CODE" "Поиск по названию"
echo "$BODY" | jq '{total: .totalCount, items: [.items[] | {id, caption}]}'

# 3. Обновление (Update)
print_header "3. ОБНОВЛЕНИЕ ТИПОВ ПОЛЕЙ"

print_info "Обновление названия и видимости..."
RESPONSE=$(curl -s -w "\n%{http_code}" \
    -X PUT "$API_HOST/api/v1/field-types/$SELECT_FIELD_ID" \
    -H "Authorization: Bearer $TOKEN" \
    -H "Content-Type: application/json" \
    -d '{"caption":"Приоритет v2","isBoardVisible":false}')
HTTP_CODE=$(echo "$RESPONSE" | tail -n1)
BODY=$(echo "$RESPONSE" | sed '$d')
check_status 200 "$HTTP_CODE" "Обновление типа поля"
echo "$BODY" | jq '{id, caption, isBoardVisible}'

# 4. Удаление (Delete)
print_header "4. УДАЛЕНИЕ ТИПОВ ПОЛЕЙ"

# Создаем временный для удаления
RESPONSE=$(curl -s -w "\n%{http_code}" \
    -X POST "$API_HOST/api/v1/field-types" \
    -H "Authorization: Bearer $TOKEN" \
    -H "Content-Type: application/json" \
    -d "{\"caption\":\"TempDelete\",\"valueType\":\"Bool\",\"itemTypeId\":$ITEM_TYPE_ID}")
HTTP_CODE=$(echo "$RESPONSE" | tail -n1)
TEMP_ID=$(echo "$RESPONSE" | sed '$d' | jq -r '.id // empty')

if [ -n "$TEMP_ID" ]; then
    print_info "Удаление временного поля (ID=$TEMP_ID)..."
    RESPONSE=$(curl -s -w "\n%{http_code}" \
        -X DELETE "$API_HOST/api/v1/field-types/$TEMP_ID" \
        -H "Authorization: Bearer $TOKEN")
    HTTP_CODE=$(echo "$RESPONSE" | tail -n1)
    check_status 204 "$HTTP_CODE" "Удаление временного поля"
fi

# 5. Пагинация
print_header "5. ПАГИНАЦИЯ"

RESPONSE=$(curl -s -w "\n%{http_code}" \
    -X GET "$API_HOST/api/v1/field-types?page=1&pageSize=2" \
    -H "Authorization: Bearer $TOKEN")
HTTP_CODE=$(echo "$RESPONSE" | tail -n1)
BODY=$(echo "$RESPONSE" | sed '$d')
check_status 200 "$HTTP_CODE" "Пагинация (page=1, pageSize=2)"
echo "$BODY" | jq '{total: .totalCount, page: .page, pageSize: .pageSize, count: (.items | length)}'

# 6. Ошибки
print_header "6. ПРОВЕРКА ОШИБОК"

RESPONSE=$(curl -s -w "\n%{http_code}" -X GET "$API_HOST/api/v1/field-types/99999" -H "Authorization: Bearer $TOKEN")
check_status 404 "$(echo "$RESPONSE" | tail -n1)" "Несуществующий ID (404)"

RESPONSE=$(curl -s -w "\n%{http_code}" -X GET "$API_HOST/api/v1/field-types")
check_status 401 "$(echo "$RESPONSE" | tail -n1)" "Без авторизации (401)"

# Итоги
print_header "РЕЗУЛЬТАТЫ"
echo -e "Всего: $((PASS+FAIL)) | ${GREEN}Пройдено: $PASS${NC} | ${RED}Провалено: $FAIL${NC}"
[ $FAIL -eq 0 ] && echo -e "${GREEN}✓ Все тесты пройдены!${NC}" || echo -e "${RED}✗ Есть ошибки!${NC}"
exit $FAIL
