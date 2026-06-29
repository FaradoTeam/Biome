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

echo "=== Тестирование API документов и комментариев ==="
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

# 2. Создаем проект (нужен для создания элементов)
echo "2. Создание проекта для тестов..."
PROJECT_RESPONSE=$(curl -s -X POST "$BASE_URL/api/v1/projects" \
    -H "Authorization: Bearer $TOKEN" \
    -H "Content-Type: application/json" \
    -d '{"caption":"Тестовый проект для документов","description":"Описание проекта"}')

PROJECT_ID=$(echo "$PROJECT_RESPONSE" | grep -o '"id":[0-9]*' | head -1 | grep -o '[0-9]*')

if [ -n "$PROJECT_ID" ]; then
    echo -e "${GREEN}OK${NC} - Проект создан (ID: $PROJECT_ID)"
else
    echo -e "${RED}ОШИБКА${NC} - Не удалось создать проект"
    exit 1
fi
echo ""

# 3. Создаем фазу
echo "3. Создание фазы..."
PHASE_RESPONSE=$(curl -s -X POST "$BASE_URL/api/v1/phases" \
    -H "Authorization: Bearer $TOKEN" \
    -H "Content-Type: application/json" \
    -d "{\"projectId\":$PROJECT_ID,\"caption\":\"Тестовая фаза\",\"description\":\"Фаза для тестов\"}")

PHASE_ID=$(echo "$PHASE_RESPONSE" | grep -o '"id":[0-9]*' | head -1 | grep -o '[0-9]*')

if [ -n "$PHASE_ID" ]; then
    echo -e "${GREEN}OK${NC} - Фаза создана (ID: $PHASE_ID)"
else
    echo -e "${RED}ОШИБКА${NC} - Не удалось создать фазу"
    exit 1
fi
echo ""

# 4. Получаем workflow
echo "4. Получение workflow..."
WORKFLOW_RESPONSE=$(curl -s -X GET "$BASE_URL/api/v1/workflows?page=1&pageSize=1" \
    -H "Authorization: Bearer $TOKEN")

WORKFLOW_ID=$(echo "$WORKFLOW_RESPONSE" | grep -o '"id":[0-9]*' | head -1 | grep -o '[0-9]*')

if [ -n "$WORKFLOW_ID" ]; then
    echo -e "${GREEN}OK${NC} - Workflow получен (ID: $WORKFLOW_ID)"
else
    echo -e "${YELLOW}WARNING${NC} - Workflow не найден, создаем новый..."
    WORKFLOW_RESPONSE=$(curl -s -X POST "$BASE_URL/api/v1/workflows" \
        -H "Authorization: Bearer $TOKEN" \
        -H "Content-Type: application/json" \
        -d '{"caption":"Тестовый workflow","description":"Для тестирования"}')
    WORKFLOW_ID=$(echo "$WORKFLOW_RESPONSE" | grep -o '"id":[0-9]*' | head -1 | grep -o '[0-9]*')
    echo -e "${GREEN}OK${NC} - Создан workflow (ID: $WORKFLOW_ID)"
fi
echo ""

# 5. Создаем тип элемента
echo "5. Создание типа элемента..."
ITEM_TYPE_RESPONSE=$(curl -s -X POST "$BASE_URL/api/v1/item-types" \
    -H "Authorization: Bearer $TOKEN" \
    -H "Content-Type: application/json" \
    -d "{\"workflowId\":$WORKFLOW_ID,\"defaultStateId\":1,\"caption\":\"Тестовый тип\",\"kind\":\"issue\"}")

ITEM_TYPE_ID=$(echo "$ITEM_TYPE_RESPONSE" | grep -o '"id":[0-9]*' | head -1 | grep -o '[0-9]*')

if [ -n "$ITEM_TYPE_ID" ]; then
    echo -e "${GREEN}OK${NC} - Тип элемента создан (ID: $ITEM_TYPE_ID)"
else
    echo -e "${RED}ОШИБКА${NC} - Не удалось создать тип элемента"
    exit 1
fi
echo ""

# 6. Создаем элемент
echo "6. Создание элемента..."
ITEM_RESPONSE=$(curl -s -X POST "$BASE_URL/api/v1/items" \
    -H "Authorization: Bearer $TOKEN" \
    -H "Content-Type: application/json" \
    -d "{\"itemTypeId\":$ITEM_TYPE_ID,\"stateId\":1,\"phaseId\":$PHASE_ID,\"caption\":\"Тестовый элемент\",\"content\":\"Содержимое элемента\"}")

