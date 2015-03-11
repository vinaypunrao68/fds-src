/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
#include <string>
#include <AccessMgr.h>

#include "AMSvcHandler.h"
#include "net/SvcProcess.h"

namespace fds {

class AMMain : public SvcProcess
{
  public:
    virtual ~AMMain() {}
    AMMain(int argc, char **argv) {
        am = AccessMgr::unique_ptr(new AccessMgr("AMMain AM Module", this));
        fds::Module *modVec[] = {
            am.get(),
            nullptr
        };

        /* Init service process */
        init<AMSvcHandler, fpi::AMSvcProcessor>(argc, argv, "platform.conf",
                "fds.am.", "am.log", modVec);
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
