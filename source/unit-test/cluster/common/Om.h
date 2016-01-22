/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_NET_OMSVCPROCESS_H_
#define SOURCE_INCLUDE_NET_OMSVCPROCESS_H_

#include <boost/shared_ptr.hpp>
#include <fdsp/svc_api_types.h>
#include <net/SvcMgr.h>
#include <net/PlatNetSvcHandler.h>
#include <net/SvcProcess.h>

/* Forward declarations */
namespace FDS_ProtocolInterface {
}  // namespace FDS_ProtocolInterface
namespace fpi = FDS_ProtocolInterface;

namespace fds {

/* Forward declarations */
struct SvcProcess;

/**
* @brief Om
*/
struct Om : SvcProcess {
    Om(int argc, char *argv[], bool initAsModule);
    virtual ~Om();

    void init(int argc, char *argv[], bool initAsModule);

    virtual void registerSvcProcess() override;

    virtual int run() override;

    /* Messages invoked by handler */
    void registerService(boost::shared_ptr<fpi::SvcInfo>& svcInfo);
    void getSvcMap(std::vector<fpi::SvcInfo> & svcMap);
    void getDLT( ::FDS_ProtocolInterface::CtrlNotifyDLTUpdate& _return, const int64_t nullarg);
    void getDMT( ::FDS_ProtocolInterface::CtrlNotifyDMTUpdate& _return, const int64_t nullarg);
    void getAllVolumeDescriptors(fpi::GetAllVolumeDescriptors& _return, const int64_t nullarg);

 protected:
    /**
    * @brief Sets up configdb used for persistence
    */
    virtual void setupConfigDb_() override;

    fds_mutex svcMapLock_;
    std::unordered_map<fpi::SvcUuid, fpi::SvcInfo, SvcUuidHash> svcMap_;
};
}  // namespace fds
#endif  // SOURCE_INCLUDE_NET_OMSVCPROCESS_H_
