/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <string>
#include <fds_uuid.h>
#include <net/net-service.h>
#include <node_platform.h>
#include <util/fds_stat.h>

#include "platform/fds_service.h"
#include "platform_net_svc.h"

namespace fds {

FdsService::FdsService(int argc, char *argv[], const std::string &log, Module **vec)
{
    fds_panic("Port this code platform independent service layer");
}

FdsService::~FdsService()
{
    delete [] svc_modules;
}

void
FdsService::proc_pre_startup()
{
    PlatformProcess::proc_pre_startup();
}

void
FdsService::proc_pre_service()
{
}

void
FdsService::plf_load_node_data()
{
    PlatformProcess::plf_load_node_data();
    if (plf_node_data.nd_has_data == 0) {
        NodeUuid uuid(fds_get_uuid64(get_uuid()));
        plf_node_data.nd_node_uuid = uuid.uuid_get_val();
        LOGNOTIFY << "NO UUID, generate one " << std::hex
            << plf_node_data.nd_node_uuid << std::dec;
    }
    // Make up some values for now.
    //
    plf_node_data.nd_has_data    = 1;
    plf_node_data.nd_magic       = 0xcafecaaf;
    plf_node_data.nd_node_number = 0;
    plf_node_data.nd_plat_port   = plf_mgr->plf_get_my_node_port();
    plf_node_data.nd_om_port     = plf_node_data.nd_plat_port + 4;
    plf_node_data.nd_flag_run_sm = 1;
    plf_node_data.nd_flag_run_dm = 1;
    plf_node_data.nd_flag_run_am = 1;
    plf_node_data.nd_flag_run_om = 1;

    // TODO(Vy): deal with error here.
    //
    plf_db->set(plf_db_key,
                std::string((const char *)&plf_node_data, sizeof(plf_node_data)));
}

}  // namespace fds
