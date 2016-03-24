/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
#include <string>
#include <AccessMgr.h>

#include "AMSvcHandler.h"
#include "net/SvcProcess.h"
#include "fdsp/common_constants.h"

namespace fds {

class AMMain : public SvcProcess
{
    std::unique_ptr<AccessMgr> am;

  public:
    virtual ~AMMain() {}
    AMMain(int argc, char **argv) {
        am.reset(new AccessMgr("AMMain AM Module", this));
        static fds::Module *modVec[] = {
            am.get(),
            nullptr
        };

        /**
         * Initialize the AMSvc
         */
        auto svc_handler = boost::make_shared<AMSvcHandler>(this, am->getProcessor());
        auto svc_processor = boost::make_shared<fpi::AMSvcProcessor>(svc_handler);

        /**
         * Note on Thrift service compatibility:
         *
         * For service that extends PlatNetSvc, add the processor twice using
         * Thrift service name as the key and again using 'PlatNetSvc' as the
         * key. Only ONE major API version is supported for PlatNetSvc.
         *
         * All other services:
         * Add Thrift service name and a processor for each major API version
         * supported.
         */
        TProcessorMap processors;

        /**
         * When using a multiplexed server, handles requests from AMSvcClient.
         */
        processors.insert(std::make_pair<std::string,
            boost::shared_ptr<apache::thrift::TProcessor>>(
                fpi::commonConstants().AM_SERVICE_NAME, svc_processor));

        /**
         * It is common for SvcLayer to route asynchronous requests using an
         * instance of PlatNetSvcClient. When using a multiplexed server, the
         * processor map must have a key for PlatNetSvc.
         */
        processors.insert(std::make_pair<std::string,
            boost::shared_ptr<apache::thrift::TProcessor>>(
                fpi::commonConstants().PLATNET_SERVICE_NAME, svc_processor));

        /* Init service process */
        init(argc,
             argv,
             "platform.conf",
             "fds.am.",
             "am.log",
             modVec,
             svc_handler,
             processors);
    }

    int run() override {
        am->run();
        std::call_once(mod_shutdown_invoked_,
                        &FdsProcess::shutdown_modules,
                        this);
        return 0;
    }
};

}  // namespace fds

int main(int argc, char **argv)
{
    fds::AMMain amMain(argc, argv);
    return amMain.main();
}
