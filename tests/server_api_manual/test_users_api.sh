#!/bin/bash

# Цвета для вывода
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m' # No Color

BASE_URL="http://localhost:8090"

echo "=== Тестирование CRUD API для пользователей ==="

# 1. Аутентификация
echo -e "\n${GREEN}1. Аутентификация...${NC}"
LOGIN_RESPONSE=$(curl -s -X POST "${BASE_URL}/api/v1/auth/login" \
  -H "Content-Type: application/json" \
  -d '{"login":"admin","password":"password"}')

TOKEN=$(echo "$LOGIN_RESPONSE" | jq -r '.access_token')
if [ "$TOKEN" == "null" ] || [ -z "$TOKEN" ]; then
    echo -e "${RED}Ошибка аутентификации!${NC}"
    exit 1
fi
echo "Токен получен: ${TOKEN:0:20}..."

# 2. Healthcheck
echo -e "\n${GREEN}2. Проверка healthcheck...${NC}"
curl -s -X GET "${BASE_URL}/health" | jq .

# 3. CREATE
echo -e "\n${GREEN}3. Создание пользователей...${NC}"

echo "3.1 Создание первого пользователя:"
curl -s -X POST "${BASE_URL}/api/v1/users" \
  -H "Authorization: Bearer ${TOKEN}" \
  -H "Content-Type: application/json" \
  -d '{"login":"test1","email":"test1@example.com","password":"TestPass1"}' | jq .

echo "3.2 Создание второго пользователя:"
curl -s -X POST "${BASE_URL}/api/v1/users" \
  -H "Authorization: Bearer ${TOKEN}" \
  -H "Content-Type: application/json" \
  -d '{"login":"test2","firstName":"Test","lastName":"User","email":"test2@example.com","password":"TestPass2","isSuperAdmin":true}' | jq .

echo "3.3 Попытка дубликата (ожидается 409):"
HTTP_CODE=$(curl -s -o /dev/null -w "%{http_code}" -X POST "${BASE_URL}/api/v1/users" \
  -H "Authorization: Bearer ${TOKEN}" \
  -H "Content-Type: application/json" \
  -d '{"login":"test1","email":"another@example.com","password":"TestPass3"}')
echo "HTTP статус: $HTTP_CODE"

# 4. READ
echo -e "\n${GREEN}4. Чтение пользователей...${NC}"

echo "4.1 Список всех пользователей:"
curl -s -X GET "${BASE_URL}/api/v1/users" \
  -H "Authorization: Bearer ${TOKEN}" | jq .

echo "4.2 Получение пользователя по ID:"
curl -s -X GET "${BASE_URL}/api/v1/users/2" \
  -H "Authorization: Bearer ${TOKEN}" | jq .

# 5. UPDATE
echo -e "\n${GREEN}5. Обновление пользователя...${NC}"

echo "5.1 Обновление имени пользователя 2:"
HTTP_CODE=$(curl -s -o /dev/null -w "%{http_code}" -X PUT "${BASE_URL}/api/v1/users/2" \
  -H "Authorization: Bearer ${TOKEN}" \
  -H "Content-Type: application/json" \
  -d '{"firstName":"UpdatedName"}')
echo "HTTP статус: $HTTP_CODE"

echo "5.2 Проверка обновления:"
curl -s -X GET "${BASE_URL}/api/v1/users/2" \
  -H "Authorization: Bearer ${TOKEN}" | jq '{id, firstName, lastName}'

# 6. DELETE
echo -e "\n${GREEN}6. Удаление пользователя...${NC}"

echo "6.1 Удаление пользователя 2:"
HTTP_CODE=$(curl -s -o /dev/null -w "%{http_code}" -X DELETE "${BASE_URL}/api/v1/users/2" \
  -H "Authorization: Bearer ${TOKEN}")
echo "HTTP статус: $HTTP_CODE"

echo "6.2 Проверка удаления (ожидается 404):"
HTTP_CODE=$(curl -s -o /dev/null -w "%{http_code}" -X GET "${BASE_URL}/api/v1/users/2" \
  -H "Authorization: Bearer ${TOKEN}")
echo "HTTP статус: $HTTP_CODE"

# 7. Проверка безопасности
echo -e "\n${GREEN}7. Проверка безопасности...${NC}"

echo "7.1 Доступ без токена (ожидается 401):"
HTTP_CODE=$(curl -s -o /dev/null -w "%{http_code}" -X GET "${BASE_URL}/api/v1/users")
echo "HTTP статус: $HTTP_CODE"

echo "7.2 Неверный токен (ожидается 401):"
HTTP_CODE=$(curl -s -o /dev/null -w "%{http_code}" -X GET "${BASE_URL}/api/v1/users" \
  -H "Authorization: Bearer invalid_token_here")
echo "HTTP статус: $HTTP_CODE"

echo -e "\n${GREEN}=== Тестирование завершено ===${NC}"
