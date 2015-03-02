/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
#include <string>
#include <AccessMgr.h>

#include "net/SvcProcess.h"

namespace fds {

class AMMain : public SvcProcess
{
  public:
    virtual ~AMMain() {}
    AMMain(int argc, char **argv, Module **mod_vec) {
        am = AccessMgr::unique_ptr(new AccessMgr("AMMain AM Module", this));
        fds::Module *am_mod_vec[] = {
            am.get(),
            nullptr
        };
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
    fds::AMMain amMain(argc, argv, am_mod_vec);
    return amMain.main();
}
