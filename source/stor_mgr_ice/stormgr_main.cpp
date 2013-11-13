/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <StorMgr.h>
#include <policy_tier.h>

int main(int argc, char *argv[]) {
  objStorMgr = new ObjectStorMgr();

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

