/*
 * This file is part of the WarheadCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _LOG_H_
#define _LOG_H_

#include "Define.h"
#include <fmt/color.h>

enum class LogLevel : uint8
{
    LOG_LEVEL_ERROR,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_TRACE,

    LOG_LEVEL_MAX
};

template <typename... Args>
void log(LogLevel logLevel, std::string_view fmt, Args&&... args)
{
    auto color = fmt::color::red;

    switch (logLevel)
    {
    case LogLevel::LOG_LEVEL_ERROR:
        break;
    case LogLevel::LOG_LEVEL_WARNING:
        color = fmt::color::light_yellow;
        break;
    case LogLevel::LOG_LEVEL_INFO:
        color = fmt::color::dark_cyan;
        break;
    case LogLevel::LOG_LEVEL_DEBUG:
        color = fmt::color::cyan;
        break;
    case LogLevel::LOG_LEVEL_TRACE:
        color = fmt::color::green;
        break;
    default:
        break;
    }

    fmt::print(fmt::emphasis::bold | fg(color), fmt::format(fmt, std::forward<Args>(args)...) + "\n");
}

#define LOG_ERROR(...) \
  log(LogLevel::LOG_LEVEL_ERROR, ##__VA_ARGS__)

#define LOG_WARN(...) \
  log(LogLevel::LOG_LEVEL_WARNING, ##__VA_ARGS__)

#define LOG_INFO(...) \
  log(LogLevel::LOG_LEVEL_INFO, ##__VA_ARGS__)

#define LOG_DEBUG(...) \
  log(LogLevel::LOG_LEVEL_DEBUG, ##__VA_ARGS__)

#define LOG_TRACE(...) \
  log(LogLevel::LOG_LEVEL_TRACE, ##__VA_ARGS__)

#endif
