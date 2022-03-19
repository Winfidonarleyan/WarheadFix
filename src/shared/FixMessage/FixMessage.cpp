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

#include "FixMessage.h"
#include "Timer.h"
#include "Errors.h"
#include "Log.h"
#include "StopWatch.h"
#include <hffix.hpp>
#include <map>

// We want Boost Date_Time support, so include these before hffix.hpp.
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>

FixMessage* FixMessage::instance()
{
    static FixMessage instance;
    return &instance;
}

bool FixMessage::IsReadLogonMessage(ByteBuffer& packet)
{
    StopWatch sw;

    std::map<int, std::string> field_dictionary;
    hffix::dictionary_init_field(field_dictionary);

    const char* message = (const char*)packet.contents();
    size_t length = packet.size();

    hffix::message_reader reader(message, length);

    if (!reader.is_valid())
    {
        LOG_ERROR("fix.message", "Invalid FIX message: '{}'", packet.ReadCString());
        return false;
    }

    try
    {
        if (reader.message_type()->value() != "A")
        {
            LOG_ERROR("fix.message", "> Message is not logon type. Message type '{}'", reader.message_type()->value().as_string());
            return false;
        }

        LOG_INFO("fix.message", "Logon message");

        hffix::message_reader::const_iterator i = reader.begin();

        if (reader.find_with_hint(hffix::tag::SenderCompID, i))
            LOG_INFO("fix.message", "SenderCompID = {}", i++->value().as_string());

        if (reader.find_with_hint(hffix::tag::BeginString, i))
            LOG_INFO("fix.message", "BeginString = {}", i++->value().as_string());

        if (reader.find_with_hint(hffix::tag::MsgSeqNum, i))
            LOG_INFO("fix.message", "MsgSeqNum = {}", i++->value().as_int<int>());

        if (reader.find_with_hint(hffix::tag::SendingTime, i))
        {
            std::chrono::time_point<std::chrono::system_clock, Milliseconds> now;
            std::string timeStr = "<error>";

            if (i++->value().as_timestamp(now))
            {
                timeStr = Warhead::Time::TimeToHumanReadable(std::chrono::duration_cast<Seconds>(now.time_since_epoch()));
            }

            LOG_INFO("fix.message", "SendingTime = {}", timeStr);
        }
    }
    catch (std::exception const& ex)
    {
        LOG_ERROR("fix.message", "> {}: Error reading fields: '{}'", __FUNCTION__, ex.what());
        return false;
    }

    LOG_DEBUG("fix.message", "> Read message in {}", sw);
    LOG_INFO("fix.message", "");
    return true;
}

bool FixMessage::IsReadNewOrderSingleMessage(ByteBuffer& packet)
{
    StopWatch sw;

    std::map<int, std::string> field_dictionary;
    hffix::dictionary_init_field(field_dictionary);

    const char* message = (const char*)packet.contents();
    size_t length = packet.size();

    hffix::message_reader reader(message, length);

    if (!reader.is_valid())
    {
        LOG_ERROR("fix.message", "Invalid FIX message: '{}'", packet.ReadCString());
        return false;
    }

    try
    {
        if (reader.message_type()->value() != "D")
        {
            LOG_ERROR("fix.message", "> Message is not New Order Single type. Message type '{}'", reader.message_type()->value().as_string());
            return false;
        }

        LOG_INFO("fix.message", "New Order Single message");

        hffix::message_reader::const_iterator i = reader.begin();

        if (reader.find_with_hint(hffix::tag::Side, i))
            LOG_INFO("fix.message", "SenderCompID = {}", i++->value().as_char() == '1' ? "Buy" : "Sell");

        if (reader.find_with_hint(hffix::tag::Symbol, i))
            LOG_INFO("fix.message", "{}", i++->value().as_string());

        if (reader.find_with_hint(hffix::tag::OrderQty, i))
            LOG_INFO("fix.message", "{}", i++->value().as_int<int>());

        if (reader.find_with_hint(hffix::tag::Price, i))
        {
            int mantissa, exponent;
            i->value().as_decimal(mantissa, exponent);
            LOG_INFO("fix.message", "@ ${} E{}", mantissa, exponent);
        }
    }
    catch (std::exception const& ex)
    {
        LOG_ERROR("fix.message", "> {}: Error reading fields: '{}'", __FUNCTION__, ex.what());
        return false;
    }

    LOG_DEBUG("fix.message", "> Read message in {}", sw);
    LOG_INFO("fix.message", "");
    return true;
}

