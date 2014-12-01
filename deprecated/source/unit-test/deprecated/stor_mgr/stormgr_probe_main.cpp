/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <sm_probe.h>
#include <iostream>
#include <fds_config.hpp>
#include <policy_tier.h>
#include <StorMgr.h>

int main(int argc, char *argv[]) {
    boost::shared_ptr<FdsConfig> config(new FdsConfig("fds.conf"));
    objStorMgr = new ObjectStorMgr(config);

  /* Instantiate a DiskManager Module instance */
  fds::Module *io_dm_vec[] = {
    &diskio::gl_dataIOMod,
    &fds::gl_tierPolicy,
    fds::objStorMgr->objStats,
    nullptr
  };
  fds::ModuleVector  io_dm(argc, argv, io_dm_vec);
  io_dm.mod_execute();

  objStorMgr->main(argc, argv, "stor_mgr.conf");

  delete objStorMgr;

  return 0;
}

