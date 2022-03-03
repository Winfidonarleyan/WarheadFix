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

#ifndef __FIX_MESSAGE_H__
#define __FIX_MESSAGE_H__

#include "ByteBuffer.h"
#include <memory>

class WH_SHARED_API FixMessage
{
    FixMessage() = default;
    ~FixMessage() = default;
    FixMessage(FixMessage const&) = delete;
    FixMessage(FixMessage&&) = delete;
    FixMessage& operator=(FixMessage const&) = delete;
    FixMessage& operator=(FixMessage&&) = delete;

public:
    static FixMessage* instance();

    void ReadMessage(ByteBuffer& packet);
    void PrepareTestMessage(ByteBuffer& packet);
};

#define sFixMessage FixMessage::instance()

#endif
