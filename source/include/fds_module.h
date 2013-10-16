#ifndef INCLUDE_FDS_MOUDLE_H_
#define INCLUDE_FDS_MOUDLE_H_

#include <boost/function.hpp>

namespace fds {

class SysParams
{
};

class ModuleVector;

class Module
{
  public:
    Module(char const *const name);
    ~Module();

    virtual void mod_init(SysParams const *const param);
    virtual void mod_startup() = 0;
    virtual void mod_shutdown() = 0;

    virtual void mod_lockstep_startup();
    virtual void mod_lockstep_shutdown();

  protected:
    friend class ModuleVector;

    // \mod_lockstep_sync
    // ------------------
    // If this module must synchronize with other modules in lock step order
    // during startup/shutdown, the constructor must fill in the mod_lockstep
    // array and size.  This function uses the array to execute _lockstep_
    // methods in this module and others in lockstep order.
    //
    virtual void mod_lockstep_sync();

    int                      mod_lstp_cnt;
    Module                   **mod_lockstep;
    Module                   **mod_intern;
    char const *const        mod_name;
    SysParams const *const   mod_params;
};

class ModuleVector
{
  public:
    ModuleVector(int argc, char **argv);
    ~ModuleVector();

    virtual void mod_timer_fn();
    virtual void mod_register(Module **mods);

    void mod_execute();
    void mod_shutdown();

  private:
    virtual void mod_mk_sysparams();

    int                      sys_mod_cnt;
    int                      sys_argc;
    char                     **sys_argv;
    Module                   **sys_mods;
    SysParams                sys_params;
};

} // namespace fds

#endif /* INCLUDE_FDS_MOUDLE_H_ */
