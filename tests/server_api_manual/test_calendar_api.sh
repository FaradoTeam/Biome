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
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${CYAN}=== Тестирование API календаря ===${NC}"
echo ""

# 1. Получаем токен
echo -e "${BLUE}1. Авторизация...${NC}"
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

# ============================================================
# 2. Тестирование стандартных дней (Standard Days)
# ============================================================

echo -e "${CYAN}=== Тестирование стандартных дней ===${NC}"
echo ""

# 2.1 Получение всех стандартных дней
echo -e "${BLUE}2.1 Получение всех стандартных дней...${NC}"
STANDARD_DAYS_RESPONSE=$(curl -s -X GET "$BASE_URL/api/v1/standard-days" \
    -H "Authorization: Bearer $TOKEN")

STANDARD_DAYS_COUNT=$(echo "$STANDARD_DAYS_RESPONSE" | grep -o '"weekDayNumber"' | wc -l)

if [ "$STANDARD_DAYS_COUNT" -ge 7 ]; then
    echo -e "${GREEN}OK${NC} - Получено стандартных дней: $STANDARD_DAYS_COUNT"
else
    echo -e "${RED}ОШИБКА${NC} - Ожидалось минимум 7 дней, получено: $STANDARD_DAYS_COUNT"
fi
echo ""

# 2.2 Получение конкретного дня недели
echo -e "${BLUE}2.2 Получение стандартного дня (понедельник, weekDayNumber=1)...${NC}"
MONDAY_RESPONSE=$(curl -s -X GET "$BASE_URL/api/v1/standard-days/1" \
    -H "Authorization: Bearer $TOKEN")

MONDAY_IS_WORK=$(echo "$MONDAY_RESPONSE" | grep -o '"isWorkDay":[^,}]*' | cut -d':' -f2)

if [ -n "$MONDAY_IS_WORK" ]; then
    if [ "$MONDAY_IS_WORK" = "true" ]; then
        echo -e "${GREEN}OK${NC} - Понедельник - рабочий день"
    else
        echo -e "${YELLOW}WARNING${NC} - Понедельник - выходной день"
    fi
else
    echo -e "${RED}ОШИБКА${NC} - Не удалось получить информацию о понедельнике"
fi
echo ""

# 2.3 Получение выходного дня
echo -e "${BLUE}2.3 Получение стандартного дня (воскресенье, weekDayNumber=0)...${NC}"
SUNDAY_RESPONSE=$(curl -s -X GET "$BASE_URL/api/v1/standard-days/0" \
    -H "Authorization: Bearer $TOKEN")

SUNDAY_IS_WORK=$(echo "$SUNDAY_RESPONSE" | grep -o '"isWorkDay":[^,}]*' | cut -d':' -f2)

if [ -n "$SUNDAY_IS_WORK" ]; then
    if [ "$SUNDAY_IS_WORK" = "false" ]; then
        echo -e "${GREEN}OK${NC} - Воскресенье - выходной день"
    else
        echo -e "${YELLOW}WARNING${NC} - Воскресенье - рабочий день"
    fi
else
    echo -e "${RED}ОШИБКА${NC} - Не удалось получить информацию о воскресенье"
fi
echo ""

# 2.4 Обновление стандартного дня (сделать пятницу выходным)
echo -e "${BLUE}2.4 Обновление стандартного дня (пятница -> выходной)...${NC}"
UPDATE_STANDARD_RESPONSE=$(curl -s -o /dev/null -w "%{http_code}" -X PUT "$BASE_URL/api/v1/standard-days/5" \
    -H "Authorization: Bearer $TOKEN" \
    -H "Content-Type: application/json" \
    -d '{"isWorkDay":false}')

if [ "$UPDATE_STANDARD_RESPONSE" = "204" ] || [ "$UPDATE_STANDARD_RESPONSE" = "200" ]; then
    echo -e "${GREEN}OK${NC} - Стандартный день обновлен (HTTP $UPDATE_STANDARD_RESPONSE)"
    
    # Проверяем, что пятница стала выходной
    FRIDAY_RESPONSE=$(curl -s -X GET "$BASE_URL/api/v1/standard-days/5" \
        -H "Authorization: Bearer $TOKEN")
    FRIDAY_IS_WORK=$(echo "$FRIDAY_RESPONSE" | grep -o '"isWorkDay":[^,}]*' | cut -d':' -f2)
    
    if [ "$FRIDAY_IS_WORK" = "false" ]; then
        echo -e "${GREEN}  OK${NC} - Пятница теперь выходной день ✓"
    else
        echo -e "${RED}  ОШИБКА${NC} - Пятница не стала выходным днем"
    fi
