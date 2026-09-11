#include "user_list_screen.h"

#include <thread>

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/table.hpp>

#include "common/log/log.h"

namespace terminal::screens
{

using namespace ftxui;

namespace
{

/// Отображает optional-строку как значение или «—».
std::string strOrDash(const std::optional<std::string>& value)
{
    return (value.has_value() && !value->empty()) ? *value : "—";
}

/// Отображает optional-флаг как Да/Нет/—.
std::string boolOrDash(
    const std::optional<bool>& value,
    const std::string& yes = "Да",
    const std::string& no = "Нет"
)
{
    if (!value.has_value())
        return "—";
    return *value ? yes : no;
}

/// Обрезает строку до заданной ширины (UTF-8 приблизительно).
std::string truncate(const std::string& s, size_t maxWidth)
{
    if (s.size() <= maxWidth)
        return s;
    if (maxWidth < 2)
        return s.substr(0, maxWidth);
    return s.substr(0, maxWidth - 1) + "…";
}

} // namespace

UserListScreen::UserListScreen(
    ftxui::ScreenInteractive& screen,
    std::shared_ptr<services::IUserService> userService,
    std::shared_ptr<NavigationManager> nav
)
    : m_screen(screen)
    , m_userService(std::move(userService))
    , m_nav(std::move(nav))
{
    // ==== Фильтры ====
    m_loginInput = Input(&m_filterLogin, "Логин");
    m_nameInput = Input(&m_filterName, "ФИО");
    m_emailInput = Input(&m_filterEmail, "Email");

    // По Enter в любом из полей — применить фильтры.
    auto applyOnEnter = [this](Event e)
    {
        if (e == Event::Return)
        {
            applyFilters();
            return true;
        }
        return false;
    };
    m_loginInput |= CatchEvent(applyOnEnter);
    m_nameInput |= CatchEvent(applyOnEnter);
    m_emailInput |= CatchEvent(applyOnEnter);

    m_blockedDropdown = Dropdown(
        &m_blockedOptions,
        &m_filterBlockedIndex
    );

    m_applyButton = Button(
        "Применить",
        [this]
        { applyFilters(); },
        ButtonOption::Ascii()
    );
    m_clearButton = Button(
        "Очистить",
        [this]
        { clearFilters(); },
        ButtonOption::Ascii()
    );

    // ==== Пагинация ====
    m_prevButton = Button(
        "◀ Назад",
        [this]
        { goToPrevPage(); },
        ButtonOption::Ascii()
    );
    m_nextButton = Button(
        "Вперёд ▶",
        [this]
        { goToNextPage(); },
        ButtonOption::Ascii()
    );
    m_refreshButton = Button(
        "Обновить",
        [this]
        { reload(); },
        ButtonOption::Ascii()
    );

    // ==== Таблица ====
    //
    // В ftxui 7.x старый Focusable(Component) убран. Чтобы таблица
    // могла получать клавиатурные события, используем скрытый
    // focusable-якорь — пустую Button. Renderer(child, fn) наследует
    // focusable-состояние от child, а fn используется для отрисовки
    // нашей таблицы. CatchEvent перехватывает клавиши до того, как
    // они дойдут до Button.

    m_tableAnchor = Button(
        " ", // пустая метка — визуально не мешает
        []() { /* no-op */ },
        ButtonOption::Ascii()
    );

    m_tableComponent = CatchEvent(
        Renderer(
            m_tableAnchor,
            [this]()
            { return renderTable(); }
        ),
        [this](Event e)
        {
            if (e == Event::ArrowUp)
            {
                if (m_selectedRow > 0)
                    --m_selectedRow;
                return true;
            }
            if (e == Event::ArrowDown)
            {
                if (m_selectedRow + 1 < static_cast<int>(m_users.size()))
                    ++m_selectedRow;
                return true;
            }
            if (e == Event::Return)
            {
                // Заглушка: экран деталей пользователя будет добавлен позже.
                if (!m_users.empty())
                {
                    m_status = "Открытие пользователя id="
                        + std::to_string(
                                   m_users[m_selectedRow].id.value_or(0)
                        )
                        + " — пока не реализовано";
                }
                return true;
            }
            return false;
        }
    );

    // ==== Общая раскладка ====
    auto filtersRow = Container::Horizontal({
        m_loginInput,
        m_nameInput,
        m_emailInput,
        m_blockedDropdown,
        m_applyButton,
        m_clearButton,
    });

    auto paginationRow = Container::Horizontal({
        m_prevButton,
        m_nextButton,
        m_refreshButton,
    });

    auto layout = Container::Vertical({
        filtersRow,
        m_tableComponent,
        paginationRow,
    });

    auto rootRenderer = Renderer(
        layout,
        [this, filtersRow, paginationRow]() -> Element
        {
            Elements children;

            // ---- Заголовок ----
            children.push_back(text("Пользователи") | bold | center);
            children.push_back(separator());

            // ---- Фильтры ----
            children.push_back(
                hbox({
                    text("Логин: ") | dim,
                    m_loginInput->Render() | size(WIDTH, EQUAL, 18),
                    text("  ФИО: ") | dim,
                    m_nameInput->Render() | size(WIDTH, EQUAL, 22),
                    text("  Email: ") | dim,
                    m_emailInput->Render() | size(WIDTH, EQUAL, 24),
                })
            );
            children.push_back(
                hbox({
                    text("Статус: ") | dim,
                    m_blockedDropdown->Render() | size(WIDTH, EQUAL, 26),
                    text("  "),
                    m_applyButton->Render(),
                    text("  "),
                    m_clearButton->Render(),
                })
            );
            children.push_back(separator());

            // ---- Таблица ----
            // Рендерим через компонент, а не через renderTable() напрямую,
            // чтобы рендеринг шёл через дерево компонентов.
            children.push_back(
                m_tableComponent->Render() | frame | flex
            );

            // ---- Пагинация и статус ----
            children.push_back(separator());
            children.push_back(
                hbox({
                    m_prevButton->Render(),
                    text("  "),
                    m_nextButton->Render(),
                    text("  "),
                    m_refreshButton->Render(),
                    filler(),
                    text("Стр. " + std::to_string(m_currentPage) + " / " + std::to_string(models::Page<models::User> { {}, m_totalCount, m_currentPage, m_pageSize }.totalPages()) + " | Всего: " + std::to_string(m_totalCount)) | dim,
                })
            );

            // ---- Сообщение о состоянии ----
            if (m_loading)
            {
                children.push_back(
                    text("Загрузка...") | center | color(Color::Yellow)
                );
            }
            else if (!m_status.empty())
            {
                children.push_back(
                    text(m_status) | center | color(Color::Red)
                );
            }

            children.push_back(
                text(
                    "Tab — переход по элементам, ↑↓ — выбор, "
                    "Enter — открыть, Esc — назад"
                )
                | dim | center
            );

            return vbox(std::move(children)) | border;
        }
    );

    m_root = CatchEvent(
        rootRenderer,
        [this](Event e)
        {
            if (e == Event::Escape)
            {
                m_nav->pop();
                return true;
            }
            return false;
        }
    );
}

Component UserListScreen::component()
{
    return m_root;
}

void UserListScreen::onResume()
{
    if (m_users.empty() && !m_loading)
    {
        reload();
    }
}

void UserListScreen::reload()
{
    if (m_loading)
        return;

    m_loading = true;
    m_status.clear();

    models::UserQuery query;
    query.page = m_currentPage;
    query.pageSize = m_pageSize;
    query.login = m_filterLogin;
    query.name = m_filterName;
    query.email = m_filterEmail;

    if (m_filterBlockedIndex == 1)
        query.isBlocked = true;
    else if (m_filterBlockedIndex == 2)
        query.isBlocked = false;

    auto* screen = &m_screen;

    std::thread(
        [this, screen, query]()
        {
            auto result = m_userService->getUsers(query).get();

            screen->Post(
                [this, result]()
                {
                    m_loading = false;

                    if (result.success)
                    {
                        m_users = std::move(result.page.items);
                        m_totalCount = result.page.totalCount;
                        m_selectedRow = 0;

                        if (m_users.empty() && m_currentPage > 1)
                        {
                            m_currentPage = 1;
                            reload();
                        }
                    }
                    else
                    {
                        m_status = "Ошибка: " + result.errorMessage;
                    }
                }
            );
        }
    ).detach();
}

void UserListScreen::applyFilters()
{
    m_currentPage = 1;
    reload();
}

void UserListScreen::clearFilters()
{
    m_filterLogin.clear();
    m_filterName.clear();
    m_filterEmail.clear();
    m_filterBlockedIndex = 0;
    m_currentPage = 1;
    reload();
}

void UserListScreen::goToPrevPage()
{
    if (m_currentPage > 1 && !m_loading)
    {
        --m_currentPage;
        reload();
    }
}

void UserListScreen::goToNextPage()
{
    const int totalPages = static_cast<int>(
        (m_totalCount + m_pageSize - 1) / m_pageSize
    );
    if (m_currentPage < totalPages && !m_loading)
    {
        ++m_currentPage;
        reload();
    }
}

Element UserListScreen::renderTable() const
{
    constexpr int wId = 6;
    constexpr int wLogin = 18;
    constexpr int wFirst = 16;
    constexpr int wLast = 18;
    constexpr int wEmail = 26;
    constexpr int wBlock = 10;
    constexpr int wAdmin = 8;

    Elements rows;

    // ---- Заголовок ----
    rows.push_back(
        hbox({
            text("ID") | bold | size(WIDTH, EQUAL, wId) | center,
            text("Логин") | bold | size(WIDTH, EQUAL, wLogin) | center,
            text("Имя") | bold | size(WIDTH, EQUAL, wFirst) | center,
            text("Фамилия") | bold | size(WIDTH, EQUAL, wLast) | center,
            text("Email") | bold | size(WIDTH, EQUAL, wEmail) | center,
            text("Блок.") | bold | size(WIDTH, EQUAL, wBlock) | center,
            text("Админ") | bold | size(WIDTH, EQUAL, wAdmin) | center,
        })
    );
    rows.push_back(separator());

    // ---- Данные ----
    if (m_users.empty())
    {
        rows.push_back(text("Нет данных") | dim | center);
    }
    else
    {
        for (size_t i = 0; i < m_users.size(); ++i)
        {
            const auto& u = m_users[i];

            auto idCell = text(
                              u.id.has_value() ? std::to_string(*u.id) : "—"
                          )
                | size(WIDTH, EQUAL, wId) | center;

            auto loginCell = text(
                                 truncate(strOrDash(u.login), wLogin - 1)
                             )
                | size(WIDTH, EQUAL, wLogin);

            auto firstCell = text(
                                 truncate(strOrDash(u.firstName), wFirst - 1)
                             )
                | size(WIDTH, EQUAL, wFirst);

            auto lastCell = text(
                                truncate(strOrDash(u.lastName), wLast - 1)
                            )
                | size(WIDTH, EQUAL, wLast);

            auto emailCell = text(
                                 truncate(strOrDash(u.email), wEmail - 1)
                             )
                | size(WIDTH, EQUAL, wEmail);

            auto blockCell = text(
                                 boolOrDash(u.isBlocked)
                             )
                | size(WIDTH, EQUAL, wBlock) | center;

            auto adminCell = text(
                                 boolOrDash(u.isSuperAdmin)
                             )
                | size(WIDTH, EQUAL, wAdmin) | center;

            auto row = hbox({
                idCell,
                loginCell,
                firstCell,
                lastCell,
                emailCell,
                blockCell,
                adminCell,
            });

            if (static_cast<int>(i) == m_selectedRow)
                row = row | inverted;

            rows.push_back(row);
        }
    }

    return vbox(std::move(rows));
}

} // namespace terminal::screens
