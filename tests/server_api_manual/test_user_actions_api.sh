#!/usr/bin/env bash

set -euo pipefail

BASE_URL="http://127.0.0.1:8090"
LOGIN="admin"
PASSWORD="password"

# Цвета
GREEN='\033[0;32m'
RED='\033[0;31m'
CYAN='\033[0;36m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo "=== Тестирование API действий и задач пользователя ==="
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

# 2. Получаем ID текущего пользователя
echo "2. Получение информации о пользователе..."
USER_RESPONSE=$(curl -s -X GET "$BASE_URL/api/v1/users" \
    -H "Authorization: Bearer $TOKEN")

USER_ID=$(echo "$USER_RESPONSE" | grep -o '"id":[0-9]*' | head -1 | grep -o '[0-9]*')

if [ -n "$USER_ID" ]; then
    echo -e "${GREEN}OK${NC} - ID пользователя: $USER_ID"
else
    echo -e "${YELLOW}WARNING${NC} - Не удалось получить ID пользователя, используем 1"
    USER_ID=1
fi
echo ""

# ============================================================
# 3. Тестирование действий пользователя (UserActions)
# ============================================================

echo -e "${CYAN}=== Тестирование действий пользователя ===${NC}"
echo ""

# 3.1 Создание действия
echo "3.1 Создание действия..."
ACTION_RESPONSE=$(curl -s -X POST "$BASE_URL/api/v1/user-actions" \
    -H "Authorization: Bearer $TOKEN" \
    -H "Content-Type: application/json" \
    -d "{\"userId\":$USER_ID,\"caption\":\"Тестовое действие\",\"description\":\"Описание тестового действия\"}")

ACTION_ID=$(echo "$ACTION_RESPONSE" | grep -o '"id":[0-9]*' | head -1 | grep -o '[0-9]*')
ACTION_CAPTION=$(echo "$ACTION_RESPONSE" | grep -o '"caption":"[^"]*"' | cut -d'"' -f4)

if [ -n "$ACTION_ID" ]; then
    echo -e "${GREEN}OK${NC} - Действие создано (ID: $ACTION_ID, caption: $ACTION_CAPTION)"
else
    echo -e "${RED}ОШИБКА${NC} - Не удалось создать действие"
    echo "Ответ: $ACTION_RESPONSE"
fi
echo ""

# 3.2 Получение списка действий
echo "3.2 Получение списка действий..."
ACTIONS_LIST_RESPONSE=$(curl -s -X GET "$BASE_URL/api/v1/user-actions" \
    -H "Authorization: Bearer $TOKEN")

ACTIONS_LIST_COUNT=$(echo "$ACTIONS_LIST_RESPONSE" | grep -o '"totalCount":[0-9]*' | grep -o '[0-9]*')

if [ -n "$ACTIONS_LIST_COUNT" ]; then
    echo -e "${GREEN}OK${NC} - Список действий получен (Всего: $ACTIONS_LIST_COUNT)"
else
    echo -e "${RED}ОШИБКА${NC} - Не удалось получить список действий"
fi
echo ""

# 3.3 Получение списка действий с пагинацией
echo "3.3 Получение списка действий с пагинацией..."
ACTIONS_PAGE_RESPONSE=$(curl -s -X GET "$BASE_URL/api/v1/user-actions?page=1&pageSize=5" \
    -H "Authorization: Bearer $TOKEN")

ACTIONS_PAGE_COUNT=$(echo "$ACTIONS_PAGE_RESPONSE" | grep -o '"totalCount":[0-9]*' | grep -o '[0-9]*')
ACTIONS_PAGE_SIZE=$(echo "$ACTIONS_PAGE_RESPONSE" | grep -o '"pageSize":[0-9]*' | grep -o '[0-9]*')

if [ -n "$ACTIONS_PAGE_COUNT" ]; then
    echo -e "${GREEN}OK${NC} - Пагинация работает (Всего: $ACTIONS_PAGE_COUNT, pageSize: $ACTIONS_PAGE_SIZE)"
else
    echo -e "${RED}ОШИБКА${NC} - Не удалось получить список с пагинацией"
fi
echo ""

# 3.4 Получение действия по ID
if [ -n "$ACTION_ID" ]; then
    echo "3.4 Получение действия по ID ($ACTION_ID)..."
    ACTION_GET_RESPONSE=$(curl -s -X GET "$BASE_URL/api/v1/user-actions/$ACTION_ID" \
        -H "Authorization: Bearer $TOKEN")

    ACTION_GET_CAPTION=$(echo "$ACTION_GET_RESPONSE" | grep -o '"caption":"[^"]*"' | cut -d'"' -f4)

    if [ -n "$ACTION_GET_CAPTION" ]; then
        echo -e "${GREEN}OK${NC} - Действие получено (caption: $ACTION_GET_CAPTION)"
    else
        echo -e "${RED}ОШИБКА${NC} - Не удалось получить действие"
    fi
    echo ""
