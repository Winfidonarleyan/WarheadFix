/*
 * This file is part of the WarheadApp Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "AuthSocketMgr.h"
#include "Config.h"
#include "StopWatch.h"
#include "GitRevision.h"
#include "Log.h"
#include "Util.h"

#ifndef _WARHEAD_FIX_CONFIG
#define _WARHEAD_FIX_CONFIG "WarheadFix.conf"
#endif

int main(int argc, char** argv)
{
    // Command line parsing to get the configuration file name
    std::string configFile = sConfigMgr->GetConfigPath() + std::string(_WARHEAD_FIX_CONFIG);
    int count = 1;

    while (count < argc)
    {
        if (strcmp(argv[count], "-c") == 0)
        {
            if (++count >= argc)
            {
                fmt::print("Runtime-Error: -c option requires an input argument\n");
                return 1;
            }
            else
                configFile = argv[count];
        }

        ++count;
    }

    if (!sConfigMgr->LoadAppConfigs(configFile))
        return 1;

    // Init logging
    sLog->Initialize();

    LOG_INFO("server", "{}", GitRevision::GetFullVersion());
    LOG_INFO("server", "");

    std::shared_ptr<Warhead::Asio::IoContext> ioContext = std::make_shared<Warhead::Asio::IoContext>();

    StopWatch sw;

    // Start the listening port (acceptor) for auth connections
    int32 port = sConfigMgr->GetOption<int32>("ServerPort", 5001);
    if (port < 0 || port > 0xFFFF)
    {
        LOG_ERROR("server", "Specified port out of allowed range (1-65535)");
        return 1;
    }

    std::string bindIp = sConfigMgr->GetOption<std::string>("BindIP", "0.0.0.0");

    if (!sAuthSocketMgr.StartNetwork(*ioContext, bindIp, port))
    {
        LOG_ERROR("server", "Failed to initialize network");
        return 1;
    }

    LOG_DEBUG("server", "Start network in {}", sw);

    std::shared_ptr<void> sAuthSocketMgrHandle(nullptr, [](void*) { sAuthSocketMgr.StopNetwork(); });

    // Start the io service worker loop
    ioContext->run();

    return 0;
}

