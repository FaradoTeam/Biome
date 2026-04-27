#pragma once

#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

#include <boost/test/unit_test.hpp>

#include <cpprest/http_client.h>
#include <cpprest/json.h>

#include "vault-server/src/api/middleware/impl/auth_middleware.h"
#include "vault-server/src/api/rest_server.h"

#include "tests/server_mocks/mock_auth_service.h"
#include "tests/server_mocks/mock_user_service.h"

using namespace web;
using namespace web::http;

namespace server::test
{

const std::string TEST_HOST = "127.0.0.1";
const uint16_t TEST_PORT = 18083;
const std::string TEST_SECRET_KEY = "your-very-long-secret-key-that-is-at-least-32-bytes-long!";

struct TestServerFixture
{
    TestServerFixture()
        : m_server(std::make_shared<RestServer>(TEST_HOST, TEST_PORT))
    {
        // Создаём реальный AuthMiddleware
        auto authMiddleware = std::make_shared<AuthMiddleware>(TEST_SECRET_KEY);

        // Создаём моки для сервисов
        m_mockAuthService = std::make_shared<MockAuthService>();
        m_mockUserService = std::make_shared<MockUserService>();

        // Настраиваем UserService с тестовыми данными
        setupDefaultUserService();

        // Настраиваем сервер
        m_server->setAuthMiddleware(authMiddleware);
        m_server->setAuthService(m_mockAuthService);
        m_server->setUserService(m_mockUserService);

        m_server->initialize();
        m_server->start();

        // Даем серверу время на полный запуск
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    void setupDefaultUserService()
    {
        services::UsersPage testPage;

        dto::User user1;
        user1.id = 1;
        user1.login = "test_user";
        user1.email = "test@example.com";
        user1.firstName = "Test";
        user1.lastName = "User";

        testPage.users = { user1 };
        testPage.totalCount = 1;

        m_mockUserService->setGetUsersResult(testPage);
        m_mockUserService->setGetUserResult(user1);

        dto::User newUser;
        newUser.id = 2;
        newUser.login = "newuser";
        newUser.email = "new@test.com";
        m_mockUserService->setCreateUserResult(newUser);

        m_mockUserService->setUpdateUserResult(user1);
        m_mockUserService->setDeleteUserResult(true);
    }

    ~TestServerFixture()
    {
        if (m_server)
        {
            m_server->stop();
        }
    }

    std::shared_ptr<RestServer> getServer() const { return m_server; }

    std::string getTestToken() const
    {
        AuthMiddleware auth(TEST_SECRET_KEY);
        return auth.generateToken("test_user_123", 3600);
    }

    web::http::client::http_client createClient() const
    {
        return web::http::client::http_client(U("http://127.0.0.1:18083"));
    }

    std::shared_ptr<RestServer> m_server; ///< Экземпляр тестируемого сервера
    std::shared_ptr<MockAuthService> m_mockAuthService;
    std::shared_ptr<MockUserService> m_mockUserService;
};

} // namespace server::test
