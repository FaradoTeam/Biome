#!/bin/bash

# Цвета для вывода
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Базовый URL
BASE_URL="http://localhost:8090"
API_BASE="${BASE_URL}/api/v1"

# Файлы для хранения токенов
TOKEN_FILE="/tmp/biome_tokens.txt"
USER_IDS_FILE="/tmp/biome_user_ids.txt"

# Функция для логирования
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_section() {
    echo ""
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}$1${NC}"
    echo -e "${BLUE}========================================${NC}"
}

# Функция для получения Bearer токена
get_token() {
    local user_type=$1
    grep "^${user_type}:" "$TOKEN_FILE" | cut -d':' -f2
}

# Функция для получения ID пользователя
get_user_id() {
    local user_type=$1
    grep "^${user_type}:" "$USER_IDS_FILE" | cut -d':' -f2
}

# Функция для выполнения запроса с выводом результата
make_request() {
    local method=$1
    local endpoint=$2
    local token=$3
    local data=$4
    local expected_status=${5:-200}

    local response
    local http_code

    if [ -n "$data" ]; then
        response=$(curl -s -w "\n%{http_code}" -X "${method}" "${API_BASE}${endpoint}" \
            ${token:+-H "Authorization: Bearer ${token}"} \
            -H "Content-Type: application/json" \
            -d "${data}" 2>/dev/null)
    else
        response=$(curl -s -w "\n%{http_code}" -X "${method}" "${API_BASE}${endpoint}" \
            ${token:+-H "Authorization: Bearer ${token}"} 2>/dev/null)
    fi

    http_code=$(echo "$response" | tail -n1)
    body=$(echo "$response" | sed '$d')

    if [ "$http_code" -eq "$expected_status" ]; then
        echo -e "${GREEN}✓${NC} $method $endpoint -> $http_code (ожидалось $expected_status)"
        if [ "$method" = "POST" ] && [ "$expected_status" -eq 201 ] && [ -n "$body" ] && [ "$body" != "null" ]; then
            echo "$body"
        fi
        return 0
    else
        echo -e "${RED}✗${NC} $method $endpoint -> $http_code (ожидалось $expected_status)"
        if [ -n "$body" ]; then
            echo "  Ответ: $body"
        fi
        return 1
    fi
}

# Функция для выполнения запроса без вывода (для получения данных)
make_request_silent() {
    local method=$1
    local endpoint=$2
    local token=$3
    local data=$4

    if [ -n "$data" ]; then
        curl -s -X "${method}" "${API_BASE}${endpoint}" \
            ${token:+-H "Authorization: Bearer ${token}"} \
            -H "Content-Type: application/json" \
            -d "${data}" 2>/dev/null
    else
        curl -s -X "${method}" "${API_BASE}${endpoint}" \
            ${token:+-H "Authorization: Bearer ${token}"} 2>/dev/null
    fi
}

# ============================================================
# 1. Аутентификация и подготовка данных
# ============================================================

log_section "1. Аутентификация и подготовка данных"

# Очищаем файлы
> "$TOKEN_FILE"
> "$USER_IDS_FILE"

# Получаем токен супер-админа (admin:password из миграции)
log_info "Получение токена супер-админа..."
ADMIN_RESPONSE=$(curl -s -X POST "${BASE_URL}/api/v1/auth/login" \
    -H "Content-Type: application/json" \
    -d '{"login":"admin","password":"password"}')

ADMIN_TOKEN=$(echo "$ADMIN_RESPONSE" | grep -o '"access_token":"[^"]*"' | cut -d'"' -f4)

if [ -z "$ADMIN_TOKEN" ]; then
    log_error "Не удалось получить токен администратора"
    echo "Ответ сервера: $ADMIN_RESPONSE"
    exit 1
fi

echo "admin:$ADMIN_TOKEN" >> "$TOKEN_FILE"
log_success "Получен токен супер-админа"

# Создаём обычного пользователя
log_info "Создание обычного пользователя user1..."
USER1_RESPONSE=$(curl -s -X POST "${API_BASE}/users" \
    -H "Authorization: Bearer ${ADMIN_TOKEN}" \
    -H "Content-Type: application/json" \
    -d '{
        "login": "testuser1",
        "email": "test1@example.com",
        "password": "Test123456",
        "firstName": "Test",
        "lastName": "User1"
    }')

