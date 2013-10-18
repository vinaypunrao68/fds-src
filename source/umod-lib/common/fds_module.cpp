/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <stdlib.h>
#include <string>
#include <iostream>
#include <boost/program_options.hpp>
#include <fds_assert.h>
#include <fds_module.h>

namespace fds {

Module::Module(char const *const name)
    : mod_name(name), mod_lstp_cnt(0), mod_lockstep(nullptr),
      mod_intern_cnt(0), mod_intern(nullptr), mod_params(nullptr) {}

Module::~Module() {}

// \Module::mod_init
// -----------------
//
void
Module::mod_init(SysParams const *const param)
{
    int    i;
    Module *intern;

    if (mod_intern != nullptr) {
        for (i = 0; mod_intern[i] != nullptr; i++) {
            mod_intern_cnt++;
            mod_intern[i]->mod_init(param);
        }
    }
}

// \Module::mod_startup
// --------------------
//
void
Module::mod_startup()
{
    int    i;
    Module *intern;

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
    Module *intern;

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
    Module *intern;

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
    Module *intern;

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
    Module *intern;

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
    Module *intern;

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
    : sys_mod_cnt(0), sys_argc(argc), sys_argv(argv), sys_mods(nullptr),
      sys_params(nullptr)
{
    sys_mods = mods;
    for (sys_mod_cnt = 0; mods[sys_mod_cnt] != nullptr; sys_mod_cnt++) {
        ;
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
    using namespace std;
    namespace po = boost::program_options;

    po::variables_map       vm;
    po::options_description desc("Formation Data Systems Command Line Options");
    desc.add_options()
        ("help", "Show this help text")
        ("fds-root", po::value<std::string>()->default_value("/fds"),
            "Set the storage root directory")
        ("fds-sim", po::value<std::string>(),
            "Set FDS simulation/unit test params:\n"
            "\t-t <thread-count> -n <disk-prefix>\n"
            "\t-d <hdd-count> -c <hdd_capacity MB>\n"
            "\t-D <ssd-count> -C <ssd_capacity MB>\n");

    po::store(po::parse_command_line(sys_argc, sys_argv, desc), vm);
    po::notify(vm);
    if (vm.count("help")) {
        cout << endl << desc << endl;
        exit(1);
    }
    sys_params = new SysParams(vm["fds-root"].as<std::string>().c_str());
    if (vm.count("fds-sim")) {
    }
}

// \ModuleVector::mod_execute
// --------------------------
//
void
ModuleVector::mod_execute()
{
    int     i;
    Module *mod;

    fds_verify(sys_mod_cnt > 0);
    fds_verify(sys_mods != nullptr);

    for (i = 0; i < sys_mod_cnt; i++) {
        mod = sys_mods[i];
        fds_verify(mod != nullptr);
        mod->mod_init(sys_params);
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

} // namespace fds
