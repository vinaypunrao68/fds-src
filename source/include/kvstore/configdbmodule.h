/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_KVSTORE_CONFIGDBMODULE_H_
#define SOURCE_INCLUDE_KVSTORE_CONFIGDBMODULE_H_

#include <fds_module.h>
#include <kvstore/configdb.h>

namespace fds {
    struct ConfigDBModule : Module  {
        ConfigDBModule();
        virtual int  mod_init(SysParams const *const param);
        virtual void mod_startup();
        virtual void mod_shutdown();
        kvstore::ConfigDB* get();
  protected:
        kvstore::ConfigDB* configDB;
    };
    extern ConfigDBModule gl_configDB;
}  // namespace fds

#endif  // SOURCE_INCLUDE_KVSTORE_CONFIGDBMODULE_H_