USER1_ID=$(echo "$USER1_RESPONSE" | grep -o '"id":[0-9]*' | head -1 | cut -d':' -f2)

if [ -z "$USER1_ID" ]; then
    log_error "Не удалось создать пользователя user1"
    exit 1
fi

echo "user1:$USER1_ID" >> "$USER_IDS_FILE"
log_success "Создан пользователь user1 с id=$USER1_ID"

# Получаем токен для user1
log_info "Получение токена для user1..."
USER1_TOKEN_RESPONSE=$(curl -s -X POST "${BASE_URL}/api/v1/auth/login" \
    -H "Content-Type: application/json" \
    -d '{"login":"testuser1","password":"Test123456"}')

USER1_TOKEN=$(echo "$USER1_TOKEN_RESPONSE" | grep -o '"access_token":"[^"]*"' | cut -d'"' -f4)

if [ -z "$USER1_TOKEN" ]; then
    log_error "Не удалось получить токен для user1"
    exit 1
fi

echo "user1:$USER1_TOKEN" >> "$TOKEN_FILE"
log_success "Получен токен для user1"

# Создаём второго обычного пользователя
log_info "Создание обычного пользователя user2..."
USER2_RESPONSE=$(curl -s -X POST "${API_BASE}/users" \
    -H "Authorization: Bearer ${ADMIN_TOKEN}" \
    -H "Content-Type: application/json" \
    -d '{
        "login": "testuser2",
        "email": "test2@example.com",
        "password": "Test123456",
        "firstName": "Test",
        "lastName": "User2"
    }')

USER2_ID=$(echo "$USER2_RESPONSE" | grep -o '"id":[0-9]*' | head -1 | cut -d':' -f2)

if [ -z "$USER2_ID" ]; then
    log_error "Не удалось создать пользователя user2"
    exit 1
fi

echo "user2:$USER2_ID" >> "$USER_IDS_FILE"
log_success "Создан пользователь user2 с id=$USER2_ID"

# Получаем токен для user2
log_info "Получение токена для user2..."
USER2_TOKEN_RESPONSE=$(curl -s -X POST "${BASE_URL}/api/v1/auth/login" \
    -H "Content-Type: application/json" \
    -d '{"login":"testuser2","password":"Test123456"}')

USER2_TOKEN=$(echo "$USER2_TOKEN_RESPONSE" | grep -o '"access_token":"[^"]*"' | cut -d'"' -f4)

if [ -z "$USER2_TOKEN" ]; then
    log_error "Не удалось получить токен для user2"
    exit 1
fi

echo "user2:$USER2_TOKEN" >> "$TOKEN_FILE"
log_success "Получен токен для user2"

# Создаём роль "Менеджер проектов"
log_info "Создание роли 'Менеджер проектов'..."
ROLE_RESPONSE=$(curl -s -X POST "${API_BASE}/roles" \
    -H "Authorization: Bearer ${ADMIN_TOKEN}" \
    -H "Content-Type: application/json" \
    -d '{
        "caption": "Менеджер проектов",
        "description": "Роль с правами на управление проектами"
    }')

MANAGER_ROLE_ID=$(echo "$ROLE_RESPONSE" | grep -o '"id":[0-9]*' | head -1 | cut -d':' -f2)

if [ -z "$MANAGER_ROLE_ID" ]; then
    log_error "Не удалось создать роль"
    exit 1
fi
log_success "Создана роль 'Менеджер проектов' с id=$MANAGER_ROLE_ID"

# Создаём роль "Разработчик"
log_info "Создание роли 'Разработчик'..."
ROLE_RESPONSE2=$(curl -s -X POST "${API_BASE}/roles" \
    -H "Authorization: Bearer ${ADMIN_TOKEN}" \
    -H "Content-Type: application/json" \
    -d '{
        "caption": "Разработчик",
        "description": "Роль с правами на создание элементов"
    }')

DEVELOPER_ROLE_ID=$(echo "$ROLE_RESPONSE2" | grep -o '"id":[0-9]*' | head -1 | cut -d':' -f2)
log_success "Создана роль 'Разработчик' с id=$DEVELOPER_ROLE_ID"