ITEM_ID=$(echo "$ITEM_RESPONSE" | grep -o '"id":[0-9]*' | head -1 | grep -o '[0-9]*')

if [ -n "$ITEM_ID" ]; then
    echo -e "${GREEN}OK${NC} - Элемент создан (ID: $ITEM_ID)"
else
    echo -e "${RED}ОШИБКА${NC} - Не удалось создать элемент"
    exit 1
fi
echo ""

# ============================================================
# 7. Тестирование документов
# ============================================================

echo -e "${CYAN}=== Тестирование документов ===${NC}"
echo ""

# 7.1 Создание документа
echo "7.1 Создание документа..."
DOC_RESPONSE=$(curl -s -X POST "$BASE_URL/api/v1/documents" \
    -H "Authorization: Bearer $TOKEN" \
    -H "Content-Type: application/json" \
    -d "{\"caption\":\"Тестовый документ\",\"path\":\"/tmp/test_doc.pdf\",\"filename\":\"test_doc.pdf\",\"size\":1024,\"mimeType\":\"application/pdf\",\"description\":\"Описание документа\"}")

DOC_ID=$(echo "$DOC_RESPONSE" | grep -o '"id":[0-9]*' | head -1 | grep -o '[0-9]*')

if [ -n "$DOC_ID" ]; then
    echo -e "${GREEN}OK${NC} - Документ создан (ID: $DOC_ID)"
else
    echo -e "${RED}ОШИБКА${NC} - Не удалось создать документ"
    echo "Ответ: $DOC_RESPONSE"
fi
echo ""

# 7.2 Получение списка документов
echo "7.2 Получение списка документов..."
DOC_LIST_RESPONSE=$(curl -s -X GET "$BASE_URL/api/v1/documents" \
    -H "Authorization: Bearer $TOKEN")

DOC_LIST_COUNT=$(echo "$DOC_LIST_RESPONSE" | grep -o '"totalCount":[0-9]*' | grep -o '[0-9]*')

if [ -n "$DOC_LIST_COUNT" ]; then
    echo -e "${GREEN}OK${NC} - Список документов получен (Всего: $DOC_LIST_COUNT)"
else
    echo -e "${RED}ОШИБКА${NC} - Не удалось получить список документов"
fi
echo ""

# 7.3 Получение документа по ID
if [ -n "$DOC_ID" ]; then
    echo "7.3 Получение документа по ID ($DOC_ID)..."
    DOC_GET_RESPONSE=$(curl -s -X GET "$BASE_URL/api/v1/documents/$DOC_ID" \
        -H "Authorization: Bearer $TOKEN")

    DOC_CAPTION=$(echo "$DOC_GET_RESPONSE" | grep -o '"caption":"[^"]*"' | cut -d'"' -f4)

    if [ -n "$DOC_CAPTION" ]; then
        echo -e "${GREEN}OK${NC} - Документ получен (Название: $DOC_CAPTION)"
    else
        echo -e "${RED}ОШИБКА${NC} - Не удалось получить документ"
    fi
    echo ""
fi

# 7.4 Обновление документа
if [ -n "$DOC_ID" ]; then
    echo "7.4 Обновление документа ($DOC_ID)..."
    DOC_UPDATE_RESPONSE=$(curl -s -X PUT "$BASE_URL/api/v1/documents/$DOC_ID" \
        -H "Authorization: Bearer $TOKEN" \
        -H "Content-Type: application/json" \
        -d "{\"caption\":\"Обновленный документ\",\"description\":\"Новое описание\"}")

    DOC_UPDATE_CAPTION=$(echo "$DOC_UPDATE_RESPONSE" | grep -o '"caption":"[^"]*"' | cut -d'"' -f4)

    if [ "$DOC_UPDATE_CAPTION" = "Обновленный документ" ]; then
        echo -e "${GREEN}OK${NC} - Документ обновлен"
    else
        echo -e "${RED}ОШИБКА${NC} - Не удалось обновить документ"
    fi
    echo ""
fi

# 7.5 Создание связи документа с элементом
if [ -n "$DOC_ID" ] && [ -n "$ITEM_ID" ]; then
    echo "7.5 Создание связи документа с элементом..."
    ITEM_DOC_RESPONSE=$(curl -s -X POST "$BASE_URL/api/v1/item-documents" \
        -H "Authorization: Bearer $TOKEN" \
        -H "Content-Type: application/json" \
        -d "{\"itemId\":$ITEM_ID,\"documentId\":$DOC_ID}")

    ITEM_DOC_ID=$(echo "$ITEM_DOC_RESPONSE" | grep -o '"id":[0-9]*' | head -1 | grep -o '[0-9]*')

    if [ -n "$ITEM_DOC_ID" ]; then
        echo -e "${GREEN}OK${NC} - Связь создана (ID: $ITEM_DOC_ID)"
    else
        echo -e "${RED}ОШИБКА${NC} - Не удалось создать связь"
    fi
    echo ""
