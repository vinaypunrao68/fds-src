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
        bool checkRes = true;
        int retCode = 0;

        svcMgr_->getOmIPPort(omIp, port);
        checkRes = getSvcMap(omIp, port, omSvcMap);
        if (!checkRes) {
            LOGERROR << "Coudn't get svc map from om";
            std::cout << "Failed to get service map from om.  Ensure om endpoint is correct and "
                << " operational.\n";
            return -1;
        }

        for (auto &svcInfo : omSvcMap) {
            std::vector<fpi::SvcInfo> svcMap;
            checkRes = true;

            LOGNORMAL << "Checking aginst " << fds::logString(svcInfo);
            std::cout << "Checking: " << svcInfo.svc_id.svc_uuid.svc_uuid << std::endl;

            if (svcInfo.svc_status == fpi::SVC_STATUS_INACTIVE_FAILED ||
                svcInfo.svc_status == fpi::SVC_STATUS_INACTIVE_STOPPED) {
                /* Getting svcmap should fail */
                if (getSvcMap(svcInfo.ip, svcInfo.svc_port, svcMap) == true) {
                    LOGERROR << "Service is up.  OM thinks it's down.  OM view: "
                        << fds::logString(svcInfo);
                    checkRes = false;
                }
            } else if (svcInfo.svc_status == fpi::SVC_STATUS_ACTIVE) {
                checkRes = getSvcMap(svcInfo.ip, svcInfo.svc_port, svcMap);
                if (checkRes) {
                    checkRes = compareSvcMaps(omSvcMap, svcMap);
                } else {
                    LOGERROR << "Service is down.  OM thinks it's up.  OM view: "
                        << fds::logString(svcInfo);
                }
            } else {
                LOGERROR << "Invalid service state.  OM view: " << fds::logString(svcInfo);
                checkRes = false;
            }

            if (checkRes) {
                LOGNORMAL << "Checking result: success";
                std::cout << "result: success\n";
            } else {
                LOGNORMAL << "Checking result: failed";
                std::cout << "result: failed.  See the log\n";
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
