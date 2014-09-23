/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_FDS_MODULE_H_
#define SOURCE_INCLUDE_FDS_MODULE_H_

#include <string>
#include <fds_types.h>

namespace fds {

/**
 * Abstract generic callback obj.
 */
class FdsCallback
{
  public:
    FdsCallback() {}
    virtual ~FdsCallback() {}
    virtual bool obj_callback() = 0;
};

/**
 * Class that maintain system variables for a
 * process.
 */
class SysParams
{
  public:
    SysParams() {}
    ~SysParams() {}

    std::string     fds_root;      /**< Root directory for FDS data */
    int             p_argc;
    char            **p_argv;
};

class ModuleVector;
const int MOD_ST_NULL       = 0x00000000;
const int MOD_ST_INIT       = 0x00000001;
const int MOD_ST_STARTUP    = 0x00000002;
const int MOD_ST_LOCKSTEP   = 0x04000000;      /**< rsvd most bits for lockstep */
const int MOD_ST_FUNCTIONAL = 0x40000000;
const int MOD_ST_SHUTDOWN   = 0x80000000;

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

    virtual const char* getName();

    // Utility to cat two module vectors.  The caller must free the result.
    //
    static Module **mod_cat(Module **v1, Module **v2);

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
    fds_uint32_t             mod_exec_state;
    Module                   **mod_lockstep;
    Module                   **mod_intern;
    char const *const        mod_name;
    SysParams const *        mod_params;
};

class ModuleVector
{
  public:
    ModuleVector(int argc, char **argv, Module **mods);
    virtual ~ModuleVector();

    virtual void mod_timer_fn();

    /**
     * Sequence of steps to bringup modules.  Module depdendencies can be detected
     * through branches of vectors.  Module vector branches can be look this:
     * +----------+----------+----------+-----+------+
     * | Module A | Module B | Module C | ... | NULL |
     * +----------+----------+----------+-----+------+
     *     |           |           |
     *     |           |           +------------------+
     *     |           +------+                       |
     *     |                  |                       |
     *     V                  V                       V
     *   +----+---+-----+   +----+---+----+-----+   +----+----+-----+
     *   | A1 | B | ... |   | A1 | A | B1 | ... |   | A1 | B1 | ... |
     *   +----+---+-----+   +----+---+----+-----+   +----+----+-----+
     */
    void mod_init_modules();
    void mod_startup_modules();
    void mod_run_locksteps();
    void mod_start_services();

    /**
     * Wrapper to bringup everything in proper order.  Only used when the caller
     * doesn't have FdsProcess in the main function.
     */
    void mod_execute();

    void mod_stop_services();
    void mod_shutdown_locksteps();
    void mod_shutdown();

    inline SysParams *get_sys_params() {
        return &sys_params;
    }
    inline char **mod_argv(int *argc) {
        *argc = sys_argc;
        return sys_argv;
    }

  private:
    virtual void mod_mk_sysparams();

    int                      sys_mod_cnt;
    int                      sys_argc;
    char                     **sys_argv;
    Module                   **sys_mods;
    SysParams                sys_params;
};

/**
 * FDS root directory tree structure, absolute path including fds-root
 */
class FdsRootDir
{
  public:
    explicit FdsRootDir(const std::string &root);
    virtual ~FdsRootDir() {}

    inline const std::string &dir_fdsroot() const { return d_fdsroot; }
    inline const std::string &dir_fds_etc() const { return d_root_etc; }
    inline const std::string &dir_fds_logs() const { return d_var_logs; }
    inline const std::string &dir_fds_var_stats() const { return d_var_stats; }
    inline const std::string &dir_fds_var_inventory() const { return d_var_inventory; }
    inline const std::string &dir_fds_var_cores() const { return d_var_cores; }
    inline const std::string &dir_fds_var_tests() const { return d_var_tests; }
    inline const std::string &dir_fds_var_tools() const { return d_var_tools; }
    inline const std::string &dir_dev() const { return d_dev; }
    inline const std::string &dir_user_repo() const { return d_user_repo; }
    inline const std::string &dir_user_repo_objs() const { return d_user_repo_objs; }
    inline const std::string &dir_user_repo_dm() const { return d_user_repo_dm; }
    inline const std::string &dir_user_repo_stats() const { return d_user_repo_stats; }
    inline const std::string &dir_user_repo_snap() const { return d_user_repo_snap; }
    inline const std::string &dir_sys_repo() const { return d_sys_repo; }
    inline const std::string &dir_sys_repo_etc() const { return d_sys_repo_etc; }
    inline const std::string &dir_sys_repo_domain() const { return d_sys_repo_domain; }
    inline const std::string &dir_sys_repo_volume() const { return d_sys_repo_volume; }
    inline const std::string &dir_sys_repo_inventory() const {
        return d_sys_repo_inventory;
    }
    inline const std::string &dir_fds_repo() const { return d_fds_repo; }

    static void fds_mkdir(char const *const path);

  protected:
    static const int         d_max_length = 256;
    std::string              d_fdsroot;
    std::string              d_root_etc;
    std::string              d_var_logs;
    std::string              d_var_cores;
    std::string              d_var_stats;
    std::string              d_var_inventory;
    std::string              d_var_tests;
    std::string              d_var_tools;
    std::string              d_dev;
    std::string              d_user_repo;
    std::string              d_user_repo_objs;
    std::string              d_user_repo_dm;
    std::string              d_user_repo_stats;
    std::string              d_user_repo_snap;
    std::string              d_sys_repo;
    std::string              d_sys_repo_etc;
    std::string              d_sys_repo_domain;
    std::string              d_sys_repo_volume;
    std::string              d_sys_repo_inventory;
    std::string              d_fds_repo;
};

}  // namespace fds

#endif  // SOURCE_INCLUDE_FDS_MODULE_H_