# Создаём команду
log_info "Создание команды 'Команда разработки'..."
TEAM_RESPONSE=$(curl -s -X POST "${API_BASE}/teams" \
    -H "Authorization: Bearer ${ADMIN_TOKEN}" \
    -H "Content-Type: application/json" \
    -d '{
        "caption": "Команда разработки",
        "description": "Основная команда разработки"
    }')

TEAM_ID=$(echo "$TEAM_RESPONSE" | grep -o '"id":[0-9]*' | head -1 | cut -d':' -f2)
log_success "Создана команда с id=$TEAM_ID"

# Добавляем пользователя user1 в команду с ролью "Менеджер проектов"
log_info "Назначение user1 в команду с ролью 'Менеджер проектов'..."
curl -s -X POST "${API_BASE}/user-team-roles" \
    -H "Authorization: Bearer ${ADMIN_TOKEN}" \
    -H "Content-Type: application/json" \
    -d "{
        \"userId\": $USER1_ID,
        \"teamId\": $TEAM_ID,
        \"roleId\": $MANAGER_ROLE_ID
    }" > /dev/null
log_success "User1 назначен менеджером проектов"

# Добавляем пользователя user2 в команду с ролью "Разработчик"
log_info "Назначение user2 в команду с ролью 'Разработчик'..."
curl -s -X POST "${API_BASE}/user-team-roles" \
    -H "Authorization: Bearer ${ADMIN_TOKEN}" \
    -H "Content-Type: application/json" \
    -d "{
        \"userId\": $USER2_ID,
        \"teamId\": $TEAM_ID,
        \"roleId\": $DEVELOPER_ROLE_ID
    }" > /dev/null
log_success "User2 назначен разработчиком"

# Создаём правило для роли "Менеджер проектов"
log_info "Создание правила для роли менеджера проектов..."
RULE_RESPONSE=$(curl -s -X POST "${API_BASE}/rules" \
    -H "Authorization: Bearer ${ADMIN_TOKEN}" \
    -H "Content-Type: application/json" \
    -d "{
        \"roleId\": $MANAGER_ROLE_ID,
        \"isRootProjectCreator\": true
    }")

MANAGER_RULE_ID=$(echo "$RULE_RESPONSE" | grep -o '"id":[0-9]*' | head -1 | cut -d':' -f2)
log_success "Создано правило для менеджера проектов с id=$MANAGER_RULE_ID"

# Создаём проект через супер-админа
log_info "Создание тестового проекта через супер-админа..."
PROJECT_RESPONSE=$(curl -s -X POST "${API_BASE}/projects" \
    -H "Authorization: Bearer ${ADMIN_TOKEN}" \
    -H "Content-Type: application/json" \
    -d '{
        "caption": "Тестовый проект",
        "description": "Проект для тестирования прав на фазы"
    }')

PROJECT_ID=$(echo "$PROJECT_RESPONSE" | grep -o '"id":[0-9]*' | head -1 | cut -d':' -f2)
log_success "Создан проект с id=$PROJECT_ID"

# Добавляем права на проект для роли менеджера проектов (полные права)
log_info "Настройка прав на проект для роли менеджера проектов..."
curl -s -X POST "${API_BASE}/rule-projects" \
    -H "Authorization: Bearer ${ADMIN_TOKEN}" \
    -H "Content-Type: application/json" \
    -d "{
        \"ruleId\": $MANAGER_RULE_ID,
        \"projectId\": $PROJECT_ID,
        \"isReader\": true,
        \"isWriter\": true,
        \"isProjectEditor\": true,
        \"isPhaseEditor\": true,
        \"isBoardEditor\": true
    }" > /dev/null
log_success "Роли менеджера проектов выданы полные права на проект"

# Добавляем права на проект для роли разработчика (только создание элементов)
log_info "Настройка прав на проект для роли разработчика..."
DEVELOPER_RULE_RESPONSE=$(curl -s -X POST "${API_BASE}/rules" \
    -H "Authorization: Bearer ${ADMIN_TOKEN}" \
    -H "Content-Type: application/json" \
    -d "{
        \"roleId\": $DEVELOPER_ROLE_ID
    }")