else
    echo -e "${RED}ОШИБКА${NC} - Не удалось обновить стандартный день (HTTP $UPDATE_STANDARD_RESPONSE)"
fi
echo ""

# 2.5 Восстановление пятницы как рабочего дня
echo -e "${BLUE}2.5 Восстановление пятницы как рабочего дня...${NC}"
RESTORE_STANDARD_RESPONSE=$(curl -s -o /dev/null -w "%{http_code}" -X PUT "$BASE_URL/api/v1/standard-days/5" \
    -H "Authorization: Bearer $TOKEN" \
    -H "Content-Type: application/json" \
    -d '{"isWorkDay":true,"beginWorkTime":"09:00","endWorkTime":"18:00","breakDuration":60}')

if [ "$RESTORE_STANDARD_RESPONSE" = "204" ] || [ "$RESTORE_STANDARD_RESPONSE" = "200" ]; then
    echo -e "${GREEN}OK${NC} - Пятница восстановлена как рабочий день (HTTP $RESTORE_STANDARD_RESPONSE)"
else
    echo -e "${RED}ОШИБКА${NC} - Не удалось восстановить пятницу (HTTP $RESTORE_STANDARD_RESPONSE)"
fi
echo ""

# ============================================================
# 3. Тестирование особых дней (Special Days)
# ============================================================

echo -e "${CYAN}=== Тестирование особых дней ===${NC}"
echo ""

# 3.1 Создание особого дня (Новый год)
echo -e "${BLUE}3.1 Создание особого дня (Новый год, 1 января 2024)...${NC}"
DATE_2024_01_01=$(date -d "2024-01-01 00:00:00" +%s 2>/dev/null || echo "1704067200")
SPECIAL_DAY_RESPONSE=$(curl -s -X POST "$BASE_URL/api/v1/special-days" \
    -H "Authorization: Bearer $TOKEN" \
    -H "Content-Type: application/json" \
    -d "{\"date\":$DATE_2024_01_01,\"isWorkDay\":false}")

SPECIAL_DAY_ID=$(echo "$SPECIAL_DAY_RESPONSE" | grep -o '"id":[0-9]*' | head -1 | grep -o '[0-9]*')

if [ -n "$SPECIAL_DAY_ID" ]; then
    echo -e "${GREEN}OK${NC} - Особый день создан (ID: $SPECIAL_DAY_ID)"
else
    echo -e "${RED}ОШИБКА${NC} - Не удалось создать особый день"
    echo "Ответ: $SPECIAL_DAY_RESPONSE"
fi
echo ""

# 3.2 Создание особого дня (8 марта - сокращенный день)
echo -e "${BLUE}3.2 Создание особого дня (8 марта, сокращенный день)...${NC}"
DATE_2024_03_08=$(date -d "2024-03-08 00:00:00" +%s 2>/dev/null || echo "1709856000")
SPECIAL_DAY_2_RESPONSE=$(curl -s -X POST "$BASE_URL/api/v1/special-days" \
    -H "Authorization: Bearer $TOKEN" \
    -H "Content-Type: application/json" \
    -d "{\"date\":$DATE_2024_03_08,\"isWorkDay\":true,\"beginWorkTime\":\"09:00\",\"endWorkTime\":\"15:00\",\"breakDuration\":30}")

SPECIAL_DAY_2_ID=$(echo "$SPECIAL_DAY_2_RESPONSE" | grep -o '"id":[0-9]*' | head -1 | grep -o '[0-9]*')

if [ -n "$SPECIAL_DAY_2_ID" ]; then
    echo -e "${GREEN}OK${NC} - Особый день создан (ID: $SPECIAL_DAY_2_ID)"
else
    echo -e "${RED}ОШИБКА${NC} - Не удалось создать особый день"
fi
echo ""

