/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <StorMgr.h>
#include <policy_tier.h>
#include <sm-platform.h>
#include <net/net-service.h>

#include "platform/platform_process.h"

class SMMain : public PlatformProcess
{
 public:
    SMMain(int argc, char *argv[]) {
        /* Construct all the modules */
        sm = new fds::ObjectStorMgr(this);
        // TODO(Rao): Get rid of this singleton
        objStorMgr  = sm;

        /* Create the dependency vector */
        static fds::Module *smVec[] = {
            &diskio::gl_dataIOMod,
            &fds::gl_SmPlatform,
            &fds::gl_NetService,
            &fds::gl_tierPolicy,
            sm,
            nullptr
        };

         /* Before calling init, close all file descriptors.  Later, we may daemonize the
         * process, in which case we may be closing all existing file descriptors while
         * threads may access the file descriptor.
         */
        closeAllFDs();

        /* Init platform process */
        init(argc, argv, "fds.sm.", "sm.log", &gl_SmPlatform, smVec);

        /* setup signal handler */
        setupSigHandler();

        /* Daemonize */
        fds_bool_t daemonizeProc = get_fds_config()->get<bool>("fds.sm.daemonize", true);
        if (true == daemonizeProc) {
            daemonize();
        }
    }

    static void SIGSEGVHandler(int sigNum, siginfo_t *sigInfo, void *context) {
        GLOGCRITICAL << "SIGSEGV at address: " << std::hex << sigInfo->si_addr
                     << " with code " << std::dec << sigInfo->si_code;

        /* Since the signal handler was originally set with SA_RESETHAND,
         * the default signal handler is restored.  After the signal handler
         * completes, the thread resumes from the the faulting address that will result in
         * another SIGSEGV (most likely), and it will invoke the default SIGSEGV signal handler,
         * which is to core dump.
         */
    }

    void setupSigHandler() {
        /* setup a process wide signal handler for SIGSEGV.
         * For synchronous signals, handle it on the faulting thread's context.
         * For asynchrnous signals, it's preferred to handle it by a dedicated thread.
         */
        struct sigaction sigAct;
        sigAct.sa_flags = (SA_SIGINFO | SA_RESETHAND);
        sigemptyset(&sigAct.sa_mask);
        sigAct.sa_sigaction = SMMain::SIGSEGVHandler;
        if (sigaction(SIGSEGV, &sigAct, NULL) == -1) {
            LOGWARN << "SIGSEGV signal handler is not set";
        }
    }

    virtual ~SMMain() {
        /* Destruct created module objects */
        // TODO(brian): Revisit this; currently sm will be deleting itself
        // when this gets called... the following line will cause a double delete
        // delete sm;
        LOGDEBUG << "ENDING SM PROCESS";
    }

    virtual int run() {
        return sm->run();
    }

    fds::ObjectStorMgr* sm;
};

int main(int argc, char *argv[])
{
    auto smMain = new SMMain(argc, argv);
    auto ret = smMain->main();
    delete smMain;
    return ret;
}

