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
        ("volume-id,i", po::value<int>(&pol_vol_id)->default_value(0),
            "policy-argument: volume id to apply the policy")
        ("domain-id,k", po::value<int>(&pol_domain_id)->default_value(0),
            "policy-argument: domain id to apply the policy")
        ("rel-prio,r", po::value<int>(&pol_rel_priority),
            "policy-arugment: relative priority")
        ("tier-schedule", po::value<std::string>(&pol_tier_sched),
            "policy-argument: set tier schedule [now|end]")
        ("tier",
            po::value<std::string>(&pol_tier_media_arg)->default_value("flash"),
            "policy-argument: set volume tier [flash|disk]")
        ("tier-pct", po::value<int>(&pol_tier_pct)->default_value(100),
            "policy-argument: set storage percentage using the tier")
        ("tier-prefetch,A",
            po::value<std::string>(&pol_tier_algo)->default_value("MRU"),
            "policy-argument: set tier prefetch algorithm [mru|random|arc]");

    po::store(po::command_line_parser(param->p_argc, param->p_argv).
            options(desc).allow_unregistered().run(), vm);
    po::notify(vm);

    if (vm.count("help")) {
        cout << desc << endl;
        return true;
    }
    int tier_opt = 0;
    pol_tier_media = opi::TIER_MEDIA_DRAM;
    if (vm.count("tier")) {
        if (pol_tier_media_arg == "flash") {
            pol_tier_media = opi::TIER_MEDIA_SSD;
        } else {
            pol_tier_media = opi::TIER_MEDIA_HDD;
        }
        tier_opt++;
    }
    if (vm.count("tier-prefetch")) {
        if (pol_tier_algo == "mru") {
            pol_tier_prefetch = opi::PREFETCH_MRU;
        } else if (pol_tier_algo == "random") {
            pol_tier_prefetch = opi::PREFETCH_RAND;
        } else {
            pol_tier_prefetch = opi::PREFETCH_ARC;
        }
        tier_opt++;
    }
    if (vm.count("tier-schedule")) {
        struct opi::tier_pol_time_unit req;

        memset(&req, 0, sizeof(req));
        if ((pol_vol_id == 0) && (pol_domain_id == 0)) {
            cout << desc << endl;
            cout << "Need volume id or domain id to apply tier schedule" << endl;
            return true;
        }
        if (pol_tier_sched == "now") {
            req.tier_period.ts_sec = TIER_SCHED_ACTIVATE;
        } else if (pol_tier_sched == "end") {
            req.tier_period.ts_sec = TIER_SCHED_DEACTIVATE;
        } else {
            cout << "Valid option is 'now' or 'end' after --tier-schedule" << endl;
            return true;
        }
        if ((pol_tier_pct < 0) || (pol_tier_pct > 100)) {
            cout << "Media tier percentage must be between 0-100%" << endl;
            return true;
        }
        if ((pol_tier_media != opi::TIER_MEDIA_SSD) &&
            (pol_tier_media != opi::TIER_MEDIA_HDD)) {
            cout << "Valid tier is 'flash' or 'disk' after --tier" << endl;
            return true;
        }
        cout << "Vol id " << pol_vol_id << ", pct " << pol_tier_pct << endl;
        cout << "Schedule " << pol_tier_sched << endl;
        cout << "Media " << pol_tier_media_arg << endl;
        cout << "Send to OM tier schedule" << endl;

        req.tier_vol_uuid      = pol_vol_id;
        req.tier_media         = pol_tier_media;
        req.tier_media_pct     = pol_tier_pct;
        req.tier_prefetch      = pol_tier_prefetch;

        cli_client->clnt_setTierPolicy(req);
    }
    return true;
}

} // namespace fds