# 3.3 Получение списка особых дней
echo -e "${BLUE}3.3 Получение списка особых дней...${NC}"
SPECIAL_DAYS_LIST=$(curl -s -X GET "$BASE_URL/api/v1/special-days" \
    -H "Authorization: Bearer $TOKEN")

SPECIAL_DAYS_COUNT=$(echo "$SPECIAL_DAYS_LIST" | grep -o '"totalCount":[0-9]*' | grep -o '[0-9]*')

if [ -n "$SPECIAL_DAYS_COUNT" ]; then
    echo -e "${GREEN}OK${NC} - Получено особых дней: $SPECIAL_DAYS_COUNT"
else
    echo -e "${RED}ОШИБКА${NC} - Не удалось получить список особых дней"
fi
echo ""

# 3.4 Получение особого дня по ID
if [ -n "$SPECIAL_DAY_ID" ]; then
    echo -e "${BLUE}3.4 Получение особого дня по ID ($SPECIAL_DAY_ID)...${NC}"
    SPECIAL_DAY_GET=$(curl -s -X GET "$BASE_URL/api/v1/special-days/$SPECIAL_DAY_ID" \
        -H "Authorization: Bearer $TOKEN")

    SPECIAL_DAY_IS_WORK=$(echo "$SPECIAL_DAY_GET" | grep -o '"isWorkDay":[^,}]*' | cut -d':' -f2)

    if [ "$SPECIAL_DAY_IS_WORK" = "false" ]; then
        echo -e "${GREEN}OK${NC} - Особый день получен, isWorkDay=false"
    else
        echo -e "${RED}ОШИБКА${NC} - Неправильный статус дня"
    fi
    echo ""
fi

# 3.5 Фильтрация особых дней по году
echo -e "${BLUE}3.5 Фильтрация особых дней по году (2024)...${NC}"
SPECIAL_DAYS_2024=$(curl -s -X GET "$BASE_URL/api/v1/special-days?year=2024" \
    -H "Authorization: Bearer $TOKEN")

SPECIAL_DAYS_2024_COUNT=$(echo "$SPECIAL_DAYS_2024" | grep -o '"totalCount":[0-9]*' | grep -o '[0-9]*')

if [ -n "$SPECIAL_DAYS_2024_COUNT" ] && [ "$SPECIAL_DAYS_2024_COUNT" -gt 0 ]; then
    echo -e "${GREEN}OK${NC} - Найдено особых дней в 2024 году: $SPECIAL_DAYS_2024_COUNT"
else
    echo -e "${YELLOW}WARNING${NC} - Не найдено особых дней в 2024 году"
fi
echo ""

# 3.6 Обновление особого дня
if [ -n "$SPECIAL_DAY_ID" ]; then
    echo -e "${BLUE}3.6 Обновление особого дня (сделать рабочим)...${NC}"
    UPDATE_SPECIAL_RESPONSE=$(curl -s -X PUT "$BASE_URL/api/v1/special-days/$SPECIAL_DAY_ID" \
        -H "Authorization: Bearer $TOKEN" \
        -H "Content-Type: application/json" \
        -d "{\"isWorkDay\":true,\"beginWorkTime\":\"10:00\",\"endWorkTime\":\"14:00\",\"breakDuration\":45}")

    UPDATE_SPECIAL_IS_WORK=$(echo "$UPDATE_SPECIAL_RESPONSE" | grep -o '"isWorkDay":[^,}]*' | cut -d':' -f2)

    if [ "$UPDATE_SPECIAL_IS_WORK" = "true" ]; then
        echo -e "${GREEN}OK${NC} - Особый день обновлен (стал рабочим)"
    else
        echo -e "${RED}ОШИБКА${NC} - Не удалось обновить особый день"
    fi
    echo ""
fi

# ============================================================
# 4. Тестирование пользовательских дней (User Days)
# ============================================================

echo -e "${CYAN}=== Тестирование пользовательских дней ===${NC}"
echo ""

# 4.1 Получение ID текущего пользователя
echo -e "${BLUE}4.1 Получение информации о текущем пользователе...${NC}"
USER_RESPONSE=$(curl -s -X GET "$BASE_URL/api/v1/users" \
    -H "Authorization: Bearer $TOKEN")

USER_ID=$(echo "$USER_RESPONSE" | grep -o '"id":[0-9]*' | head -1 | grep -o '[0-9]*')

