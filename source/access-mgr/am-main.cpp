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

        /* Init service process */
        // Note on Thrift service compatibility:
        // If a backward incompatible change arises, pass additional pairs of
        // processor and Thrift service name to SvcProcess::init(). Similarly,
        // if the Thrift service API wants to be broken up.
        init(argc,
             argv,
             "platform.conf",
             "fds.am.",
             "am.log",
             modVec,
             svc_handler,
             svc_processor,
             fpi::commonConstants().AM_SERVICE_NAME);
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
