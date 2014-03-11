/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <stdlib.h>
#include <string>
#include <iostream>
#include <boost/program_options.hpp>
#include <boost/program_options/parsers.hpp>
#include <fds_assert.h>
#include <fds_module.h>
#include <fds_process.h>

namespace fds {

Module::Module(char const *const name)
    : mod_lstp_cnt(0), mod_intern_cnt(0), mod_lockstep(nullptr), mod_intern(nullptr),
      mod_name(name), mod_params(nullptr) {}

Module::~Module() {}

// \Module::mod_init
// -----------------
//
int
Module::mod_init(SysParams const *const param)
{
    int    i;
    // Module *intern;

    mod_params = param;
    if (mod_intern != nullptr) {
        for (i = 0; mod_intern[i] != nullptr; i++) {
            mod_intern_cnt++;
            mod_intern[i]->mod_init(param);
        }
    }
    return 0;
}

// \Module::mod_startup
// --------------------
//
void
Module::mod_startup()
{
    int    i;
    // Module *intern;

    if (mod_intern != nullptr) {
        for (i = 0; mod_intern[i] != nullptr; i++) {
            mod_intern[i]->mod_startup();
        }
        fds_verify(i == mod_intern_cnt);
    }
}

// \Module::mod_lockstep_startup
// -----------------------------
//
void
Module::mod_lockstep_startup()
{
    int    i;
    // Module *intern;

    if (mod_intern != nullptr) {
        for (i = 0; mod_intern[i] != nullptr; i++) {
            mod_intern[i]->mod_lockstep_startup();
        }
        fds_verify(i == mod_intern_cnt);
    }
    // Block & wait for all dependent lockstep modules to finish.
    mod_lockstep_sync();
}

// \Module::mod_enable_service
// ---------------------------
//
void
Module::mod_enable_service()
{
    int    i;
    // Module *intern;

    if (mod_intern != nullptr) {
        for (i = 0; mod_intern[i] != nullptr; i++) {
            mod_intern[i]->mod_enable_service();
        }
        fds_verify(i == mod_intern_cnt);
    }
}

// \Module::mod_disable_service
// ----------------------------
//
void
Module::mod_disable_service()
{
    int    i;
    // Module *intern;

    if (mod_intern != nullptr) {
        fds_verify(mod_intern_cnt != 0);
        for (i = mod_intern_cnt - 1; i >= 0; i--) {
            fds_verify(mod_intern[i] != nullptr);
            mod_intern[i]->mod_disable_service();
        }
    }
}

// \Module::mod_lockstep_shutdown
// ------------------------------
//
void
Module::mod_lockstep_shutdown()
{
    int    i;
    // Module *intern;

    if (mod_intern != nullptr) {
        fds_verify(mod_intern_cnt != 0);
        for (i = mod_intern_cnt - 1; i >= 0; i--) {
            fds_verify(mod_intern[i] != nullptr);
            mod_intern[i]->mod_lockstep_shutdown();
        }
    }
    // Block & wait for all dependent lockstep modules to finish.
    mod_lockstep_sync();
}

// \Module::mod_shutdown
// ---------------------
//
void
Module::mod_shutdown()
{
    int    i;
    // Module *intern;

    if (mod_intern != nullptr) {
        fds_verify(mod_intern_cnt != 0);
        for (i = mod_intern_cnt - 1; i >= 0; i--) {
            fds_verify(mod_intern[i] != nullptr);
            mod_intern[i]->mod_shutdown();
        }
    }
}

// \Module::mod_lockstep_sync
// --------------------------
//
void
Module::mod_lockstep_sync()
{
}

// ----------------------------------------------------------------------------

// \ModuleVector
// -------------
//
ModuleVector::ModuleVector(int argc, char **argv, Module **mods)
    : sys_mod_cnt(0), sys_argc(argc), sys_argv(argv), sys_mods(nullptr)
{
    sys_mods = mods;
    if (sys_mods != NULL) {
        for (sys_mod_cnt = 0; mods[sys_mod_cnt] != nullptr; sys_mod_cnt++) {
            /*
             * Do some check for each module?
             */
        }
    }
    mod_mk_sysparams();
}

ModuleVector::~ModuleVector() {}

// \mod_timer_fn
// -------------
//
void
ModuleVector::mod_timer_fn()
{
}

// \ModuleVector::mod_mk_sysparams
// -------------------------------
//
void
ModuleVector::mod_mk_sysparams()
{
    namespace po = boost::program_options;

    int                     thr_cnt, hdd_cnt, ssd_cnt,
                            hdd_cap, ssd_cap, log_severity;
    fds_uint32_t            service_port;
    po::variables_map       vm;
    po::options_description desc("Formation Data Systems Command Line Options");

    desc.add_options()
            ("help,h", "Show this help text")
            ("fds-root", po::value<std::string>()->default_value("/fds"),
             "Set the storage root directory")
            ("hdd-root", po::value<std::string>()->default_value("hdd"),
             "Set the hdd storage root directory relative to root")
            ("ssd-root", po::value<std::string>()->default_value("ssd"),
             "Set the ssd storage root directory relative to root")
            ("threads", po::value<int>(&thr_cnt)->default_value(10),
             "Number of threads in system thread pool")
            ("sim-prefix", po::value<std::string>()->default_value("sd"),
             "Prefix names for disk devices simulation")
            ("hdd-count", po::value<int>(&hdd_cnt)->default_value(12),
             "Number of HDD disks")
            ("hdd-capacity", po::value<int>(&hdd_cap)->default_value(100),
             "HDD capacity in MB")
            ("ssd-count", po::value<int>(&ssd_cnt)->default_value(2),
             "Number of SSD disks")
            ("ssd-capacity", po::value<int>(&ssd_cap)->default_value(10),
             "SSD capacity in MB")
            ("log-severity", po::value<int>(&log_severity)->default_value(2),
             "Severity logging level")
            ("port",
             po::value<fds_uint32_t>(&service_port)->default_value(6900),
             "Service recieve port");

    // Save a copy (or clone?) in case individual module needs it.
    sys_params.p_argc = sys_argc;
    sys_params.p_argv = sys_argv;
    po::store(po::command_line_parser(sys_argc, sys_argv).
              options(desc).allow_unregistered().run(), vm);
    po::notify(vm);
    sys_params.fds_root  = vm["fds-root"].as<std::string>();
    sys_params.fds_root += '/';

    sys_params.hdd_root += vm["hdd-root"].as<std::string>();
    sys_params.hdd_root += '/';

    sys_params.ssd_root += vm["ssd-root"].as<std::string>();
    sys_params.ssd_root += '/';

    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return;
    }
    if (vm.count("sim-prefix")) {
        SimEnvParams *sim  =
                new SimEnvParams(vm["sim-prefix"].as<std::string>());
        sys_params.fds_sim = sim;
        sim->sim_hdd_mb    = hdd_cap;
        sim->sim_ssd_mb    = ssd_cap;
    }
    sys_params.sys_hdd_cnt = hdd_cnt;
    sys_params.sys_ssd_cnt = ssd_cnt;
    sys_params.log_severity = log_severity;
    sys_params.service_port = service_port;

