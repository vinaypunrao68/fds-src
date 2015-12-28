/*
 * Copyright 2015 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_PLATFORM_INCLUDE_FLAP_DECTECTOR_H_
#define SOURCE_PLATFORM_INCLUDE_FLAP_DECTECTOR_H_

// System Includes
extern "C"
{
   #include <time.h>
   #include <stdint.h>
}

// STD C++ Library includes
#include <map>
#include <mutex>

namespace fds
{
    namespace pm
    {
        class FlapDetector
        {
            public:
                class ServiceRecord
                {
                    public:
                        time_t    m_timeStamp;
                        uint32_t  m_restartCount;
                };

                FlapDetector(uint32_t count, uint32_t timeout);
                ~FlapDetector();

                void removeService (int);
                bool isServiceFlapping (int);

            protected:

            private:
                std::mutex                    m_appMapMutex;
                std::map <int, ServiceRecord> m_appMap;

                uint32_t                      m_timeWindow;
                uint32_t                      m_flapLimit;
                
                void addService (int);
        };
    }  // namespace pm
}  // namespace fds

#endif  // SOURCE_PLATFORM_INCLUDE_FLAP_DECTECTOR_H_
