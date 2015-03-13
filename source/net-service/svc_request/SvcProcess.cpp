/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
#include <boost/algorithm/string.hpp>
#include <fds_process.h>
#include <fdsp/PlatNetSvc.h>
#include <net/PlatNetSvcHandler.h>
#include <fdsp/OMSvc.h>
#include <util/timeutils.h>
#include <net/SvcMgr.h>
#include <net/net_utils.h>
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
{
    auto handler = boost::make_shared<PlatNetSvcHandler>(this);
    auto processor = boost::make_shared<fpi::PlatNetSvcProcessor>(handler);
    init(argc, argv, def_cfg_file, base_path, def_log_file, mod_vec,
            handler, processor);
}

SvcProcess::SvcProcess(int argc, char *argv[],
                               const std::string &def_cfg_file,
                               const std::string &base_path,
                               const std::string &def_log_file,
                               fds::Module **mod_vec,
                               PlatNetSvcHandlerPtr handler,
                               fpi::PlatNetSvcProcessorPtr processor)
{
    init(argc, argv, def_cfg_file, base_path, def_log_file, mod_vec,
            handler, processor);
}

SvcProcess::~SvcProcess()
{
}

void SvcProcess::init(int argc, char *argv[],
                          const std::string &def_cfg_file,
                          const std::string &base_path,
                          const std::string &def_log_file,
                          fds::Module **mod_vec,
                          PlatNetSvcHandlerPtr handler,
                          fpi::PlatNetSvcProcessorPtr processor)
{
    /* Set up process related services such as logger, timer, etc. */
    FdsProcess::init(argc, argv, def_cfg_file, base_path, def_log_file, mod_vec);

    /* Extract service name pm/sm/dm/am etc */
    std::vector<std::string> strs;
    boost::split(strs, base_path, boost::is_any_of("."));
    fds_verify(strs.size() >= 2);
    svcName_ = strs[1];

    /* Setup Config db for persistence */
    setupConfigDb_();

    /* Populate service information */
    setupSvcInfo_();

    /* Set up service layer */
    setupSvcMgr_(handler, processor);
}

void SvcProcess::proc_pre_startup()
{
    LOGNOTIFY << "Registering the service with om";
    /* Default implementation registers with OM.  Until registration complets
     * this will not return
     */
    auto config = get_conf_helper();
    bool registerWithOM = !(config.get<bool>("testing.standalone"));
    if (registerWithOM) {
        registerSvcProcess();
    }

    /* After registraion is complete other modules can proceed.  See FdsProcess::main() */
}

void SvcProcess::registerSvcProcess()
{
    LOGNOTIFY << "register service process ( parent )" << fds::logDetailedString(svcInfo_);
    std::vector<fpi::SvcInfo> svcMap;

    do {
        try {
            /* This will block until we get a connection.  All the call below should be
             * idempotent.  In case of failure we just retry until we succeed
             */
            auto omSvcRpc = svcMgr_->getNewOMSvcClient();

            omSvcRpc->registerService(svcInfo_);
            LOGNOTIFY << "registerService() call completed";

            omSvcRpc->getSvcMap(svcMap, 0);
            LOGNOTIFY << "got service map.  size: " << svcMap.size();
            break;
        } catch (Exception &e) {
            LOGWARN << "Failed to register: " << e.what() << ".  Retrying...";
        } catch (...) {
            LOGWARN << "Failed to register: unknown excpeption" << ".  Retrying...";
        }
    } while (true);

    svcMgr_->updateSvcMap(svcMap);
}

SvcMgr* SvcProcess::getSvcMgr() {
    return svcMgr_.get();
}

void SvcProcess::setupConfigDb_()
{
    LOGNOTIFY << "setting up configDB";
    // TODO(Rao): Set up configdb
    // fds_panic("Unimpl");
}

void SvcProcess::setupSvcInfo_()
{
    auto config = get_conf_helper();

    fpi::SvcUuid platformUuid;
    platformUuid.svc_uuid = static_cast<int64_t>(config.get_abs<fds_uint64_t>(
            "fds.pm.platform_uuid"));
    auto platformPort = config.get_abs<int>("fds.pm.platform_port");
    auto svcType = SvcMgr::mapToSvcType(svcName_);

    svcInfo_.svc_type = svcType;
    svcInfo_.svc_id.svc_uuid = SvcMgr::mapToSvcUuid(platformUuid,
                                                    svcType);
    svcInfo_.ip = net::get_local_ip(config.get_abs<std::string>("fds.nic_if"));
    svcInfo_.svc_port = SvcMgr::mapToSvcPort(platformPort, svcType);
    svcInfo_.svc_status = fpi::SVC_STATUS_ACTIVE;

    // TODO(Rao): set up correct incarnation no
    svcInfo_.incarnationNo = util::getTimeStampSeconds();
    svcInfo_.name = svcName_;

    fds_assert(svcInfo_.svc_id.svc_uuid.svc_uuid != 0);
    fds_assert(svcInfo_.svc_port != 0);

    LOGNOTIFY << "Service info(from base): " << fds::logString(svcInfo_);
}

void SvcProcess::setupSvcMgr_(PlatNetSvcHandlerPtr handler,
                              fpi::PlatNetSvcProcessorPtr processor)
{
    LOGNOTIFY << "setup service manager";

    handler->mod_init(nullptr);

    svcMgr_.reset(new SvcMgr(this, handler, processor, svcInfo_));
    /* This will start SvcServer instance */
    svcMgr_->mod_init(nullptr);
}

}  // namespace fds