if [ -n "$USER_ID" ]; then
    echo -e "${GREEN}OK${NC} - ID пользователя: $USER_ID"
else
    echo -e "${YELLOW}WARNING${NC} - Не удалось получить ID пользователя, используем ID=1"
    USER_ID=1
fi
echo ""

# 4.2 Создание пользовательского дня (отпуск)
echo -e "${BLUE}4.2 Создание пользовательского дня (отпуск, 15 июля 2024)...${NC}"
DATE_2024_07_15=$(date -d "2024-07-15 00:00:00" +%s 2>/dev/null || echo "1721001600")
USER_DAY_RESPONSE=$(curl -s -X POST "$BASE_URL/api/v1/user-days" \
    -H "Authorization: Bearer $TOKEN" \
    -H "Content-Type: application/json" \
    -d "{\"userId\":$USER_ID,\"date\":$DATE_2024_07_15,\"isWorkDay\":false,\"description\":\"Отпуск\"}")

USER_DAY_ID=$(echo "$USER_DAY_RESPONSE" | grep -o '"id":[0-9]*' | head -1 | grep -o '[0-9]*')

if [ -n "$USER_DAY_ID" ]; then
    echo -e "${GREEN}OK${NC} - Пользовательский день создан (ID: $USER_DAY_ID)"
else
    echo -e "${RED}ОШИБКА${NC} - Не удалось создать пользовательский день"
    echo "Ответ: $USER_DAY_RESPONSE"
fi
echo ""

# 4.3 Создание пользовательского дня (больничный)
echo -e "${BLUE}4.3 Создание пользовательского дня (больничный, 16 июля 2024)...${NC}"
DATE_2024_07_16=$(date -d "2024-07-16 00:00:00" +%s 2>/dev/null || echo "1721088000")
USER_DAY_2_RESPONSE=$(curl -s -X POST "$BASE_URL/api/v1/user-days" \
    -H "Authorization: Bearer $TOKEN" \
    -H "Content-Type: application/json" \
    -d "{\"userId\":$USER_ID,\"date\":$DATE_2024_07_16,\"isWorkDay\":false,\"description\":\"Больничный\"}")

USER_DAY_2_ID=$(echo "$USER_DAY_2_RESPONSE" | grep -o '"id":[0-9]*' | head -1 | grep -o '[0-9]*')

if [ -n "$USER_DAY_2_ID" ]; then
    echo -e "${GREEN}OK${NC} - Пользовательский день создан (ID: $USER_DAY_2_ID)"
else
    echo -e "${RED}ОШИБКА${NC} - Не удалось создать пользовательский день"
fi
echo ""

# 4.4 Получение списка пользовательских дней
echo -e "${BLUE}4.4 Получение списка пользовательских дней...${NC}"
USER_DAYS_LIST=$(curl -s -X GET "$BASE_URL/api/v1/user-days" \
    -H "Authorization: Bearer $TOKEN")

USER_DAYS_COUNT=$(echo "$USER_DAYS_LIST" | grep -o '"totalCount":[0-9]*' | grep -o '[0-9]*')

if [ -n "$USER_DAYS_COUNT" ]; then
    echo -e "${GREEN}OK${NC} - Получено пользовательских дней: $USER_DAYS_COUNT"
else
    echo -e "${RED}ОШИБКА${NC} - Не удалось получить список пользовательских дней"
fi
echo ""

# 4.5 Получение пользовательского дня по ID
if [ -n "$USER_DAY_ID" ]; then
    echo -e "${BLUE}4.5 Получение пользовательского дня по ID ($USER_DAY_ID)...${NC}"
    USER_DAY_GET=$(curl -s -X GET "$BASE_URL/api/v1/user-days/$USER_DAY_ID" \
        -H "Authorization: Bearer $TOKEN")

    USER_DAY_DESCRIPTION=$(echo "$USER_DAY_GET" | grep -o '"description":"[^"]*"' | cut -d'"' -f4)

    if [ "$USER_DAY_DESCRIPTION" = "Отпуск" ]; then
        echo -e "${GREEN}OK${NC} - Пользовательский день получен (Описание: $USER_DAY_DESCRIPTION)"
    else
        echo -e "${RED}ОШИБКА${NC} - Неправильное описание дня"
    fi
    echo ""
fi