fi

# 7.6 Получение документов элемента
if [ -n "$ITEM_ID" ]; then
    echo "7.6 Получение документов элемента ($ITEM_ID)..."
    ITEM_DOCS_RESPONSE=$(curl -s -X GET "$BASE_URL/api/v1/items/$ITEM_ID/documents" \
        -H "Authorization: Bearer $TOKEN")

    ITEM_DOCS_COUNT=$(echo "$ITEM_DOCS_RESPONSE" | grep -o '"id"[^}]*' | wc -l)

    echo -e "${GREEN}OK${NC} - Получено документов элемента: $ITEM_DOCS_COUNT"
    echo ""
fi

# 7.7 Получение элементов документа
if [ -n "$DOC_ID" ]; then
    echo "7.7 Получение элементов документа ($DOC_ID)..."
    DOC_ITEMS_RESPONSE=$(curl -s -X GET "$BASE_URL/api/v1/documents/$DOC_ID/items" \
        -H "Authorization: Bearer $TOKEN")

    DOC_ITEMS_COUNT=$(echo "$DOC_ITEMS_RESPONSE" | grep -o '"id"[^}]*' | wc -l)

    echo -e "${GREEN}OK${NC} - Получено элементов документа: $DOC_ITEMS_COUNT"
    echo ""
fi

# ============================================================
# 8. Тестирование комментариев
# ============================================================

echo -e "${CYAN}=== Тестирование комментариев ===${NC}"
echo ""

# 8.1 Создание комментария
echo "8.1 Создание комментария..."
COMMENT_RESPONSE=$(curl -s -X POST "$BASE_URL/api/v1/comments" \
    -H "Authorization: Bearer $TOKEN" \
    -H "Content-Type: application/json" \
    -d "{\"itemId\":$ITEM_ID,\"content\":\"Тестовый комментарий\"}")

COMMENT_ID=$(echo "$COMMENT_RESPONSE" | grep -o '"id":[0-9]*' | head -1 | grep -o '[0-9]*')

if [ -n "$COMMENT_ID" ]; then
    echo -e "${GREEN}OK${NC} - Комментарий создан (ID: $COMMENT_ID)"
else
    echo -e "${RED}ОШИБКА${NC} - Не удалось создать комментарий"
    echo "Ответ: $COMMENT_RESPONSE"
fi
echo ""

# 8.2 Получение списка комментариев
echo "8.2 Получение списка комментариев..."
COMMENT_LIST_RESPONSE=$(curl -s -X GET "$BASE_URL/api/v1/comments" \
    -H "Authorization: Bearer $TOKEN")

COMMENT_LIST_COUNT=$(echo "$COMMENT_LIST_RESPONSE" | grep -o '"totalCount":[0-9]*' | grep -o '[0-9]*')

if [ -n "$COMMENT_LIST_COUNT" ]; then
    echo -e "${GREEN}OK${NC} - Список комментариев получен (Всего: $COMMENT_LIST_COUNT)"
else
    echo -e "${RED}ОШИБКА${NC} - Не удалось получить список комментариев"
fi
echo ""

# 8.3 Получение комментариев элемента
if [ -n "$ITEM_ID" ]; then
    echo "8.3 Получение комментариев элемента ($ITEM_ID)..."
    ITEM_COMMENTS_RESPONSE=$(curl -s -X GET "$BASE_URL/api/v1/items/$ITEM_ID/comments" \
        -H "Authorization: Bearer $TOKEN")

    ITEM_COMMENTS_COUNT=$(echo "$ITEM_COMMENTS_RESPONSE" | grep -o '"id"[^}]*' | wc -l)

    echo -e "${GREEN}OK${NC} - Получено комментариев элемента: $ITEM_COMMENTS_COUNT"
    echo ""
fi

