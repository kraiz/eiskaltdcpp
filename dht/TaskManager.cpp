/*
* Copyright (C) 2009 Big Muscle, http://strongdc.sf.net
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include "stdafx.h"

#include "BootstrapManager.h"
#include "DHT.h"
#include "IndexManager.h"
#include "SearchManager.h"
#include "TaskManager.h"

#include "../dcpp/SettingsManager.h"
#include "../dcpp/ShareManager.h"
#include "../dcpp/TimerManager.h"

namespace dht
{

    TaskManager::TaskManager(void) :
        nextPublishTime(GET_TICK()), nextRepublishTime(GET_TICK()), nextSearchTime(GET_TICK()), nextSelfLookup(GET_TICK() + 3*60*1000),
        nextFirewallCheck(GET_TICK() + FWCHECK_TIME), lastBootstrap(0)
    {
        TimerManager::getInstance()->addListener(this);
    }

    TaskManager::~TaskManager(void)
    {
        TimerManager::getInstance()->removeListener(this);
    }

    // TimerManagerListener
    void TaskManager::on(TimerManagerListener::Second, uint64_t aTick) throw()
    {
        if(DHT::getInstance()->isConnected())
        {
            if(!DHT::getInstance()->isFirewalled() && IndexManager::getInstance()->getPublish() && aTick >= nextPublishTime)
            {
                // publish next file
                IndexManager::getInstance()->publishNextFile();
                nextPublishTime = aTick + PUBLISH_TIME;
            }
        }
        else
        {
            if(aTick - lastBootstrap > 15000 || (DHT::getInstance()->getNodesCount() == 0 && aTick - lastBootstrap >= 2000))
            {
                // bootstrap if we doesn't know any remote node
                BootstrapManager::getInstance()->process();
                lastBootstrap = aTick;
            }
        }

        if(aTick >= nextSearchTime)
        {
            SearchManager::getInstance()->processSearches();
            nextSearchTime = aTick + SEARCH_PROCESSTIME;
        }

        if(aTick >= nextSelfLookup)
        {
            // find myself in the network
            SearchManager::getInstance()->findNode(dcpp::ClientManager::getInstance()->getMe()->getCID());
            nextSelfLookup = aTick + SELF_LOOKUP_TIMER;
        }

        if(aTick >= nextFirewallCheck)
        {
            DHT::getInstance()->setRequestFWCheck();
            nextFirewallCheck = aTick + FWCHECK_TIME;
        }
    }

    void TaskManager::on(TimerManagerListener::Minute, uint64_t aTick) throw()
    {
        if(!DHT::getInstance()->isFirewalled() && DHT::getInstance()->isConnected() && IndexManager::getInstance()->getPublish() && aTick >= nextRepublishTime)
        {
            // republish all files
            ShareManager::getInstance()->publish();
            nextRepublishTime = aTick + REPUBLISH_TIME;
        }

        // remove dead nodes
        DHT::getInstance()->checkExpiration(aTick);
        IndexManager::getInstance()->checkExpiration(aTick);

        DHT::getInstance()->saveData();
    }

}
