/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
#include <boost/algorithm/string.hpp>
#include <fds_process.h>
#include <fdsp/PlatNetSvc.h>
#include <net/PlatNetSvcHandler.h>
#include "fdsp/common_constants.h"
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
    init(argc, argv, false, def_cfg_file, base_path, def_log_file, mod_vec,
            handler, processor, fpi::commonConstants().PLATNET_SERVICE_NAME);
}

SvcProcess::SvcProcess(int argc, char *argv[],
                       const std::string &def_cfg_file,
                       const std::string &base_path,
                       const std::string &def_log_file,
                       fds::Module **mod_vec,
                       PlatNetSvcHandlerPtr handler,
                       fpi::PlatNetSvcProcessorPtr processor,
                       const std::string& thriftServiceName)
: SvcProcess(argc, argv, false, def_cfg_file, base_path, def_log_file, mod_vec,
            handler, processor, thriftServiceName)
{
}

SvcProcess::SvcProcess(int argc, char *argv[],
                       bool initAsModule,
                       const std::string &def_cfg_file,
                       const std::string &base_path,
                       const std::string &def_log_file,
                       fds::Module **mod_vec,
                       PlatNetSvcHandlerPtr handler,
                       fpi::PlatNetSvcProcessorPtr processor,
                       const std::string &thriftServiceName)
{
    init(argc, argv, initAsModule, def_cfg_file,
         base_path, def_log_file, mod_vec, handler, processor, thriftServiceName);
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
                      fpi::PlatNetSvcProcessorPtr processor,
                      const std::string& thriftServiceName)
{
    init(argc, argv, false, def_cfg_file,
         base_path, def_log_file, mod_vec, handler, processor, thriftServiceName);
}
void SvcProcess::init(int argc, char *argv[],
                      bool initAsModule,
                      const std::string &def_cfg_file,
                      const std::string &base_path,
                      const std::string &def_log_file,
                      fds::Module **mod_vec,
                      PlatNetSvcHandlerPtr handler,
                      fpi::PlatNetSvcProcessorPtr processor,
                      const std::string &thriftServiceName)
{
    if (!initAsModule) {
        /* Set up process related services such as logger, timer, etc. */
        FdsProcess::init(argc, argv, def_cfg_file, base_path, def_log_file, mod_vec);
    } else {
        LOGDEBUG << "Initializing as module";
        initAsModule_(argc, argv, def_cfg_file, base_path, mod_vec);
    }

    /* Extract service name pm/sm/dm/am etc */
    std::vector<std::string> strs;
    // FIXME: What if there's two dots in the name?
    boost::split(strs, base_path, boost::is_any_of("."));
    fds_verify(strs.size() >= 2);
    svcName_ = strs[1];

    /* Setup Config db for persistence */
    setupConfigDb_();

    /* Populate service information */
    setupSvcInfo_();

    /* Set up service layer */
    setupSvcMgr_(handler, processor, thriftServiceName);
}

void SvcProcess::initAsModule_(int argc, char *argv[],
                               const std::string &def_cfg_file,
                               const std::string &base_path,
                               fds::Module **mod_vec)
{
    fds_verify(g_fdslog != nullptr);
    std::string  fdsroot, cfgfile;
    mod_vectors_ = new ModuleVector(argc, argv, mod_vec);
    fdsroot      = mod_vectors_->get_sys_params()->fds_root;
    proc_root    = new FdsRootDir(fdsroot);
    cfgfile = proc_root->dir_fds_etc() + def_cfg_file;
    /* Check if config file exists */
    struct stat buf;
    if (stat(cfgfile.c_str(), &buf) == -1) {
        // LOGGER isn't ready at this point yet.
        std::cerr << "Configuration file " << cfgfile << " not found. Exiting."
            << std::endl;
        exit(ERR_DISK_READ_FAILED);
    }

    setup_config(argc, argv, cfgfile, base_path);
    setup_cntrs_mgr(net::get_my_hostname());
    setup_timer_service();
    int num_thr = conf_helper_.get<int>("threadpool.num_threads", 10);
    proc_thrp   = new fds_threadpool(num_thr);
}

void SvcProcess::start_modules()
{
    /* NOTE: This sequnce is copied from fds_process::start_modules().  For
     * SvcProcess we need to register with OM as part of startup sequence.
     * This is very complicated and needs to be simplified.
     * Overall in my observation mod_init(), mod_startup(), registerSvcProcess(),
     * mod_start_services() seems to be common sequence.  However even this can
     * be further simplfied to mod_init() (pre-register), registerSvcProcess(),
     * mod_startup() (post register).  It'd be nice to change names as well
     * I guess it will stay this way until we find time to refactor this code.
     */


    /* Defer any incoming requests over the network */
    svcMgr_->getSvcRequestHandler()->setHandlerState(PlatNetSvcHandler::DEFER_REQUESTS);

    mod_vectors_->mod_init_modules();

    /* The process should have all objects allocated in proper place. */
    proc_pre_startup();

    /* The false flags runs module appended by the pre_startup() call. */
    mod_vectors_->mod_init_modules(false);

    /* Do FDS process startup sequence. */
    mod_vectors_->mod_startup_modules();
    mod_vectors_->mod_startup_modules(false);

    /* Register with OM */
    LOGNOTIFY << "Registering the service with om";
    /* Default implementation registers with OM.  Until registration completes
     * this will not return
     */
    auto config = get_conf_helper();
    bool registerWithOM = !(config.get<bool>("testing.standalone", false));
    if (registerWithOM) {
        registerSvcProcess();
    }

    /*  Start to run the main process. */
    proc_pre_service();
    mod_vectors_->mod_start_services();
    mod_vectors_->mod_start_services(false);

    mod_vectors_->mod_run_locksteps();

    /* At this time service is ready for network requests.  Any pending request
     * are drained.
     */
    svcMgr_->getSvcRequestHandler()->setHandlerState(PlatNetSvcHandler::ACCEPT_REQUESTS);

}