# 8.4 Получение комментария по ID
if [ -n "$COMMENT_ID" ]; then
    echo "8.4 Получение комментария по ID ($COMMENT_ID)..."
    COMMENT_GET_RESPONSE=$(curl -s -X GET "$BASE_URL/api/v1/comments/$COMMENT_ID" \
        -H "Authorization: Bearer $TOKEN")

    COMMENT_CONTENT=$(echo "$COMMENT_GET_RESPONSE" | grep -o '"content":"[^"]*"' | cut -d'"' -f4)

    if [ -n "$COMMENT_CONTENT" ]; then
        echo -e "${GREEN}OK${NC} - Комментарий получен (Содержание: $COMMENT_CONTENT)"
    else
        echo -e "${RED}ОШИБКА${NC} - Не удалось получить комментарий"
    fi
    echo ""
fi

# 8.5 Обновление комментария
if [ -n "$COMMENT_ID" ]; then
    echo "8.5 Обновление комментария ($COMMENT_ID)..."
    COMMENT_UPDATE_RESPONSE=$(curl -s -X PUT "$BASE_URL/api/v1/comments/$COMMENT_ID" \
        -H "Authorization: Bearer $TOKEN" \
        -H "Content-Type: application/json" \
        -d "{\"content\":\"Обновленный комментарий\"}")

    COMMENT_UPDATE_CONTENT=$(echo "$COMMENT_UPDATE_RESPONSE" | grep -o '"content":"[^"]*"' | cut -d'"' -f4)

    if [ "$COMMENT_UPDATE_CONTENT" = "Обновленный комментарий" ]; then
        echo -e "${GREEN}OK${NC} - Комментарий обновлен"
    else
        echo -e "${RED}ОШИБКА${NC} - Не удалось обновить комментарий"
    fi
    echo ""
fi

# 8.6 Создание связи комментария с документом
if [ -n "$COMMENT_ID" ] && [ -n "$DOC_ID" ]; then
    echo "8.6 Создание связи комментария с документом..."
    COMMENT_DOC_RESPONSE=$(curl -s -X POST "$BASE_URL/api/v1/comment-documents" \
        -H "Authorization: Bearer $TOKEN" \
        -H "Content-Type: application/json" \
        -d "{\"commentId\":$COMMENT_ID,\"documentId\":$DOC_ID}")

    COMMENT_DOC_ID=$(echo "$COMMENT_DOC_RESPONSE" | grep -o '"id":[0-9]*' | head -1 | grep -o '[0-9]*')

    if [ -n "$COMMENT_DOC_ID" ]; then
        echo -e "${GREEN}OK${NC} - Связь создана (ID: $COMMENT_DOC_ID)"
    else
        echo -e "${RED}ОШИБКА${NC} - Не удалось создать связь"
    fi
    echo ""
fi

# 8.7 Получение документов комментария
if [ -n "$COMMENT_ID" ]; then
    echo "8.7 Получение документов комментария ($COMMENT_ID)..."
    COMMENT_DOCS_RESPONSE=$(curl -s -X GET "$BASE_URL/api/v1/comments/$COMMENT_ID/documents" \
        -H "Authorization: Bearer $TOKEN")

    COMMENT_DOCS_COUNT=$(echo "$COMMENT_DOCS_RESPONSE" | grep -o '"id"[^}]*' | wc -l)

    echo -e "${GREEN}OK${NC} - Получено документов комментария: $COMMENT_DOCS_COUNT"
    echo ""
fi

# 8.8 Получение комментариев документа
if [ -n "$DOC_ID" ]; then
    echo "8.8 Получение комментариев документа ($DOC_ID)..."
    DOC_COMMENTS_RESPONSE=$(curl -s -X GET "$BASE_URL/api/v1/documents/$DOC_ID/comments" \
        -H "Authorization: Bearer $TOKEN")

    DOC_COMMENTS_COUNT=$(echo "$DOC_COMMENTS_RESPONSE" | grep -o '"id"[^}]*' | wc -l)

    echo -e "${GREEN}OK${NC} - Получено комментариев документа: $DOC_COMMENTS_COUNT"
    echo ""
fi

# ============================================================
# 9. Удаление (очистка)
# ============================================================

echo -e "${CYAN}=== Очистка ===${NC}"
echo ""

# 9.1 Удаление связи комментария с документом
if [ -n "$COMMENT_DOC_ID" ]; then
    echo "9.1 Удаление связи комментария с документом..."
    RESPONSE=$(curl -s -o /dev/null -w "%{http_code}" -X DELETE "$BASE_URL/api/v1/comment-documents/$COMMENT_DOC_ID" \
        -H "Authorization: Bearer $TOKEN")

    if [ "$RESPONSE" = "204" ] || [ "$RESPONSE" = "200" ]; then
        echo -e "${GREEN}OK${NC} - Связь удалена (HTTP $RESPONSE)"
    else
        echo -e "${RED}ОШИБКА${NC} - Не удалось удалить связь (HTTP $RESPONSE)"
    fi
    echo ""
