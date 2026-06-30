#include "common/log/log.h"

#include "user_day_service.h"

namespace server
{
namespace services
{

UserDayService::UserDayService(
    std::shared_ptr<repositories::IUserDayRepository> userDayRepo,
    std::shared_ptr<repositories::IUserRepository> userRepo,
    std::shared_ptr<IAuthorizationService> authzService
)
    : m_userDayRepo(std::move(userDayRepo))
    , m_userRepo(std::move(userRepo))
    , m_authzService(std::move(authzService))
{
    if (!m_userDayRepo)
    {
        throw std::runtime_error("UserDayService: репозиторий пользовательских дней не инициализирован");
    }
    if (!m_userRepo)
    {
        throw std::runtime_error("UserDayService: репозиторий пользователей не инициализирован");
    }
    if (!m_authzService)
    {
        throw std::runtime_error("UserDayService: сервис авторизации не инициализирован");
    }
}

UserDaysPage UserDayService::getUserDays(
    int page,
    int pageSize,
    int64_t userId,
    std::optional<int64_t> filterUserId,
    std::optional<common::DateTime> dateFrom,
    std::optional<common::DateTime> dateTo
)
{
    if (userId <= 0)
    {
        LOG_WARN << "getUserDays: попытка доступа без аутентификации";
        return { {}, 0 };
    }

    if (page < 1)
        page = 1;
    if (pageSize < 1)
        pageSize = 20;
    if (pageSize > 100)
        pageSize = 100;

    // Проверяем доступ к фильтру по пользователю
    if (filterUserId.has_value() && *filterUserId != userId && !m_authzService->isSuperAdmin(userId))
    {
        LOG_WARN
            << "getUserDays: пользователь " << userId
            << " пытается получить дни другого пользователя " << *filterUserId;
        return { {}, 0 };
    }

    // Проверяем, что пользователь существует
    if (filterUserId.has_value() && !userExists(*filterUserId))
    {
        LOG_WARN << "getUserDays: пользователь не найден, userId=" << *filterUserId;
        return { {}, 0 };
    }

    // Для обычного пользователя показываем только его дни
    std::optional<int64_t> effectiveUserId = filterUserId;
    if (!m_authzService->isSuperAdmin(userId) && !effectiveUserId.has_value())
    {
        effectiveUserId = userId;
    }

    auto [days, total] = m_userDayRepo->findAll(
        page,
        pageSize,
        effectiveUserId,
        dateFrom,
        dateTo
    );

    return { days, total };
}

std::optional<dto::UserDay> UserDayService::getUserDay(
    int64_t id,
    int64_t userId
)
{
    if (userId <= 0)
    {
        LOG_WARN << "getUserDay: попытка доступа без аутентификации";
        return std::nullopt;
    }

    if (id <= 0)
    {
        LOG_WARN << "getUserDay: неверный ID " << id;
        return std::nullopt;
    }

    auto userDay = m_userDayRepo->findById(id);
    if (!userDay.has_value())
    {
        return std::nullopt;
    }

    // Проверяем доступ
    if (!canManageUserDay(*userDay->userId, userId))
    {
        LOG_WARN
            << "getUserDay: пользователь " << userId
            << " не имеет доступа к дню " << id;
        return std::nullopt;
    }

    return userDay;
}

std::optional<dto::UserDay> UserDayService::getUserDayByUserAndDate(
    int64_t userId,
    const common::DateTime& date,
    int64_t currentUserId
)
{
    if (currentUserId <= 0)
    {
        LOG_WARN << "getUserDayByUserAndDate: попытка доступа без аутентификации";
        return std::nullopt;
    }

    if (!userExists(userId))
    {
        LOG_WARN << "getUserDayByUserAndDate: пользователь не найден, userId=" << userId;
        return std::nullopt;
    }

    // Проверяем доступ
    if (!canManageUserDay(userId, currentUserId))
    {
        LOG_WARN
            << "getUserDayByUserAndDate: пользователь " << currentUserId
            << " не имеет доступа к дню пользователя " << userId;
        return std::nullopt;
    }

    return m_userDayRepo->findByUserAndDate(userId, date);
}

std::optional<dto::UserDay> UserDayService::createUserDay(
    const dto::UserDay& userDay,
    int64_t currentUserId
)
{
    // 1. Валидация
    std::string errorMessage;
    if (!validateUserDay(userDay, errorMessage))
    {
        LOG_WARN << "createUserDay: " << errorMessage;
        return std::nullopt;
    }

    // 2. Проверяем доступ
    if (!userDay.userId.has_value())
    {
        LOG_WARN << "createUserDay: отсутствует userId";
        return std::nullopt;
    }

    if (!canManageUserDay(*userDay.userId, currentUserId))
    {
        LOG_WARN
            << "createUserDay: пользователь " << currentUserId
            << " не имеет прав на создание дня для пользователя " << *userDay.userId;
        return std::nullopt;
    }

    // 3. Проверяем, что пользователь существует
    if (!userExists(*userDay.userId))
    {
        LOG_WARN << "createUserDay: пользователь не найден, userId=" << *userDay.userId;
        return std::nullopt;
    }

    // 4. Проверяем, не существует ли уже день для этой даты
    if (userDay.date.has_value())
    {
        auto existing = m_userDayRepo->findByUserAndDate(*userDay.userId, *userDay.date);
        if (existing.has_value())
        {
            LOG_WARN
                << "createUserDay: день уже существует для userId=" << *userDay.userId
                << " и даты";
            return std::nullopt;
        }
    }

    // 5. Создаём
    int64_t newId = m_userDayRepo->create(userDay);
    if (newId <= 0)
    {
        LOG_ERROR << "createUserDay: не удалось создать пользовательский день";
        return std::nullopt;
    }

    LOG_INFO
        << "Пользовательский день создан: id=" << newId
        << ", userId=" << *userDay.userId
        << ", пользователь=" << currentUserId;

    return m_userDayRepo->findById(newId);
}

std::optional<dto::UserDay> UserDayService::updateUserDay(
    const dto::UserDay& userDay,
    int64_t currentUserId
)
{
    // 1. Проверяем существование
    if (!userDay.id.has_value())
    {
        LOG_WARN << "updateUserDay: отсутствует ID";
        return std::nullopt;
    }

    auto existing = m_userDayRepo->findById(*userDay.id);
    if (!existing.has_value())
    {
        LOG_WARN << "updateUserDay: пользовательский день не найден, id=" << *userDay.id;
        return std::nullopt;
    }

    // 2. Проверяем доступ
    if (!canManageUserDay(*existing->userId, currentUserId))
    {
        LOG_WARN
            << "updateUserDay: пользователь " << currentUserId
            << " не имеет прав на изменение дня " << *userDay.id;
        return std::nullopt;
    }

    // 3. Валидация
    std::string errorMessage;
    if (!validateUserDay(userDay, errorMessage))
    {
        LOG_WARN << "updateUserDay: " << errorMessage;
        return std::nullopt;
    }

    // 4. Если меняется userId, проверяем существование нового пользователя
    if (userDay.userId.has_value() && *userDay.userId != *existing->userId)
    {
        if (!userExists(*userDay.userId))
        {
            LOG_WARN << "updateUserDay: новый пользователь не найден, userId=" << *userDay.userId;
            return std::nullopt;
        }
    }

    // 5. Если меняется userId или date, проверяем конфликты
    int64_t targetUserId = userDay.userId.has_value() ? *userDay.userId : *existing->userId;
    common::DateTime targetDate = userDay.date.has_value() ? *userDay.date : *existing->date;

    if ((userDay.userId.has_value() && *userDay.userId != *existing->userId) || (userDay.date.has_value() && *userDay.date != *existing->date))
    {
        auto conflicting = m_userDayRepo->findByUserAndDate(targetUserId, targetDate);
        if (conflicting.has_value() && *conflicting->id != *userDay.id)
        {
            LOG_WARN
                << "updateUserDay: конфликт, день для userId=" << targetUserId
                << " и даты уже существует";
            return std::nullopt;
        }
    }

    // 6. Обновляем
    if (!m_userDayRepo->update(userDay))
    {
        LOG_ERROR << "updateUserDay: не удалось обновить день, id=" << *userDay.id;
        return std::nullopt;
    }

    LOG_INFO
        << "Пользовательский день обновлён: id=" << *userDay.id
        << ", пользователь=" << currentUserId;

    return m_userDayRepo->findById(*userDay.id);
}

UserDayResult UserDayService::deleteUserDay(
    int64_t id,
    int64_t currentUserId
)
{
    UserDayResult result;

    // 1. Проверяем существование
    if (id <= 0)
    {
        result.errorMessage = "Неверный идентификатор";
        result.errorCode = 400;
        return result;
    }

    auto existing = m_userDayRepo->findById(id);
    if (!existing.has_value())
    {
        result.errorMessage = "Пользовательский день не найден";
        result.errorCode = 404;
        return result;
    }

    // 2. Проверяем доступ
    if (!canManageUserDay(*existing->userId, currentUserId))
    {
        result.errorMessage = "Недостаточно прав для удаления дня";
        result.errorCode = 403;
        LOG_WARN
            << "deleteUserDay: пользователь " << currentUserId
            << " не имеет прав на удаление дня " << id;
        return result;
    }

    // 3. Удаляем
    if (!m_userDayRepo->remove(id))
    {
        result.errorMessage = "Не удалось удалить пользовательский день";
        result.errorCode = 500;
        LOG_ERROR << "deleteUserDay: ошибка удаления, id=" << id;
        return result;
    }

    result.success = true;
    LOG_INFO
        << "Пользовательский день удалён: id=" << id
        << ", пользователь=" << currentUserId;

    return result;
}

int64_t UserDayService::deleteUserDaysByUser(
    int64_t userId,
    int64_t currentUserId
)
{
    if (userId <= 0)
    {
        LOG_WARN << "deleteUserDaysByUser: неверный userId " << userId;
        return 0;
    }

    // Проверяем доступ
    if (!canManageUserDay(userId, currentUserId))
    {
        LOG_WARN
            << "deleteUserDaysByUser: пользователь " << currentUserId
            << " не имеет прав на удаление дней пользователя " << userId;
        return 0;
    }

    // Проверяем, что пользователь существует
    if (!userExists(userId))
    {
        LOG_WARN << "deleteUserDaysByUser: пользователь не найден, userId=" << userId;
        return 0;
    }

    return m_userDayRepo->removeByUserId(userId);
}

bool UserDayService::canManageUserDay(int64_t targetUserId, int64_t currentUserId)
{
    // Супер-админ может управлять днями всех пользователей
    if (m_authzService->isSuperAdmin(currentUserId))
    {
        return true;
    }

    // Обычный пользователь может управлять только своими днями
    return targetUserId == currentUserId;
}

bool UserDayService::userExists(int64_t userId)
{
    return m_userRepo->findById(userId).has_value();
}

bool UserDayService::validateUserDay(
    const dto::UserDay& userDay,
    std::string& errorMessage
)
{
    if (!userDay.userId.has_value())
    {
        errorMessage = "Идентификатор пользователя обязателен";
        return false;
    }

    if (!userDay.date.has_value())
    {
        errorMessage = "Дата обязательна для заполнения";
        return false;
    }

    if (userDay.isWorkDay.has_value() && *userDay.isWorkDay)
    {
        // Если день рабочий, время должно быть указано
        if (!userDay.beginWorkTime.has_value() || userDay.beginWorkTime->empty())
        {
            errorMessage = "Для рабочего дня необходимо указать время начала работы";
            return false;
        }

        if (!userDay.endWorkTime.has_value() || userDay.endWorkTime->empty())
        {
            errorMessage = "Для рабочего дня необходимо указать время окончания работы";
            return false;
        }
    }
    else if (userDay.isWorkDay.has_value() && !*userDay.isWorkDay)
    {
        // Для выходного дня время должно быть null
        if (userDay.beginWorkTime.has_value() || userDay.endWorkTime.has_value())
        {
            errorMessage = "Для выходного дня время работы должно быть пустым";
            return false;
        }

        if (userDay.breakDuration.has_value() && *userDay.breakDuration > 0)
        {
            errorMessage = "Для выходного дня длительность перерыва должна быть 0";
            return false;
        }
    }

    return true;
}

} // namespace services
} // namespace server
