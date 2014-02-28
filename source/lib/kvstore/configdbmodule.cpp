/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <kvstore/configdbmodule.h>
#include <fds_config.hpp>
#include <fds_process.h>
#include <util/Log.h>
#include <string>
namespace fds {
ConfigDBModule gl_configDB;

ConfigDBModule::ConfigDBModule() : Module("configdb") {
}

int ConfigDBModule::mod_init(SysParams const *const param) {
    LOGDEBUG << "configdb.module init";

    // get data from config
    FdsConfigAccessor conf(g_fdsprocess->get_conf_helper());
    configDB = new kvstore::ConfigDB(
        conf.get<std::string>("configdb.host", "localhost"),
        conf.get<int>("configdb.port", 6379),
        conf.get<int>("configdb.poolsize", 10));
    return 0;
}

void ConfigDBModule::mod_startup() {
    LOGDEBUG <<"configdb.module startup";
}

void ConfigDBModule::mod_shutdown() {
    LOGDEBUG <<"configdb.module shutdown";
    delete configDB;
}

kvstore::ConfigDB* ConfigDBModule::get() {
    return configDB;
}

}  // namespace fds
