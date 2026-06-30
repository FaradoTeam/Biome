#include "common/log/log.h"

#include "standard_day_service.h"

namespace server
{
namespace services
{

StandardDayService::StandardDayService(
    std::shared_ptr<repositories::IStandardDayRepository> standardDayRepo,
    std::shared_ptr<IAuthorizationService> authzService
)
    : m_standardDayRepo(std::move(standardDayRepo))
    , m_authzService(std::move(authzService))
{
    if (!m_standardDayRepo)
    {
        throw std::runtime_error("StandardDayService: репозиторий не инициализирован");
    }
    if (!m_authzService)
    {
        throw std::runtime_error("StandardDayService: сервис авторизации не инициализирован");
    }
}

std::vector<dto::StandardDay> StandardDayService::getAllStandardDays(
    int64_t userId
)
{
    // Проверяем права: только супер-админ может просматривать настройки календаря
    // или любой аутентифицированный пользователь (зависит от требований)
    // В данном случае разрешаем всем аутентифицированным
    if (userId <= 0)
    {
        LOG_WARN << "getAllStandardDays: попытка доступа без аутентификации";
        return {};
    }

    return m_standardDayRepo->findAll();
}

std::optional<dto::StandardDay> StandardDayService::getStandardDayByWeekDay(
    int weekDayNumber,
    int64_t userId
)
{
    if (userId <= 0)
    {
        LOG_WARN << "getStandardDayByWeekDay: попытка доступа без аутентификации";
        return std::nullopt;
    }

    if (weekDayNumber < 0 || weekDayNumber > 6)
    {
        LOG_WARN << "getStandardDayByWeekDay: неверный номер дня недели " << weekDayNumber;
        return std::nullopt;
    }

    return m_standardDayRepo->findByWeekDayNumber(weekDayNumber);
}

StandardDayResult StandardDayService::updateStandardDay(
    const dto::StandardDay& standardDay,
    int64_t userId
)
{
    StandardDayResult result;

    // 1. Проверяем права
    if (!canModifyCalendar(userId))
    {
        result.errorMessage = "Недостаточно прав для изменения стандартного дня";
        result.errorCode = 403;
        LOG_WARN << "updateStandardDay: пользователь " << userId << " не имеет прав";
        return result;
    }

    // 2. Валидация
    std::string errorMessage;
    if (!validateStandardDay(standardDay, errorMessage))
    {
        result.errorMessage = errorMessage;
        result.errorCode = 400;
        LOG_WARN << "updateStandardDay: " << errorMessage;
        return result;
    }

    // 3. Проверяем существование
    if (!standardDay.weekDayNumber.has_value())
    {
        result.errorMessage = "Номер дня недели обязателен";
        result.errorCode = 400;
        return result;
    }

    auto existing = m_standardDayRepo->findByWeekDayNumber(*standardDay.weekDayNumber);
    if (!existing.has_value())
    {
        result.errorMessage = "Стандартный день не найден";
        result.errorCode = 404;
        return result;
    }

    // 4. Обновляем
    if (!m_standardDayRepo->update(standardDay))
    {
        result.errorMessage = "Не удалось обновить стандартный день";
        result.errorCode = 500;
        LOG_ERROR << "updateStandardDay: ошибка обновления дня " << *standardDay.weekDayNumber;
        return result;
    }

    result.success = true;
    LOG_INFO
        << "Стандартный день обновлён: weekDayNumber=" << *standardDay.weekDayNumber
        << ", пользователь=" << userId;

    return result;
}

bool StandardDayService::canModifyCalendar(int64_t userId)
{
    // Только супер-админ может изменять настройки календаря
    return m_authzService->isSuperAdmin(userId);
}

bool StandardDayService::validateStandardDay(
    const dto::StandardDay& standardDay,
    std::string& errorMessage
)
{
    if (!standardDay.weekDayNumber.has_value())
    {
        errorMessage = "Номер дня недели обязателен";
        return false;
    }

    int weekDayNumber = *standardDay.weekDayNumber;
    if (weekDayNumber < 0 || weekDayNumber > 6)
    {
        errorMessage = "Номер дня недели должен быть в диапазоне 0-6";
        return false;
    }

    if (standardDay.isWorkDay.has_value() && *standardDay.isWorkDay)
    {
        // Если день рабочий, время должно быть указано
        if (!standardDay.beginWorkTime.has_value() || standardDay.beginWorkTime->empty())
        {
            errorMessage = "Для рабочего дня необходимо указать время начала работы";
            return false;
        }

        if (!standardDay.endWorkTime.has_value() || standardDay.endWorkTime->empty())
        {
            errorMessage = "Для рабочего дня необходимо указать время окончания работы";
            return false;
        }

        // Проверка формата времени HH:MM
        // TODO: добавить проверку формата
    }
    else if (standardDay.isWorkDay.has_value() && !*standardDay.isWorkDay)
    {
        // Для выходного дня время должно быть null
        if (standardDay.beginWorkTime.has_value() || standardDay.endWorkTime.has_value())
        {
            errorMessage = "Для выходного дня время работы должно быть пустым";
            return false;
        }

        if (standardDay.breakDuration.has_value() && *standardDay.breakDuration > 0)
        {
            errorMessage = "Для выходного дня длительность перерыва должна быть 0";
            return false;
        }
    }

    return true;
}

} // namespace services
} // namespace server
