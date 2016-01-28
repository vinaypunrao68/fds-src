/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_FDS_MODULE_H_
#define SOURCE_INCLUDE_FDS_MODULE_H_

#include <string>
#include <boost/thread/condition.hpp>
#include <fds_types.h>
#include <concurrency/Mutex.h>

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
const int MOD_ST_LOCKSTEP   = 0x04000000;
const int MOD_ST_LSTEP_DONE = 0x00100000;
const int MOD_ST_LSTEP_DOWN = 0x00200000;
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
    virtual void mod_enable_service();

    // If the async argument is true, the caller must call mod_lockstep_done() to run
    // the next module's method.
    //
    virtual void mod_lockstep_start_service();
    virtual void mod_lockstep_done();

    virtual void mod_lockstep_shutdown();
    virtual void mod_lockstep_shutdown_done();

    virtual void mod_disable_service();
    virtual void mod_shutdown() = 0;

    virtual const char* getName();

    // Utility to cat two module vectors.  The caller must free the result.
    //
    static Module **mod_cat(Module **v1, Module **v2);

    /**
     * Used for modules derived to have ability to set / query test mode
     */
    inline void setTestMode(fds_bool_t value) {
        LOGDEBUG << "Setting Test mode to " << value;
        testMode = value;
    }

    inline fds_bool_t isInTestMode() {
        return testMode;
    }

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

    int                      mod_lstp_idx;
    int                      mod_intern_cnt;
    fds_uint32_t             mod_exec_state;
    Module                 **mod_intern;
    ModuleVector            *mod_owner;
    char const *const        mod_name;
    SysParams const *        mod_params;
  private:
    fds_bool_t               testMode;
};

class ModuleVector
{
  public:
    /* We shouldn't have more dynamic vector module than this. */
    static const int mod_max_added_vec = 16;

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
    void mod_init_modules(bool predef = true);
    void mod_startup_modules(bool predef = true);
    void mod_start_services(bool predef = true);
    void mod_run_locksteps();

    /**
     * Wrapper to bringup everything in proper order.  Only used when the caller
     * doesn't have FdsProcess in the main function.
     */
    void mod_execute();

    void mod_shutdown_locksteps();
    void mod_stop_services(bool predef = true);
    void mod_shutdown(bool predef = true);

    /**
     * Assign modules to be coordinated in lockstep.
     * Lockstep modules are executed in the order from index 0.  The caller must free
     * the Module vector if needed.  The input array must be terminated by NULL.
     *
     * If this obj already has lock step vector, the input will be appended.  Their
     * lockstep methods are called in the final lockstep order.
     */
    void mod_assign_locksteps(Module **mods);

    /**
     * Append module allocated and not in the static list.  The best place to call
     * this function is in proc_pre_startup() call in fds_process.
     */
    void mod_append(Module *mod);

    inline SysParams *get_sys_params() {
        return &sys_params;
    }
    inline char **mod_argv(int *argc) {
        *argc = sys_argc;
        return sys_argv;
    }

  private:
    friend class Module;

    int                      sys_mod_cnt;
    int                      sys_add_cnt;
    int                      sys_lckstp_cnt;
    int                      sys_argc;
    char                   **sys_argv;
    Module                 **sys_mods;              /* static module vector. */
    Module                 **sys_add_mods;          /* dynamic module vector. */
    Module                 **sys_lckstps;
    SysParams                sys_params;

    boost::condition        *sys_waitq;
    fds_mutex                sys_mtx;

    void     mod_mk_sysparams();
    void     mod_done_lockstep(Module *mod, bool shutdown = false);
    Module **mod_select_vec(bool predef, int *mod_cnt);
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
    inline const std::string &dir_filetransfer() const { return d_filetransfer; }
    inline const std::string &dir_user_repo() const { return d_user_repo; }
    inline const std::string &dir_user_repo_objs() const { return d_user_repo_objs; }
    inline const std::string &dir_user_repo_dm() const { return d_user_repo_dm; }
    inline const std::string &dir_timeline_dm() const { return d_timeline_dm; }
    inline const std::string &dir_user_repo_stats() const { return d_user_repo_stats; }
    inline const std::string &dir_sys_repo() const { return d_sys_repo; }
    inline const std::string &dir_sys_repo_etc() const { return d_sys_repo_etc; }
    inline const std::string &dir_sys_repo_domain() const { return d_sys_repo_domain; }
    inline const std::string &dir_sys_repo_volume() const { return d_sys_repo_volume; }
    inline const std::string &dir_sys_repo_dm() const { return d_sys_repo_dm; }
    inline const std::string &dir_sys_repo_stats() const { return d_sys_repo_stats; }
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
    std::string              d_sys_repo;
    std::string              d_sys_repo_etc;
    std::string              d_sys_repo_domain;
    std::string              d_sys_repo_volume;
    std::string              d_sys_repo_inventory;
    std::string              d_sys_repo_dm;
    std::string              d_timeline_dm;
    std::string              d_sys_repo_stats;
    std::string              d_fds_repo;
    std::string              d_filetransfer;
};

}  // namespace fds

#endif  // SOURCE_INCLUDE_FDS_MODULE_H_
