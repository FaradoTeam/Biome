#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>

#include "../../app/navigation_manager.h"
#include "../../services/iuser_service.h"

#include "../screen.h"

namespace terminal::screens
{

/**
 * @brief Экран списка пользователей с фильтрацией в заголовке.
 *
 * Особенности:
 *  - Фильтры вводятся в полях над таблицей, применяются на сервере.
 *  - Навигация по строкам: ↑ / ↓ (работает, когда фокус на таблице).
 *  - Пагинация: кнопки «◀ Назад» / «Вперёд ▶».
 *  - Обновление данных: кнопка «Обновить».
 */
class UserListScreen final : public Screen
{
public:
    UserListScreen(
        ftxui::ScreenInteractive& screen,
        std::shared_ptr<services::IUserService> userService,
        std::shared_ptr<NavigationManager> nav
    );

    std::string title() const override { return "Пользователи"; }
    ftxui::Component component() override;
    void onResume() override;

private:
    void reload(); ///< Перезагрузить текущую страницу с текущими фильтрами
    void applyFilters(); ///< Сбросить страницу на 1 и перезагрузить
    void clearFilters(); ///< Очистить фильтры и перезагрузить
    void goToPrevPage();
    void goToNextPage();

    ftxui::Element renderTable() const;

private:
    ftxui::ScreenInteractive& m_screen;
    std::shared_ptr<services::IUserService> m_userService;
    std::shared_ptr<NavigationManager> m_nav;

    // --- Фильтры ---
    std::string m_filterLogin;
    std::string m_filterName;
    std::string m_filterEmail;
    int m_filterBlockedIndex { 0 }; ///< 0 — Все, 1 — Да, 2 — Нет

    // --- Пагинация ---
    int m_currentPage { 1 };
    int m_pageSize { 20 };
    int64_t m_totalCount { 0 };

    // --- Данные ---
    std::vector<models::User> m_users;
    int m_selectedRow { 0 };

    // --- UI-состояние ---
    bool m_loading { false };
    std::string m_status;

    // --- Компоненты ---
    ftxui::Component m_loginInput;
    ftxui::Component m_nameInput;
    ftxui::Component m_emailInput;
    ftxui::Component m_blockedDropdown;
    ftxui::Component m_applyButton;
    ftxui::Component m_clearButton;
    ftxui::Component m_prevButton;
    ftxui::Component m_nextButton;
    ftxui::Component m_refreshButton;

    /// Скрытая focusable-кнопка-якорь под таблицу.
    ftxui::Component m_tableAnchor;
    ftxui::Component m_tableComponent;
    ftxui::Component m_root;

    std::vector<std::string> m_blockedOptions {
        "Все", "Только заблокированные", "Только активные"
    };
};

} // namespace terminal::screens
