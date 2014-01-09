/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <string>

#include <fds_assert.h>
#include <fds_process.h>

namespace fds {
/* static declarations */
FdsProcess* FdsProcess::fds_process_ = NULL;

/* Methods */
FdsProcess::FdsProcess(const std::string &config_path)
: config_(new FdsConfig(config_path))
{
    fds_assert(fds_process_ == NULL);

    fds_process_ = this;
}

FdsProcess::~FdsProcess()
{
    fds_process_ = NULL;

    int rc = pthread_kill(sig_tid_, SIGTERM);
    assert(rc == 0);
    rc = pthread_join(sig_tid_, NULL);
    assert(rc == 0);
}

void FdsProcess::setup(int argc, char *argv[], fds::Module **mod_vec)
{
    setup_sig_handler();

    setup_mod_vector(mod_vec);
}

void*
FdsProcess::sig_handler(void*)
{
    sigset_t ctrl_c_sigs;
    sigemptyset(&ctrl_c_sigs);
    sigaddset(&ctrl_c_sigs, SIGHUP);
    sigaddset(&ctrl_c_sigs, SIGINT);
    sigaddset(&ctrl_c_sigs, SIGTERM);

    while (true)
    {
        int signum = 0;
        int rc = sigwait(&ctrl_c_sigs, &signum);
        if (rc == EINTR)
        {
            /*
             * Some sigwait() implementations incorrectly return EINTR
             * when interrupted by an unblocked caught signal
             */
            continue;
        }
        assert(rc == 0);

        if (FdsProcess::fds_process_) {
            FdsProcess::fds_process_->interrupt_cb(signum);
        } else {
            return NULL;
        }
    }
    return NULL;
}

/**
 * ICE inspired way of handling signals.  This can possibly be
 * replaced by boost::asio::signal_set.  This needs to be investigated
 */
void FdsProcess::setup_sig_handler()
{
    /*
     * We will block ctrl+c like signals in the main thread.  All
     * other threads will have signals blocked as well.  We will
     * create a thread to listen for signals
     */
    sigset_t ctrl_c_sigs;
    sigemptyset(&ctrl_c_sigs);
    sigaddset(&ctrl_c_sigs, SIGHUP);
    sigaddset(&ctrl_c_sigs, SIGINT);
    sigaddset(&ctrl_c_sigs, SIGTERM);

    int rc = pthread_sigmask(SIG_BLOCK, &ctrl_c_sigs, 0);
    fds_assert(rc == 0);

    // Joinable thread
    rc = pthread_create(&sig_tid_, 0, FdsProcess::sig_handler, 0);
    fds_assert(rc == 0);
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
