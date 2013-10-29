#ifndef INCLUDE_FDS_MOUDLE_H_
#define INCLUDE_FDS_MOUDLE_H_

#include <string>
#include <boost/function.hpp>

namespace fds {

class UnitTestParams
{
  public:
};

class SimEnvParams
{
  public:
    SimEnvParams(const std::string &prefix) : sim_disk_prefix(prefix) {}
    ~SimEnvParams() {}

    std::string              sim_disk_prefix;
    int                      sim_hdd_mb;
    int                      sim_ssd_mb;
};

class SysParams
{
  public:
    SysParams() : fds_sim(nullptr), fds_utp(nullptr) {}
    ~SysParams() {}

    int                      sys_num_thr;
    int                      sys_hdd_cnt;
    int                      sys_ssd_cnt;
    std::string              fds_root;
    std::string              hdd_root;
    std::string              ssd_root;
    SimEnvParams             *fds_sim;
    UnitTestParams           *fds_utp;
};

class ModuleVector;

class Module
{
  public:
    Module(char const *const name);
    ~Module();

    // Define standard sequence of bring up and shutdown a module and
    // its services.
    //
    virtual void mod_init(SysParams const *const param);
    virtual void mod_startup() = 0;
    virtual void mod_lockstep_startup();
    virtual void mod_enable_service();

    virtual void mod_disable_service();
    virtual void mod_lockstep_shutdown();
    virtual void mod_shutdown() = 0;

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
    int                      mod_intern_cnt;
    Module                   **mod_lockstep;
    Module                   **mod_intern;
    char const *const        mod_name;
    SysParams const *const   mod_params;
};

class ModuleVector
{
  public:
    ModuleVector(int argc, char **argv, Module **mods);
    ~ModuleVector();

    virtual void mod_timer_fn();

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