# 4.6 Получение пользовательского дня по пользователю и дате
if [ -n "$USER_ID" ]; then
    echo -e "${BLUE}4.6 Получение пользовательского дня по пользователю и дате (15 июля 2024)...${NC}"
    USER_DAY_BY_DATE=$(curl -s -X GET "$BASE_URL/api/v1/users/$USER_ID/days/$DATE_2024_07_15" \
        -H "Authorization: Bearer $TOKEN")

    USER_DAY_BY_DATE_DESC=$(echo "$USER_DAY_BY_DATE" | grep -o '"description":"[^"]*"' | cut -d'"' -f4)

    if [ "$USER_DAY_BY_DATE_DESC" = "Отпуск" ]; then
        echo -e "${GREEN}OK${NC} - День получен (Описание: $USER_DAY_BY_DATE_DESC)"
    else
        echo -e "${RED}ОШИБКА${NC} - Не удалось получить день по пользователю и дате"
    fi
    echo ""
fi

# 4.7 Фильтрация пользовательских дней по пользователю
echo -e "${BLUE}4.7 Фильтрация пользовательских дней по пользователю ($USER_ID)...${NC}"
USER_DAYS_FILTERED=$(curl -s -X GET "$BASE_URL/api/v1/user-days?userId=$USER_ID" \
    -H "Authorization: Bearer $TOKEN")

USER_DAYS_FILTERED_COUNT=$(echo "$USER_DAYS_FILTERED" | grep -o '"totalCount":[0-9]*' | grep -o '[0-9]*')

if [ -n "$USER_DAYS_FILTERED_COUNT" ]; then
    echo -e "${GREEN}OK${NC} - Найдено дней для пользователя: $USER_DAYS_FILTERED_COUNT"
else
    echo -e "${RED}ОШИБКА${NC} - Не удалось отфильтровать дни по пользователю"
fi
echo ""

# 4.8 Обновление пользовательского дня
if [ -n "$USER_DAY_ID" ]; then
    echo -e "${BLUE}4.8 Обновление пользовательского дня (сделать рабочим)...${NC}"
    UPDATE_USER_DAY_RESPONSE=$(curl -s -X PUT "$BASE_URL/api/v1/user-days/$USER_DAY_ID" \
        -H "Authorization: Bearer $TOKEN" \
        -H "Content-Type: application/json" \
        -d "{\"isWorkDay\":true,\"beginWorkTime\":\"09:00\",\"endWorkTime\":\"13:00\",\"breakDuration\":0,\"description\":\"Сокращенный день\"}")

    UPDATE_USER_DAY_IS_WORK=$(echo "$UPDATE_USER_DAY_RESPONSE" | grep -o '"isWorkDay":[^,}]*' | cut -d':' -f2)

    if [ "$UPDATE_USER_DAY_IS_WORK" = "true" ]; then
        echo -e "${GREEN}OK${NC} - Пользовательский день обновлен"
    else
        echo -e "${RED}ОШИБКА${NC} - Не удалось обновить пользовательский день"
    fi
    echo ""
fi

# ============================================================
# 5. Удаление (очистка)
# ============================================================

echo -e "${CYAN}=== Очистка ===${NC}"
echo ""

# 5.1 Удаление пользовательского дня
if [ -n "$USER_DAY_ID" ]; then
    echo -e "${BLUE}5.1 Удаление пользовательского дня ($USER_DAY_ID)...${NC}"
    RESPONSE=$(curl -s -o /dev/null -w "%{http_code}" -X DELETE "$BASE_URL/api/v1/user-days/$USER_DAY_ID" \
        -H "Authorization: Bearer $TOKEN")

    if [ "$RESPONSE" = "204" ] || [ "$RESPONSE" = "200" ]; then
        echo -e "${GREEN}OK${NC} - Пользовательский день удален (HTTP $RESPONSE)"
    else
        echo -e "${RED}ОШИБКА${NC} - Не удалось удалить пользовательский день (HTTP $RESPONSE)"
    fi
    echo ""
fi

