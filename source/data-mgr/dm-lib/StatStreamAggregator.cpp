/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <fds_process.h>
#include <net/net-service.h>
#include <StatStreamAggregator.h>

namespace fds {

StatStreamAggregator::StatStreamAggregator(char const *const name)
        : Module(name) {
}

StatStreamAggregator::~StatStreamAggregator() {
}

int StatStreamAggregator::mod_init(SysParams const *const param)
{
    Module::mod_init(param);
    return 0;
}

void StatStreamAggregator::mod_startup()
{
}

void StatStreamAggregator::mod_shutdown()
{
}

Error StatStreamAggregator::attachVolume(fds_volid_t volume_id) {
    Error err(ERR_OK);
    LOGDEBUG << "Will monitor stats for vol " << std::hex
             << volume_id << std::dec;
    // TODO(xxx) open file where we are going to log stats
    // for this volume
    return err;
}

Error StatStreamAggregator::detachVolume(fds_volid_t volume_id) {
    Error err(ERR_OK);
    LOGDEBUG << "Will stop monitoring stats for vol " << std::hex
             << volume_id << std::dec;
    return err;
}

Error StatStreamAggregator::registerStream(fds_uint32_t reg_id) {
    Error err(ERR_OK);
    LOGDEBUG << "Will start streaming stats: reg id " << reg_id;
    return err;
}

Error StatStreamAggregator::deregisterStream(fds_uint32_t reg_id) {
    Error err(ERR_OK);
    LOGDEBUG << "Will stop streaming stats: reg id " << reg_id;
    return err;
}

Error
StatStreamAggregator::handleModuleStatStream(const StatStreamMsgPtr& stream_msg) {
    Error err(ERR_OK);
    LOGDEBUG << "Handling stats";
    return err;
}

}  // namespace fds
