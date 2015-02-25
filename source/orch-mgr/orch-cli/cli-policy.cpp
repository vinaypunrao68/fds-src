/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <cli-policy.h>
#include <string>
#include <iostream>
#include <boost/program_options.hpp>
#include <boost/program_options/parsers.hpp>
#include <fdsCli.h>
#include <fds_assert.h>

namespace fds {

OrchCliModule gl_OMCli("OM CLI");

// OrchCliModule
// -------------
//
OrchCliModule::OrchCliModule(char const *const name)
    : Module(name), vol_policy()
{
}

// \mod_init
// ---------
//
int
OrchCliModule::mod_init(SysParams const *const param)
{
    Module::mod_init(param);
    return 0;
}

// \mod_startup
// ------------
//
void
OrchCliModule::mod_startup()
{
}

// \mod_shutdown
// -------------
//
void
OrchCliModule::mod_shutdown()
{
}

// \mod_run
// --------
//
void
OrchCliModule::mod_run()
{
    vol_policy.cli_init_connection();
    vol_policy.cli_exec_cmdline(mod_params);
}

// \cli_init_connection
// --------------------
//
int
CliComponent::cli_init_connection()
{
    return 0;
}

// \cli_init_ice_connection
// ------------------------
// TODO(thrift)
int
CliComponent::cli_init_thrift_connection(int om_port)
{
    std::string        om_ip;
    FdsConfigAccessor  config(g_fdsprocess->get_conf_helper());

    config.set_base_path("");
    if (om_port == 0) {
        om_port = config.get<int>("fds.om.PortNumber");
    }
    om_ip = config.get<std::string>("fds.om.IPAddress");

    // cli_client = new Thrift_VolPolicyClnt(om_ip, om_port);
    return 0;
}

// \cli_exec_cmdline
// -----------------
//
bool
VolPolicyCLI::cli_exec_cmdline(SysParams const *const param)
{
    namespace po = boost::program_options;

    po::variables_map       vm;
    po::options_description desc("Volume Policy Command Line Options");
    desc.add_options()
        ("help,h", "Show volume policy options")
        ("om_port", po::value<int>(&pol_orch_port)->default_value(0),
            "OM config port")
        ("policy-create", po::value<std::string>(&pol_name),
            "Create policy <policy-name> <policy-arguments>")
        ("policy-delete", po::value<std::string>(&pol_name),
            "Delete policy <policy-name> -p <policy-id>")
        ("policy-modify", po::value<std::string>(&pol_name),
            "Modify policy <policy-name> <policy-arguments>")
        ("policy-show", po::value<std::string>(&pol_name),
            "Show current policy")
        ("volume-policy,p", po::value<int>(&pol_id),
            "policy-argument: policy-id")
        ("iops-min,g", po::value<double>(&pol_min_iops),
            "policy-argument: min volume iops")
        ("iops-max,m", po::value<double>(&pol_max_iops),
            "policy-argument: max volume iops")
        ("throttle-level,t", po::value<float>(&pol_throttle_level),
            "policy-argument: throttle traffic level 0-100")
        ("volume-id,i", po::value<int>(&pol_vol_id)->default_value(0),
            "policy-argument: volume id to apply the policy")
        ("domain-id,k", po::value<int>(&pol_domain_id)->default_value(0),
            "policy-argument: domain id to apply the policy")
        ("rel-prio,r", po::value<int>(&pol_rel_priority),
         "policy-arugment: relative priority");

    po::store(po::command_line_parser(param->p_argc, param->p_argv).
            options(desc).allow_unregistered().run(), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return true;
    }
    // JUST DELETE THIS FILE
    return true;
}

}  // namespace fds
