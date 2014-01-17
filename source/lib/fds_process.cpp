/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <string>

#include <fds_assert.h>
#include <fds_process.h>
#include <util/Log.h>

namespace fds {

/* Processwide globals from fds_process.h */
FdsProcess *g_fdsprocess = NULL;
fds_log *g_fdslog = NULL;

FdsProcess::FdsProcess(int argc, char *argv[],
                       const std::string &config_path,
                       const std::string &base_path)
{
    fds_verify(g_fdsprocess == NULL);

    /* Initialize process wide globals */
    g_fdsprocess = this;
    /* Set up the signal handler.  We should do this before creating any threads */
    setup_sig_handler();

    /* Setup config */
    setup_config(argc, argv, config_path, base_path);

    /* Create a global logger.  Logger is created here because we need the file
     * name from config
     */
    g_fdslog = new fds_log(conf_helper_.get<std::string>("logfile"));
}

FdsProcess::~FdsProcess()
{
    /* cleanup process wide globals */
    g_fdsprocess = NULL;
    delete g_fdslog;

    /* Terminate signal handling thread */
    int rc = pthread_kill(sig_tid_, SIGTERM);
    fds_assert(rc == 0);
    rc = pthread_join(sig_tid_, NULL);
    fds_assert(rc == 0);
}

void FdsProcess::setup(int argc, char *argv[],
                       fds::Module **mod_vec)
{
    /* Execute module vector */
    setup_mod_vector(argc, argv, mod_vec);
}

void FdsProcess::setup_config(int argc, char *argv[],
                       const std::string &default_config_file,
                       const std::string &base_path)
{
    boost::shared_ptr<FdsConfig> config(new FdsConfig(default_config_file,
                                                      argc, argv));
    conf_helper_.init(config, base_path);
}

void*
FdsProcess::sig_handler(void* param)
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
        fds_assert(rc == 0);

        if (g_fdsprocess) {
            g_fdsprocess->interrupt_cb(signum);
            return NULL;
        } else {
            return reinterpret_cast<void*>(NULL);
        }
    }
    return reinterpret_cast<void*>(NULL);
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

void FdsProcess::setup_mod_vector(int argc, char *argv[], fds::Module **mod_vec)
{
    if (!mod_vec) {
        return;
    }

    fds::ModuleVector mod_vec_obj(argc, argv, mod_vec);
    mod_vec_obj.mod_execute();
}

void FdsProcess::interrupt_cb(int signum)
{
    exit(signum);
}

}  // namespace fds
