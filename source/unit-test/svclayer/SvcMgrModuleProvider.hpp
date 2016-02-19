/*
 * Copyright 2015 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_SVCMGR_MODULE_PROVIDER_H_
#define SOURCE_INCLUDE_SVCMGR_MODULE_PROVIDER_H_

#include <string>
#include <boost/make_shared.hpp>
#include <fds_module_provider.h>
#include <net/SvcMgr.h>
#include <fdsp_utils.h>
#include "fdsp/common_constants.h"

namespace fds {

/**
* @brief Testing helper for providing the necessary modules for svc mgr
*/
struct SvcMgrModuleProvider : CommonModuleProviderIf {
    SvcMgrModuleProvider(const std::string &configFile, fds_int64_t uuid, int port) {
        /* config */
        std::string configBasePath = "fds.dm.";
        boost::shared_ptr<FdsConfig> config(new FdsConfig(configFile, 0, {nullptr}));
        configHelper_.init(config, configBasePath);
        config->set("fds.dm.svc.uuid", uuid);
        config->set("fds.dm.svc.port", port);

        /* timer */
        timer_.reset(new FdsTimer());

        /* counters */
        cntrsMgr_.reset(new FdsCountersMgr(configBasePath));

        /* threadpool */
        threadpool_.reset(new fds_threadpool(3));

        /* service mgr */
        auto handler = boost::make_shared<PlatNetSvcHandler>(this);
        auto processor = boost::make_shared<fpi::PlatNetSvcProcessor>(handler);
        fpi::SvcInfo svcInfo;
        svcInfo.svc_id.svc_uuid.svc_uuid = 
                static_cast<int64_t>(configHelper_.get<fds_uint64_t>("svc.uuid", 0));
        svcInfo.ip = net::get_local_ip(configHelper_.get_abs<std::string>("fds.nic_if", "lo"));
        svcInfo.svc_port = configHelper_.get<int>("svc.port");
        svcInfo.incarnationNo = util::getTimeStampSeconds();
        svcMgr_.reset(new SvcMgr(this, handler, processor, svcInfo, fpi::commonConstants().PLATNET_SERVICE_NAME));
        svcMgr_->mod_init(nullptr);
    }
    ~SvcMgrModuleProvider() {
       timer_->destroy(); 
    }
    virtual FdsConfigAccessor get_conf_helper() const override {
        return configHelper_;
    }

    virtual boost::shared_ptr<FdsConfig> get_fds_config() const override {
        return configHelper_.get_fds_config();
    }

    virtual FdsTimerPtr getTimer() const override { return timer_; }

    virtual boost::shared_ptr<FdsCountersMgr> get_cntrs_mgr() const override {
        return cntrsMgr_;
    }

    virtual fds_threadpool *proc_thrpool() const override { return threadpool_.get(); }

    virtual SvcMgr* getSvcMgr() override { return svcMgr_.get(); }

 protected:
    FdsConfigAccessor configHelper_;
    FdsTimerPtr timer_;
    boost::shared_ptr<FdsCountersMgr> cntrsMgr_;
    fds_threadpoolPtr threadpool_;
    boost::shared_ptr<SvcMgr> svcMgr_;
};

using SvcMgrModuleProviderPtr = boost::shared_ptr<SvcMgrModuleProvider>;

}  // namespace fds

#endif   // SOURCE_INCLUDE_SVCMGR_MODULE_PROVIDER_H_