fi

# 9.2 Удаление связи документа с элементом
if [ -n "$ITEM_DOC_ID" ]; then
    echo "9.2 Удаление связи документа с элементом..."
    RESPONSE=$(curl -s -o /dev/null -w "%{http_code}" -X DELETE "$BASE_URL/api/v1/item-documents/$ITEM_DOC_ID" \
        -H "Authorization: Bearer $TOKEN")

    if [ "$RESPONSE" = "204" ] || [ "$RESPONSE" = "200" ]; then
        echo -e "${GREEN}OK${NC} - Связь удалена (HTTP $RESPONSE)"
    else
        echo -e "${RED}ОШИБКА${NC} - Не удалось удалить связь (HTTP $RESPONSE)"
    fi
    echo ""
fi

# 9.3 Удаление комментария
if [ -n "$COMMENT_ID" ]; then
    echo "9.3 Удаление комментария..."
    RESPONSE=$(curl -s -o /dev/null -w "%{http_code}" -X DELETE "$BASE_URL/api/v1/comments/$COMMENT_ID" \
        -H "Authorization: Bearer $TOKEN")

    if [ "$RESPONSE" = "204" ] || [ "$RESPONSE" = "200" ]; then
        echo -e "${GREEN}OK${NC} - Комментарий удален (HTTP $RESPONSE)"
    else
        echo -e "${RED}ОШИБКА${NC} - Не удалось удалить комментарий (HTTP $RESPONSE)"
    fi
    echo ""
fi

# 9.4 Удаление документа
if [ -n "$DOC_ID" ]; then
    echo "9.4 Удаление документа..."
    RESPONSE=$(curl -s -o /dev/null -w "%{http_code}" -X DELETE "$BASE_URL/api/v1/documents/$DOC_ID" \
        -H "Authorization: Bearer $TOKEN")

    if [ "$RESPONSE" = "204" ] || [ "$RESPONSE" = "200" ]; then
        echo -e "${GREEN}OK${NC} - Документ удален (HTTP $RESPONSE)"
    else
        echo -e "${RED}ОШИБКА${NC} - Не удалось удалить документ (HTTP $RESPONSE)"
    fi
    echo ""
fi

# 9.5 Удаление элемента
if [ -n "$ITEM_ID" ]; then
    echo "9.5 Удаление элемента..."
    RESPONSE=$(curl -s -o /dev/null -w "%{http_code}" -X DELETE "$BASE_URL/api/v1/items/$ITEM_ID" \
        -H "Authorization: Bearer $TOKEN")

    if [ "$RESPONSE" = "204" ] || [ "$RESPONSE" = "200" ]; then
        echo -e "${GREEN}OK${NC} - Элемент удален (HTTP $RESPONSE)"
    else
        echo -e "${RED}ОШИБКА${NC} - Не удалось удалить элемент (HTTP $RESPONSE)"
    fi
    echo ""
fi

# 9.6 Удаление фазы
if [ -n "$PHASE_ID" ]; then
    echo "9.6 Удаление фазы..."
    RESPONSE=$(curl -s -o /dev/null -w "%{http_code}" -X DELETE "$BASE_URL/api/v1/phases/$PHASE_ID" \
        -H "Authorization: Bearer $TOKEN")

    if [ "$RESPONSE" = "204" ] || [ "$RESPONSE" = "200" ]; then
        echo -e "${GREEN}OK${NC} - Фаза удалена (HTTP $RESPONSE)"
    else
        echo -e "${RED}ОШИБКА${NC} - Не удалось удалить фазу (HTTP $RESPONSE)"
    fi
    echo ""
fi

# 9.7 Удаление проекта
if [ -n "$PROJECT_ID" ]; then
    echo "9.7 Удаление проекта..."
    RESPONSE=$(curl -s -o /dev/null -w "%{http_code}" -X DELETE "$BASE_URL/api/v1/projects/$PROJECT_ID" \
        -H "Authorization: Bearer $TOKEN")

    if [ "$RESPONSE" = "204" ] || [ "$RESPONSE" = "200" ]; then
        echo -e "${GREEN}OK${NC} - Проект удален (HTTP $RESPONSE)"
    else
        echo -e "${RED}ОШИБКА${NC} - Не удалось удалить проект (HTTP $RESPONSE)"
    fi
    echo ""
fi

echo -e "${GREEN}=== Тестирование завершено ===${NC}"
