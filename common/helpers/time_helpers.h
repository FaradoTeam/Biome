// TODO : Удалить этот файл)
#pragma once

#include "common/types.h"

namespace dto
{

inline int64_t timePointToSeconds(const common::DateTime& timePoint)
{
    return common::timePointToSeconds(timePoint);
}

inline common::DateTime secondsToTimePoint(int64_t seconds)
{
    return common::secondsToTimePoint(seconds);
}

} // namespace dto
