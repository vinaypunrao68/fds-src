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
#include <fds_module_provider.h>
#include <fds_globals.h>
#include <fds_module.h>
#include <fds_config.hpp>
#include <graphite_client.h>
#include <util/Log.h>
#include <concurrency/ThreadPool.h>

namespace fds {

/* Forward declarations */
class FlagsMap;
class FdsCountersMgr;

/* These are exposed to make it easy to access them */
extern FdsProcess* g_fdsprocess;
extern fds_log* g_fdslog;
// TODO(Rao): Remove this global.  Other than g_fdslog, we shouldn't have any globals
extern boost::shared_ptr<FdsCountersMgr> g_cntrs_mgr;
extern fds_log* GetLog();

/* Helper functions to init process globals. Only invoke these if you
 * aren't deriving from fds_process
 */
void init_process_globals(const std::string &log_name);
void init_process_globals(fds_log *log);
void destroy_process_globals();

/**
 * Generic process class.  It provides the following capabilities
 * 1. Signal handling.  Can be overridden.
 * 2. Configuration from file and command line
 * 3. Module vector based initialization
 * 4. Main function to co-ordinate the init, setup and run methods.
 */
class FdsProcess : public boost::noncopyable,
                   public HasLogger,
                   virtual public CommonModuleProviderIf {
 public:
    FdsProcess();
    FdsProcess(int argc, char *argv[],
               const std::string &def_cfg_file,
               const std::string &base_path, Module **mod_vec);
    FdsProcess(int argc, char *argv[],
               const std::string &def_cfg_file,
               const std::string &base_path,
               const std::string &def_log_file,  Module **mod_vec);

    /* Constructs FdsProcess object */
    /**
     * @param argc
     * @param argv
     * @param def_cfg_file - default configuration file path
     * @param base_path - base path to stanza to describing the process config
     * @param def_log_file - default log file path
     * @param mod_vec - module vectors for the process.
     */
    virtual void init(int argc, char *argv[],
                      const std::string &def_cfg_file,
                      const std::string &base_path,
                      const std::string &def_log_file,  Module **mod_vec);

    /**
     * Add dynamic module created during proc_pre_startup() call.  It's not correct to
     * make this call outside the proc_pre_startup() call.
     */
    inline void proc_add_module(Module *mod)
    {
        mod_vectors_->mod_append(mod);
    }

    /**
     * Assign modules so that their lockstep methods will be called in the lockstep order.
     * Note that a lockstep method may be async, which requires the owner to call the
     * completion callback.
     *
     * Example: proc_assign_locksteps(3, &A, &B, &C, NULL).
     *   A : platform library to establish domain connectivity.
     *   B : plugin OM's workflow.
     *   C : old OMClient.
     */
    inline void proc_assign_locksteps(Module **mods)
    {
        mod_vectors_->mod_assign_locksteps(mods);
    }

    virtual ~FdsProcess();

    /**
     * Override this method to provide your setup.
     * By default module vector based startup sequence is performed here.
     * When you override make sure to invoke base class setup to ensure
     * module vector is executed.
     */
    virtual void proc_pre_startup();
    virtual void proc_pre_service();

    /**
     * Main processing loop goes here
     */
    virtual int run() = 0;

    /**
     * main method to coordinate the init of vector modules in various steps, setup
     * and run.  Return the value from the run() method.
     * Sequence in main:
     *    For each module, call mod_init().
     *    Call proc_pre_startup()
     *    Call start_modules()
     *    Call run()
     *    Call shutdown_modules()
     */
    virtual int main();
    /**
     *    For each module, call mod_startup().
     *    For each module, call mod_lockstep()
     *    Call proc_pre_service()
     *    For each module, call mod_enable_service()
     */
    virtual void start_modules();
    /**
     *    For each module, call mod_stop_services()
     *    For each module, call mod_shutdown_locksteps()
     *    For each module, call shutdown().
     */
    virtual void shutdown_modules();

    /**
     * Handler function for Ctrl+c like signals.  Default implementation
     * just calls exit(0).
     * @param signum
     */
    virtual void interrupt_cb(int signum);

    /* Moduel provider overrides */
    /**
     * Returns the global fds config helper object
     */
    virtual FdsConfigAccessor get_conf_helper() const override;

    /**
     * Returns config object
     * @return
     */
    virtual boost::shared_ptr<FdsConfig> get_fds_config() const override;

    /**
    * @brief Returns process wide timer
    *
    * @return
    */
    virtual FdsTimerPtr getTimer() const override;

    /**
     * Return global counter manager
     * @return
     */
    virtual boost::shared_ptr<FdsCountersMgr> get_cntrs_mgr() const override;

    /**
     * Return the fds root directory obj.
     */
    virtual const FdsRootDir *proc_fdsroot() const  override
    {
        return proc_root;
    }

    virtual fds_threadpool *proc_thrpool() const override
    {
        return proc_thrp;
    }

    virtual util::Properties* getProperties() override;

    /**
    * @brief Deamonize the process if '--foreground' arg isn't specified
    *
    * @param argc
    * @param argv[]
    */
    static void checkAndDaemonize(int argc, char *argv[]);

    /**
    * @brief Daemonize the process
    */
    static void daemonize();

    /**
    * @brief Close all fds.  Redirect standards streams to /dev/null
    */
    static void closeAllFDs();

    std::string getProcId() const {
       return proc_id;
    }

 protected:
    // static members/methods
    static void* sig_handler(void* param);
    static void atExitHandler();

 protected:
    virtual void setup_config(int argc, char *argv[],
               const std::string &default_config_path,
               const std::string &base_path);
    virtual void log_config(libconfig::Setting& root);

    virtual void setup_sig_handler();
    virtual void setup_cntrs_mgr(const std::string &mgr_id);
    virtual void setup_timer_service();
    virtual void setup_graphite();
    virtual void setupAtExitHandler();

    /* Signal handler thread */
    std::unique_ptr<pthread_t> sig_tid_;

    /* Process wide config accessor */
    FdsConfigAccessor conf_helper_;

    util::Properties properties;

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

    /* Flag to indicate whether modulue shutdown has been invoked or not */
    std::once_flag mod_shutdown_invoked_;

    /* FdsRootDir globals. */
    FdsRootDir   *proc_root;

    /* Process wide thread pool. */
    fds_threadpool *proc_thrp;

    /* Name of proc */
    std::string proc_id;
};

}  // namespace fds

#endif  // SOURCE_INCLUDE_FDS_PROCESS_H_
