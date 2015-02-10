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
#include <om-svc-handler.h>
#include <OMSvcProcess.h>

namespace fds {

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
    boost::shared_ptr<OmSvcHandler> handler = boost::make_shared<OmSvcHandler>();
    boost::shared_ptr<OMSvcProcessor> processor = boost::make_shared<OMSvcProcessor>(handler);

    /* Set up process related services such as logger, timer, etc. */
    SvcProcess::init(argc, argv, "platform.conf", "fds.om", "om.log", nullptr, processor);
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
    fds_panic("Unimpl");
}

void OMSvcProcess::setupSvcInfo_()
{
    /* For now nothing to do */
}

int OMSvcProcess::run() {
    while (true) {
        sleep(1000);
    }
    return 0;
}

}  // namespace fds

int main(int argc, char* argv[])
{
    std::unique_ptr<fds::OMSvcProcess> process(new fds::OMSvcProcess(argc, argv));
    process->main();
    return 0;
}
