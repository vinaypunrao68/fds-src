/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_NET_SVCPROCESS_H_
#define SOURCE_INCLUDE_NET_SVCPROCESS_H_

#include <string>
#include <unordered_map>

#include <boost/shared_ptr.hpp>

#include <fds_process.h>
#include <fdsp/svc_types_types.h>
#include <net/SvcServer.h>
#include <net/PlatNetSvcHandler.h>

#include "fdsp/common_constants.h"

/* Forward declarations */
namespace FDS_ProtocolInterface {
    class PlatNetSvcProcessor;
    using PlatNetSvcProcessorPtr = boost::shared_ptr<PlatNetSvcProcessor>;
}  // namespace FDS_ProtocolInterface
namespace fpi = FDS_ProtocolInterface;

namespace fds {

/* Forward declarations */
class FdsProcess;
struct SvcMgr; 

typedef std::unordered_map<std::string, boost::shared_ptr<apache::thrift::TProcessor>> TProcessorMap;

/**
* @brief Base class for all Services
* On top of what fds_process, it provides
* 1. SvcMgr (Responsible for managing service layer)
* 2. Config db for persistence
* 3. Service registration
*/
struct SvcProcess : FdsProcess, SvcServerListener {

    SvcProcess();
    /**
     * Initializes the process using a base PlatNetSvcHandler, which can handle
     * asynchronous service requests.
     */
    SvcProcess(int argc, char *argv[],
               const std::string &def_cfg_file,
               const std::string &base_path,
               const std::string &def_log_file,
               fds::Module **mod_vec);

    /**
     * @param def_cfg_file Name of the configuration file for the process
     *  Example: "platform.conf".
     *
     * @param base_path The '.' delimited path to the configuration file data
     *  for this process. Example: "fds.om." for an OM process.
     *
     * @param def_log_file Name of the default log file for the process.
     *  Example: "om.log".
     *
     * @param mod_vec Collection of modules used by the process.
     *
     * @param asyncHandler A service handler for asynchronous requests. The
     *  processor for this handler must be included in the processors map.
     *  Exposed external to the map of TProcessor objects because it requires
     *  special initialization. A PlatNetSvcHandler is more than just a Thrift
     *  service handler. It also IS-A module.
     *
     * @param processors A collection of processors keyed by unique Thrift
     *  service name. A processor has a service handler.
     */
    SvcProcess(int argc, char *argv[],
               const std::string &def_cfg_file,
               const std::string &base_path,
               const std::string &def_log_file,
               fds::Module **mod_vec,
               PlatNetSvcHandlerPtr asyncHandler,
               TProcessorMap& processors);

    /**
     * @param initAsModule When true, initializes process as module.
     *
     * @param def_cfg_file Name of the configuration file for the process
     *  Example: "platform.conf".
     *
     * @param base_path The '.' delimited path to the configuration file data
     *  for this process. Example: "fds.om." for an OM process.
     *
     * @param def_log_file Name of the default log file for the process.
     *  Example: "om.log".
     *
     * @param mod_vec Collection of modules used by the process.
     *
     * @param asyncHandler A service handler for asynchronous requests. The
     *  processor for this handler must be included in the processors map.
     *  Exposed external to the map of TProcessor objects because it requires
     *  special initialization. A PlatNetSvcHandler is more than just a Thrift
     *  service handler. It also IS-A module.
     *
     * @param processors A collection of processors keyed by unique Thrift
     *  service name. A processor has a service handler.
     */
    SvcProcess(int argc, char *argv[],
               bool initAsModule,
               const std::string &def_cfg_file,
               const std::string &base_path,
               const std::string &def_log_file,
               fds::Module **mod_vec,
               PlatNetSvcHandlerPtr asyncHandler,
               TProcessorMap& processors);

    virtual ~SvcProcess();

    /**
     * Make the base class init() visible, otherwise the overload
     * below hides it making it unreachable.
     */
    using FdsProcess::init;

