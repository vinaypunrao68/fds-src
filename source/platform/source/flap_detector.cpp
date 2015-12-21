/*
 * Copyright 2015 by Formation Data Systems, Inc.
 */

// FDS Includes
#include "util/Log.h"

// Platform Includes
#include "platform/flap_detector.h"

namespace fds
{
    namespace pm
    {
        FlapDetector::FlapDetector (uint32_t count, uint32_t timeout) : m_appMap(), m_timeWindow (timeout), m_flapLimit (count)
        {
        }

        FlapDetector::~FlapDetector()
        {
        }

        void FlapDetector::addService (int serviceIndex)
        {
            LOGDEBUG << "No flap detector record found (service index = " << serviceIndex << "), creating a record.";

            ServiceRecord record;

            record.m_timeStamp = time (nullptr);
            record.m_restartCount = 0;

            m_appMap[serviceIndex] = record;
        }

        void FlapDetector::removeService (int serviceIndex)
        {
            std::lock_guard <decltype (m_appMapMutex)> lock (m_appMapMutex);

            std::map <int, ServiceRecord>::iterator mapIter = m_appMap.find (serviceIndex);

            if (m_appMap.end() != mapIter)
            {
                LOGERROR << "Preparing to remove a service (service index = " << serviceIndex << ") from the flap detector.";

                m_appMap.erase (mapIter);
            }
            else
            {
                // This shouldn't happen, but lets log an error
                LOGDEBUG << "No flap detector record found (service index = " << serviceIndex << "), removing a record.";
            }
        }

        bool FlapDetector::isServiceFlapping (int serviceIndex)
        {
            // If either setting is 0, flap detection is disabled.
            if (0 == m_flapLimit || 0 == m_timeWindow)
            {
                return false;
            }

            std::lock_guard <decltype (m_appMapMutex)> lock (m_appMapMutex);

            std::map <int, ServiceRecord>::iterator mapIter = m_appMap.find (serviceIndex);

            if (m_appMap.end() != mapIter)
            {
                // Service has restarted
                time_t timeNow = time (nullptr);

                if (timeNow - m_timeWindow < mapIter->second.m_timeStamp)
                {
                    // Service last restarted within timeWindow

                    if (++mapIter->second.m_restartCount > m_flapLimit)
                    {
                        LOGERROR << "Service is flapping, will not attempt to restart the service again (service index = " << serviceIndex << ").";
                        m_appMap.erase (mapIter);
                        return true;
                    }
                    LOGDEBUG << "Bumped service index = " << serviceIndex << " flap count to " << mapIter->second.m_restartCount << ".";
                }
                else
                {
                    LOGDEBUG << "Reset timer for service index = " << serviceIndex << " and set flap count to 1.";
                    // New flapping event, reset
                    mapIter->second.m_timeStamp = timeNow;
                    mapIter->second.m_restartCount = 1;
                }
            }
            else
            {
                // No service record exists, so add one
                addService (serviceIndex);
            }

            return false;
        }
    }  // namespace pm
}  // namespace fds
