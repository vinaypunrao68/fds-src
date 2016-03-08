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
         * Because asynchronous service requests are routed manually, any new
         * PlatNetSvc version MUST extend a previous PlatNetSvc version.
         * Only ONE version of PlatNetSvc API can be included in the list of
         * multiplexed services.
         *
         * For other new major service API versions (not PlatNetSvc), pass
         * additional pairs of processor and Thrift service name.
         */
        TProcessorMap processors;
        processors.insert(std::make_pair<std::string,
            boost::shared_ptr<apache::thrift::TProcessor>>(
                fpi::commonConstants().AM_SERVICE_NAME, svc_processor));

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
