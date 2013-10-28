/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <cli-policy.h>
#include <string>
#include <iostream>
#include <boost/program_options.hpp>
#include <boost/program_options/parsers.hpp>
#include <fdsCli.h>

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
    return cli_init_ice_connection();
}

// \cli_init_ice_connection
// ------------------------
//
int
CliComponent::cli_init_ice_connection()
{
    int                   om_port;
    std::string           om_ip;
    std::ostringstream    serv;
    Ice::CommunicatorPtr  comm = FdsCli::communicator();
    Ice::PropertiesPtr    props = comm->getProperties();

    om_port = props->getPropertyAsInt("OrchMgr.ConfigPort");
    om_ip   = props->getProperty("OrchMgr.IPAddress");
    serv << ORCH_MGR_POLICY_ID << ": tcp -h " << om_ip << " -p " << om_port;

    cli_client = new Ice_VolPolicyClnt(comm, serv.str());
    return 0;
}

// \cli_exec_cmdline
// -----------------
//
bool
VolPolicyCLI::cli_exec_cmdline(SysParams const *const param)
{
    using namespace std;
    namespace po = boost::program_options;

    po::variables_map       vm;
    po::options_description desc("Volume Policy Command Line Options");
    desc.add_options()
        ("help,h", "Show volume policy options")
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
        ("rel-prio,r", po::value<int>(&pol_rel_priority),
            "policy-arugment: relative priority")
        ("tier-schedule", po::value<std::string>()->default_value("now"),
            "policy-argument: set tier schedule")
        ("tier", po::value<std::string>()->default_value("flash"),
            "policy-argument: set volume tier")
        ("tier-pct", po::value<int>(&pol_tier_pct)->default_value(100),
            "policy-argument: set storage percentage using the tier")
        ("tier-prefetch,A", po::value<std::string>()->default_value("MRU"),
            "policy-argument: set tier prefetch algorithm [mru|random|arc]");

    po::store(po::command_line_parser(param->p_argc, param->p_argv).
            options(desc).allow_unregistered().run(), vm);
    po::notify(vm);

    if (vm.count("help")) {
        cout << desc << endl;
        return true;
    }
    int tier_opt = 0;
    if (vm.count("tier")) {
        std::string tier = vm["tier"].as<std::string>();
        if (tier == std::string("flash")) {
            pol_tier_media = opi::TIER_MEDIA_SSD;
        } else {
            pol_tier_media = opi::TIER_MEDIA_HDD;
        }
        tier_opt++;
    }
    if (vm.count("tier-prefetch")) {
        std::string prefetch = vm["tier-prefetch"].as<std::string>();
        if (prefetch == std::string("mru")) {
            pol_tier_prefetch = opi::PREFETCH_MRU;
        } else if (prefetch == std::string("random")) {
            pol_tier_prefetch = opi::PREFETCH_RAND;
        } else {
            pol_tier_prefetch = opi::PREFETCH_ARC;
        }
        tier_opt++;
    }
    if (vm.count("tier-schedule")) {
        struct opi::tier_pol_time_unit req;

        std::cout << "Send to OM tier schedule" << endl;
        cli_client->clnt_setTierPolicy(req);
        /*
        cli_proxy = opi::orch_PolicyReqPrx::checkedCast(proxy);
        cli_proxy->applyTierPolicy(req);
        */
    }
    return true;
}

} // namespace fds
