/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <StorMgr.h>

#include <SMSvcHandler.h>
#include <net/SvcProcess.h>
#include "fdsp/common_constants.h"

class SMMain : public SvcProcess
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
            sm,
            nullptr
        };

        /* Init platform process */
        // Note on Thrift service compatibility:
        // If a backward incompatible change arises, pass additional pairs of
        // processor and Thrift service name to SvcProcess::init(). Similarly,
        // if the Thrift service API wants to be broken up.
        init<fds::SMSvcHandler, FDS_ProtocolInterface::SMSvcProcessor>(argc, argv, "platform.conf", "fds.sm.",
                "sm.log", smVec, fds::fpi::commonConstants().SM_SERVICE_NAME);

        /* setup signal handler */
        setupSigHandler();
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
    /* Based on command line arg --foreground is set, don't daemonize the process */
    fds::FdsProcess::checkAndDaemonize(argc, argv);

    auto smMain = new SMMain(argc, argv);
    auto ret = smMain->main();
    delete smMain;
    return ret;
}