fi

# 3.5 Фильтрация действий по пользователю
echo "3.5 Фильтрация действий по пользователю ($USER_ID)..."
ACTIONS_FILTER_RESPONSE=$(curl -s -X GET "$BASE_URL/api/v1/user-actions?userId=$USER_ID" \
    -H "Authorization: Bearer $TOKEN")

ACTIONS_FILTER_COUNT=$(echo "$ACTIONS_FILTER_RESPONSE" | grep -o '"totalCount":[0-9]*' | grep -o '[0-9]*')

if [ -n "$ACTIONS_FILTER_COUNT" ]; then
    echo -e "${GREEN}OK${NC} - Фильтрация по пользователю работает (Найдено: $ACTIONS_FILTER_COUNT)"
else
    echo -e "${RED}ОШИБКА${NC} - Не удалось отфильтровать действия"
fi
echo ""

# ============================================================
# 4. Тестирование задач пользователя (UserTodos)
# ============================================================

echo -e "${CYAN}=== Тестирование задач пользователя ===${NC}"
echo ""

# 4.1 Создание задачи
echo "4.1 Создание задачи..."
TODO_RESPONSE=$(curl -s -X POST "$BASE_URL/api/v1/user-todos" \
    -H "Authorization: Bearer $TOKEN" \
    -H "Content-Type: application/json" \
    -d "{\"userId\":$USER_ID,\"caption\":\"Тестовая задача\",\"isDone\":false}")

TODO_ID=$(echo "$TODO_RESPONSE" | grep -o '"id":[0-9]*' | head -1 | grep -o '[0-9]*')
TODO_CAPTION=$(echo "$TODO_RESPONSE" | grep -o '"caption":"[^"]*"' | cut -d'"' -f4)

if [ -n "$TODO_ID" ]; then
    echo -e "${GREEN}OK${NC} - Задача создана (ID: $TODO_ID, caption: $TODO_CAPTION)"
else
    echo -e "${RED}ОШИБКА${NC} - Не удалось создать задачу"
    echo "Ответ: $TODO_RESPONSE"
fi
echo ""

# 4.2 Создание выполненной задачи
echo "4.2 Создание выполненной задачи..."
TODO_DONE_RESPONSE=$(curl -s -X POST "$BASE_URL/api/v1/user-todos" \
    -H "Authorization: Bearer $TOKEN" \
    -H "Content-Type: application/json" \
    -d "{\"userId\":$USER_ID,\"caption\":\"Выполненная задача\",\"isDone\":true}")

TODO_DONE_ID=$(echo "$TODO_DONE_RESPONSE" | grep -o '"id":[0-9]*' | head -1 | grep -o '[0-9]*')
TODO_DONE_STATUS=$(echo "$TODO_DONE_RESPONSE" | grep -o '"isDone":[^,}]*' | cut -d':' -f2 | tr -d ' ')

if [ -n "$TODO_DONE_ID" ]; then
    echo -e "${GREEN}OK${NC} - Выполненная задача создана (ID: $TODO_DONE_ID, isDone: $TODO_DONE_STATUS)"
else
    echo -e "${RED}ОШИБКА${NC} - Не удалось создать выполненную задачу"
fi
echo ""

# 4.3 Получение списка задач
echo "4.3 Получение списка задач..."
TODOS_LIST_RESPONSE=$(curl -s -X GET "$BASE_URL/api/v1/user-todos" \
    -H "Authorization: Bearer $TOKEN")

TODOS_LIST_COUNT=$(echo "$TODOS_LIST_RESPONSE" | grep -o '"totalCount":[0-9]*' | grep -o '[0-9]*')

if [ -n "$TODOS_LIST_COUNT" ]; then
    echo -e "${GREEN}OK${NC} - Список задач получен (Всего: $TODOS_LIST_COUNT)"
else
    echo -e "${RED}ОШИБКА${NC} - Не удалось получить список задач"
fi
echo ""

# 4.4 Получение списка задач с пагинацией
echo "4.4 Получение списка задач с пагинацией..."
TODOS_PAGE_RESPONSE=$(curl -s -X GET "$BASE_URL/api/v1/user-todos?page=1&pageSize=5" \
    -H "Authorization: Bearer $TOKEN")

TODOS_PAGE_COUNT=$(echo "$TODOS_PAGE_RESPONSE" | grep -o '"totalCount":[0-9]*' | grep -o '[0-9]*')
TODOS_PAGE_SIZE=$(echo "$TODOS_PAGE_RESPONSE" | grep -o '"pageSize":[0-9]*' | grep -o '[0-9]*')

