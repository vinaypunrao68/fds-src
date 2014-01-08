/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

/*
 *   Command line  interface.
 */

#ifndef SOURCE_ORCH_MGR_ORCH_CLI_FDSCLI_H_
#define SOURCE_ORCH_MGR_ORCH_CLI_FDSCLI_H_

#include <iostream>
#include <vector>
#include "boost/lexical_cast.hpp"
#include "boost/optional.hpp"
#include "boost/program_options.hpp"
#include <string>
#include <Ice/Ice.h>
#include <fds_types.h>
#include <fdsp/FDSP.h>
#include <fds_err.h>
#include <util/Log.h>

#include <fds_module.h>

namespace fds {

    extern std::ostringstream tcpProxyStr;
    extern FDS_ProtocolInterface::FDSP_ConfigPathReqPrx  cfgPrx;

    class FdsCli :
    virtual public Ice::Application,
        public Module {
  private:
        fds_log *cli_log;

  public:
        FdsCli();
        ~FdsCli();

        int  mod_init(SysParams const *const param);
        void mod_startup();
        void mod_shutdown();

        void runServer();

        virtual int run(int argc, char* argv[]);
        // void interruptCallback(int);
        fds_log* GetLog();
        int fdsCliPraser(int argc, char* argv[]);
        void InitCfgMsgHdr(
            const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr);
    };

}  // namespace fds

#endif  // SOURCE_ORCH_MGR_ORCH_CLI_FDSCLI_H_