void SvcProcess::shutdown_modules()
{
    LOGNOTIFY << "Shuttingdown modules";

    shutdownGate_.open();

    mod_vectors_->mod_stop_services();
    mod_vectors_->mod_shutdown_locksteps();
    mod_vectors_->mod_shutdown();

    LOGNOTIFY << "Stopping server";
    svcMgr_->stopServer();

    LOGNOTIFY << "Stopping timer";
    timer_servicePtr_->destroy();

    LOGNOTIFY << "Destroying cntrs mgr";
    cntrs_mgrPtr_->reset();

    LOGNOTIFY << "Stopping threadpool";
    proc_thrp->stop();
}

void SvcProcess::registerSvcProcess()
{
    LOGNOTIFY << "register service process ( parent )" << fds::logDetailedString(svcInfo_);
    std::vector<fpi::SvcInfo> svcMap;

    int numAttempts = 0;

    // microseconds
    int waitTime = 10 * 1000;   // 10 milliseconds
    int maxWaitTime = 500 * 1000;  // 500 milliseconds

    do {
        try {
            ++numAttempts;

            LOGDEBUG << "Attempting connection to OM [" << numAttempts << "]";

            /* This will block until we get a connection.  All the call below should be
             * idempotent.  In case of failure we just retry until we succeed
             */
            auto omSvcRpc = svcMgr_->getNewOMSvcClient();

            omSvcRpc->registerService(svcInfo_);
            LOGNOTIFY << "registerService() call completed: " << fds::logString(svcInfo_);

            omSvcRpc->getSvcMap(svcMap, 0);
            LOGNOTIFY << "got service map.  size: " << svcMap.size();

            break;
        } catch (Exception &e) {
            LOGWARN << "Failed to register: " << e.what() << ".  Retrying...";
        } catch(const std::runtime_error& re) {
            // specific handling for runtime_error
            LOGWARN << "Runtime error: " << re.what() << std::endl;
        }  catch(const std::exception& ex) {
            // specific handling for all exceptions extending std::exception, except
            // std::runtime_error which is handled explicitly
            LOGWARN << "Error occurred: " << ex.what() << std::endl;
        }catch (...) {
            LOGWARN << "Failed to register: unknown exception" << ".  Retrying...";
        }

        if (numAttempts % 5 == 0) {
            waitTime *= 2;
            if (waitTime > maxWaitTime)
                waitTime = maxWaitTime;
        }
        usleep(waitTime);

    } while (true);

    svcMgr_->updateSvcMap(svcMap);
}

SvcMgr* SvcProcess::getSvcMgr() {
    return svcMgr_.get();
}

void SvcProcess::setupConfigDb_()
{
    LOGNOTIFY << "setting up configDB";
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
    svcInfo_.ip = net::get_local_ip(config.get_abs<std::string>("fds.nic_if", ""));

    // get the service port as an offset from the platform port.
    svcInfo_.svc_port = SvcMgr::mapToSvcPort(platformPort, svcType);

    // OM port is handled differently than other services.  It is based on fds.common.om_port
    // configuration (potentially overridden by command line option, but that part is handled
    // before we get here)
    // TODO: this would make sense to move to OrchMgr::setupSvcInfo_ but if so the message
    // logged below would have the mapped OM port instead of the configured OM port and be
    // potentially confusing if not using the default port value.
    if ( svcType ==  fpi::FDSP_ORCH_MGR ) {
        svcInfo_.svc_port = config.get_abs<int>("fds.common.om_port", svcInfo_.svc_port);
    }
    svcInfo_.svc_status = fpi::SVC_STATUS_ACTIVE;

    svcInfo_.incarnationNo = util::getTimeStampSeconds();
    svcInfo_.name = svcName_;

    // With this method of building the FdsProcess, no signal handler thread is started.
    FdsProcess::sig_tid_ready = true;

    fds_assert(svcInfo_.svc_id.svc_uuid.svc_uuid != 0);
    fds_assert(svcInfo_.svc_port != 0);

    LOGNOTIFY << "Service info(from base): " << fds::logString(svcInfo_);
}

void SvcProcess::setupSvcMgr_(PlatNetSvcHandlerPtr handler,
                              fpi::PlatNetSvcProcessorPtr processor,
                              const std::string &thriftServiceName)
{
    LOGNOTIFY << "setup service manager";

    if (handler->mod_init(nullptr) != 0) {
        LOGERROR << "Failed to initialize service handler.  Throwing an exception";
        throw std::runtime_error("Failed to initialize service handler");
    }

    svcMgr_.reset(new SvcMgr(this, handler, processor, svcInfo_, thriftServiceName));
    svcMgr_->setSvcServerListener(this);
    /* This will start SvcServer instance */
    if (svcMgr_->mod_init(nullptr) != 0) {
        LOGERROR << "Failed to initialize service manager.  Throwing an exception";
        throw std::runtime_error("Failed to initialize service manager");
    }
}

void SvcProcess::notifyServerDown(const Error &e) {
    LOGERROR << "Svc server down " << e << ".  Bringing the service down";
    std::call_once(mod_shutdown_invoked_,
                    &FdsProcess::shutdown_modules,
                    this);
}

void SvcProcess::updateMsgHandler(const fpi::FDSPMsgTypeId msgId,
                                  const PlatNetSvcHandler::FdspMsgHandler &handler)
{
    auto reqHandler = getSvcMgr()->getSvcRequestHandler();
    reqHandler->updateHandler(msgId, handler);
}

}  // namespace fds
