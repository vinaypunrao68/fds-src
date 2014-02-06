/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#ifndef ORCH_CLI_ORCH_POLICY_H_
#define ORCH_CLI_ORCH_POLICY_H_

#include <boost/program_options.hpp>
#include <fds_module.h>
#include <net-proxies/vol_policy.h>
#include <fdsp/FDSP_ConfigPathReq.h>

namespace fds {
class CliComponent
{
  public:
    CliComponent(char const *const name)
        : cli_comp(name) {}
    ~CliComponent() {}

    virtual int  cli_init_connection();
    virtual bool cli_exec_cmdline(fds::SysParams const *const param) = 0;

    void setCliClient(
        boost::shared_ptr<fdp::FDSP_ConfigPathReqClient> clnt) {
        cli_client = clnt;
    }

  protected:
    char const *const           cli_comp;
    boost::shared_ptr<fdp::FDSP_ConfigPathReqClient> cli_client;

    int cli_init_thrift_connection(int port);
};

class VolPolicyCLI : public virtual CliComponent
{
  public:
    VolPolicyCLI()
            : CliComponent("Volume Policy") {}
    ~VolPolicyCLI() {}

    virtual bool cli_exec_cmdline(SysParams const *const param);

  private:
    double                      pol_vol_capacity;
    double                      pol_min_iops;
    double                      pol_max_iops;
    float                       pol_throttle_level;
    int                         pol_orch_port;
    int                         pol_vol_id;
    int                         pol_domain_id;
    int                         pol_id;
    int                         pol_rel_priority;
    int                         pol_tier_pct;
    std::string                 pol_name;
    std::string                 pol_tier_media_arg;
    std::string                 pol_tier_sched;
    std::string                 pol_tier_domain;
    std::string                 pol_tier_algo;
    fdp::tier_media_type_e      pol_tier_media;
    fdp::tier_prefetch_type_e   pol_tier_prefetch;
};

class OrchCliModule : public Module
{
  public:
    OrchCliModule(char const *const name);
    ~OrchCliModule() {}

    void setCliClient(
        boost::shared_ptr<fdp::FDSP_ConfigPathReqClient> clnt) {
        vol_policy.setCliClient(clnt);
    }

    virtual int  mod_init(SysParams const *const param);
    virtual void mod_startup();
    virtual void mod_shutdown();

    void mod_run();

  private:
    int                         orch_port;
    std::string                 orch_serv;
    VolPolicyCLI                vol_policy;
};

extern OrchCliModule            gl_OMCli;

} // namespace fds

#endif /* ORCH_CLI_ORCH_POLICY_H_ */
