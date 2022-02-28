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
#include "ByteBuffer.h"
#include "Errors.h"
#include <hffix.hpp>
#include <map>

// We want Boost Date_Time support, so include these before hffix.hpp.
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>

void ReadMessage(ByteBuffer& packet)
{
    std::map<int, std::string> field_dictionary;
    hffix::dictionary_init_field(field_dictionary);

    const char* message = (const char*)packet.contents();
    size_t length = packet.size();

    hffix::message_reader reader(message, length);

    if (reader.is_valid())
    {
        // Here is a complete message. Read fields out of the reader.
        try
        {
            if (reader.message_type()->value() == "A")
            {
                std::cout << "Logon message\n";

                hffix::message_reader::const_iterator i = reader.begin();

                if (reader.find_with_hint(hffix::tag::SenderCompID, i))
                    std::cout
                    << "SenderCompID = "
                    << i++->value() << '\n';

                if (reader.find_with_hint(hffix::tag::MsgSeqNum, i))
                    std::cout
                    << "MsgSeqNum    = "
                    << i++->value().as_int<int>() << '\n';

                if (reader.find_with_hint(hffix::tag::SendingTime, i))
                    std::cout
                    << "SendingTime  = "
                    << i++->value().as_timestamp() << '\n';

                std::cout
                    << "The next field is "
                    << hffix::field_name(i->tag(), field_dictionary)
                    << " = " << i->value() << '\n';

                std::cout << '\n';
            }
            else if (reader.message_type()->value() == "D")
            {
                std::cout << "New Order Single message\n";

                hffix::message_reader::const_iterator i = reader.begin();

                if (reader.find_with_hint(hffix::tag::Side, i))
                    std::cout <<
                    (i++->value().as_char() == '1' ? "Buy " : "Sell ");

                if (reader.find_with_hint(hffix::tag::Symbol, i))
                    std::cout << i++->value() << " ";

                if (reader.find_with_hint(hffix::tag::OrderQty, i))
                    std::cout << i++->value().as_int<int>();

                if (reader.find_with_hint(hffix::tag::Price, i)) {
                    int mantissa, exponent;
                    i->value().as_decimal(mantissa, exponent);
                    std::cout << " @ $" << mantissa << "E" << exponent;
                    ++i;
                }

                std::cout << "\n\n";
            }

        }
        catch (std::exception const& ex)
        {
            LOG_ERROR("> {}: Error reading fields: '{}'", __FUNCTION__, ex.what());
        }
    }
    else
        ABORT("Invalid FIX message: '{}'", packet.ReadCString());
}

void AuthSession::Start()
{
    std::string ip_address = GetRemoteIpAddress().to_string();
    LOG_TRACE("Accepted connection from {}:{}", GetRemoteIpAddress().to_string(), GetRemotePort());

    AsyncRead();
}

void AuthSession::OnClose()
{
    LOG_TRACE("End connection from {}:{}", GetRemoteIpAddress().to_string(), GetRemotePort());
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
    ByteBuffer buffer(std::move(MessageBuffer(packet)));

    ReadMessage(buffer);

    packet.Reset();

    AsyncRead();
}

void AuthSession::SendPacket(ByteBuffer& packet)
{
    if (!IsOpen())
    {
        LOG_ERROR("> Can't send packet. Socket is close");
        return;
    }

    if (packet.empty())
    {
        LOG_ERROR("> Can't send packet. Packet is empty");
        return;
    }

    MessageBuffer buffer(packet.size());
    buffer.Write(packet.contents(), packet.size());
    QueuePacket(std::move(buffer));
}