    /**
    * @brief Initializes the necessary services.
    * Init process is
    * 1. Set up configdb
    * 2. Set up service layer
    * 3. Do the necessary registration work
    * 4. Startup the passedin modules
    *
    * @param argc
    * @param argv[]
    * @param initAsModule
    * @param def_cfg_file
    * @param base_path
    * @param def_log_file
    * @param mod_vec
    * @param asyncHandler
    * @param processors
    */
    void init(int argc, char *argv[],
              bool initAsModule,
              const std::string &def_cfg_file,
              const std::string &base_path,
              const std::string &def_log_file,
              fds::Module **mod_vec,
              PlatNetSvcHandlerPtr asyncHandler,
              TProcessorMap& processors);
    void init(int argc, char *argv[],
              const std::string &def_cfg_file,
              const std::string &base_path,
              const std::string &def_log_file,
              fds::Module **mod_vec,
              PlatNetSvcHandlerPtr asyncHandler,
              TProcessorMap& processors);

    /**
     * @brief The entry point of where all the modules listed in the mod_vectors
     * get started.
     * It does the following:
     * 1. (Pre-register) - Tells each module to initialize.
     * 2. (Register) - Registers with the OM (using registerSvcProcess() below)
     * 3. (Post-register) - Tells each module to start services that can now start
     * 	  after having registered with the OM.
     */
    virtual void start_modules() override;

    /**
    * @brief Shuts down
    * 1. First all the registered module
    * 2. SvcServer, timer, Process threadpool
    */
    virtual void shutdown_modules() override;

    /**
    * @brief Registers the service.  Default implementation will register the service
    * with OM.
    * Override this behavior depending on the service type.
    */
    virtual void registerSvcProcess();

    virtual SvcMgr* getSvcMgr() override;

    /* SvcServerListener notifications */
    virtual void notifyServerDown(const Error &e) override;

    void updateMsgHandler(const fpi::FDSPMsgTypeId msgId,
                          const PlatNetSvcHandler::FdspMsgHandler &handler);

 protected:
    void initAsModule_(int argc, char *argv[],
                       const std::string &def_cfg_file,
                       const std::string &base_path,
                       fds::Module **mod_vec);
    /**
    * @brief Sets up configdb used for persistence
    */
    virtual void setupConfigDb_();

    /**
    * @brief Populates service information
    */
    virtual void setupSvcInfo_();

    /**
    * @brief Sets up service layer manager
    *
    * @detail
    * When this class was originally designed, all FDS Thrift servers were
    * non-multiplexed. As a consequence, only one Thrift service handler/
    * processor was allowed. The handler took care of asynchronous requests
    * as well as any specialized synchronous requests for the FDS service
    * type (there are subtypes of PlatNetSvcHandler).
    *
    * Now multiplexed Thrift servers are supported. A service manager might
    * use multiple handler/processor objects. Because asynchronous service
    * requests are routed manually, any new PlatNetSvc version MUST extend a
    * previous PlatNetSvc version. Only ONE major API version is supported
    * for PlatNetSvc.
    *
    * @param asyncHandler A service handler for asynchronous requests. The
    *  processor for this handler must be included in the processors map.
    *  Exposed external to the map of TProcessor objects because it requires
    *  special initialization. A PlatNetSvcHandler is more than just a Thrift
    *  service handler. It also IS-A module.
    *
    * @param processors A collection of processors keyed by unique Thrift
    *  service name. A processor has a service handler.
    */
    virtual void setupSvcMgr_(PlatNetSvcHandlerPtr asyncHandler, TProcessorMap& processors);

    /* TODO(Rao): Include persistence as well */
    boost::shared_ptr<SvcMgr> svcMgr_;
    fpi::SvcInfo              svcInfo_;
    std::string               svcName_;
};
}  // namespace fds
#endif  // SOURCE_INCLUDE_NET_SVCPROCESS_H_