if [ -n "$TODOS_PAGE_COUNT" ]; then
    echo -e "${GREEN}OK${NC} - Пагинация работает (Всего: $TODOS_PAGE_COUNT, pageSize: $TODOS_PAGE_SIZE)"
else
    echo -e "${RED}ОШИБКА${NC} - Не удалось получить список с пагинацией"
fi
echo ""

# 4.5 Получение задачи по ID
if [ -n "$TODO_ID" ]; then
    echo "4.5 Получение задачи по ID ($TODO_ID)..."
    TODO_GET_RESPONSE=$(curl -s -X GET "$BASE_URL/api/v1/user-todos/$TODO_ID" \
        -H "Authorization: Bearer $TOKEN")

    TODO_GET_CAPTION=$(echo "$TODO_GET_RESPONSE" | grep -o '"caption":"[^"]*"' | cut -d'"' -f4)

    if [ -n "$TODO_GET_CAPTION" ]; then
        echo -e "${GREEN}OK${NC} - Задача получена (caption: $TODO_GET_CAPTION)"
    else
        echo -e "${RED}ОШИБКА${NC} - Не удалось получить задачу"
    fi
    echo ""
fi

# 4.6 Фильтрация задач по пользователю
echo "4.6 Фильтрация задач по пользователю ($USER_ID)..."
TODOS_FILTER_RESPONSE=$(curl -s -X GET "$BASE_URL/api/v1/user-todos?userId=$USER_ID" \
    -H "Authorization: Bearer $TOKEN")

TODOS_FILTER_COUNT=$(echo "$TODOS_FILTER_RESPONSE" | grep -o '"totalCount":[0-9]*' | grep -o '[0-9]*')

if [ -n "$TODOS_FILTER_COUNT" ]; then
    echo -e "${GREEN}OK${NC} - Фильтрация по пользователю работает (Найдено: $TODOS_FILTER_COUNT)"
else
    echo -e "${RED}ОШИБКА${NC} - Не удалось отфильтровать задачи"
fi
echo ""

# 4.7 Фильтрация задач по статусу выполнения
echo "4.7 Фильтрация задач по статусу выполнения (isDone=false)..."
TODOS_NOT_DONE_RESPONSE=$(curl -s -X GET "$BASE_URL/api/v1/user-todos?isDone=false" \
    -H "Authorization: Bearer $TOKEN")

TODOS_NOT_DONE_COUNT=$(echo "$TODOS_NOT_DONE_RESPONSE" | grep -o '"totalCount":[0-9]*' | grep -o '[0-9]*')

if [ -n "$TODOS_NOT_DONE_COUNT" ]; then
    echo -e "${GREEN}OK${NC} - Фильтрация по статусу работает (Невыполненных: $TODOS_NOT_DONE_COUNT)"
else
    echo -e "${RED}ОШИБКА${NC} - Не удалось отфильтровать задачи по статусу"
fi
echo ""

# 4.8 Обновление задачи
if [ -n "$TODO_ID" ]; then
    echo "4.8 Обновление задачи ($TODO_ID)..."
    TODO_UPDATE_RESPONSE=$(curl -s -X PUT "$BASE_URL/api/v1/user-todos/$TODO_ID" \
        -H "Authorization: Bearer $TOKEN" \
        -H "Content-Type: application/json" \
        -d "{\"caption\":\"Обновленная задача\",\"isDone\":true}")

    TODO_UPDATE_CAPTION=$(echo "$TODO_UPDATE_RESPONSE" | grep -o '"caption":"[^"]*"' | cut -d'"' -f4)
    TODO_UPDATE_STATUS=$(echo "$TODO_UPDATE_RESPONSE" | grep -o '"isDone":[^,}]*' | cut -d':' -f2 | tr -d ' ')

    if [ "$TODO_UPDATE_CAPTION" = "Обновленная задача" ] && [ "$TODO_UPDATE_STATUS" = "true" ]; then
        echo -e "${GREEN}OK${NC} - Задача обновлена (caption: $TODO_UPDATE_CAPTION, isDone: $TODO_UPDATE_STATUS)"
    else
        echo -e "${RED}ОШИБКА${NC} - Не удалось обновить задачу"
    fi
    echo ""
fi

