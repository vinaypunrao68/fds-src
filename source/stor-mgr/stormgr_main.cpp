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

        auto pHandler = boost::make_shared<fds::SMSvcHandler>(this);
        auto pProcessor = boost::make_shared<FDS_ProtocolInterface::SMSvcProcessor>(pHandler);

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
         * When using a multiplexed server, handles requests from SMSvcClient.
         */
        processors.insert(std::make_pair<std::string,
            boost::shared_ptr<apache::thrift::TProcessor>>(
                FDS_ProtocolInterface::commonConstants().SM_SERVICE_NAME, pProcessor));

        /**
         * It is common for SvcLayer to route asynchronous requests using an
         * instance of PlatNetSvcClient. When using a multiplexed server, the
         * processor map must have a key for PlatNetSvc.
         */
        processors.insert(std::make_pair<std::string,
            boost::shared_ptr<apache::thrift::TProcessor>>(
                FDS_ProtocolInterface::commonConstants().PLATNET_SERVICE_NAME, pProcessor));

        /* Init platform process */
        init(argc, argv, "platform.conf", "fds.sm.", "sm.log", smVec, pHandler, processors);

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

