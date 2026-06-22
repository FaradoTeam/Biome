#!/usr/bin/env bash

set -euo pipefail

BASE_URL="http://127.0.0.1:8090"
LOGIN="admin"
PASSWORD="password"

GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

echo "=== Тестирование API фаз ==="
echo ""

# 1. Авторизация
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

# 2. Создаем проект для фазы
echo "2. Создание тестового проекта..."
PROJECT_RESPONSE=$(curl -s -X POST "$BASE_URL/api/v1/projects" \
    -H "Authorization: Bearer $TOKEN" \
    -H "Content-Type: application/json" \
    -d '{"caption":"Проект для фаз","description":"Тестовый проект"}')

PROJECT_ID=$(echo "$PROJECT_RESPONSE" | grep -o '"id":[0-9]*' | head -1 | grep -o '[0-9]*')
echo -e "${GREEN}OK${NC} - Проект создан (ID: $PROJECT_ID)"
echo ""

# 3. Создаем фазу
echo "3. Создание фазы..."
RESPONSE=$(curl -s -w "\n%{http_code}" -X POST "$BASE_URL/api/v1/phases" \
    -H "Authorization: Bearer $TOKEN" \
    -H "Content-Type: application/json" \
    -d "{\"caption\":\"Фаза 1\",\"description\":\"Первая фаза\",\"projectId\":$PROJECT_ID}")

HTTP_CODE=$(echo "$RESPONSE" | tail -1)
BODY=$(echo "$RESPONSE" | sed '$d')

if [ "$HTTP_CODE" = "201" ]; then
    echo -e "${GREEN}OK${NC} - Фаза создана (HTTP $HTTP_CODE)"
    PHASE_ID=$(echo "$BODY" | grep -o '"id":[0-9]*' | head -1 | grep -o '[0-9]*')
    echo "ID фазы: $PHASE_ID"
else
    echo -e "${RED}ОШИБКА${NC} - HTTP $HTTP_CODE"
    echo "Ответ: $BODY"
    exit 1
fi
echo ""

# 4. Получаем список фаз проекта
echo "4. Список фаз проекта $PROJECT_ID..."
RESPONSE=$(curl -s -w "\n%{http_code}" -X GET "$BASE_URL/api/v1/phases?projectId=$PROJECT_ID" \
    -H "Authorization: Bearer $TOKEN")

HTTP_CODE=$(echo "$RESPONSE" | tail -1)
BODY=$(echo "$RESPONSE" | sed '$d')

if [ "$HTTP_CODE" = "200" ]; then
    echo -e "${GREEN}OK${NC} - Список получен (HTTP $HTTP_CODE)"
    echo "Всего фаз: $(echo "$BODY" | grep -o '"totalCount":[0-9]*' | grep -o '[0-9]*')"
else
    echo -e "${RED}ОШИБКА${NC} - HTTP $HTTP_CODE"
fi
echo ""

# 5. Получаем фазу по ID
echo "5. Получение фазы по ID ($PHASE_ID)..."
RESPONSE=$(curl -s -w "\n%{http_code}" -X GET "$BASE_URL/api/v1/phases/$PHASE_ID" \
    -H "Authorization: Bearer $TOKEN")

HTTP_CODE=$(echo "$RESPONSE" | tail -1)
BODY=$(echo "$RESPONSE" | sed '$d')

if [ "$HTTP_CODE" = "200" ]; then
    echo -e "${GREEN}OK${NC} - Фаза получена (HTTP $HTTP_CODE)"
    echo "Название: $(echo "$BODY" | grep -o '"caption":"[^"]*"' | cut -d'"' -f4)"
else
    echo -e "${RED}ОШИБКА${NC} - HTTP $HTTP_CODE"
fi
echo ""

# 6. Обновляем фазу
echo "6. Обновление фазы..."
RESPONSE=$(curl -s -w "\n%{http_code}" -X PUT "$BASE_URL/api/v1/phases/$PHASE_ID" \
    -H "Authorization: Bearer $TOKEN" \
    -H "Content-Type: application/json" \
    -d '{"caption":"Обновленная фаза","description":"Новое описание фазы"}')

HTTP_CODE=$(echo "$RESPONSE" | tail -1)
BODY=$(echo "$RESPONSE" | sed '$d')

if [ "$HTTP_CODE" = "200" ]; then
    echo -e "${GREEN}OK${NC} - Фаза обновлена (HTTP $HTTP_CODE)"
    echo "Новое название: $(echo "$BODY" | grep -o '"caption":"[^"]*"' | cut -d'"' -f4)"
else
    echo -e "${RED}ОШИБКА${NC} - HTTP $HTTP_CODE"
fi
echo ""

# 7. Архивируем фазу
echo "7. Архивирование фазы..."
RESPONSE=$(curl -s -w "\n%{http_code}" -X DELETE "$BASE_URL/api/v1/phases/$PHASE_ID" \
    -H "Authorization: Bearer $TOKEN")

HTTP_CODE=$(echo "$RESPONSE" | tail -1)

if [ "$HTTP_CODE" = "204" ]; then
    echo -e "${GREEN}OK${NC} - Фаза архивирована (HTTP $HTTP_CODE)"
else
    echo -e "${RED}ОШИБКА${NC} - HTTP $HTTP_CODE"
fi

echo ""
echo -e "${GREEN}=== Тестирование завершено ===${NC}"
