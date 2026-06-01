#include "common/log/log.h"

#include "phase_service.h"

namespace
{
bool validatePhase(const dto::Phase& phase, std::string& errorMessage)
{
    // Проверка обязательного поля caption
    if (!phase.caption.has_value() || phase.caption->empty())
    {
        errorMessage = "Название фазы обязательно для заполнения";
        return false;
    }

    // Проверка длины caption
    if (phase.caption->length() > 255)
    {
        errorMessage = "Название фазы не может превышать 255 символов";
        return false;
    }

    // Проверка обязательного поля projectId
    if (!phase.projectId.has_value())
    {
        errorMessage = "Идентификатор проекта обязателен";
        return false;
    }

    // Проверка длины description (если указано)
    if (phase.description.has_value() && phase.description->length() > 1000)
    {
        errorMessage = "Описание фазы не может превышать 1000 символов";
        return false;
    }

    // Проверка корректности дат (если указаны)
    if (phase.beginDate.has_value() && phase.endDate.has_value())
    {
        if (*phase.beginDate > *phase.endDate)
        {
            errorMessage = "Дата начала не может быть позже даты окончания";
            return false;
        }
    }

    return true;
}
} // namespace

namespace server
{
namespace services
{

PhaseService::PhaseService(
    std::shared_ptr<repositories::IPhaseRepository> phaseRepo,
    std::shared_ptr<IAuthorizationService> authzService
)
    : m_phaseRepo(std::move(phaseRepo))
    , m_authzService(std::move(authzService))
{
    if (!m_phaseRepo)
    {
        throw std::runtime_error("PhaseRepository не может быть пустым");
    }
    if (!m_authzService)
    {
        throw std::runtime_error("AuthorizationService не может быть пустым");
    }
}

PhasesPage PhaseService::phases(
    int page,
    int pageSize,
    int64_t userId,
    std::optional<int64_t> projectId,
    std::optional<bool> isArchive
)
{
    if (page < 1)
        page = 1;
    if (pageSize < 1)
        pageSize = 20;

    // Если указан projectId, проверяем права на чтение этого проекта
    if (projectId.has_value())
    {
        auto authz = m_authzService->canReadProject(userId, *projectId);
        if (!authz.granted)
        {
            LOG_WARN
                << "Пользователь " << userId
                << " не имеет доступа на чтение к проекту " << *projectId;
            return { {}, 0 };
        }
    }

    // Получаем фазы из репозитория
    auto [phases, total] = m_phaseRepo->findAll(
        page,
        pageSize,
        projectId,
        isArchive
    );

    // Если projectId не указан, фильтруем фазы по правам доступа
    if (!projectId.has_value() && !m_authzService->isSuperAdmin(userId))
    {
        std::vector<dto::Phase> filteredPhases;
        for (const auto& phase : phases)
        {
            if (!phase.projectId.has_value())
                continue;

            auto authz = m_authzService->canReadProject(userId, *phase.projectId);
            if (authz.granted)
            {
                filteredPhases.push_back(phase);
            }
        }
        return { filteredPhases, static_cast<int64_t>(filteredPhases.size()) };
    }

    return { phases, total };
}

std::optional<dto::Phase> PhaseService::phase(
    int64_t id,
    int64_t userId
)
{
    if (id <= 0)
    {
        LOG_WARN << "phase: неверный id " << id;
        return std::nullopt;
    }

    // Получаем фазу
    auto phase = m_phaseRepo->findById(id);
    if (!phase.has_value())
    {
        LOG_DEBUG << "phase: фаза " << id << " не найдена";
        return std::nullopt;
    }

    // Проверяем права на чтение проекта
    if (!phase->projectId.has_value())
    {
        LOG_WARN << "phase: фаза " << id << " не имеет projectId";
        return std::nullopt;
    }

    auto authz = m_authzService->canReadProject(userId, *phase->projectId);
    if (!authz.granted)
    {
        LOG_WARN
            << "Пользователь " << userId
            << " не имеет доступа на чтение к фазе " << id;
        return std::nullopt;
    }

    return phase;
}

std::optional<dto::Phase> PhaseService::createPhase(
    const dto::Phase& phase,
    int64_t userId
)
{
    // Валидация
    std::string errorMessage;
    if (!validatePhase(phase, errorMessage))
    {
        LOG_WARN << "createPhase: проверка не пройдена - " << errorMessage;
        return std::nullopt;
    }

    // Проверяем права на редактирование фаз в проекте
    auto authz = m_authzService->canEditPhases(userId, *phase.projectId);
    if (!authz.granted)
    {
        LOG_WARN
            << "Пользователь " << userId
            << " не имеет прав на создание фазы в проекте " << *phase.projectId;
        return std::nullopt;
    }

    // Создаём фазу
    const int64_t newId = m_phaseRepo->create(phase);
    if (newId <= 0)
    {
        LOG_ERROR << "createPhase: не удалось создать фазу";
        return std::nullopt;
    }

    LOG_INFO
        << "Пользователь " << userId
        << " создал фазу id=" << newId
        << " в проекте " << *phase.projectId;

    return m_phaseRepo->findById(newId);
}

std::optional<dto::Phase> PhaseService::updatePhase(
    const dto::Phase& phase,
    int64_t userId
)
{
    if (!phase.id.has_value())
    {
        LOG_WARN << "updatePhase: отсутствует ID фазы";
        return std::nullopt;
    }

    // Получаем существующую фазу
    auto existing = m_phaseRepo->findById(*phase.id);
    if (!existing.has_value())
    {
        LOG_WARN << "updatePhase: фаза " << *phase.id << " не найдена";
        return std::nullopt;
    }

    if (!existing->projectId.has_value())
    {
        LOG_WARN << "updatePhase: фаза " << *phase.id << " не имеет projectId";
        return std::nullopt;
    }

    // Проверяем права на редактирование фаз в проекте
    auto authz = m_authzService->canEditPhases(userId, *existing->projectId);
    if (!authz.granted)
    {
        LOG_WARN
            << "Пользователь " << userId
            << " не имеет прав на обновление фазы " << *phase.id;
        return std::nullopt;
    }

    // Если меняется projectId, проверяем права на новый проект
    if (phase.projectId.has_value() && *phase.projectId != *existing->projectId)
    {
        auto newProjectAuthz = m_authzService->canEditPhases(userId, *phase.projectId);
        if (!newProjectAuthz.granted)
        {
            LOG_WARN
                << "Пользователь " << userId
                << " не имеет прав на перемещение фазы в проект " << *phase.projectId;
            return std::nullopt;
        }
    }

    // Выполняем обновление
    if (!m_phaseRepo->update(phase))
    {
        LOG_ERROR << "updatePhase: не удалось обновить фазу " << *phase.id;
        return std::nullopt;
    }

    LOG_INFO
        << "Пользователь " << userId
        << " обновил фазу " << *phase.id;

    return m_phaseRepo->findById(*phase.id);
}

bool PhaseService::archivePhase(
    int64_t id,
    int64_t userId
)
{
    if (id <= 0)
    {
        LOG_WARN << "archivePhase: неверный id " << id;
        return false;
    }

    // Получаем фазу
    auto phase = m_phaseRepo->findById(id);
    if (!phase.has_value())
    {
        LOG_WARN << "archivePhase: фаза " << id << " не найдена";
        return false;
    }

    if (!phase->projectId.has_value())
    {
        LOG_WARN << "archivePhase: фаза " << id << " не имеет projectId";
        return false;
    }

    // Проверяем права на редактирование фаз в проекте
    auto authz = m_authzService->canEditPhases(userId, *phase->projectId);
    if (!authz.granted)
    {
        LOG_WARN
            << "Пользователь " << userId
            << " не имеет прав на архивацию фазы " << id;
        return false;
    }

    // Архивируем
    if (!m_phaseRepo->archive(id))
    {
        LOG_ERROR << "archivePhase: не удалось архивировать фазу " << id;
        return false;
    }

    LOG_INFO
        << "Пользователь " << userId
        << " архивировал фазу " << id;
    return true;
}

bool PhaseService::restorePhase(
    int64_t id,
    int64_t userId
)
{
    if (id <= 0)
    {
        LOG_WARN << "restorePhase: неверный id " << id;
        return false;
    }

    // Получаем фазу
    auto phase = m_phaseRepo->findById(id);
    if (!phase.has_value())
    {
        LOG_WARN << "restorePhase: фаза " << id << " не найдена";
        return false;
    }

    if (!phase->projectId.has_value())
    {
        LOG_WARN << "restorePhase: фаза " << id << " не имеет projectId";
        return false;
    }

    // Проверяем права на редактирование фаз в проекте
    auto authz = m_authzService->canEditPhases(userId, *phase->projectId);
    if (!authz.granted)
    {
        LOG_WARN
            << "Пользователь " << userId
            << " не имеет прав на восстановление фазы " << id;
        return false;
    }

    // Восстанавливаем
    if (!m_phaseRepo->restore(id))
    {
        LOG_ERROR << "restorePhase: не удалось восстановить фазу " << id;
        return false;
    }

    LOG_INFO
        << "Пользователь " << userId
        << " восстановил фазу " << id;
    return true;
}

} // namespace services
} // namespace server