# 5.2 Удаление второго пользовательского дня
if [ -n "$USER_DAY_2_ID" ]; then
    echo -e "${BLUE}5.2 Удаление пользовательского дня ($USER_DAY_2_ID)...${NC}"
    RESPONSE=$(curl -s -o /dev/null -w "%{http_code}" -X DELETE "$BASE_URL/api/v1/user-days/$USER_DAY_2_ID" \
        -H "Authorization: Bearer $TOKEN")

    if [ "$RESPONSE" = "204" ] || [ "$RESPONSE" = "200" ]; then
        echo -e "${GREEN}OK${NC} - Пользовательский день удален (HTTP $RESPONSE)"
    else
        echo -e "${RED}ОШИБКА${NC} - Не удалось удалить пользовательский день (HTTP $RESPONSE)"
    fi
    echo ""
fi

# 5.3 Удаление особого дня
if [ -n "$SPECIAL_DAY_ID" ]; then
    echo -e "${BLUE}5.3 Удаление особого дня ($SPECIAL_DAY_ID)...${NC}"
    RESPONSE=$(curl -s -o /dev/null -w "%{http_code}" -X DELETE "$BASE_URL/api/v1/special-days/$SPECIAL_DAY_ID" \
        -H "Authorization: Bearer $TOKEN")

    if [ "$RESPONSE" = "204" ] || [ "$RESPONSE" = "200" ]; then
        echo -e "${GREEN}OK${NC} - Особый день удален (HTTP $RESPONSE)"
    else
        echo -e "${RED}ОШИБКА${NC} - Не удалось удалить особый день (HTTP $RESPONSE)"
    fi
    echo ""
fi

# 5.4 Удаление второго особого дня
if [ -n "$SPECIAL_DAY_2_ID" ]; then
    echo -e "${BLUE}5.4 Удаление особого дня ($SPECIAL_DAY_2_ID)...${NC}"
    RESPONSE=$(curl -s -o /dev/null -w "%{http_code}" -X DELETE "$BASE_URL/api/v1/special-days/$SPECIAL_DAY_2_ID" \
        -H "Authorization: Bearer $TOKEN")

    if [ "$RESPONSE" = "204" ] || [ "$RESPONSE" = "200" ]; then
        echo -e "${GREEN}OK${NC} - Особый день удален (HTTP $RESPONSE)"
    else
        echo -e "${RED}ОШИБКА${NC} - Не удалось удалить особый день (HTTP $RESPONSE)"
    fi
    echo ""
fi

# 5.5 Удаление всех пользовательских дней пользователя
if [ -n "$USER_ID" ]; then
    echo -e "${BLUE}5.5 Удаление всех пользовательских дней пользователя ($USER_ID)...${NC}"
    RESPONSE=$(curl -s -o /dev/null -w "%{http_code}" -X DELETE "$BASE_URL/api/v1/users/$USER_ID/days" \
        -H "Authorization: Bearer $TOKEN")

    if [ "$RESPONSE" = "204" ] || [ "$RESPONSE" = "200" ]; then
        echo -e "${GREEN}OK${NC} - Все дни пользователя удалены (HTTP $RESPONSE)"
    else
        echo -e "${RED}ОШИБКА${NC} - Не удалось удалить дни пользователя (HTTP $RESPONSE)"
    fi
    echo ""
fi

# ============================================================
# 6. Итоговый отчет
# ============================================================

echo -e "${CYAN}=== Итоговый отчет ===${NC}"
echo ""

# Проверяем, что стандартные дни вернулись к исходному состоянию
echo -e "${BLUE}Проверка стандартных дней после тестов...${NC}"
FINAL_STANDARD_RESPONSE=$(curl -s -X GET "$BASE_URL/api/v1/standard-days/5" \
    -H "Authorization: Bearer $TOKEN")

FINAL_IS_WORK=$(echo "$FINAL_STANDARD_RESPONSE" | grep -o '"isWorkDay":[^,}]*' | cut -d':' -f2)
FINAL_BEGIN_TIME=$(echo "$FINAL_STANDARD_RESPONSE" | grep -o '"beginWorkTime":"[^"]*"' | cut -d'"' -f4)

if [ "$FINAL_IS_WORK" = "true" ] && [ "$FINAL_BEGIN_TIME" = "09:00" ]; then
    echo -e "${GREEN}OK${NC} - Стандартные дни в исходном состоянии"
else
    echo -e "${YELLOW}WARNING${NC} - Стандартные дни не в исходном состоянии"
fi

echo ""
echo -e "${GREEN}=== Тестирование календаря завершено ===${NC}"
