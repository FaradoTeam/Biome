#include "common/log/log.h"

#include "special_day_service.h"

namespace server
{
namespace services
{

SpecialDayService::SpecialDayService(
    std::shared_ptr<repositories::ISpecialDayRepository> specialDayRepo,
    std::shared_ptr<IAuthorizationService> authzService
)
    : m_specialDayRepo(std::move(specialDayRepo))
    , m_authzService(std::move(authzService))
{
    if (!m_specialDayRepo)
    {
        throw std::runtime_error("SpecialDayService: репозиторий не инициализирован");
    }
    if (!m_authzService)
    {
        throw std::runtime_error("SpecialDayService: сервис авторизации не инициализирован");
    }
}

SpecialDaysPage SpecialDayService::getSpecialDays(
    int page,
    int pageSize,
    int64_t userId,
    std::optional<int> year,
    std::optional<int> month
)
{
    if (userId <= 0)
    {
        LOG_WARN << "getSpecialDays: попытка доступа без аутентификации";
        return { {}, 0 };
    }

    if (page < 1)
        page = 1;
    if (pageSize < 1)
        pageSize = 20;
    if (pageSize > 100)
        pageSize = 100;

    // Проверяем год и месяц
    if (year.has_value() && (*year < 1970 || *year > 2100))
    {
        LOG_WARN << "getSpecialDays: неверный год " << *year;
        return { {}, 0 };
    }

    if (month.has_value() && (*month < 1 || *month > 12))
    {
        LOG_WARN << "getSpecialDays: неверный месяц " << *month;
        return { {}, 0 };
    }

    auto [days, total] = m_specialDayRepo->findAll(page, pageSize, year, month);
    return { days, total };
}

std::optional<dto::SpecialDay> SpecialDayService::getSpecialDay(
    int64_t id,
    int64_t userId
)
{
    if (userId <= 0)
    {
        LOG_WARN << "getSpecialDay: попытка доступа без аутентификации";
        return std::nullopt;
    }

    if (id <= 0)
    {
        LOG_WARN << "getSpecialDay: неверный ID " << id;
        return std::nullopt;
    }

    return m_specialDayRepo->findById(id);
}

std::optional<dto::SpecialDay> SpecialDayService::getSpecialDayByDate(
    const common::DateTime& date,
    int64_t userId
)
{
    if (userId <= 0)
    {
        LOG_WARN << "getSpecialDayByDate: попытка доступа без аутентификации";
        return std::nullopt;
    }

    return m_specialDayRepo->findByDate(date);
}

std::optional<dto::SpecialDay> SpecialDayService::createSpecialDay(
    const dto::SpecialDay& specialDay,
    int64_t userId
)
{
    // 1. Проверяем права
    if (!canModifyCalendar(userId))
    {
        LOG_WARN << "createSpecialDay: пользователь " << userId << " не имеет прав";
        return std::nullopt;
    }

    // 2. Валидация
    std::string errorMessage;
    if (!validateSpecialDay(specialDay, errorMessage))
    {
        LOG_WARN << "createSpecialDay: " << errorMessage;
        return std::nullopt;
    }

    // 3. Проверяем, что дата не совпадает с существующей
    if (specialDay.date.has_value())
    {
        auto existing = m_specialDayRepo->findByDate(*specialDay.date);
        if (existing.has_value())
        {
            LOG_WARN << "createSpecialDay: особый день для даты уже существует";
            return std::nullopt;
        }
    }

    // 4. Создаём
    int64_t newId = m_specialDayRepo->create(specialDay);
    if (newId <= 0)
    {
        LOG_ERROR << "createSpecialDay: не удалось создать особый день";
        return std::nullopt;
    }

    LOG_INFO
        << "Особый день создан: id=" << newId
        << ", пользователь=" << userId;

    return m_specialDayRepo->findById(newId);
}

std::optional<dto::SpecialDay> SpecialDayService::updateSpecialDay(
    const dto::SpecialDay& specialDay,
    int64_t userId
)
{
    // 1. Проверяем права
    if (!canModifyCalendar(userId))
    {
        LOG_WARN << "updateSpecialDay: пользователь " << userId << " не имеет прав";
        return std::nullopt;
    }

    // 2. Валидация
    std::string errorMessage;
    if (!validateSpecialDay(specialDay, errorMessage))
    {
        LOG_WARN << "updateSpecialDay: " << errorMessage;
        return std::nullopt;
    }

    // 3. Проверяем существование
    if (!specialDay.id.has_value())
    {
        LOG_WARN << "updateSpecialDay: отсутствует ID";
        return std::nullopt;
    }

    auto existing = m_specialDayRepo->findById(*specialDay.id);
    if (!existing.has_value())
    {
        LOG_WARN << "updateSpecialDay: особый день не найден, id=" << *specialDay.id;
        return std::nullopt;
    }

    // 4. Обновляем
    if (!m_specialDayRepo->update(specialDay))
    {
        LOG_ERROR << "updateSpecialDay: не удалось обновить особый день, id=" << *specialDay.id;
        return std::nullopt;
    }

    LOG_INFO
        << "Особый день обновлён: id=" << *specialDay.id
        << ", пользователь=" << userId;

    return m_specialDayRepo->findById(*specialDay.id);
}

SpecialDayResult SpecialDayService::deleteSpecialDay(
    int64_t id,
    int64_t userId
)
{
    SpecialDayResult result;

    // 1. Проверяем права
    if (!canModifyCalendar(userId))
    {
        result.errorMessage = "Недостаточно прав для удаления особого дня";
        result.errorCode = 403;
        LOG_WARN << "deleteSpecialDay: пользователь " << userId << " не имеет прав";
        return result;
    }

    // 2. Проверяем существование
    if (id <= 0)
    {
        result.errorMessage = "Неверный идентификатор";
        result.errorCode = 400;
        return result;
    }

    auto existing = m_specialDayRepo->findById(id);
    if (!existing.has_value())
    {
        result.errorMessage = "Особый день не найден";
        result.errorCode = 404;
        return result;
    }

    // 3. Удаляем
    if (!m_specialDayRepo->remove(id))
    {
        result.errorMessage = "Не удалось удалить особый день";
        result.errorCode = 500;
        LOG_ERROR << "deleteSpecialDay: ошибка удаления, id=" << id;
        return result;
    }

    result.success = true;
    LOG_INFO
        << "Особый день удалён: id=" << id
        << ", пользователь=" << userId;

    return result;
}

bool SpecialDayService::canModifyCalendar(int64_t userId)
{
    return m_authzService->isSuperAdmin(userId);
}

bool SpecialDayService::validateSpecialDay(
    const dto::SpecialDay& specialDay,
    std::string& errorMessage
)
{
    if (!specialDay.date.has_value())
    {
        errorMessage = "Дата обязательна для заполнения";
        return false;
    }

    if (specialDay.isWorkDay.has_value() && *specialDay.isWorkDay)
    {
        // Если день рабочий, время должно быть указано
        if (!specialDay.beginWorkTime.has_value() || specialDay.beginWorkTime->empty())
        {
            errorMessage = "Для рабочего дня необходимо указать время начала работы";
            return false;
        }

        if (!specialDay.endWorkTime.has_value() || specialDay.endWorkTime->empty())
        {
            errorMessage = "Для рабочего дня необходимо указать время окончания работы";
            return false;
        }
    }
    else if (specialDay.isWorkDay.has_value() && !*specialDay.isWorkDay)
    {
        // Для выходного дня время должно быть null
        if (specialDay.beginWorkTime.has_value() || specialDay.endWorkTime.has_value())
        {
            errorMessage = "Для выходного дня время работы должно быть пустым";
            return false;
        }

        if (specialDay.breakDuration.has_value() && *specialDay.breakDuration > 0)
        {
            errorMessage = "Для выходного дня длительность перерыва должна быть 0";
            return false;
        }
    }

    return true;
}

} // namespace services
} // namespace server