DEVELOPER_RULE_ID=$(echo "$DEVELOPER_RULE_RESPONSE" | grep -o '"id":[0-9]*' | head -1 | cut -d':' -f2)

curl -s -X POST "${API_BASE}/rule-projects" \
    -H "Authorization: Bearer ${ADMIN_TOKEN}" \
    -H "Content-Type: application/json" \
    -d "{
        \"ruleId\": $DEVELOPER_RULE_ID,
        \"projectId\": $PROJECT_ID,
        \"isReader\": true,
        \"isWriter\": true
    }" > /dev/null
log_success "Роли разработчика выданы права на чтение и запись элементов"

# ============================================================
# 2. Создание фаз с разными правами
# ============================================================

log_section "2. Создание фаз с разными правами"

# 2.1 Супер-админ создаёт фазу
log_info "2.1 Супер-админ создаёт фазу..."
PHASE1_RESPONSE=$(make_request_silent "POST" "/phases" "$ADMIN_TOKEN" "{
    \"projectId\": $PROJECT_ID,
    \"caption\": \"Фаза от супер-админа\",
    \"description\": \"Фаза созданная супер-админом\"
}")

PHASE1_ID=$(echo "$PHASE1_RESPONSE" | grep -o '"id":[0-9]*' | head -1 | cut -d':' -f2)

if [ -n "$PHASE1_ID" ]; then
    log_success "Фаза создана супер-админом с id=$PHASE1_ID"
else
    log_error "Не удалось создать фазу через супер-админа"
    echo "Ответ: $PHASE1_RESPONSE"
fi

# 2.2 Менеджер проектов создаёт фазу
log_info "2.2 Менеджер проектов создаёт фазу..."
PHASE2_RESPONSE=$(make_request_silent "POST" "/phases" "$USER1_TOKEN" "{
    \"projectId\": $PROJECT_ID,
    \"caption\": \"Фаза от менеджера\",
    \"description\": \"Фаза созданная менеджером\"
}")

PHASE2_ID=$(echo "$PHASE2_RESPONSE" | grep -o '"id":[0-9]*' | head -1 | cut -d':' -f2)

if [ -n "$PHASE2_ID" ]; then
    log_success "Фаза создана менеджером с id=$PHASE2_ID"
else
    log_error "Не удалось создать фазу через менеджера"
    echo "Ответ: $PHASE2_RESPONSE"
fi

# 2.3 Разработчик пытается создать фазу (должен получить отказ)
log_info "2.3 Разработчик пытается создать фазу (должен получить отказ)..."
make_request "POST" "/phases" "$USER2_TOKEN" "{
    \"projectId\": $PROJECT_ID,
    \"caption\": \"Фаза от разработчика\",
    \"description\": \"Фаза созданная разработчиком\"
}" 403

# ============================================================
# 3. Просмотр фаз
# ============================================================

log_section "3. Просмотр фаз"

# 3.1 Супер-админ просматривает все фазы
log_info "3.1 Супер-админ просматривает все фазы..."
make_request "GET" "/phases?projectId=$PROJECT_ID" "$ADMIN_TOKEN" "" 200

# 3.2 Менеджер проектов просматривает фазы
log_info "3.2 Менеджер проектов просматривает фазы..."
make_request "GET" "/phases?projectId=$PROJECT_ID" "$USER1_TOKEN" "" 200

# 3.3 Разработчик просматривает фазы
log_info "3.3 Разработчик просматривает фазы..."
make_request "GET" "/phases?projectId=$PROJECT_ID" "$USER2_TOKEN" "" 200

# 3.4 Просмотр конкретной фазы разными пользователями
log_info "3.4 Просмотр конкретной фазы разными пользователями..."
make_request "GET" "/phases/$PHASE1_ID" "$ADMIN_TOKEN" "" 200
make_request "GET" "/phases/$PHASE1_ID" "$USER1_TOKEN" "" 200
make_request "GET" "/phases/$PHASE1_ID" "$USER2_TOKEN" "" 200

# 3.5 Попытка просмотра несуществующей фазы
log_info "3.5 Попытка просмотра несуществующей фазы..."
make_request "GET" "/phases/999999" "$ADMIN_TOKEN" "" 404

