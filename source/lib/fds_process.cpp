/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <string>

#include <fds_assert.h>
#include <fds_process.h>

namespace fds {

/* static declarations */
FdsProcess* FdsProcess::fds_process_ = NULL;

void FdsProcess::sigint_handler(int signum)
{
    if (fds_process_) {
        fds_process_->interrupt_cb(signum);
    } else {
        exit(signum);
    }
}

/* Methods */
FdsProcess::FdsProcess(const std::string &config_path)
: config_(new FdsConfig(config_path))
{
    fds_assert(fds_process_ == NULL);

    fds_process_ = this;
}

void FdsProcess::setup(int argc, char *argv[], fds::Module **mod_vec)
{
    setup_mod_vector(mod_vec);

    /* Now we can start processing signals */
    setup_sig_handler();
}

void FdsProcess::setup_sig_handler()
{
    /* setup signal handler */
    if (std::signal(SIGINT, FdsProcess::sigint_handler) == SIG_ERR) {
        throw FdsException("Failed to create signal handler");
    }
}

void FdsProcess::setup_mod_vector(fds::Module **mod_vec)
{
    // todo: implement this
}

void FdsProcess::interrupt_cb(int signum)
{
    exit(signum);
}
}  // namespace fds
