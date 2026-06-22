#!/usr/bin/env bash

set -euo pipefail

BASE_URL="http://127.0.0.1:8090"
LOGIN="admin"
PASSWORD="password"

# Цвета
GREEN='\033[0;32m'
RED='\033[0;31m'
CYAN='\033[0;36m'
NC='\033[0m'

echo "=== Тестирование API проектов ==="
echo ""

# 1. Получаем токен
echo "1. Авторизация..."
TOKEN=$(curl -s -X POST "$BASE_URL/api/v1/auth/login" \
    -H "Content-Type: application/json" \
    -d "{\"login\":\"$LOGIN\",\"password\":\"$PASSWORD\"}" | \
    grep -o '"access_token":"[^"]*"' | cut -d'"' -f4)

if [ -z "$TOKEN" ]; then
    echo -e "${RED}Ошибка авторизации${NC}"
    exit 1
fi
echo -e "${GREEN}OK${NC} - Токен получен"
echo ""

# 2. Создаем проект
echo "2. Создание проекта..."
RESPONSE=$(curl -s -w "\n%{http_code}" -X POST "$BASE_URL/api/v1/projects" \
    -H "Authorization: Bearer $TOKEN" \
    -H "Content-Type: application/json" \
    -d '{"caption":"Тестовый проект","description":"Описание проекта"}')

HTTP_CODE=$(echo "$RESPONSE" | tail -1)
BODY=$(echo "$RESPONSE" | sed '$d')

if [ "$HTTP_CODE" = "201" ]; then
    echo -e "${GREEN}OK${NC} - Проект создан (HTTP $HTTP_CODE)"
    echo "Ответ: $BODY"

    # Извлекаем ID простым способом
    PROJECT_ID=$(echo "$BODY" | grep -o '"id":[0-9]*' | head -1 | grep -o '[0-9]*')
    echo "ID проекта: $PROJECT_ID"
else
    echo -e "${RED}ОШИБКА${NC} - HTTP $HTTP_CODE"
    echo "Ответ: $BODY"
    exit 1
fi
echo ""

# 3. Получаем список проектов
echo "3. Получение списка проектов..."
RESPONSE=$(curl -s -w "\n%{http_code}" -X GET "$BASE_URL/api/v1/projects" \
    -H "Authorization: Bearer $TOKEN")

HTTP_CODE=$(echo "$RESPONSE" | tail -1)
BODY=$(echo "$RESPONSE" | sed '$d')

if [ "$HTTP_CODE" = "200" ]; then
    echo -e "${GREEN}OK${NC} - Список получен (HTTP $HTTP_CODE)"
    echo "Всего проектов: $(echo "$BODY" | grep -o '"totalCount":[0-9]*' | grep -o '[0-9]*')"
else
    echo -e "${RED}ОШИБКА${NC} - HTTP $HTTP_CODE"
fi
echo ""

# 4. Получаем проект по ID
if [ -n "$PROJECT_ID" ]; then
    echo "4. Получение проекта по ID ($PROJECT_ID)..."
    RESPONSE=$(curl -s -w "\n%{http_code}" -X GET "$BASE_URL/api/v1/projects/$PROJECT_ID" \
        -H "Authorization: Bearer $TOKEN")

    HTTP_CODE=$(echo "$RESPONSE" | tail -1)
    BODY=$(echo "$RESPONSE" | sed '$d')

    if [ "$HTTP_CODE" = "200" ]; then
        echo -e "${GREEN}OK${NC} - Проект получен (HTTP $HTTP_CODE)"
        echo "Название: $(echo "$BODY" | grep -o '"caption":"[^"]*"' | cut -d'"' -f4)"
    else
        echo -e "${RED}ОШИБКА${NC} - HTTP $HTTP_CODE"
    fi
    echo ""
fi

# 5. Обновляем проект
if [ -n "$PROJECT_ID" ]; then
    echo "5. Обновление проекта ($PROJECT_ID)..."
    RESPONSE=$(curl -s -w "\n%{http_code}" -X PUT "$BASE_URL/api/v1/projects/$PROJECT_ID" \
        -H "Authorization: Bearer $TOKEN" \
        -H "Content-Type: application/json" \
        -d '{"caption":"Обновленный проект","description":"Новое описание"}')

    HTTP_CODE=$(echo "$RESPONSE" | tail -1)
    BODY=$(echo "$RESPONSE" | sed '$d')

    if [ "$HTTP_CODE" = "200" ]; then
        echo -e "${GREEN}OK${NC} - Проект обновлен (HTTP $HTTP_CODE)"
        echo "Новое название: $(echo "$BODY" | grep -o '"caption":"[^"]*"' | cut -d'"' -f4)"
    else
        echo -e "${RED}ОШИБКА${NC} - HTTP $HTTP_CODE"
    fi
    echo ""
fi

# 6. Удаляем проект (архивируем)
if [ -n "$PROJECT_ID" ]; then
    echo "6. Архивирование проекта ($PROJECT_ID)..."
    RESPONSE=$(curl -s -w "\n%{http_code}" -X DELETE "$BASE_URL/api/v1/projects/$PROJECT_ID" \
        -H "Authorization: Bearer $TOKEN")

    HTTP_CODE=$(echo "$RESPONSE" | tail -1)

    if [ "$HTTP_CODE" = "204" ]; then
        echo -e "${GREEN}OK${NC} - Проект архивирован (HTTP $HTTP_CODE)"
    else
        echo -e "${RED}ОШИБКА${NC} - HTTP $HTTP_CODE"
    fi
fi

echo ""
echo -e "${GREEN}=== Тестирование завершено ===${NC}"