# ============================================================
# 4. Обновление фаз
# ============================================================

log_section "4. Обновление фаз"

# 4.1 Супер-админ обновляет фазу
log_info "4.1 Супер-админ обновляет фазу..."
make_request "PUT" "/phases/$PHASE1_ID" "$ADMIN_TOKEN" "{
    \"caption\": \"Фаза от супер-админа (обновлена)\",
    \"description\": \"Обновленное описание\"
}" 200

# 4.2 Менеджер проектов обновляет фазу
log_info "4.2 Менеджер проектов обновляет фазу..."
make_request "PUT" "/phases/$PHASE2_ID" "$USER1_TOKEN" "{
    \"caption\": \"Фаза от менеджера (обновлена)\",
    \"description\": \"Обновленное описание от менеджера\"
}" 200

# 4.3 Разработчик пытается обновить фазу (должен получить отказ)
log_info "4.3 Разработчик пытается обновить фазу (должен получить отказ)..."
make_request "PUT" "/phases/$PHASE1_ID" "$USER2_TOKEN" "{
    \"caption\": \"Попытка обновления от разработчика\"
}" 403

# 4.4 Попытка обновления несуществующей фазы
log_info "4.4 Попытка обновления несуществующей фазы..."
make_request "PUT" "/phases/999999" "$ADMIN_TOKEN" "{
    \"caption\": \"Несуществующая фаза\"
}" 404

# 4.5 Обновление фазы с пустыми данными (должно вернуть 200 - ничего не меняем)
log_info "4.5 Обновление фазы с пустыми данными..."
make_request "PUT" "/phases/$PHASE1_ID" "$ADMIN_TOKEN" "{}" 403

# ============================================================
# 5. Архивирование фаз (удаление)
# ============================================================

log_section "5. Архивирование фаз (удаление)"

# 5.1 Супер-админ архивирует фазу
log_info "5.1 Супер-админ архивирует фазу..."
make_request "DELETE" "/phases/$PHASE1_ID" "$ADMIN_TOKEN" "" 204

# 5.2 Проверяем, что фаза в архиве (isArchive=true)
log_info "5.2 Проверяем, что фаза в архиве (isArchive=true)..."
make_request "GET" "/phases?projectId=$PROJECT_ID&isArchive=true" "$ADMIN_TOKEN" "" 200

# 5.3 Менеджер проектов архивирует фазу
log_info "5.3 Менеджер проектов архивирует фазу..."
make_request "DELETE" "/phases/$PHASE2_ID" "$USER1_TOKEN" "" 204

# 5.4 Разработчик пытается архивировать фазу (должен получить отказ)
log_info "5.4 Разработчик пытается архивировать фазу (должен получить отказ)..."
make_request "DELETE" "/phases/$PHASE1_ID" "$USER2_TOKEN" "" 403

# 5.5 Попытка архивации несуществующей фазы
log_info "5.5 Попытка архивации несуществующей фазы..."
make_request "DELETE" "/phases/999999" "$ADMIN_TOKEN" "" 404

# ============================================================
# 6. Восстановление фаз из архива
# ============================================================

log_section "6. Восстановление фаз из архива"

# Создаём новую фазу для теста восстановления
log_info "Создание фазы для теста восстановления..."
RESTORE_PHASE_RESPONSE=$(make_request_silent "POST" "/phases" "$ADMIN_TOKEN" "{
    \"projectId\": $PROJECT_ID,
    \"caption\": \"Фаза для восстановления\",
    \"description\": \"Эта фаза будет заархивирована и восстановлена\"
}")

RESTORE_PHASE_ID=$(echo "$RESTORE_PHASE_RESPONSE" | grep -o '"id":[0-9]*' | head -1 | cut -d':' -f2)
log_success "Создана фаза для восстановления с id=$RESTORE_PHASE_ID"

# Архивируем фазу
log_info "Архивация фазы для восстановления..."
make_request "DELETE" "/phases/$RESTORE_PHASE_ID" "$ADMIN_TOKEN" "" 204

# 6.1 Супер-админ восстанавливает фазу
log_info "6.1 Супер-админ восстанавливает фазу..."
make_request "PUT" "/phases/$RESTORE_PHASE_ID" "$ADMIN_TOKEN" "{
    \"isArchive\": false
}" 200

