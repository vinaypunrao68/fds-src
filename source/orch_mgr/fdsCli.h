/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

/*
 *   Command line  interface.
 */

#ifndef FDS_CLI_H
#define FDS_CLI_H

#include <iostream>
#include <vector>
#include "boost/lexical_cast.hpp"
#include "boost/optional.hpp"
#include "boost/program_options.hpp"
#include <string>
#include <Ice/Ice.h>
#include "include/fds_types.h"
#include "fdsp/fdsp_types.h"
#include "fdsp/FDSP.h"
#include "include/fds_err.h"
#include "util/Log.h"




using namespace FDSP_Types;
using namespace FDS_ProtocolInterface;

namespace fds {

   std::ostringstream tcpProxyStr;
   FDSP_ConfigPathReqPrx  cfgPrx;

  class FdsCli : virtual public Ice::Application {
  private:

  public:
    FdsCli();
    ~FdsCli();

   virtual int run(int argc, char* argv[]);
//   void interruptCallback(int);
//   fds_log* GetLog();
   int fdsCliPraser(int argc, char* argv[]);
   void InitCfgMsgHdr(const FDSP_MsgHdrTypePtr& msg_hdr);
   };

}  // namespace fds

#endif  // FDS_CLI_H
