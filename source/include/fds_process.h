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
#include <fds_module.h>
#include <fds_config.hpp>


namespace fds {

/**
 * Generic process class.  It provides the following capabilities
 * 1. Signal handling.  Can be overridden.
 * 2. Module vector based initialization
 */
class FdsProcess : public boost::noncopyable {
 public:
    /**
     *
     * @param config_path - configuration path
     */
    explicit FdsProcess(const std::string &config_path);
    virtual ~FdsProcess();

    /**
     * Override this method to provide your setup.
     * By default sets up signal handler and module vector based
     * startup sequence is performed here.  Signal handling is performed
     * on a separte thread.
     * @param argc
     * @param argv
     * @param mod_vec
     */
    virtual void setup(int argc, char *argv[], fds::Module **mod_vec);

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

 protected:
    // static members/methods
    static void* sig_handler(void* param);
    static FdsProcess *fds_process_;

 protected:
    virtual void setup_sig_handler();
    virtual void setup_mod_vector(fds::Module **mod_vec);

    /* Signal handler thread */
    pthread_t sig_tid_;

    /* Process wide config */
    boost::shared_ptr<FdsConfig> config_;
    // todo: Following should be there
    // logger
};

}  // namespace fds

#endif  // SOURCE_INCLUDE_FDS_PROCESS_H_
