/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <string>
#include <fds_process.h>
#include <AmDispatcher.h>

namespace fds {

AmDispatcher::AmDispatcher(const std::string &modName)
        : Module(modName.c_str()) {
    FdsConfigAccessor conf(g_fdsprocess->get_fds_config(), "fds.am.");
    uturnAll = conf.get<fds_bool_t>("testing.uturn_dispatcher_all");
}

AmDispatcher::~AmDispatcher() {
}

int
AmDispatcher::mod_init(SysParams const *const param) {
    Module::mod_init(param);
    return 0;
}

void
AmDispatcher::mod_startup() {
}

void
AmDispatcher::mod_shutdown() {
}

}  // namespace fds