    // Make the FDS root directory.
    FdsRootDir::fds_mkdir(sys_params.fds_root.c_str());
}

// \ModuleVector::mod_execute
// --------------------------
//
void
ModuleVector::mod_execute()
{
    int     i, bailout;
    Module *mod;

    if (sys_mod_cnt == 0) {
        return;
    }
    fds_verify(sys_mods != nullptr);

    bailout = 0;
    for (i = 0; i < sys_mod_cnt; i++) {
        mod = sys_mods[i];
        fds_verify(mod != nullptr);
        bailout += mod->mod_init(&sys_params);
    }
    if (bailout != 0) {
        exit(1);
    }
    for (i = 0; i < sys_mod_cnt; i++) {
        mod = sys_mods[i];
        mod->mod_startup();
    }
    for (i = 0; i < sys_mod_cnt; i++) {
        mod = sys_mods[i];
        mod->mod_lockstep_startup();
    }
    for (i = 0; i < sys_mod_cnt; i++) {
        mod = sys_mods[i];
        mod->mod_enable_service();
    }
}

// \ModuleVector::mod_shutdown
// ---------------------------
//
void
ModuleVector::mod_shutdown()
{
    int    i;
    Module *mod;

    if (sys_mod_cnt > 0) {
        for (i = sys_mod_cnt - 1; i >= 0; i--) {
            mod = sys_mods[i];
            fds_verify(mod != nullptr);
            mod->mod_disable_service();
        }
        for (i = sys_mod_cnt - 1; i >= 0; i--) {
            mod = sys_mods[i];
            mod->mod_lockstep_shutdown();
        }
        for (i = sys_mod_cnt - 1; i >= 0; i--) {
            mod = sys_mods[i];
            mod->mod_shutdown();
        }
    }
}

}  // namespace fds
