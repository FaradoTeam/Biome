#pragma once

#include <iomanip>
#include <sstream>

#include <openssl/evp.h>

#include "../migration.h"

namespace db
{
namespace migrations
{

/**
 * @brief Миграция v2: Добавление дефолтных данных.
 *
 * Добавляет:
 * - Супер-администратора (login: admin, password: password)
 * - Базовый рабочий процесс с состояниями на русском языке
 * - StandardDay (7 записей: ПН-ПТ рабочие 9:00-18:00, СБ-ВС выходные)
 */
class V2_DefaultData final : public IMigration
{
public:
    unsigned int version() const override
    {
        return 2;
    }

    std::string description() const override
    {
        return "Добавляет дефолтного супер-админа, базовый рабочий процесс "
               "и стандартные рабочие дни.";
    }

    void up(std::shared_ptr<IConnection> connection) override
    {
        // ============================================================
        // 1. Добавление супер-администратора
        // ============================================================
        {
            const std::string passwordHash = hashPassword("password");

            auto stmt = connection->prepareStatement(
                "INSERT INTO User (login, firstName, lastName, email, passwordHash, "
                "needChangePassword, isBlocked, isSuperAdmin, isHidden) "
                "VALUES (:login, :firstName, :lastName, :email, :passwordHash, "
                ":needChangePassword, :isBlocked, :isSuperAdmin, :isHidden)"
            );

            stmt->bindString("login", "admin");
            stmt->bindString("firstName", "Системный");
            stmt->bindString("lastName", "Администратор");
            stmt->bindString("email", "admin@mail.local");
            stmt->bindString("passwordHash", passwordHash);
            stmt->bindInt64("needChangePassword", 1);
            stmt->bindInt64("isBlocked", 0);
            stmt->bindInt64("isSuperAdmin", 1);
            stmt->bindInt64("isHidden", 0);

            stmt->execute();
        }

        // ============================================================
        // 2. Добавление дефолтного рабочего процесса
        // ============================================================
        int64_t workflowId = 0;
        {
            auto stmt = connection->prepareStatement(
                "INSERT INTO Workflow (caption, description) "
                "VALUES (:caption, :description)"
            );

            stmt->bindString("caption", "Базовый процесс");
            stmt->bindString("description", "Стандартный рабочий процесс с базовыми состояниями");

            stmt->execute();
            workflowId = connection->lastInsertId();
        }

        // Состояния (States) для рабочего процесса
        int64_t newId = 0;
        int64_t inProgressId = 0;
        int64_t reviewId = 0;
        int64_t doneId = 0;
        int64_t archiveId = 0;

        // Новая
        {
            auto stmt = connection->prepareStatement(
                "INSERT INTO State (workflowId, caption, description, weight, "
                "orderNumber, isArchive, isQueue) "
                "VALUES (:workflowId, :caption, :description, :weight, "
                ":orderNumber, :isArchive, :isQueue)"
            );

            stmt->bindInt64("workflowId", workflowId);
            stmt->bindString("caption", "Новая");
            stmt->bindString("description", "Новая задача, ожидает начала работ");
            stmt->bindInt64("weight", 0);
            stmt->bindInt64("orderNumber", 1);
            stmt->bindInt64("isArchive", 0);
            stmt->bindInt64("isQueue", 0);
            stmt->execute();

            newId = connection->lastInsertId();
        }

        // В работе
        {
            auto stmt = connection->prepareStatement(
                "INSERT INTO State (workflowId, caption, description, weight, "
                "orderNumber, isArchive, isQueue) "
                "VALUES (:workflowId, :caption, :description, :weight, "
                ":orderNumber, :isArchive, :isQueue)"
            );

            stmt->bindInt64("workflowId", workflowId);
            stmt->bindString("caption", "В работе");
            stmt->bindString("description", "Задача находится в процессе выполнения");
            stmt->bindInt64("weight", 50);
            stmt->bindInt64("orderNumber", 2);
            stmt->bindInt64("isArchive", 0);
            stmt->bindInt64("isQueue", 0);
            stmt->execute();

            inProgressId = connection->lastInsertId();
        }

        // На проверке
        {
            auto stmt = connection->prepareStatement(
                "INSERT INTO State (workflowId, caption, description, weight, "
                "orderNumber, isArchive, isQueue) "
                "VALUES (:workflowId, :caption, :description, :weight, "
                ":orderNumber, :isArchive, :isQueue)"
            );

            stmt->bindInt64("workflowId", workflowId);
            stmt->bindString("caption", "На проверке");
            stmt->bindString("description", "Задача ожидает проверки/ревью");
            stmt->bindInt64("weight", 75);
            stmt->bindInt64("orderNumber", 3);
            stmt->bindInt64("isArchive", 0);
            stmt->bindInt64("isQueue", 0);
            stmt->execute();

            reviewId = connection->lastInsertId();
        }

        // Завершена
        {
            auto stmt = connection->prepareStatement(
                "INSERT INTO State (workflowId, caption, description, weight, "
                "orderNumber, isArchive, isQueue) "
                "VALUES (:workflowId, :caption, :description, :weight, "
                ":orderNumber, :isArchive, :isQueue)"
            );

            stmt->bindInt64("workflowId", workflowId);
            stmt->bindString("caption", "Завершена");
            stmt->bindString("description", "Задача успешно завершена");
            stmt->bindInt64("weight", 100);
            stmt->bindInt64("orderNumber", 4);
            stmt->bindInt64("isArchive", 0);
            stmt->bindInt64("isQueue", 0);
            stmt->execute();

            doneId = connection->lastInsertId();
        }

        // Архив
        {
            auto stmt = connection->prepareStatement(
                "INSERT INTO State (workflowId, caption, description, weight, "
                "orderNumber, isArchive, isQueue) "
                "VALUES (:workflowId, :caption, :description, :weight, "
                ":orderNumber, :isArchive, :isQueue)"
            );

            stmt->bindInt64("workflowId", workflowId);
            stmt->bindString("caption", "Архив");
            stmt->bindString("description", "Задача помещена в архив");
            stmt->bindInt64("weight", 100);
            stmt->bindInt64("orderNumber", 5);
            stmt->bindInt64("isArchive", 1);
            stmt->bindInt64("isQueue", 0);
            stmt->execute();

            archiveId = connection->lastInsertId();
        }

        // Переходы (Edges) между состояниями
        {
            auto stmt = connection->prepareStatement(
                "INSERT INTO Edge (beginStateId, endStateId) "
                "VALUES (:beginStateId, :endStateId)"
            );

            // Новая → В работе
            stmt->bindInt64("beginStateId", newId);
            stmt->bindInt64("endStateId", inProgressId);
            stmt->execute();
            stmt->reset();

            // В работе → На проверке
            stmt->bindInt64("beginStateId", inProgressId);
            stmt->bindInt64("endStateId", reviewId);
            stmt->execute();
            stmt->reset();

            // На проверке → В работе (возврат на доработку)
            stmt->bindInt64("beginStateId", reviewId);
            stmt->bindInt64("endStateId", inProgressId);
            stmt->execute();
            stmt->reset();

            // На проверке → Завершена
            stmt->bindInt64("beginStateId", reviewId);
            stmt->bindInt64("endStateId", doneId);
            stmt->execute();
            stmt->reset();

            // В работе → Завершена (прямое завершение без проверки)
            stmt->bindInt64("beginStateId", inProgressId);
            stmt->bindInt64("endStateId", doneId);
            stmt->execute();
            stmt->reset();

            // Завершена → Архив
            stmt->bindInt64("beginStateId", doneId);
            stmt->bindInt64("endStateId", archiveId);
            stmt->execute();
        }

        // ============================================================
        // 3. Добавление StandardDay (7 записей)
        // ============================================================
        {
            auto stmt = connection->prepareStatement(
                "INSERT INTO StandardDay (weekDayNumber, weekOrder, isWorkDay, "
                "beginWorkTime, endWorkTime, breakDuration) "
                "VALUES (:weekDayNumber, :weekOrder, :isWorkDay, "
                ":beginWorkTime, :endWorkTime, :breakDuration)"
            );

            // Понедельник (1) - рабочий
            stmt->bindInt64("weekDayNumber", 1);
            stmt->bindInt64("weekOrder", 0);
            stmt->bindInt64("isWorkDay", 1);
            stmt->bindString("beginWorkTime", "09:00");
            stmt->bindString("endWorkTime", "18:00");
            stmt->bindInt64("breakDuration", 60);
            stmt->execute();
            stmt->reset();

            // Вторник (2) - рабочий
            stmt->bindInt64("weekDayNumber", 2);
            stmt->bindInt64("weekOrder", 0);
            stmt->bindInt64("isWorkDay", 1);
            stmt->bindString("beginWorkTime", "09:00");
            stmt->bindString("endWorkTime", "18:00");
            stmt->bindInt64("breakDuration", 60);
            stmt->execute();
            stmt->reset();

            // Среда (3) - рабочий
            stmt->bindInt64("weekDayNumber", 3);
            stmt->bindInt64("weekOrder", 0);
            stmt->bindInt64("isWorkDay", 1);
            stmt->bindString("beginWorkTime", "09:00");
            stmt->bindString("endWorkTime", "18:00");
            stmt->bindInt64("breakDuration", 60);
            stmt->execute();
            stmt->reset();

            // Четверг (4) - рабочий
            stmt->bindInt64("weekDayNumber", 4);
            stmt->bindInt64("weekOrder", 0);
            stmt->bindInt64("isWorkDay", 1);
            stmt->bindString("beginWorkTime", "09:00");
            stmt->bindString("endWorkTime", "18:00");
            stmt->bindInt64("breakDuration", 60);
            stmt->execute();
            stmt->reset();

            // Пятница (5) - рабочий
            stmt->bindInt64("weekDayNumber", 5);
            stmt->bindInt64("weekOrder", 0);
            stmt->bindInt64("isWorkDay", 1);
            stmt->bindString("beginWorkTime", "09:00");
            stmt->bindString("endWorkTime", "18:00");
            stmt->bindInt64("breakDuration", 60);
            stmt->execute();
            stmt->reset();

            // Суббота (6) - выходной
            stmt->bindInt64("weekDayNumber", 6);
            stmt->bindInt64("weekOrder", 0);
            stmt->bindInt64("isWorkDay", 0);
            stmt->bindNull("beginWorkTime");
            stmt->bindNull("endWorkTime");
            stmt->bindInt64("breakDuration", 0);
            stmt->execute();
            stmt->reset();

            // Воскресенье (0) - выходной
            stmt->bindInt64("weekDayNumber", 0);
            stmt->bindInt64("weekOrder", 0);
            stmt->bindInt64("isWorkDay", 0);
            stmt->bindNull("beginWorkTime");
            stmt->bindNull("endWorkTime");
            stmt->bindInt64("breakDuration", 0);
            stmt->execute();
        }
    }

