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

#include "AuthSession.h"
#include "FixMessage.h"
#include "Timer.h"
#include "ByteBuffer.h"
#include "Errors.h"
#include "StopWatch.h"
#include <map>

constexpr auto FIX_PROTOCOL_SUPPORT = "FIX.5.0";

struct AuthHandler
{
    AuthStatus status;
    bool (AuthSession::* handler)();
};

std::unordered_map<std::string, AuthHandler> AuthSession::InitHandlers()
{
    std::unordered_map<std::string, AuthHandler> handlers;

    handlers["A"] = { AuthStatus::NotAuthed,    &AuthSession::HandleLogonMessage };
    handlers["D"] = { AuthStatus::Authed,       &AuthSession::HandleNewOrderSingleMessage };

    return handlers;
}

std::unordered_map<std::string, AuthHandler> const Handlers = AuthSession::InitHandlers();

void AuthSession::Start()
{
    LOG_TRACE("auth", "Accepted connection from {}:{}", GetRemoteIpAddress().to_string(), GetRemotePort());
    AsyncRead();
}

void AuthSession::OnClose()
{
    LOG_TRACE("auth", "End connection from {}:{}", GetRemoteIpAddress().to_string(), GetRemotePort());
}

bool AuthSession::Update()
{
    if (!AuthSocket::Update())
        return false;

    return true;
}

void AuthSession::ReadHandler()
{
    MessageBuffer& packet = GetReadBuffer();

    // Check protocol
    {
        MessageBuffer header;
        header.Resize(7);
        header.Write(packet.GetReadPointer() + 2, 7);

        ByteBuffer buffer(std::move(header));

        std::string fixProtocol;
        buffer >> fixProtocol;

        if (fixProtocol != FIX_PROTOCOL_SUPPORT)
        {
            LOG_ERROR("auth", "> Client {}:{} using unsupport protocol {}", GetRemoteIpAddress().to_string(), GetRemotePort(), fixProtocol);
            CloseSocket();
            return;
        }
    }

    ByteBuffer buffer(std::move(MessageBuffer(packet)));

    std::string cmd = sFixMessage->GetCommand(buffer);
    auto const& itr = Handlers.find(cmd);
    if (itr == Handlers.end())
    {
        LOG_ERROR("auth", "> Client {}:{} using unknown command '{}'", GetRemoteIpAddress().to_string(), GetRemotePort(), cmd);
        //CloseSocket();
        return;
    }

    if (_status != itr->second.status)
    {
        CloseSocket();
        return;
    }

    if (!sFixMessage->IsValidCommand(buffer, cmd))
    {
        LOG_ERROR("auth", "> Client {}:{} using invalid command '{}'", GetRemoteIpAddress().to_string(), GetRemotePort(), cmd);
        //CloseSocket();
        return;
    }

    if (!(*this.*itr->second.handler)())
    {
        CloseSocket();
        return;
    }

    packet.Reset();

    AsyncRead();
}

void AuthSession::SendPacket(ByteBuffer& packet)
{
    if (!IsOpen())
    {
        LOG_ERROR("auth", "> Can't send packet. Socket is close");
        return;
    }

    if (packet.empty())
    {
        LOG_ERROR("auth", "> Can't send packet. Packet is empty");
        return;
    }

    MessageBuffer buffer(packet.size());
    buffer.Write(packet.contents(), packet.size());
    QueuePacket(std::move(buffer));
}

bool AuthSession::HandleLogonMessage()
{
    ByteBuffer buffer(std::move(MessageBuffer(GetReadBuffer())));

    if (sFixMessage->IsReadLogonMessage(buffer))
    {
        _status = AuthStatus::Authed;

        sFixMessage->PrepareTestMessage(buffer);
        SendPacket(buffer);

        return true;
    }

    return false;
}

bool AuthSession::HandleNewOrderSingleMessage()
{
    ByteBuffer buffer(std::move(MessageBuffer(GetReadBuffer())));
    return sFixMessage->IsReadNewOrderSingleMessage(buffer);
}
