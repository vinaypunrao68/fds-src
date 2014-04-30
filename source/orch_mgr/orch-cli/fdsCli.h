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
#include <fds_error.h>
#include <util/Log.h>
#include <fds_process.h>
#include <NetSession.h>

namespace fds {

    typedef boost::shared_ptr<FDS_ProtocolInterface::FDSP_ConfigPathReqClient>
            FDSP_ConfigPathReqClientPtr;

    class FdsCli : public FdsProcess {
  private:
        fds_log *cli_log;
        boost::shared_ptr<netSessionTbl> net_session_tbl;
        std::string my_node_name;
        fds_uint32_t om_cfg_port;
        fds_uint32_t om_ip;

  public:
        FdsCli(int argc, char *argv[],
               const std::string &def_cfg_file,
               const std::string &base_path,
               const std::string &def_log_file,  Module **mod_vec);
        ~FdsCli();
        int run() override;
        int fdsCliParser(int argc, char* argv[]);

  private: /* methods */
        void InitCfgMsgHdr(
            FDS_ProtocolInterface::FDSP_MsgHdrType* msg_hdr);
        FDS_ProtocolInterface::FDSP_VolType stringToVolType(
            const std::string& vol_type);
        FDS_ProtocolInterface::FDSP_MediaPolicy stringToMediaPolicy(
            const std::string& media_policy);
        std::string mediaPolicyToString(
            const FDS_ProtocolInterface::FDSP_MediaPolicy media_policy);
        FDS_ProtocolInterface::FDSP_ScavengerCmd stringToScavengerCommand(
            const std::string& cmd);
        FDSP_ConfigPathReqClientPtr startClientSession();
        void endClientSession();
    };

}  // namespace fds

#endif  // SOURCE_ORCH_MGR_ORCH_CLI_FDSCLI_H_
