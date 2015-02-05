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
OMSvcProcess::OMSvcProcess(int argc, char *argv[],
                               const std::string &def_cfg_file,
                               const std::string &base_path,
                               const std::string &def_log_file,
                               fds::Module **mod_vec)
{
    init(argc, argv, def_cfg_file, base_path, def_log_file, mod_vec);
}

OMSvcProcess::~OMSvcProcess()
{
}

void OMSvcProcess::init(int argc, char *argv[],
                        const std::string &def_cfg_file,
                        const std::string &base_path,
                        const std::string &def_log_file,
                        fds::Module **mod_vec)
{
    /* Set up OMsvcProcessor for handling messages */
    boost::shared_ptr<OmSvcHandler> handler = boost::make_shared<OmSvcHandler>();
    boost::shared_ptr<OMSvcProcessor> processor = boost::make_shared<OMSvcProcessor>(handler);

    /* Set up process related services such as logger, timer, etc. */
    SvcProcess::init(argc, argv, def_cfg_file, base_path, def_log_file, mod_vec, processor);
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

}  // namespace fds

int main()
{
    return 0;
}