void FixMessage::PrepareTestMessage(ByteBuffer& packet)
{
    using namespace boost::posix_time;
    using namespace boost::gregorian;

    int seq_send(1); // Sending sequence number.

    char buffer[1 << 12];

    std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();

    // We'll put a FIX Logon message in the buffer.
    hffix::message_writer logon(buffer, buffer + sizeof(buffer));

    logon.push_back_header("FIX.5.0"); // Write BeginString and BodyLength.

    // Logon MsgType.
    logon.push_back_string(hffix::tag::MsgType, "A");
    logon.push_back_string(hffix::tag::SenderCompID, "AAAA");
    logon.push_back_string(hffix::tag::TargetCompID, "BBBB");
    logon.push_back_int(hffix::tag::MsgSeqNum, seq_send++);
    logon.push_back_timestamp(hffix::tag::SendingTime, now);

    // No encryption.
    logon.push_back_int(hffix::tag::EncryptMethod, 0);
    // 10 second heartbeat interval.
    logon.push_back_int(hffix::tag::HeartBtInt, 10);

    logon.push_back_trailer(); // write CheckSum.

    // Now the Logon message is written to the buffer.

    // Add a FIX New Order - Single message to the buffer, after the Logon
    // message.
    hffix::message_writer new_order(logon.message_end(), buffer + sizeof(buffer));

    new_order.push_back_header("FIX.5.0");

    // New Order - Single
    new_order.push_back_string(hffix::tag::MsgType, "D");
    // Required Standard Header field.
    new_order.push_back_string(hffix::tag::SenderCompID, "AAAA");
    new_order.push_back_string(hffix::tag::TargetCompID, "BBBB");
    new_order.push_back_int(hffix::tag::MsgSeqNum, seq_send++);
    new_order.push_back_timestamp(hffix::tag::SendingTime, now);
    new_order.push_back_string(hffix::tag::ClOrdID, "A1");
    // Automated execution.
    new_order.push_back_char(hffix::tag::HandlInst, '1');
    // Ticker symbol OIH.
    new_order.push_back_string(hffix::tag::Symbol, "OIH");
    // Buy side.
    new_order.push_back_char(hffix::tag::Side, '1');
    new_order.push_back_timestamp(hffix::tag::TransactTime, now);
    // 100 shares.
    new_order.push_back_int(hffix::tag::OrderQty, 100);
    // Limit order.
    new_order.push_back_char(hffix::tag::OrdType, '2');
    // Limit price $500.01 = 50001*(10^-2). The push_back_decimal() method
    // takes a decimal floating point number of the form mantissa*(10^exponent).
    new_order.push_back_decimal(hffix::tag::Price, 50001, -2);
    // Good Till Cancel.
    new_order.push_back_char(hffix::tag::TimeInForce, '1');

    new_order.push_back_trailer(); // write CheckSum.

    //Now the New Order message is in the buffer after the Logon message.

    packet.resize(1 << 12);
    packet << buffer;
}

bool FixMessage::IsValidCommand(ByteBuffer& packet, std::string_view command)
{
    const char* message = (const char*)packet.contents();
    size_t length = packet.size();

    hffix::message_reader reader(message, length);

    if (reader.is_valid() && std::string(command) == reader.message_type()->value())
        return true;

    return false;
}

std::string FixMessage::GetCommand(ByteBuffer& packet)
{
    const char* message = (const char*)packet.contents();
    size_t length = packet.size();

    hffix::message_reader reader(message, length);

    if (reader.is_valid())
        return reader.message_type()->value().as_string();

    return "";
}
