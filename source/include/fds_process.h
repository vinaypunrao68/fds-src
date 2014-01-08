/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#ifndef _FDS_PROCESS_H_
#define _FDS_PROCESS_H_

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
    FdsProcess(const std::string &config_path);

    virtual void setup(int argc, char *argv[], fds::Module **mod_vec);
    virtual void run() = 0;
    virtual void interrupt_cb(int signum);

protected:
    // static members/methods
    static void sigint_handler(int);
    static FdsProcess *fds_process_;

protected:
    virtual void setup_sig_handler();
    virtual void setup_mod_vector(fds::Module **mod_vec);

    boost::shared_ptr<FdsConfig> config_;
    // todo: Following should be there
    // logger
};

}  // namespace fds

#endif