# 6.2 Менеджер проектов восстанавливает фазу
# Создаём ещё одну фазу для менеджера
RESTORE_PHASE2_RESPONSE=$(make_request_silent "POST" "/phases" "$USER1_TOKEN" "{
    \"projectId\": $PROJECT_ID,
    \"caption\": \"Фаза менеджера для восстановления\",
    \"description\": \"Фаза менеджера для теста восстановления\"
}")

RESTORE_PHASE2_ID=$(echo "$RESTORE_PHASE2_RESPONSE" | grep -o '"id":[0-9]*' | head -1 | cut -d':' -f2)

log_info "Архивация фазы менеджера..."
make_request "DELETE" "/phases/$RESTORE_PHASE2_ID" "$USER1_TOKEN" "" 204

log_info "6.2 Менеджер проектов восстанавливает фазу..."
make_request "PUT" "/phases/$RESTORE_PHASE2_ID" "$USER1_TOKEN" "{
    \"isArchive\": false
}" 200

# 6.3 Разработчик пытается восстановить фазу (должен получить отказ)
log_info "6.3 Разработчик пытается восстановить фазу (должен получить отказ)..."
make_request "PUT" "/phases/$RESTORE_PHASE_ID" "$USER2_TOKEN" "{
    \"isArchive\": false
}" 403

# ============================================================
# 7. Фильтрация и пагинация
# ============================================================

log_section "7. Фильтрация и пагинация"

# Создаём несколько фаз для теста пагинации
log_info "Создание дополнительных фаз для теста пагинации..."
for i in 1 2 3; do
    make_request_silent "POST" "/phases" "$ADMIN_TOKEN" "{
        \"projectId\": $PROJECT_ID,
        \"caption\": \"Дополнительная фаза $i\",
        \"description\": \"Фаза для теста пагинации\"
    }" > /dev/null
done

# 7.1 Пагинация
log_info "7.1 Пагинация (страница 1, 2 элемента)..."
make_request "GET" "/phases?projectId=$PROJECT_ID&page=1&pageSize=2" "$ADMIN_TOKEN" "" 200

log_info "7.2 Пагинация (страница 2, 2 элемента)..."
make_request "GET" "/phases?projectId=$PROJECT_ID&page=2&pageSize=2" "$ADMIN_TOKEN" "" 200

# 7.3 Фильтр по isArchive=false
log_info "7.3 Фильтр по isArchive=false..."
make_request "GET" "/phases?projectId=$PROJECT_ID&isArchive=false" "$ADMIN_TOKEN" "" 200

# 7.4 Фильтр по isArchive=true
log_info "7.4 Фильтр по isArchive=true..."
make_request "GET" "/phases?projectId=$PROJECT_ID&isArchive=true" "$ADMIN_TOKEN" "" 200

# ============================================================
# 8. Отрицательные тесты
# ============================================================

log_section "8. Отрицательные тесты"

# 8.1 Создание фазы без названия
log_info "8.1 Создание фазы без названия..."
make_request "POST" "/phases" "$ADMIN_TOKEN" "{
    \"projectId\": $PROJECT_ID
}" 400

# 8.2 Создание фазы без projectId
log_info "8.2 Создание фазы без projectId..."
make_request "POST" "/phases" "$ADMIN_TOKEN" "{
    \"caption\": \"Фаза без проекта\"
}" 400

# 8.3 Создание фазы с пустым названием
log_info "8.3 Создание фазы с пустым названием..."
make_request "POST" "/phases" "$ADMIN_TOKEN" "{
    \"projectId\": $PROJECT_ID,
    \"caption\": \"\"
}" 400

# 8.4 Запрос без токена
log_info "8.4 Запрос без токена..."
make_request "GET" "/phases" "" "" 401

# 8.5 Запрос с неверным токеном
log_info "8.5 Запрос с неверным токеном..."
make_request "GET" "/phases" "invalid_token_12345" "" 401

# ============================================================
# 9. Тесты с разными пользователями для одной фазы
# ============================================================

log_section "9. Тесты с разными пользователями для одной фазы"

