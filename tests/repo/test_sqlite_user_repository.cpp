#include <cstdio> // для std::remove
#include <filesystem>

#include <boost/test/unit_test.hpp>

#include "common/dto/user.h"

#include "repo/sqlite/sqlite_user_repository.h"
#include "storage/database_factory.h"
#include "storage/idatabase.h"
#include "storage/sqlite/sqlite_database.h"

namespace server::test
{

struct UserRepositoryFixture
{
    UserRepositoryFixture()
    {
        // Создаем временный файл для тестовой БД
        m_tempDbPath = std::filesystem::temp_directory_path() / "farado_test_repo.db";

        // Удаляем файл если он существует от предыдущего запуска
        std::remove(m_tempDbPath.c_str());

        // Создаем файловую БД
        db::DatabaseConfig config;
        config["database"] = m_tempDbPath.string();

        m_database = std::make_shared<db::SqliteDatabase>();
        m_database->initialize(config);

        // Используем ОДНО соединение для создания схемы
        auto conn = m_database->connection();

        // Создаем схему
        conn->execute(R"(
            CREATE TABLE User (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                login TEXT NOT NULL UNIQUE,
                firstName TEXT,
                middleName TEXT,
                lastName TEXT,
                email TEXT NOT NULL UNIQUE,
                passwordHash TEXT NOT NULL,
                needChangePassword INTEGER NOT NULL DEFAULT 1,
                isBlocked INTEGER NOT NULL DEFAULT 0,
                isSuperAdmin INTEGER NOT NULL DEFAULT 0,
                isHidden INTEGER NOT NULL DEFAULT 0
            )
        )");

        // Создаем репозиторий с тем же объектом базы данных
        m_repository = std::make_unique<repositories::SqliteUserRepository>(m_database);
    }

    /**
     * @brief Создает тестового пользователя.
     */
    dto::User createTestUser(const std::string& login = "testuser", const std::string& email = "test@example.com")
    {
        dto::User user;
        user.login = login;
        user.firstName = "Test";
        user.lastName = "User";
        user.email = email;
        user.needChangePassword = true;
        user.isBlocked = false;
        user.isSuperAdmin = false;
        user.isHidden = false;
        return user;
    }

    ~UserRepositoryFixture()
    {
        // Важно: сначала уничтожаем репозиторий, потом БД
        m_repository.reset();

        if (m_database)
        {
            m_database->shutdown();
            m_database.reset();
        }

        // Удаляем временный файл БД
        if (!m_tempDbPath.empty() && std::filesystem::exists(m_tempDbPath))
        {
            std::error_code ec;
            std::filesystem::remove(m_tempDbPath, ec);
        }
    }

