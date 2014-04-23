/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_FDS_PROCESS_H_
#define SOURCE_INCLUDE_FDS_PROCESS_H_

#include <pthread.h>
#include <csignal>

#include <string>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <fds_globals.h>
#include <fds_module.h>
#include <fds_config.hpp>
#include <fds_counters.h>
#include <graphite_client.h>
#include <util/Log.h>
#include <concurrency/ThreadPool.h>

namespace fds {

/* These are exposed to make it easy to access them */
extern FdsProcess* g_fdsprocess;
extern fds_log* g_fdslog;
extern boost::shared_ptr<FdsCountersMgr> g_cntrs_mgr;
extern fds_log* GetLog();

/* Helper functions to init process globals. Only invoke these if you
 * aren't deriving from fds_process 
 */
void init_process_globals(const std::string &log_name);
void init_process_globals(fds_log *log);

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
    std::string              d_sys_repo;
    std::string              d_sys_repo_etc;
    std::string              d_sys_repo_domain;
    std::string              d_sys_repo_volume;
    std::string              d_sys_repo_inventory;
    std::string              d_fds_repo;
};

/**
 * Generic process class.  It provides the following capabilities
 * 1. Signal handling.  Can be overridden.
 * 2. Configuration from file and command line
 * 3. Module vector based initialization
 * 4. Main function to co-ordinate the init, setup and run methods.
 */
class FdsProcess : public boost::noncopyable, public HasLogger {
 public:
    /**
     * @param argc
     * @param argv
     * @param def_cfg_file - default configuration file path
     * @param base_path - base path to stanza to describing the process config
     * @param def_log_file - default log file path
     * @param mod_vec - module vectors for the process.
     */
    FdsProcess(int argc, char *argv[],
               const std::string &def_cfg_file,
               const std::string &base_path,
               const std::string &def_log_file,  Module **mod_vec);
    FdsProcess(int argc, char *argv[],
               const std::string &def_cfg_file,
               const std::string &base_path, Module **mod_vec)
        : FdsProcess(argc, argv, def_cfg_file, base_path, "", mod_vec) {}
    /* Exposed for mock testing */
    FdsProcess() {}

    virtual ~FdsProcess();

    /**
     * Override this method to provide your setup.
     * By default module vector based startup sequence is performed here.  
     * When you override make sure to invoke base class setup to ensure
     * module vector is executed.
     */
    virtual void setup();  /* DO not use, use main() instead, will clean up old calls. */
    virtual void proc_setup();
    virtual void proc_start_run();

    /**
     * Main processing loop goes here
     */
    virtual int run() = 0;

    /**
     * main method to coordinate the init of vector modules in various steps, setup
     * and run.  Return the value from the run() method.
     * Sequence in main:
     *    For each module, call mod_init().
     *    Call proc)setup()
     *    For each module, call mod_startup().
     *    For each module, call mod_lockstep()
     *    Call proc_start_run()
     *    For each module, call mod_enable_service()
     *    Call run()
     *    For each module, call mod_stop_services()
     *    For each module, call mod_shutdown_locksteps()
     *    For each module, call shutdown().
     */
    virtual int main();

    /**
     * Handler function for Ctrl+c like signals.  Default implementation
     * just calls exit(0).
     * @param signum
     */
    virtual void interrupt_cb(int signum);

    /**
     * Returns the global fds config helper object
     */
    FdsConfigAccessor get_conf_helper() const;

    /**
     * Returns config object
     * @return
     */
    boost::shared_ptr<FdsConfig> get_fds_config() const;

    /**
     * Return global counter manager
     * @return
     */
    boost::shared_ptr<FdsCountersMgr> get_cntrs_mgr() const;
    /**
     * Return the fds root directory obj.
     */
    inline const FdsRootDir *proc_fdsroot() const { return proc_root; }
    inline fds_threadpool *proc_thrpool() { return proc_thrp; }

    void daemonize();

 protected:
    // static members/methods
    static void* sig_handler(void* param);

 protected:
    virtual void setup_config(int argc, char *argv[],
               const std::string &default_config_path,
               const std::string &base_path);

    virtual void setup_sig_handler();
    virtual void setup_cntrs_mgr(const std::string &mgr_id);
    virtual void setup_timer_service();
    virtual void setup_graphite();

    /* Signal handler thread */
    pthread_t sig_tid_;

    /* Process wide config accessor */
    FdsConfigAccessor conf_helper_;

    /* Process wide counters manager */
    boost::shared_ptr<FdsCountersMgr> cntrs_mgrPtr_;

    /* Process wide timer service.  Not enabled by default.  It needs
     * to be explicitly enabled
     */
    boost::shared_ptr<FdsTimer> timer_servicePtr_;

    /* Graphite client for exporting stats.  Only enabled based on config and a
     * compiler directive DEV_BUILD (this isn't done yet)
     */
    boost::shared_ptr<GraphiteClient> graphitePtr_;

    /* Module vectors. */
    ModuleVector *mod_vectors_;

    /* FdsRootDir globals. */
    FdsRootDir   *proc_root;

    /* Process wide thread pool. */
    fds_threadpool *proc_thrp;
};

}  // namespace fds

#endif  // SOURCE_INCLUDE_FDS_PROCESS_H_