    void down(std::shared_ptr<IConnection> connection) override
    {
        // Удаляем StandardDay (все 7 записей)
        connection->execute("DELETE FROM StandardDay");

        // Удаляем переходы (Edges)
        connection->execute("DELETE FROM Edge");

        // Удаляем состояния (States)
        connection->execute("DELETE FROM State");

        // Удаляем дефолтный Workflow
        connection->execute("DELETE FROM Workflow WHERE caption = 'Базовый процесс'");

        // Удаляем супер-админа
        connection->execute("DELETE FROM User WHERE login = 'admin'");
    }

private:
    /**
     * @brief Хеширует пароль с использованием SHA-256.
     * @param password Пароль в открытом виде
     * @return Хеш пароля в hex-формате
     */
    static std::string hashPassword(const std::string& password)
    {
        unsigned char hash[EVP_MAX_MD_SIZE];
        unsigned int hashLen = 0;

        EVP_MD_CTX* ctx = EVP_MD_CTX_new();
        EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr);
        EVP_DigestUpdate(ctx, password.c_str(), password.length());
        EVP_DigestFinal_ex(ctx, hash, &hashLen);
        EVP_MD_CTX_free(ctx);

        std::stringstream ss;
        for (unsigned int i = 0; i < hashLen; ++i)
        {
            ss
                << std::hex << std::setw(2) << std::setfill('0')
                << static_cast<int>(hash[i]);
        }
        return ss.str();
    }
};

} // namespace migrations
} // namespace db
