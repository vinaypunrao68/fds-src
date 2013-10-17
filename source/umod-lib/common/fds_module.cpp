/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <fds_assert.h>
#include <fds_module.h>

namespace fds {

Module::Module(char const *const name)
    : mod_name(name), mod_lstp_cnt(0), mod_lockstep(nullptr),
      mod_intern(nullptr), mod_params(nullptr) {}

Module::~Module() {}

// \Module::mod_init
// -----------------
//
void
Module::mod_init(SysParams const *const param)
{
    int     i;
    Module *intern;

    if (mod_intern != nullptr) {
        for (i = 0; mod_intern[i] != nullptr; i++) {
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
    int     i;
    Module *intern;

    if (mod_intern != nullptr) {
        for (i = 0; mod_intern[i] != nullptr; i++) {
            mod_intern[i]->mod_startup();
        }
    }
}

// \Module::mod_lockstep_startup
// -----------------------------
//
void
Module::mod_lockstep_startup()
{
}

// \Module::mod_enable_service
// ---------------------------
void
Module::mod_enable_service()
{
    int     i;
    Module *intern;

    if (mod_intern != nullptr) {
        for (i = 0; mod_intern[i] != nullptr; i++) {
            mod_intern[i]->mod_enable_service();
        }
    }
}

// \Module::mod_disable_service
// ----------------------------
//
void
Module::mod_disable_service()
{
}

// \Module::mod_lockstep_shutdown
// ------------------------------
//
void
Module::mod_lockstep_shutdown()
{
}

// \Module::mod_shutdown
// ---------------------
//
void
Module::mod_shutdown()
{
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
    for (sys_mod_cnt = 0; mods[sys_mod_cnt] != nullptr; sys_mod_cnt++) {
        ;
    }
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

        mod->mod_init(&sys_params);
    }
    for (i = 0; i < sys_mod_cnt; i++) {
        mod = sys_mods[i];
        mod->mod_startup();
    }
}

// \ModuleVector::mod_shutdown
// ---------------------------
//
void
ModuleVector::mod_shutdown()
{
}

} // namespace fds
