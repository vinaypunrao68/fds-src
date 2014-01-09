/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_FDS_MODULE_H_
#define SOURCE_INCLUDE_FDS_MODULE_H_

#include <fds_types.h>

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
    explicit SimEnvParams(const std::string &prefix) :
    sim_disk_prefix(prefix) {
    }
    ~SimEnvParams() {}

    std::string              sim_disk_prefix;
    int                      sim_hdd_mb;
    int                      sim_ssd_mb;
};

/**
 * Class that maintain system variables for a
 * process.
 */
class SysParams
{
  public:
    SysParams() : fds_sim(nullptr), fds_utp(nullptr) {}
    ~SysParams() {}

    int             sys_num_thr;
    int             sys_hdd_cnt;   /**< Number of HDD devices */
    int             sys_ssd_cnt;   /**< Number of SSD devices */
    int             log_severity;  /**< Severity level for logger */
    fds_uint32_t    service_port;  /**< Port for service to listen */
    fds_uint32_t    control_port;  /**< Port for control to listen */
    fds_uint32_t    config_port;   /**< Port to connect for config */
    std::string     fds_root;      /**< Root directory for FDS data */
    std::string     hdd_root;      /**< Root directory for HDD devices */
    std::string     ssd_root;      /**< Root directory for SSD devices */
    SimEnvParams    *fds_sim;
    UnitTestParams  *fds_utp;

    int             p_argc;
    char            **p_argv;
};

class ModuleVector;

class Module
{
  public:
    explicit Module(char const *const name);
    virtual ~Module();

    // Define standard sequence of bring up and shutdown a module and
    // its services.
    //
    virtual int  mod_init(SysParams const *const param);
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
    SysParams const *        mod_params;
};

class ModuleVector
{
  public:
    ModuleVector(int argc, char **argv, Module **mods);
    ~ModuleVector();

    virtual void mod_timer_fn();

    void mod_execute();
    void mod_shutdown();

    SysParams* get_sys_params() {return &sys_params;}
    static void mod_mkdir(char const *const path);

  private:
    virtual void mod_mk_sysparams();

    int                      sys_mod_cnt;
    int                      sys_argc;
    char                     **sys_argv;
    Module                   **sys_mods;
    SysParams                sys_params;
};

}  // namespace fds

#endif  // SOURCE_INCLUDE_FDS_MODULE_H_
