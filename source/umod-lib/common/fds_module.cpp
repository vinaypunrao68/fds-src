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
    : mod_lstp_cnt(0), mod_intern_cnt(0), mod_exec_state(MOD_ST_NULL),
      mod_lockstep(NULL), mod_intern(NULL), mod_name(name), mod_params(NULL) {}

Module::~Module() {}

// \Module::mod_init
// -----------------
//
int
Module::mod_init(SysParams const *const param)
{
    int i;

    if (mod_exec_state & MOD_ST_INIT) {
        return 0;
    }
    mod_params = param;
    mod_exec_state |= MOD_ST_INIT;

    if (mod_intern != NULL) {
        for (i = 0; mod_intern[i] != NULL; i++) {
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
    int i;

    if (mod_exec_state & MOD_ST_STARTUP) {
        return;
    }
    mod_exec_state |= MOD_ST_STARTUP;
    if (mod_intern != NULL) {
        for (i = 0; mod_intern[i] != NULL; i++) {
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
    int i;

    if (mod_exec_state & MOD_ST_STARTUP) {
        return;
    }
    mod_exec_state |= MOD_ST_STARTUP;
    if (mod_intern != NULL) {
        for (i = 0; mod_intern[i] != NULL; i++) {
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
    int i;

    if (mod_exec_state & MOD_ST_FUNCTIONAL) {
        return; //TODO: confirm this change
    }
    mod_exec_state |= MOD_ST_FUNCTIONAL;
    if (mod_intern != NULL) {
        for (i = 0; mod_intern[i] != NULL; i++) {
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

    if (mod_exec_state & MOD_ST_SHUTDOWN) {
        return;
    }
    mod_exec_state |= MOD_ST_SHUTDOWN;
    if (mod_intern != NULL) {
        fds_verify(mod_intern_cnt != 0);
        for (i = mod_intern_cnt - 1; i >= 0; i--) {
            fds_verify(mod_intern[i] != NULL);
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

    if (mod_intern != NULL) {
        fds_verify(mod_intern_cnt != 0);
        for (i = mod_intern_cnt - 1; i >= 0; i--) {
            fds_verify(mod_intern[i] != NULL);
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

    if (mod_intern != NULL) {
        fds_verify(mod_intern_cnt != 0);
        for (i = mod_intern_cnt - 1; i >= 0; i--) {
            fds_verify(mod_intern[i] != NULL);
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
// ----------------------------------------------------------------------------
ModuleVector::ModuleVector(int argc, char **argv, Module **mods)
    : sys_mod_cnt(0), sys_argc(argc), sys_argv(argv), sys_mods(NULL)
{
    sys_mods = mods;
    if (sys_mods != NULL) {
        for (sys_mod_cnt = 0; mods[sys_mod_cnt] != NULL; sys_mod_cnt++) {
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
    po::variables_map       vm;
    po::options_description desc("Formation Data Systems Command Line Options");

    desc.add_options()
            ("help,h", "Show this help text")
            ("fds-root", po::value<std::string>()->default_value("/fds"),
             "Set the storage root directory");

    // Save a copy (or clone?) in case individual module needs it.
    sys_params.p_argc = sys_argc;
    sys_params.p_argv = sys_argv;
    po::store(po::command_line_parser(sys_argc, sys_argv).
              options(desc).allow_unregistered().run(), vm);
    po::notify(vm);
    sys_params.fds_root  = vm["fds-root"].as<std::string>();
    sys_params.fds_root += '/';

    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return;
    }
    // Make the FDS root directory.
    FdsRootDir::fds_mkdir(sys_params.fds_root.c_str());
}

// mod_init_modules
// ----------------
//
void
ModuleVector::mod_init_modules()
{
    int     i, bailout;
    Module *mod;

    if (sys_mod_cnt == 0) {
        return;
    }
    fds_verify(sys_mods != NULL);

    bailout = 0;
    for (i = 0; i < sys_mod_cnt; i++) {
        mod = sys_mods[i];
        fds_verify(mod != NULL);
        bailout += mod->mod_init(&sys_params);
    }
    if (bailout != 0) {
        exit(1);
    }
}

// mod_startup_modules
// -------------------
//
void
ModuleVector::mod_startup_modules()
{
    for (int i = 0; i < sys_mod_cnt; i++) {
        Module *mod = sys_mods[i];
        mod->mod_startup();
    }
}

// mod_run_locksteps
// -----------------
//
void
ModuleVector::mod_run_locksteps()
{
    for (int i = 0; i < sys_mod_cnt; i++) {
        Module *mod = sys_mods[i];
        mod->mod_lockstep_startup();
    }
}

// mod_start_services
// ------------------
//
void
ModuleVector::mod_start_services()
{
    for (int i = 0; i < sys_mod_cnt; i++) {
        Module *mod = sys_mods[i];
        mod->mod_enable_service();
    }
}

// mod_stop_services
// -----------------
//
void
ModuleVector::mod_stop_services()
{
    if (sys_mod_cnt == 0) {
        return;
    }
    for (int i = sys_mod_cnt - 1; i >= 0; i--) {
        Module *mod = sys_mods[i];
        fds_verify(mod != NULL);
        mod->mod_disable_service();
    }
}

// mod_shutdown_locksteps
// ----------------------
//
void
ModuleVector::mod_shutdown_locksteps()
{
    if (sys_mod_cnt == 0) {
        return;
    }
    for (int i = sys_mod_cnt - 1; i >= 0; i--) {
        Module *mod = sys_mods[i];
        mod->mod_lockstep_shutdown();
    }
}

// mod_execute
// -----------
// Wrapper to bringup everything in proper order.  Used when the caller doesn't have
// FdsProcess object.
//
void
ModuleVector::mod_execute()
{
    if (sys_mod_cnt == 0) {
        return;
    }
    fds_verify(sys_mods != NULL);

    mod_init_modules();
    mod_startup_modules();
    mod_run_locksteps();
    mod_start_services();
}

// \ModuleVector::mod_shutdown
// ---------------------------
//
void
ModuleVector::mod_shutdown()
{
    if (sys_mod_cnt > 0) {
        for (int i = sys_mod_cnt - 1; i >= 0; i--) {
            Module *mod = sys_mods[i];
            mod->mod_shutdown();
        }
    }
}

}  // namespace fds
