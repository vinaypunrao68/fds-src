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

const char* Module::getName() {
    return mod_name;
}

// mod_cat
// -------
//
/* static */ Module **
Module::mod_cat(Module **v1, Module **v2)
{
    int      i, j, cnt;
    Module **ret;

    cnt = 0;
    if (v1 != NULL) {
        for (; v1[cnt] != NULL; cnt++) {}
        cnt++;
    }
    if (v2 != NULL) {
        for (; v2[cnt] != NULL; cnt++) {}
        cnt++;
    }
    if (cnt == 0) {
        return NULL;
    }
    i   = 0;
    ret = new Module * [cnt];
    if (v1 != NULL) {
        for (i = 0; v1[i] != NULL; i++) {
            ret[i] = v1[i];
        }
    }
    if (v2 != NULL) {
        for (j = 0; v2[j] != NULL; j++, i++) {
            ret[i] = v2[j];
        }
    }
    ret[i] = NULL;
    fds_verify(i <= cnt);

    return ret;
}

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
            if ((mod_intern[i]->mod_exec_state & MOD_ST_INIT) == 0) {
                mod_intern[i]->mod_init(param);
                mod_intern[i]->mod_exec_state |= MOD_ST_INIT;
            }
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
            if ((mod_intern[i]->mod_exec_state & MOD_ST_STARTUP) == 0) {
                mod_intern[i]->mod_startup();
                mod_intern[i]->mod_exec_state |= MOD_ST_STARTUP;
            }
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
        return;
    }
    mod_exec_state |= MOD_ST_FUNCTIONAL;
    if (mod_intern != NULL) {
        for (i = 0; mod_intern[i] != NULL; i++) {
            if ((mod_intern[i]->mod_exec_state & MOD_ST_FUNCTIONAL) == 0) {
                mod_intern[i]->mod_enable_service();
                mod_intern[i]->mod_exec_state |= MOD_ST_FUNCTIONAL;
            }
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
    int     i, bailout, retval;
    Module *mod;

    if (sys_mod_cnt == 0) {
        return;
    }
    fds_verify(sys_mods != NULL);

    bailout = 0;
    for (i = 0; i < sys_mod_cnt; i++) {
        mod = sys_mods[i];
        fds_verify(mod != NULL);
        if ((mod->mod_exec_state & MOD_ST_INIT) == 0) {
            bailout += mod->mod_init(&sys_params);
            mod->mod_exec_state |= MOD_ST_INIT;
            GLOGERROR << "module init failed : " << i << " : " << mod->getName();
        }
    }
    if (bailout != 0) {
        fds_panic("module init failed");
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
        if ((mod->mod_exec_state & MOD_ST_STARTUP) == 0) {
            mod->mod_startup();
            mod->mod_exec_state |= MOD_ST_STARTUP;
        }
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
        if ((mod->mod_exec_state & MOD_ST_FUNCTIONAL) == 0) {
            mod->mod_enable_service();
            mod->mod_exec_state |= MOD_ST_FUNCTIONAL;
        }
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

// --------------------------------------------------------------------------------------
// FDS root directory layout
// --------------------------------------------------------------------------------------
FdsRootDir::FdsRootDir(const std::string &root)
    : d_fdsroot(root),
      d_root_etc(root      + std::string("etc/")),
      d_var_logs(root      + std::string("var/logs/")),
      d_var_cores(root     + std::string("var/cores/")),
      d_var_stats(root     + std::string("var/stats/")),
      d_var_inventory(root + std::string("var/inventory/")),
      d_var_tests(root     + std::string("var/tests/")),
      d_var_tools(root     + std::string("var/tools/")),
      d_dev(root           + std::string("dev/")),
      d_user_repo(root     + std::string("user-repo/")),
      d_user_repo_objs(d_user_repo + std::string("objects/")),
      d_user_repo_dm(d_user_repo   + std::string("dm-names/")),
      d_user_repo_stats(d_user_repo   + std::string("vol-stats/")),
      d_user_repo_snap(d_user_repo   + std::string("snap/")),

      d_sys_repo(root      + std::string("sys-repo/")),
      d_sys_repo_etc(d_sys_repo       + std::string("etc/")),
      d_sys_repo_domain(d_sys_repo    + std::string("domain/")),
      d_sys_repo_volume(d_sys_repo    + std::string("volume/")),
      d_sys_repo_inventory(d_sys_repo + std::string("inventory/")),
      d_fds_repo(root      + std::string("fds-repo/")) {}

/*
  * C++ API to create a directory recursively.  Bail out w/out cleaning up when having
 * errors other than EEXIST.
 */
void
FdsRootDir::fds_mkdir(char const *const path)
{
    size_t len;
    char   tmp[d_max_length], *p;

    len = snprintf(tmp, sizeof(tmp), "%s", path);
    if (tmp[len - 1] == '/') {
        tmp[len - 1] = '\0';
    }
    umask(0);
    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            if (mkdir(tmp, S_IRWXU) != 0) {
                if (errno == EACCES) {
                    std::cout << "Don't have permission to create " << path << std::endl;
                    exit(1);
                }
                fds_verify(errno == EEXIST);
            }
            *p = '/';
        }
    }
    mkdir(tmp, S_IRWXU);
}

}  // namespace fds
