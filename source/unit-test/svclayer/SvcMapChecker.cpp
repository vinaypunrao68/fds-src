/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
#include <net/SvcProcess.h>
#include <net/SvcMgr.h>
#include <fdsp_utils.h>
#include <fdsp/OMSvc.h>
#include <fdsp/PlatNetSvc.h>

namespace fds{
class SvcMapChecker : public SvcProcess
{
 public:
     SvcMapChecker(int argc, char *argv[])
         : SvcProcess(argc, argv, "platform.conf", "fds.test.", "svcmapcheckerlog", nullptr)
    {
    }

    bool getSvcMap(const std::string &ip, int port, std::vector<fpi::SvcInfo> &svcMap) {
        static auto svcInfoComparator = [](const fpi::SvcInfo &lhs, const fpi::SvcInfo &rhs) {
            return lhs.svc_id.svc_uuid.svc_uuid < rhs.svc_id.svc_uuid.svc_uuid;
        };

        try {
            int64_t nullarg = 0;
            auto client = allocRpcClient<fpi::PlatNetSvcClient>(
                                                ip,
                                                port,
                                                1);
            client->getSvcMap(svcMap, nullarg);
            std::sort(svcMap.begin(), svcMap.end(), svcInfoComparator);
        } catch (...) {
            return false;
        }
        return true;
    }

    virtual void registerSvcProcess() override
    {
        // don't do anything..we don't need to register with om
    }

    bool compareSvcMaps(const std::vector<fpi::SvcInfo> &omSvcMap,
                        const std::vector<fpi::SvcInfo> &svcMap)
    {
        bool ret = true;
        std::vector<fpi::SvcInfo>::const_iterator itr1 = omSvcMap.cbegin();
        std::vector<fpi::SvcInfo>::const_iterator itr2 = svcMap.cbegin();

        while (itr1 != omSvcMap.cend() && itr2 != svcMap.cend()) {
            if (itr1->svc_id.svc_uuid.svc_uuid < itr2->svc_id.svc_uuid.svc_uuid) {
                LOGERROR << "Entry only exists in om svc map: " << fds::logString(*itr1);
                itr1++;
                ret = false;
            } else if (itr1->svc_id.svc_uuid.svc_uuid > itr2->svc_id.svc_uuid.svc_uuid) {
                LOGERROR << "Entry only exists in svc svc map: " << fds::logString(*itr2);
                itr2++;
                ret = false;
            } else {
                itr1++;
                itr2++;
            }
        }
        while (itr1 != omSvcMap.cend()) {
            LOGERROR << "Entry only exists in om svc map: " << fds::logString(*itr1);
            itr1++;
            ret = false;
        }
        while (itr2 != svcMap.cend()) {
            LOGERROR << "Entry only exists in svc svc map: " << fds::logString(*itr2);
            itr2++;
            ret = false;
        }
        return ret;
    }

    virtual int run() {
        /* OM svc map */
        std::string omIp;
        uint32_t port;
        std::vector<fpi::SvcInfo> omSvcMap;
        bool ret;
        int retCode = 0;

        svcMgr_->getOmIPPort(omIp, port);
        ret = getSvcMap(omIp, port, omSvcMap);
        if (!ret) {
            LOGERROR << "Coudn't get svc map from om";
            return -1;
        }

        for (auto &svcInfo : omSvcMap) {
            std::vector<fpi::SvcInfo> svcMap;

            LOGNORMAL << "Comparing " << fds::logString(svcInfo);

            if (svcInfo.svc_status == fpi::SVC_STATUS_INACTIVE) {
                /* Getting svcmap should fail */
                ret = getSvcMap(svcInfo.ip, svcInfo.svc_port, svcMap);
                if (ret == true) {
                    LOGERROR << "Service is up.  OM thinks it's down.  OM view: "
                        << fds::logString(svcInfo);
                    retCode = -1;
                }
            } else if (svcInfo.svc_status == fpi::SVC_STATUS_ACTIVE) {
                ret = getSvcMap(svcInfo.ip, svcInfo.svc_port, svcMap);
                if (ret) {
                    ret = compareSvcMaps(omSvcMap, svcMap);
                    if (!ret) {
                        retCode = -1;
                    }
                } else {
                    LOGERROR << "Service is down.  OM thinks it's up.  OM view: "
                        << fds::logString(svcInfo);
                    retCode = -1;
                }
            } else {
                LOGERROR << "Invalid service state.  OM view: " << fds::logString(svcInfo);
                retCode = -1;
            }
        }
        return retCode;
    }
};
}  // namespace fds

int main(int argc, char *argv[])
{
    
    auto checker = new fds::SvcMapChecker(argc, argv);
    auto ret = checker->main();
    delete checker;
    return ret;
}
