#pragma once

#include <cstdint>
#include <vector>

namespace terminal::models
{

/**
 * @brief Универсальная страница результатов (пагинация).
 */
template <typename T>
struct Page final
{
    std::vector<T> items;
    int64_t totalCount { 0 };
    int page { 1 };
    int pageSize { 20 };

    int totalPages() const
    {
        if (pageSize <= 0)
            return 1;
        return static_cast<int>((totalCount + pageSize - 1) / pageSize);
    }
};

} // namespace terminal::models
