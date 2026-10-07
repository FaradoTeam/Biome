#pragma once

#include <string>
#include <vector>

#include "common/helpers/string_helper.h"

#include "../migration.h"

namespace db
{
namespace migrations
{

/**
 * @brief Миграция v3: добавление нормализованных полей для поиска.
 *
 * SQLite-функция LOWER() работает с кириллицей только при сборке с ICU,
 * поэтому нормализуем строки в C++ через common::toLowerCase() и храним
 * результаты в отдельных столбцах searchLogin / searchName / searchEmail.
 */
class V3_UserSearchFields final : public IMigration
{
public:
    unsigned int version() const override { return 3; }

    std::string description() const override
    {
        return "Добавляет нормализованные поля searchLogin, searchName, "
               "searchEmail в таблицу User.";
    }

    void up(std::shared_ptr<IConnection> connection) override
    {
        connection->execute("ALTER TABLE User ADD COLUMN searchLogin TEXT");
        connection->execute("ALTER TABLE User ADD COLUMN searchName  TEXT");
        connection->execute("ALTER TABLE User ADD COLUMN searchEmail TEXT");

        // Заполняем существующие записи, читая их и нормализуя в C++.
        std::vector<int64_t> ids;
        std::vector<std::string> searchLogins;
        std::vector<std::string> searchNames;
        std::vector<std::string> searchEmails;

        {
            auto stmt = connection->prepareStatement(
                "SELECT id, login, firstName, middleName, lastName, email FROM User"
            );
            auto rs = stmt->executeQuery();
            while (rs->next())
            {
                ids.push_back(rs->valueInt64("id"));

                const std::string login = rs->valueString("login");

                const std::string firstName = rs->isNull("firstName")
                    ? std::string()
                    : rs->valueString("firstName");
                const std::string middleName = rs->isNull("middleName")
                    ? std::string()
                    : rs->valueString("middleName");
                const std::string lastName = rs->isNull("lastName")
                    ? std::string()
                    : rs->valueString("lastName");

                const std::string email = rs->valueString("email");

                std::string fullName;
                if (!lastName.empty())
                    fullName += lastName;
                if (!firstName.empty())
                {
                    if (!fullName.empty())
                        fullName += ' ';
                    fullName += firstName;
                }
                if (!middleName.empty())
                {
                    if (!fullName.empty())
                        fullName += ' ';
                    fullName += middleName;
                }

                searchLogins.push_back(common::toLowerCase(login));
                searchNames.push_back(common::toLowerCase(fullName));
                searchEmails.push_back(common::toLowerCase(email));
            }
        }

        for (size_t i = 0; i < ids.size(); ++i)
        {
            auto update = connection->prepareStatement(
                "UPDATE User SET searchLogin = :sl, searchName = :sn, "
                "searchEmail = :se WHERE id = :id"
            );
            update->bindString("sl", searchLogins[i]);
            update->bindString("sn", searchNames[i]);
            update->bindString("se", searchEmails[i]);
            update->bindInt64("id", ids[i]);
            update->execute();
        }
    }

    void down(std::shared_ptr<IConnection> connection) override
    {
        // SQLite >= 3.35 поддерживает DROP COLUMN; версия в проекте — 3.51.
        connection->execute("ALTER TABLE User DROP COLUMN searchEmail");
        connection->execute("ALTER TABLE User DROP COLUMN searchName");
        connection->execute("ALTER TABLE User DROP COLUMN searchLogin");
    }
};

} // namespace migrations
} // namespace db
