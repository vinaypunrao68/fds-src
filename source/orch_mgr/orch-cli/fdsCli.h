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
#include <string>
#include "boost/lexical_cast.hpp"
#include "boost/optional.hpp"
#include "boost/program_options.hpp"

#include <fds_types.h>
#include <fdsp/FDSP_types.h>
#include <fds_config.hpp>
#include <fds_err.h>
#include <util/Log.h>
#include <fds_module.h>

namespace fds {

    extern std::ostringstream tcpProxyStr;
    //    extern FDS_ProtocolInterface::FDSP_ConfigPathReqPrx  cfgPrx;

    class FdsCli :
    public Module {
  private:
        fds_log *cli_log;
        boost::shared_ptr<FdsConfig> om_config;

  public:
        explicit FdsCli(const boost::shared_ptr<FdsConfig>& omconf);
        ~FdsCli();

        int  mod_init(SysParams const *const param);
        void mod_startup();
        void mod_shutdown();

        int run(int argc, char* argv[]);

        fds_log* GetLog();
        int fdsCliParser(int argc, char* argv[]);

  private: /* methods */
        void InitCfgMsgHdr(
            FDS_ProtocolInterface::FDSP_MsgHdrType* msg_hdr);
        FDS_ProtocolInterface::FDSP_VolType stringToVolType(
            const std::string& vol_type);
    };

}  // namespace fds

#endif  // SOURCE_ORCH_MGR_ORCH_CLI_FDSCLI_H_
