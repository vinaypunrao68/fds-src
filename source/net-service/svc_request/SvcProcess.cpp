/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
#include <fds_process.h>
#include <fdsp/PlatNetSvc.h>
#include <net/SvcMgr.h>
#include <net/SvcProcess.h>

namespace fds {
SvcProcess::SvcProcess(int argc, char *argv[],
                               const std::string &def_cfg_file,
                               const std::string &base_path,
                               const std::string &def_log_file,
                               fds::Module **mod_vec,
                               fpi::PlatNetSvcProcessorPtr processor)
{
    init(argc, argv, def_cfg_file, base_path, def_log_file, mod_vec, processor);
}

SvcProcess::~SvcProcess()
{
}

void SvcProcess::init(int argc, char *argv[],
                          const std::string &def_cfg_file,
                          const std::string &base_path,
                          const std::string &def_log_file,
                          fds::Module **mod_vec,
                          fpi::PlatNetSvcProcessorPtr processor)
{
    /* Set up process related services such as logger, timer, etc. */
    FdsProcess::init(argc, argv, def_cfg_file, base_path, def_log_file, mod_vec);

    /* Setup Config db for persistence */
    setupConfigDb_();

    /* Set up service layer */
    setupSvcMgr_(processor);

    /* Default implementation registers with OM.  Until registration complets
     * this will not return
     */
    registerSvcProcess();

    /* After registraion is complete other modules can proceed */
}

void SvcProcess::setupConfigDb_()
{
    LOGNOTIFY;
    // TODO(Rao): Set up configdb
}

void SvcProcess::setupSvcMgr_(fpi::PlatNetSvcProcessorPtr processor)
{
    LOGNOTIFY;
    svcMgr_.reset(new SvcMgr(processor));
    /* This will start SvcServer instance */
    svcMgr_->mod_init(nullptr);
}

void SvcProcess::registerSvcProcess()
{
    LOGNOTIFY;
}

}  // namespace fds
