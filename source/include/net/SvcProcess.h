/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_NET_SVCPROCESS_H_
#define SOURCE_INCLUDE_NET_SVCPROCESS_H_

#include <boost/shared_ptr.hpp>

#include <fds_process.h>
#include <fdsp/svc_types_types.h>
#include <net/SvcServer.h>
#include <net/PlatNetSvcHandler.h>

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

/**
* @brief Base class for all Services
* On top of what fds_process, it provides
* 1. SvcMgr (Responsible for managing service layer)
* 2. Config db for persistence
* 2. Service registration
*/
struct SvcProcess : FdsProcess, SvcServerListener {
    SvcProcess();
    SvcProcess(int argc, char *argv[],
               const std::string &def_cfg_file,
               const std::string &base_path,
               const std::string &def_log_file,
               fds::Module **mod_vec);
    SvcProcess(int argc, char *argv[],
               const std::string &def_cfg_file,
               const std::string &base_path,
               const std::string &def_log_file,
               fds::Module **mod_vec,
               PlatNetSvcHandlerPtr handler,
               fpi::PlatNetSvcProcessorPtr processor,
               const std::string &thriftServiceName);
    SvcProcess(int argc, char *argv[],
               bool initAsModule,
               const std::string &def_cfg_file,
               const std::string &base_path,
               const std::string &def_log_file,
               fds::Module **mod_vec,
               PlatNetSvcHandlerPtr handler,
               fpi::PlatNetSvcProcessorPtr processor,
               const std::string &strThiftServiceName);
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
    * 3. Do the necessary registration workd
    * 4. Startup the passedin modules
    *
    * @param argc
    * @param argv[]
    * @param initAsModule
    * @param def_cfg_file
    * @param base_path
    * @param def_log_file
    * @param mod_vec
    * @param processor
    */
    void init(int argc, char *argv[],
              bool initAsModule,
              const std::string &def_cfg_file,
              const std::string &base_path,
              const std::string &def_log_file,
              fds::Module **mod_vec,
              PlatNetSvcHandlerPtr handler,
              fpi::PlatNetSvcProcessorPtr processor,
              const std::string &thriftServiceName);
    void init(int argc, char *argv[],
              const std::string &def_cfg_file,
              const std::string &base_path,
              const std::string &def_log_file,
              fds::Module **mod_vec,
              PlatNetSvcHandlerPtr handler,
              fpi::PlatNetSvcProcessorPtr processor,
              const std::string &thriftServiceName);

    /**
     * Note on Thrift service compatibility:
     * If a backward incompatible change arises, pass additional pairs of
     * processor and Thrift service name to SvcProcess::init(). Similarly,
     * if the Thrift service API wants to be broken up.
     *
     * @tparam Handler of type PlatNetSvcHandler
     * @tparam Processor of type fpi::PlatNetSvcProcessor (or a subtype)
     */
    template<typename Handler, typename Processor>
    void init(int argc, char *argv[],
              const std::string &def_cfg_file,
              const std::string &base_path,
              const std::string &def_log_file,
              fds::Module **mod_vec,
              const std::string &thriftServiceName) {
        auto handler = boost::make_shared<Handler>(this);
        auto processor = boost::make_shared<Processor>(handler);
        init(argc, argv, false, def_cfg_file, base_path, def_log_file, mod_vec,
                handler, processor, thriftServiceName);
    }

    /**
     * Note on Thrift service compatibility:
     * If a backward incompatible change arises, pass additional pairs of
     * processor and Thrift service name to SvcProcess::init(). Similarly,
     * if the Thrift service API wants to be broken up.
     *
     * @tparam Handler of type PlatNetSvcHandler
     * @tparam Processor of type fpi::PlatNetSvcProcessor (or a subtype)
     */
    template<typename Handler, typename Processor>
    void init(int argc, char *argv[], bool initAsModule,
              const std::string &def_cfg_file,
              const std::string &base_path,
              const std::string &def_log_file,
              fds::Module **mod_vec,
              const std::string& thriftServiceName) {
        auto handler = boost::make_shared<Handler>(this);
        auto processor = boost::make_shared<Processor>(handler);
        init(argc, argv, initAsModule, def_cfg_file, base_path, def_log_file, mod_vec,
                handler, processor, thriftServiceName);
    }

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
    * @param processor
    */
    virtual void setupSvcMgr_(PlatNetSvcHandlerPtr handler,
                              fpi::PlatNetSvcProcessorPtr processor,
                              const std::string &thriftServiceName);

    /* TODO(Rao): Include persistence as well */
    boost::shared_ptr<SvcMgr> svcMgr_;
    fpi::SvcInfo              svcInfo_;
    std::string               svcName_;
};
}  // namespace fds
#endif  // SOURCE_INCLUDE_NET_SVCPROCESS_H_
