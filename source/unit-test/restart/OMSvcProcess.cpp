/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
#include <fds_process.h>
#include <fdsp/fds_service_types.h>
#include <net/SvcProcess.h>
#include <fdsp/PlatNetSvc.h>
#include <fdsp/OMSvc.h>
#include <net/SvcMgr.h>
#include <net/SvcProcess.h>
#include <OMSvcProcess.h>
// #include <OMSvcHandler2.h>

namespace fds {
struct OMSvcHandler2 : virtual public fpi::OMSvcIf, public PlatNetSvcHandler {
  OMSvcHandler2() {
    // Your initialization goes here
  }

  void registerService(const fpi::SvcInfo& svcInfo) {
    // Don't do anything here. This stub is just to keep cpp compiler happy
  }


  void registerService(boost::shared_ptr<fpi::SvcInfo>& svcInfo) {
    // Your implementation goes here
    printf("registerService\n");
  }

  void getSvcMap(std::vector<fpi::SvcInfo> & _return, const int32_t nullarg) {
    // Don't do anything here. This stub is just to keep cpp compiler happy
  }


  void getSvcMap(std::vector<fpi::SvcInfo> & _return, boost::shared_ptr<int32_t>& nullarg) {
    // Your implementation goes here
    printf("getSvcMap\n");
  }

};

OMSvcProcess::OMSvcProcess(int argc, char *argv[])
                               
{
    init(argc, argv);
}

OMSvcProcess::~OMSvcProcess()
{
}

void OMSvcProcess::init(int argc, char *argv[])
{
    /* Set up OMsvcProcessor for handling messages */
    boost::shared_ptr<OMSvcHandler2> handler = boost::make_shared<OMSvcHandler2>();
    boost::shared_ptr<fpi::OMSvcProcessor> processor = boost::make_shared<fpi::OMSvcProcessor>(handler);

    /* Set up process related services such as logger, timer, etc. */
    SvcProcess::init(argc, argv, "platform.conf", "fds.om.", "om.log", nullptr, processor);
}

void OMSvcProcess::registerSvcProcess()
{
    LOGNOTIFY;
    /* For now nothing to do */
}

void OMSvcProcess::setupConfigDb_()
{
    LOGNOTIFY;
    // TODO(Rao): Set up configdb
    // fds_panic("Unimpl");
}

int OMSvcProcess::run() {
    LOGNOTIFY << "Doing work";
    while (true) {
        sleep(1000);
    }
    return 0;
}

void OMSvcProcess::registerService(boost::shared_ptr<fpi::SvcInfo>& svcInfo)
{
    GLOGNOTIFY << "Incoming update: " << fds::logString(svcInfo);

    fds_scoped_lock l(svcMapLock_);
    auto mapItr = svcMap_.find(svcInfo->svc_id.svc_uuid);
    if (mapItr == svcMap_.end()) {
        GLOGNOTIFY << "Added new service";
        svcMap_.emplace(std::make_pair(svcInfo->svc_id.svc_uuid, *svcInfo));
    } else if (mapItr->second.incarnationNo < svcInfo->incarnationNo) {
        GLOGNOTIFY << "Updated service";
        mapItr->second = *svcInfo;
    } else {
        GLOGNOTIFY << "Ignored service";
    }
}

void OMSvcProcess::getSvcMap(std::vector<fpi::SvcInfo> & svcMap)
{
    fds_scoped_lock l(svcMapLock_);
    for (auto &kv : svcMap) {
        svcMap.push_back(kv.second);
    }
}

}  // namespace fds

int main(int argc, char* argv[])
{
    std::unique_ptr<fds::OMSvcProcess> process(new fds::OMSvcProcess(argc, argv));
    process->main();
    return 0;
}
