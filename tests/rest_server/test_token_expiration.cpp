// ============================================================================
// Тест истечения срока действия JWT токенов
// Проверяет, что токены корректно становятся невалидными после истечения TTL
// ============================================================================

#include <chrono>
#include <thread>

#include <boost/test/unit_test.hpp>

#include "vault-server/src/api/middleware/impl/auth_middleware.h"

#include "test_server_fixture.h"

using namespace web;
using namespace web::http;
using namespace web::http::client;

namespace server::test
{

// ============================================================================
// ТЕСТ ИСТЕЧЕНИЯ СРОКА ДЕЙСТВИЯ ТОКЕНА
// ============================================================================

/**
 * @brief Проверка, что токен перестает работать после истечения TTL
 *
 * Сценарий теста:
 * 1. Генерируем токен с очень коротким сроком жизни (2 секунды)
 * 2. Сразу проверяем, что токен рабочий — доступ к /api/items успешен
 * 3. Ждем 3 секунды (больше, чем TTL)
 * 4. Проверяем, что тот же токен теперь отвергается с кодом 401
 *
 * Важно: тест использует реальные задержки в секундах, поэтому
 * может быть чувствителен к загрузке системы. При флаке рекомендуется
 * увеличить время ожидания.
 */
BOOST_FIXTURE_TEST_CASE(test_token_expiration, TestServerFixture)
{
    auto client = createClient();

    // Генерируем токен с коротким сроком действия (2 секунды)
    // Это позволяет тесту выполняться быстро, но достаточно для проверки
    AuthMiddleware auth(TEST_SECRET_KEY);
    std::string shortLivedToken = auth.generateToken("test_user", 2);

    // Сразу после генерации токен должен быть валидным
    {
        http_request request { methods::GET };
        request.set_request_uri("/api/items");
        request.headers().add("Authorization", "Bearer " + shortLivedToken);

        auto response = client.request(request).get();
        BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    }

    // Ожидаем истечения срока действия токена
    // Ждем 3 секунды — больше, чем TTL (2 секунды), чтобы гарантировать истечение
    // Плюс небольшой запас на погрешность системного таймера
    std::this_thread::sleep_for(std::chrono::seconds(3));

    // Токен больше не должен работать — проверяем что сервер возвращает 401
    {
        http_request request { methods::GET };
        request.set_request_uri("/api/items");
        request.headers().add("Authorization", "Bearer " + shortLivedToken);

        auto response = client.request(request).get();
        BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
    }
}

} // namespace server::test
