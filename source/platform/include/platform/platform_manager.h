/*
 * Copyright 2015 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_PLATFORM_INCLUDE_PLATFORM_MANAGER_H_
#define SOURCE_PLATFORM_INCLUDE_PLATFORM_MANAGER_H_

#include <fds_config.hpp>
#include <fds_module.h>
#include <kvstore/platformdb.h>
#include <fdsp/pm_data_types.h>
#include <fds_typedefs.h>
#include <disk_plat_module.h>
#include <util/properties.h>

namespace fds
{
    namespace pm
    {
        enum
        {
            JAVA_AM,
            BARE_AM,
            DATA_MANAGER,
            STORAGE_MANAGER
        };

        const std::string JAVA_AM_CLASS_NAME = "com.formationds.am.Main";
        const std::string BARE_AM_NAME       = "bare_am";
        const std::string DM_NAME            = "DataMgr";
        const std::string SM_NAME            = "StorMgr";

        const std::string JAVA_CLASSPATH_OPTIONS       = "/fds/lib/java/fds-1.0-bin/fds-1.0/*:/fds/lib/java/classes";                 // No spaces in this value

        class PlatformManager : public Module
        {
            public:
                PlatformManager();

                /* Overrides from Module */
                virtual int  mod_init(SysParams const *const param) override;
                virtual void mod_startup()
                {
                }

                virtual void mod_shutdown() override;

                int  run();

                pid_t startAM();
                pid_t startSM();
                pid_t startDM();

                void determineDiskCapability();
                void activateServices(const fpi::ActivateServicesMsgPtr &activateMsg);
                const fpi::NodeInfo& getNodeInfo()
                {
                    return nodeInfo;
                }

                util::Properties& getProperties()
                {
                    return props;
                }

                void loadProperties();

                void startProcess (int id);
                void stopProcess (int id);

            protected:
                bool sendNodeCapabilityToOM();

                fds_int64_t getNodeUUID(fpi::FDSP_MgrIdType svcType);

            private:
                FdsConfigAccessor                  *conf;
                fpi::FDSP_AnnounceDiskCapability    diskCapability;

                kvstore::PlatformDB                *db;
                fpi::NodeInfo                       nodeInfo;
                std::string                         rootDir;
                util::Properties                    props;

                std::map<int, std::string>          m_idToAppNameMap;
        };
    }  // namespace pm
}  // namespace fds

#endif  // SOURCE_PLATFORM_INCLUDE_PLATFORM_MANAGER_H_
