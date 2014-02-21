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
 * Generic process class.  It provides the following capabilities
 * 1. Signal handling.  Can be overridden.
 * 2. Configuration from file and command line
 * 3. Module vector based initialization
 */
class FdsProcess : public boost::noncopyable {
 public:
    /**
     * @param argc
     * @param argv
     * @param default_config_file - default configuration file path
     * @param base_path - base path to stanza to describing the process config
     */
    FdsProcess(int argc, char *argv[],
               const std::string &default_config_file,
               const std::string &base_path);

    virtual ~FdsProcess();

    /**
     * Override this method to provide your setup.
     * By default module vector based startup sequence is performed here.  
     * When you override make sure to invoke base class setup to ensure
     * module vector is executed.
     * @param argc
     * @param argv
     * @param mod_vec
     */
    virtual void setup(int argc, char *argv[],
                       fds::Module **mod_vec);

    /**
     * Main processing code goes here
     */
    virtual void run() = 0;

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
    virtual void setup_mod_vector(int argc, char *argv[],
                                  fds::Module **mod_vec);

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
};

}  // namespace fds

#endif  // SOURCE_INCLUDE_FDS_PROCESS_H_