    std::filesystem::path m_tempDbPath;
    std::shared_ptr<db::SqliteDatabase> m_database;
    std::unique_ptr<repositories::SqliteUserRepository> m_repository;
};

BOOST_FIXTURE_TEST_SUITE(SqliteUserRepositoryTests, UserRepositoryFixture)

BOOST_AUTO_TEST_CASE(test_create_user_success)
{
    // Arrange
    auto user = createTestUser();
    const std::string passwordHash = "hashed_password_123";

    // Act
    int64_t userId = m_repository->create(user, passwordHash);

    // Assert
    BOOST_CHECK_GT(userId, 0);
    BOOST_CHECK(m_repository->existsByLogin("testuser"));
}

BOOST_AUTO_TEST_CASE(test_create_user_duplicate_login_fails)
{
    // Arrange
    auto user = createTestUser();
    m_repository->create(user, "hash1");

    // Act & Assert - попытка создать с тем же логином должна бросить исключение
    BOOST_CHECK_THROW(
        m_repository->create(user, "hash2"),
        std::exception
    );
}

BOOST_AUTO_TEST_CASE(test_create_user_duplicate_email_fails)
{
    // Arrange
    auto user1 = createTestUser("user1", "same@example.com");
    m_repository->create(user1, "hash1");

    auto user2 = createTestUser("user2", "same@example.com");

    // Act & Assert
    BOOST_CHECK_THROW(
        m_repository->create(user2, "hash2"),
        std::exception
    );
}

BOOST_AUTO_TEST_CASE(test_find_by_login_success)
{
    // Arrange
    auto user = createTestUser("john_doe", "john@example.com");
    user.firstName = "John";
    user.lastName = "Doe";
    user.isSuperAdmin = true;

    int64_t userId = m_repository->create(user, "hash123");
    BOOST_REQUIRE_GT(userId, 0);

    // Act
    auto found = m_repository->findByLogin("john_doe");

    // Assert
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK(found->id.has_value());
    BOOST_CHECK_EQUAL(found->id.value(), userId);
    BOOST_CHECK(found->login.has_value());
    BOOST_CHECK_EQUAL(found->login.value(), "john_doe");
    BOOST_CHECK(found->firstName.has_value());
    BOOST_CHECK_EQUAL(found->firstName.value(), "John");
    BOOST_CHECK(found->lastName.has_value());
    BOOST_CHECK_EQUAL(found->lastName.value(), "Doe");
    BOOST_CHECK(found->email.has_value());
    BOOST_CHECK_EQUAL(found->email.value(), "john@example.com");
    BOOST_CHECK(found->isSuperAdmin.has_value());
    BOOST_CHECK(found->isSuperAdmin.value());
    BOOST_CHECK(found->isBlocked.has_value());
    BOOST_CHECK(!found->isBlocked.value());
}

BOOST_AUTO_TEST_CASE(test_find_by_login_not_found)
{
    // Act
    auto found = m_repository->findByLogin("nonexistent");

    // Assert
    BOOST_CHECK(!found.has_value());
}

BOOST_AUTO_TEST_CASE(test_find_by_login_empty_returns_nullopt)
{
    // Act
    auto found = m_repository->findByLogin("");

    // Assert
    BOOST_CHECK(!found.has_value());
}

BOOST_AUTO_TEST_CASE(test_find_by_id_success)
{
    // Arrange
    auto user = createTestUser("alice", "alice@example.com");
    int64_t userId = m_repository->create(user, "hash456");
    BOOST_REQUIRE_GT(userId, 0);

    // Act
    auto found = m_repository->findById(userId);

    // Assert
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK(found->id.has_value());
    BOOST_CHECK_EQUAL(found->id.value(), userId);
    BOOST_CHECK(found->login.has_value());
    BOOST_CHECK_EQUAL(found->login.value(), "alice");
}

BOOST_AUTO_TEST_CASE(test_find_by_id_not_found)
{
    // Act
    auto found = m_repository->findById(99999);

    // Assert
    BOOST_CHECK(!found.has_value());
}

BOOST_AUTO_TEST_CASE(test_find_by_id_invalid_returns_nullopt)
{
    // Act
    auto found = m_repository->findById(-1);

    // Assert
    BOOST_CHECK(!found.has_value());
}

BOOST_AUTO_TEST_CASE(test_update_password_success)
{
    // Arrange
    auto user = createTestUser();
    int64_t userId = m_repository->create(user, "old_hash");
    BOOST_REQUIRE_GT(userId, 0);

    // Act
    bool result = m_repository->updatePassword(userId, "new_secure_hash");

    // Assert
    BOOST_CHECK(result);

    // Проверяем через прямой запрос к БД
    auto conn = m_repository->connection();
    auto stmt = conn->prepareStatement(
        "SELECT passwordHash FROM User WHERE id = :id"
    );
    stmt->bindInt64("id", userId);
    auto rs = stmt->executeQuery();
    BOOST_REQUIRE(rs->next());
    BOOST_CHECK_EQUAL(rs->valueString("passwordHash"), "new_secure_hash");
}

BOOST_AUTO_TEST_CASE(test_update_password_nonexistent_user)
{
    // Act
    bool result = m_repository->updatePassword(99999, "some_hash");

    // Assert
    BOOST_CHECK(!result);
}

BOOST_AUTO_TEST_CASE(test_update_need_change_password_success)
{
    // Arrange
    auto user = createTestUser();
    user.needChangePassword = true;
    int64_t userId = m_repository->create(user, "hash");
    BOOST_REQUIRE_GT(userId, 0);

    // Act - меняем флаг на false
    bool result = m_repository->updateNeedChangePassword(userId, false);

    // Assert
    BOOST_CHECK(result);

    auto found = m_repository->findById(userId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK(found->needChangePassword.has_value());
    BOOST_CHECK(!found->needChangePassword.value());

    // Act - меняем обратно на true
    result = m_repository->updateNeedChangePassword(userId, true);

    // Assert
    BOOST_CHECK(result);
    found = m_repository->findById(userId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK(found->needChangePassword.has_value());
    BOOST_CHECK(found->needChangePassword.value());
}

BOOST_AUTO_TEST_CASE(test_update_need_change_password_nonexistent)
{
    // Act
    bool result = m_repository->updateNeedChangePassword(99999, false);

    // Assert
    BOOST_CHECK(!result);
}

BOOST_AUTO_TEST_CASE(test_exists_by_login_true)
{
    // Arrange
    auto user = createTestUser("existing_user");
    m_repository->create(user, "hash");

    // Act
    bool exists = m_repository->existsByLogin("existing_user");

    // Assert
    BOOST_CHECK(exists);
}

BOOST_AUTO_TEST_CASE(test_exists_by_login_false)
{
    // Act
    bool exists = m_repository->existsByLogin("nonexistent_user");

    // Assert
    BOOST_CHECK(!exists);
}

BOOST_AUTO_TEST_CASE(test_exists_by_login_empty_returns_false)
{
    // Act
    bool exists = m_repository->existsByLogin("");

    // Assert
    BOOST_CHECK(!exists);
}

BOOST_AUTO_TEST_CASE(test_user_with_nullable_fields)
{
    // Arrange
    dto::User user;
    user.login = "minimal_user";
    user.email = "minimal@example.com";
    // firstName, middleName, lastName остаются std::nullopt
    user.needChangePassword = true;
    user.isBlocked = false;
    user.isSuperAdmin = false;
    user.isHidden = false;

    // Act
    int64_t userId = m_repository->create(user, "hash");

    // Assert
    BOOST_CHECK_GT(userId, 0);

    auto found = m_repository->findById(userId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK(!found->firstName.has_value());
    BOOST_CHECK(!found->middleName.has_value());
    BOOST_CHECK(!found->lastName.has_value());
}

BOOST_AUTO_TEST_CASE(test_user_with_all_flags_enabled)
{
    // Arrange
    dto::User user;
    user.login = "super_user";
    user.email = "super@example.com";
    user.isBlocked = true;
    user.isSuperAdmin = true;
    user.isHidden = true;
    user.needChangePassword = false;

    // Act
    int64_t userId = m_repository->create(user, "hash");

    // Assert
    BOOST_CHECK_GT(userId, 0);

    auto found = m_repository->findById(userId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK(found->isBlocked.has_value());
    BOOST_CHECK(found->isBlocked.value());
    BOOST_CHECK(found->isSuperAdmin.has_value());
    BOOST_CHECK(found->isSuperAdmin.value());
    BOOST_CHECK(found->isHidden.has_value());
    BOOST_CHECK(found->isHidden.value());
    BOOST_CHECK(found->needChangePassword.has_value());
    BOOST_CHECK(!found->needChangePassword.value());
}

BOOST_AUTO_TEST_CASE(test_multiple_users_independent)
{
    // Arrange
    auto user1 = createTestUser("user1", "user1@example.com");
    auto user2 = createTestUser("user2", "user2@example.com");
    user2.firstName = "Second";

    // Act
    int64_t id1 = m_repository->create(user1, "hash1");
    int64_t id2 = m_repository->create(user2, "hash2");

    // Assert
    BOOST_CHECK_GT(id1, 0);
    BOOST_CHECK_GT(id2, 0);
    BOOST_CHECK_NE(id1, id2);

    auto found1 = m_repository->findById(id1);
    auto found2 = m_repository->findById(id2);

    BOOST_REQUIRE(found1.has_value());
    BOOST_REQUIRE(found2.has_value());

    BOOST_CHECK(found1->login.has_value());
    BOOST_CHECK_EQUAL(found1->login.value(), "user1");
    BOOST_CHECK(found2->login.has_value());
    BOOST_CHECK_EQUAL(found2->login.value(), "user2");

    BOOST_CHECK(found1->firstName.has_value());
    BOOST_CHECK_EQUAL(found1->firstName.value(), "Test");
    BOOST_CHECK(found2->firstName.has_value());
    BOOST_CHECK_EQUAL(found2->firstName.value(), "Second");
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace server::test
