/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
#include <fds_process.h>
#include <fdsp/PlatNetSvc.h>
#include <net/PlatNetSvcHandler.h>
#include <fdsp/OMSvc.h>
#include <net/SvcMgr.h>
#include <net/SvcProcess.h>

namespace fds {

SvcProcess::SvcProcess()
{
}

SvcProcess::SvcProcess(int argc, char *argv[],
                       const std::string &def_cfg_file,
                       const std::string &base_path,
                       const std::string &def_log_file,
                       fds::Module **mod_vec)
    : SvcProcess(argc, argv, def_cfg_file,
                 base_path, def_log_file,
                 mod_vec,
                 boost::make_shared<fpi::PlatNetSvcProcessor>(
                     boost::make_shared<PlatNetSvcHandler>()))
{
}

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

    /* Populate service information */
    setupSvcInfo_();

    /* Set up service layer */
    setupSvcMgr_(processor);

    /* Default implementation registers with OM.  Until registration complets
     * this will not return
     */
    registerSvcProcess();

    /* After registraion is complete other modules can proceed.  See FdsProcess::main() */
}

void SvcProcess::registerSvcProcess()
{
    LOGNOTIFY;

    do {
        try {
            /* This will block until we get a connection */
            auto omSvcRpc = svcMgr_->getNewOMSvcClient();
            omSvcRpc->registerService(svcInfo_);
            break;
        } catch (Exception &e) {
            LOGWARN << "Failed to register: " << e.what() << ".  Retrying...";
        } catch (...) {
            LOGWARN << "Failed to register: unknown excpeption" << ".  Retrying...";
        }
    } while (true);
}

void SvcProcess::setupConfigDb_()
{
    LOGNOTIFY;
    // TODO(Rao): Set up configdb
    fds_panic("Unimpl");
}

void SvcProcess::setupSvcInfo_()
{
    auto config = gModuleProvider->get_conf_helper();
    svcInfo_.svc_id.svc_uuid.svc_uuid = static_cast<int64_t>(config.get<long long>("svc.uuid"));
    svcInfo_.svc_port = config.get<int>("svc.port");
    // TODO(Rao): set up svc info
}

void SvcProcess::setupSvcMgr_(fpi::PlatNetSvcProcessorPtr processor)
{
    LOGNOTIFY;
    svcMgr_.reset(new SvcMgr(processor, svcInfo_));
    /* This will start SvcServer instance */
    svcMgr_->mod_init(nullptr);
}

}  // namespace fds
