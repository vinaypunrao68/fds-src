/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <string>
#include <am-nbd.h>
#include <fdsn-server.h>
#include <util/fds_stat.h>
#include <am-platform.h>
#include <net/net-service.h>
#include <AccessMgr.h>

namespace fds {

class AMMain : public PlatformProcess
{
  public:
    virtual ~AMMain() {}
    AMMain(int argc, char **argv,
               Platform *platform, Module **mod_vec)
        : PlatformProcess(argc, argv, "fds.am.", "am.log", platform, mod_vec) {}

    void proc_pre_startup() override
    {
        int    argc;
        char **argv;

        PlatformProcess::proc_pre_startup();

        argv = mod_vectors_->mod_argv(&argc);
        am = AccessMgr::unique_ptr(new AccessMgr("AMMain AM Module",
                                                 this));
        proc_add_module(am.get());
        Module *lckstp[] = { am.get(), NULL };
        proc_assign_locksteps(lckstp);
    }
    int run() override {
        am->run();
        return 0;
    }

  private:
    AccessMgr::unique_ptr am;
};

}  // namespace fds

int main(int argc, char **argv)
{
    fds::Module *am_mod_vec[] = {
        &fds::gl_fds_stat,
        &fds::gl_AmPlatform,
        &fds::gl_NetService,
        &fds::gl_NbdBlockMod,
        nullptr
    };

    fds::AMMain amMain(argc, argv, &fds::gl_AmPlatform, am_mod_vec);
    return amMain.main();
}