# 4.9 Частичное обновление задачи (только статус)
if [ -n "$TODO_ID" ]; then
    echo "4.9 Частичное обновление задачи (только статус)..."
    TODO_PARTIAL_RESPONSE=$(curl -s -X PUT "$BASE_URL/api/v1/user-todos/$TODO_ID" \
        -H "Authorization: Bearer $TOKEN" \
        -H "Content-Type: application/json" \
        -d "{\"isDone\":false}")

    TODO_PARTIAL_STATUS=$(echo "$TODO_PARTIAL_RESPONSE" | grep -o '"isDone":[^,}]*' | cut -d':' -f2 | tr -d ' ')

    if [ "$TODO_PARTIAL_STATUS" = "false" ]; then
        echo -e "${GREEN}OK${NC} - Частичное обновление выполнено (isDone: $TODO_PARTIAL_STATUS)"
    else
        echo -e "${RED}ОШИБКА${NC} - Не удалось выполнить частичное обновление"
    fi
    echo ""
fi

# ============================================================
# 5. Удаление (очистка)
# ============================================================

echo -e "${CYAN}=== Очистка ===${NC}"
echo ""

# 5.1 Удаление задачи
if [ -n "$TODO_ID" ]; then
    echo "5.1 Удаление задачи ($TODO_ID)..."
    RESPONSE=$(curl -s -o /dev/null -w "%{http_code}" -X DELETE "$BASE_URL/api/v1/user-todos/$TODO_ID" \
        -H "Authorization: Bearer $TOKEN")

    if [ "$RESPONSE" = "204" ] || [ "$RESPONSE" = "200" ]; then
        echo -e "${GREEN}OK${NC} - Задача удалена (HTTP $RESPONSE)"
    else
        echo -e "${RED}ОШИБКА${NC} - Не удалось удалить задачу (HTTP $RESPONSE)"
    fi
    echo ""
fi

# 5.2 Удаление выполненной задачи
if [ -n "$TODO_DONE_ID" ]; then
    echo "5.2 Удаление выполненной задачи ($TODO_DONE_ID)..."
    RESPONSE=$(curl -s -o /dev/null -w "%{http_code}" -X DELETE "$BASE_URL/api/v1/user-todos/$TODO_DONE_ID" \
        -H "Authorization: Bearer $TOKEN")

    if [ "$RESPONSE" = "204" ] || [ "$RESPONSE" = "200" ]; then
        echo -e "${GREEN}OK${NC} - Выполненная задача удалена (HTTP $RESPONSE)"
    else
        echo -e "${RED}ОШИБКА${NC} - Не удалось удалить выполненную задачу (HTTP $RESPONSE)"
    fi
    echo ""
fi

# 5.3 Удаление действия
if [ -n "$ACTION_ID" ]; then
    echo "5.3 Удаление действия ($ACTION_ID)..."
    RESPONSE=$(curl -s -o /dev/null -w "%{http_code}" -X DELETE "$BASE_URL/api/v1/user-actions/$ACTION_ID" \
        -H "Authorization: Bearer $TOKEN")

    if [ "$RESPONSE" = "204" ] || [ "$RESPONSE" = "200" ]; then
        echo -e "${GREEN}OK${NC} - Действие удалено (HTTP $RESPONSE)"
    else
        echo -e "${RED}ОШИБКА${NC} - Не удалось удалить действие (HTTP $RESPONSE)"
    fi
    echo ""
fi

# 5.4 Проверка, что список пустой
echo "5.4 Проверка, что список действий пустой..."
ACTIONS_EMPTY_RESPONSE=$(curl -s -X GET "$BASE_URL/api/v1/user-actions" \
    -H "Authorization: Bearer $TOKEN")

ACTIONS_EMPTY_COUNT=$(echo "$ACTIONS_EMPTY_RESPONSE" | grep -o '"totalCount":[0-9]*' | grep -o '[0-9]*')

if [ "$ACTIONS_EMPTY_COUNT" = "0" ]; then
    echo -e "${GREEN}OK${NC} - Список действий пуст ($ACTIONS_EMPTY_COUNT)"
else
    echo -e "${YELLOW}WARNING${NC} - В списке действий остались записи ($ACTIONS_EMPTY_COUNT)"
fi
echo ""

# 5.5 Проверка, что список задач пустой
echo "5.5 Проверка, что список задач пустой..."
TODOS_EMPTY_RESPONSE=$(curl -s -X GET "$BASE_URL/api/v1/user-todos" \
    -H "Authorization: Bearer $TOKEN")

TODOS_EMPTY_COUNT=$(echo "$TODOS_EMPTY_RESPONSE" | grep -o '"totalCount":[0-9]*' | grep -o '[0-9]*')

if [ "$TODOS_EMPTY_COUNT" = "0" ]; then
    echo -e "${GREEN}OK${NC} - Список задач пуст ($TODOS_EMPTY_COUNT)"
else
    echo -e "${YELLOW}WARNING${NC} - В списке задач остались записи ($TODOS_EMPTY_COUNT)"
fi
echo ""

echo -e "${GREEN}=== Тестирование API действий и задач завершено ===${NC}"