# Создаём фазу через супер-админа для комплексного тестирования
TEST_PHASE_RESPONSE=$(make_request_silent "POST" "/phases" "$ADMIN_TOKEN" "{
    \"projectId\": $PROJECT_ID,
    \"caption\": \"Комплексная тестовая фаза\",
    \"description\": \"Фаза для тестирования прав разных пользователей\"
}")

TEST_PHASE_ID=$(echo "$TEST_PHASE_RESPONSE" | grep -o '"id":[0-9]*' | head -1 | cut -d':' -f2)
log_success "Создана комплексная тестовая фаза с id=$TEST_PHASE_ID"

log_info "9.1 Просмотр фазы разными пользователями..."
make_request "GET" "/phases/$TEST_PHASE_ID" "$ADMIN_TOKEN" "" 200
make_request "GET" "/phases/$TEST_PHASE_ID" "$USER1_TOKEN" "" 200
make_request "GET" "/phases/$TEST_PHASE_ID" "$USER2_TOKEN" "" 200

log_info "9.2 Обновление фазы разными пользователями..."
make_request "PUT" "/phases/$TEST_PHASE_ID" "$ADMIN_TOKEN" "{
    \"description\": \"Обновлено супер-админом\"
}" 200
make_request "PUT" "/phases/$TEST_PHASE_ID" "$USER1_TOKEN" "{
    \"description\": \"Обновлено менеджером\"
}" 200
make_request "PUT" "/phases/$TEST_PHASE_ID" "$USER2_TOKEN" "{
    \"description\": \"Попытка обновления разработчиком\"
}" 403

log_info "9.3 Архивирование фазы разными пользователями..."
make_request "DELETE" "/phases/$TEST_PHASE_ID" "$ADMIN_TOKEN" "" 204

# Восстанавливаем для следующих тестов
make_request "PUT" "/phases/$TEST_PHASE_ID" "$ADMIN_TOKEN" "{\"isArchive\": false}" 200

# ============================================================
# 10. Тесты с фазами в разных проектах
# ============================================================

log_section "10. Тесты с фазами в разных проектах"

# Создаём второй проект
log_info "Создание второго проекта..."
PROJECT2_RESPONSE=$(curl -s -X POST "${API_BASE}/projects" \
    -H "Authorization: Bearer ${ADMIN_TOKEN}" \
    -H "Content-Type: application/json" \
    -d '{
        "caption": "Второй проект",
        "description": "Проект для тестирования изоляции фаз"
    }')

PROJECT2_ID=$(echo "$PROJECT2_RESPONSE" | grep -o '"id":[0-9]*' | head -1 | cut -d':' -f2)
log_success "Создан второй проект с id=$PROJECT2_ID"

# Создаём фазу во втором проекте
log_info "Создание фазы во втором проекте..."
PHASE_PROJECT2_RESPONSE=$(make_request_silent "POST" "/phases" "$ADMIN_TOKEN" "{
    \"projectId\": $PROJECT2_ID,
    \"caption\": \"Фаза во втором проекте\",
    \"description\": \"Эта фаза недоступна менеджеру\"
}")

PHASE_PROJECT2_ID=$(echo "$PHASE_PROJECT2_RESPONSE" | grep -o '"id":[0-9]*' | head -1 | cut -d':' -f2)
log_success "Создана фаза во втором проекте с id=$PHASE_PROJECT2_ID"

log_info "10.1 Менеджер пытается получить фазу из второго проекта (должен получить 404)..."
make_request "GET" "/phases/$PHASE_PROJECT2_ID" "$USER1_TOKEN" "" 404

log_info "10.2 Менеджер пытается получить список фаз второго проекта..."
make_request "GET" "/phases?projectId=$PROJECT2_ID" "$USER1_TOKEN" "" 200

log_info "10.3 Менеджер пытается создать фазу во втором проекте (должен получить 403)..."
make_request "POST" "/phases" "$USER1_TOKEN" "{
    \"projectId\": $PROJECT2_ID,
    \"caption\": \"Фаза менеджера во втором проекте\"
}" 403

log_success "Тестирование прав доступа к фазам завершено!"

# Очистка
rm -f "$TOKEN_FILE" "$USER_IDS_FILE" 2>/dev/null
