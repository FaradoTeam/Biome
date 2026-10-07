#include <stdexcept>

#include <boost/algorithm/string.hpp>

#include "common/helpers/string_helper.h"
#include "common/log/log.h"

#include "storage/idatabase.h"

#include "sqlite_user_repository.h"

namespace
{
/**
 * @brief Преобразует строку результата в объект User.
 */
dto::User mapRowToUser(db::IResultSet& rs)
{
    dto::User user;
    user.id = rs.valueInt64("id");
    user.login = rs.valueString("login");

    // Опциональные поля могут быть NULL
    if (!rs.isNull("firstName"))
        user.firstName = rs.valueString("firstName");
    if (!rs.isNull("middleName"))
        user.middleName = rs.valueString("middleName");
    if (!rs.isNull("lastName"))
        user.lastName = rs.valueString("lastName");

    user.email = rs.valueString("email");

    // Добавлено чтение needChangePassword
    user.needChangePassword = rs.valueInt64("needChangePassword") != 0;

    user.isBlocked = rs.valueInt64("isBlocked") != 0;
    user.isSuperAdmin = rs.valueInt64("isSuperAdmin") != 0;
    user.isHidden = rs.valueInt64("isHidden") != 0;

    return user;
}
} // namespace

namespace server
{
namespace repositories
{

SqliteUserRepository::SqliteUserRepository(std::shared_ptr<db::IDatabase> database)
    : m_database(std::move(database))
{
    if (!m_database)
    {
        throw std::runtime_error("SqliteUserRepository: database is null");
    }
}

std::optional<dto::User> SqliteUserRepository::findByLogin(const std::string& login)
{
    if (login.empty())
    {
        LOG_WARN << "findByLogin: указан пустой логин";
        return std::nullopt;
    }

    try
    {
        auto conn = m_database->connection();
        auto stmt = conn->prepareStatement(
            "SELECT id, login, firstName, middleName, lastName, email, "
            "needChangePassword, isBlocked, isSuperAdmin, isHidden "
            "FROM User WHERE login = :login"
        );

        stmt->bindString("login", login);
        auto rs = stmt->executeQuery();

        if (rs->next())
        {
            return mapRowToUser(*rs);
        }

        return std::nullopt;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Не удалось выполнить поиск логина: " << e.what();
        throw;
    }
}

std::optional<dto::User> SqliteUserRepository::findById(int64_t id)
{
    if (id <= 0)
    {
        LOG_WARN << "findById: неверный идентификатор " << id;
        return std::nullopt;
    }

    try
    {
        auto conn = m_database->connection();
        auto stmt = conn->prepareStatement(
            "SELECT id, login, firstName, middleName, lastName, email, "
            "needChangePassword, isBlocked, isSuperAdmin, isHidden "
            "FROM User WHERE id = :id"
        );

        stmt->bindInt64("id", id);
        auto rs = stmt->executeQuery();

        if (rs->next())
        {
            return mapRowToUser(*rs);
        }

        return std::nullopt;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка поиска пользователя по id: " << e.what();
        throw;
    }
}

std::string SqliteUserRepository::passwordHash(int64_t userId)
{
    if (userId <= 0)
    {
        LOG_WARN << "getPasswordHash: неверный userId " << userId;
        return "";
    }

    try
    {
        auto conn = m_database->connection();
        auto stmt = conn->prepareStatement(
            "SELECT passwordHash FROM User WHERE id = :id"
        );

        stmt->bindInt64("id", userId);
        auto rs = stmt->executeQuery();

        if (rs->next() && !rs->isNull("passwordHash"))
        {
            return rs->valueString("passwordHash");
        }

        return "";
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Не удалось получить hash пароля: " << e.what();
        return "";
    }
}

bool SqliteUserRepository::updatePassword(int64_t userId, const std::string& passwordHash)
{
    if (userId <= 0 || passwordHash.empty())
    {
        LOG_WARN << "updatePassword: недопустимые параметры";
        return false;
    }

    try
    {
        auto conn = m_database->connection();
        auto stmt = conn->prepareStatement(
            "UPDATE User SET passwordHash = :passwordHash WHERE id = :id"
        );

        stmt->bindString("passwordHash", passwordHash);
        stmt->bindInt64("id", userId);

        int64_t affected = stmt->execute();
        return affected > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка обновления пароля: " << e.what();
        return false;
    }
}

bool SqliteUserRepository::updateNeedChangePassword(int64_t userId, bool needChange)
{
    if (userId <= 0)
    {
        LOG_WARN << "updateNeedChangePassword: неверный userId " << userId;
        return false;
    }

    try
    {
        auto conn = m_database->connection();
        auto stmt = conn->prepareStatement(
            "UPDATE User SET needChangePassword = :needChange WHERE id = :id"
        );

        stmt->bindInt64("needChange", needChange ? 1 : 0);
        stmt->bindInt64("id", userId);

        int64_t affected = stmt->execute();
        return affected > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR
            << "Ошибка обновления флага принудительной смены пароля: "
            << e.what();
        return false;
    }
}

int64_t SqliteUserRepository::create(
    const dto::User& user,
    const std::string& passwordHash
)
{
    if (!user.login.has_value() || user.login->empty()
        || !user.email.has_value() || user.email->empty()
        || passwordHash.empty())
    {
        LOG_WARN << "Создание пользователя: отсутствуют обязательные поля";
        return 0;
    }

    // Нормализуем ФИО в том же порядке, что отображается в UI:
    // lastName firstName middleName.
    std::string fullName;
    if (user.lastName.has_value() && !user.lastName->empty())
        fullName += *user.lastName;
    if (user.firstName.has_value() && !user.firstName->empty())
    {
        if (!fullName.empty())
            fullName += ' ';
        fullName += *user.firstName;
    }
    if (user.middleName.has_value() && !user.middleName->empty())
    {
        if (!fullName.empty())
            fullName += ' ';
        fullName += *user.middleName;
    }

    const std::string searchLogin = common::toLowerCase(*user.login);
    const std::string searchName = common::toLowerCase(fullName);
    const std::string searchEmail = common::toLowerCase(*user.email);

    try
    {
        auto conn = m_database->connection();
        auto stmt = conn->prepareStatement(
            "INSERT INTO User (login, firstName, middleName, lastName, email, "
            "passwordHash, needChangePassword, isBlocked, isSuperAdmin, isHidden, "
            "searchLogin, searchName, searchEmail) "
            "VALUES (:login, :firstName, :middleName, :lastName, :email, "
            ":passwordHash, :needChangePassword, :isBlocked, :isSuperAdmin, :isHidden, "
            ":searchLogin, :searchName, :searchEmail)"
        );

        stmt->bindString("login", user.login.value());

        if (user.firstName.has_value())
            stmt->bindString("firstName", *user.firstName);
        else
            stmt->bindNull("firstName");
        if (user.middleName.has_value())
            stmt->bindString("middleName", *user.middleName);
        else
            stmt->bindNull("middleName");
        if (user.lastName.has_value())
            stmt->bindString("lastName", *user.lastName);
        else
            stmt->bindNull("lastName");

        stmt->bindString("email", user.email.value());
        stmt->bindString("passwordHash", passwordHash);
        stmt->bindInt64("needChangePassword", user.needChangePassword.value_or(true) ? 1 : 0);
        stmt->bindInt64("isBlocked", user.isBlocked.value_or(false) ? 1 : 0);
        stmt->bindInt64("isSuperAdmin", user.isSuperAdmin.value_or(false) ? 1 : 0);
        stmt->bindInt64("isHidden", user.isHidden.value_or(false) ? 1 : 0);

        stmt->bindString("searchLogin", searchLogin);
        stmt->bindString("searchName", searchName);
        stmt->bindString("searchEmail", searchEmail);

        stmt->execute();
        return conn->lastInsertId();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка создания пользователя: " << e.what();
        throw;
    }
}

bool SqliteUserRepository::existsByLogin(const std::string& login)
{
    if (login.empty())
        return false;

    try
    {
        auto conn = m_database->connection();
        auto stmt = conn->prepareStatement(
            "SELECT 1 FROM User WHERE login = :login LIMIT 1"
        );

        stmt->bindString("login", login);
        auto rs = stmt->executeQuery();

        return rs->next();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR
            << "Ошибка проверки существования пользователя по логину: "
            << e.what();
        return false;
    }
}

std::pair<std::vector<dto::User>, int64_t> SqliteUserRepository::findAll(
    int page,
    int pageSize,
    const std::string& login,
    const std::string& name,
    const std::string& email,
    std::optional<bool> isBlocked,
    std::optional<int64_t> id,
    const std::string& sortField,
    bool sortAscending
)
{
    std::vector<dto::User> users;
    int64_t totalCount = 0;

    try
    {
        auto conn = m_database->connection();

        // Все входные строки фильтра нормализуем в C++ через ICU.
        // После этого регистр в SQLite-функциях не имеет значения —
        // обе стороны сравнения уже в одном регистре.
        const std::string loginLower = common::toLowerCase(login);
        const std::string nameLower = common::toLowerCase(name);
        const std::string emailLower = common::toLowerCase(email);

        std::vector<std::string> whereClauses;

        if (id.has_value() && *id > 0)
            whereClauses.push_back("id = :id");

        // instr(X, Y) возвращает позицию подстроки Y в X (1-based).
        // 0 — если Y не найдена. В отличие от LIKE, instr() не выполняет
        // никаких регистровых преобразований и корректно работает
        // с кириллицей, потому что обе строки уже нормализованы в C++.
        if (!loginLower.empty())
            whereClauses.push_back("instr(searchLogin, :login) > 0");

        if (!nameLower.empty())
            whereClauses.push_back("instr(searchName, :name) > 0");

        if (!emailLower.empty())
            whereClauses.push_back("instr(searchEmail, :email) > 0");

        if (isBlocked.has_value())
            whereClauses.push_back("isBlocked = :isBlocked");

        std::string whereClause;
        if (!whereClauses.empty())
        {
            whereClause = " WHERE " + boost::algorithm::join(whereClauses, " AND ");
        }

        // ORDER BY. Для составного ключа (ФИО) направление разворачивается
        // на каждый столбец — иначе ASC/DESC в SQLite применится
        // только к последнему столбцу выражения.
        const std::string dir = sortAscending ? "ASC" : "DESC";
        std::string orderBy;

        if (sortField == "id")
            orderBy = "id " + dir;
        else if (sortField == "login")
            orderBy = "login " + dir;
        else if (sortField == "name")
            orderBy = "lastName " + dir + ", firstName " + dir + ", middleName " + dir;
        else if (sortField == "email")
            orderBy = "email " + dir;
        else if (sortField == "status")
            orderBy = "isBlocked " + dir;
        else
            orderBy = "login " + dir;

        // 1. COUNT
        auto countStmt = conn->prepareStatement(
            "SELECT COUNT(*) FROM User" + whereClause
        );
        if (id.has_value() && *id > 0)
            countStmt->bindInt64("id", *id);
        if (!loginLower.empty())
            countStmt->bindString("login", loginLower);
        if (!nameLower.empty())
            countStmt->bindString("name", nameLower);
        if (!emailLower.empty())
            countStmt->bindString("email", emailLower);
        if (isBlocked.has_value())
            countStmt->bindInt64("isBlocked", isBlocked.value() ? 1 : 0);

        auto countRs = countStmt->executeQuery();
        if (countRs->next())
            totalCount = countRs->valueInt64(0);

        if (totalCount == 0 || (page - 1) * pageSize >= totalCount)
            return { users, totalCount };

        // 2. Страница
        const int offset = (page - 1) * pageSize;
        auto stmt = conn->prepareStatement(
            "SELECT id, login, firstName, middleName, lastName, email, "
            "needChangePassword, isBlocked, isSuperAdmin, isHidden "
            "FROM User"
            + whereClause
            + " ORDER BY " + orderBy
            + " LIMIT :limit OFFSET :offset"
        );

        if (id.has_value() && *id > 0)
            stmt->bindInt64("id", *id);
        if (!loginLower.empty())
            stmt->bindString("login", loginLower);
        if (!nameLower.empty())
            stmt->bindString("name", nameLower);
        if (!emailLower.empty())
            stmt->bindString("email", emailLower);
        if (isBlocked.has_value())
            stmt->bindInt64("isBlocked", isBlocked.value() ? 1 : 0);

        stmt->bindInt64("limit", pageSize);
        stmt->bindInt64("offset", offset);

        auto rs = stmt->executeQuery();
        while (rs->next())
            users.push_back(mapRowToUser(*rs));
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения списка пользователей: " << e.what();
        throw;
    }

    return { users, totalCount };
}

bool SqliteUserRepository::update(const dto::User& user)
{
    if (!user.id.has_value())
    {
        LOG_WARN << "update: отсутствует ID пользователя";
        return false;
    }

    try
    {
        auto conn = m_database->connection();
        std::vector<std::string> setClauses;

        if (user.firstName.has_value())
            setClauses.push_back("firstName = :firstName");
        if (user.middleName.has_value())
            setClauses.push_back("middleName = :middleName");
        if (user.lastName.has_value())
            setClauses.push_back("lastName = :lastName");
        if (user.email.has_value())
            setClauses.push_back("email = :email");
        if (user.needChangePassword.has_value())
            setClauses.push_back("needChangePassword = :needChangePassword");
        if (user.isBlocked.has_value())
            setClauses.push_back("isBlocked = :isBlocked");
        if (user.isSuperAdmin.has_value())
            setClauses.push_back("isSuperAdmin = :isSuperAdmin");
        if (user.isHidden.has_value())
            setClauses.push_back("isHidden = :isHidden");

        if (setClauses.empty())
        {
            LOG_WARN << "update: нет полей для обновления";
            return false;
        }

        std::string sql = "UPDATE User SET "
            + boost::algorithm::join(setClauses, ", ")
            + " WHERE id = :id";

        auto stmt = conn->prepareStatement(sql);

        if (user.firstName.has_value())
            stmt->bindString("firstName", *user.firstName);
        if (user.middleName.has_value())
            stmt->bindString("middleName", *user.middleName);
        if (user.lastName.has_value())
            stmt->bindString("lastName", *user.lastName);
        if (user.email.has_value())
            stmt->bindString("email", *user.email);
        if (user.needChangePassword.has_value())
            stmt->bindInt64("needChangePassword", *user.needChangePassword ? 1 : 0);
        if (user.isBlocked.has_value())
            stmt->bindInt64("isBlocked", *user.isBlocked ? 1 : 0);
        if (user.isSuperAdmin.has_value())
            stmt->bindInt64("isSuperAdmin", *user.isSuperAdmin ? 1 : 0);
        if (user.isHidden.has_value())
            stmt->bindInt64("isHidden", *user.isHidden ? 1 : 0);

        stmt->bindInt64("id", *user.id);

        const int64_t affected = stmt->execute();
        if (affected == 0)
            return false;

        // Пересчитываем нормализованные поля из актуального состояния БД.
        auto current = findById(*user.id);
        if (current.has_value())
        {
            std::string fullName;
            if (current->lastName.has_value() && !current->lastName->empty())
                fullName += *current->lastName;
            if (current->firstName.has_value() && !current->firstName->empty())
            {
                if (!fullName.empty())
                    fullName += ' ';
                fullName += *current->firstName;
            }
            if (current->middleName.has_value() && !current->middleName->empty())
            {
                if (!fullName.empty())
                    fullName += ' ';
                fullName += *current->middleName;
            }

            auto updateSearch = conn->prepareStatement(
                "UPDATE User SET searchLogin = :sl, searchName = :sn, "
                "searchEmail = :se WHERE id = :id"
            );
            updateSearch->bindString("sl", common::toLowerCase(current->login.value_or("")));
            updateSearch->bindString("sn", common::toLowerCase(fullName));
            updateSearch->bindString("se", common::toLowerCase(current->email.value_or("")));
            updateSearch->bindInt64("id", *current->id);
            updateSearch->execute();
        }

        return true;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка обновления пользователя: " << e.what();
        return false;
    }
}

bool SqliteUserRepository::remove(int64_t id)
{
    if (id <= 0)
    {
        LOG_WARN << "remove: неверный идентификатор " << id;
        return false;
    }

    try
    {
        auto conn = m_database->connection();
        auto stmt = conn->prepareStatement("DELETE FROM User WHERE id = :id");
        stmt->bindInt64("id", id);

        int64_t affected = stmt->execute();
        return affected > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка удаления пользователя: " << e.what();
        return false;
    }
}

std::shared_ptr<db::IConnection> SqliteUserRepository::connection() const
{
    return m_database->connection();
}

} // namespace repositories
} // namespace server
