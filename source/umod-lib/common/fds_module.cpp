#include <fds_assert.h>
#include <fds_module.h>

namespace fds {

Module::Module(char const *const name)
    : mod_name(name), mod_lstp_cnt(0), mod_lockstep(nullptr),
      mod_intern(nullptr), mod_params(nullptr) {}

Module::~Module() {}

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

void
Module::mod_lockstep_startup()
{
}

void
Module::mod_lockstep_shutdown()
{
}

void
Module::mod_lockstep_sync()
{
}

// \ModuleVector
// -------------
//
ModuleVector::ModuleVector(int argc, char **argv)
    : sys_mod_cnt(0), sys_argc(argc), sys_argv(argv), sys_mods(nullptr) {}

ModuleVector::~ModuleVector() {}

// \mod_timer_fn
// -------------
//
void
ModuleVector::mod_timer_fn()
{
}

void
ModuleVector::mod_register(Module **mods)
{
    sys_mods = mods;
    for (sys_mod_cnt = 0; mods[sys_mod_cnt] != nullptr; sys_mod_cnt++) {
        ; // no code
    }
}

void
ModuleVector::mod_mk_sysparams()
{
}

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

void
ModuleVector::mod_shutdown()
{
}

} // namespace fds
