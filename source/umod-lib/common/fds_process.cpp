/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <stdlib.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string>
#include <iostream>
#include <fiu-local.h>
#include <fds_assert.h>
#include <fds_process.h>
#include <net/net_utils.h>
#include <util/process.h>  // For print_stacktrace().
#include <util/stringutils.h>
#include <unistd.h>
#include <syslog.h>
#include <execinfo.h>
#include <chrono>

#include <sys/time.h>
#include <sys/resource.h>
extern const char * versRev;
extern const char * versDate;
extern const char * machineArch;
extern const char * buildStrTmpl;
namespace fds {

/*
 * These thread synchronization components coordinate activity between
 * the ctor and dtor in their management of the signal handler thread.
 * Without them, we may experience "thread sleep delima" (a thread misses
 * its wakeup call and ends up sleeping forever) when an FdsProcess instance
 * is destroyed soon after it is created (as happens with our gtest unit tests).
 */
bool FdsProcess::sig_tid_ready = false;
std::condition_variable FdsProcess::sig_tid_start_cv_;
std::mutex FdsProcess::sig_tid_start_mutex_;
std::condition_variable FdsProcess::sig_tid_stop_cv_;
std::mutex FdsProcess::sig_tid_stop_mutex_;

/*
 * This array is a simple way to define which signals are of interest
 * and how they should be handled. For those designated with handler
 * function SIG_DFL, there is likely some special handling required such
 * as block them to be waited for by the signal handling thread or some other
 * special treatment. At any rate, we need them here so that we can
 * properly label them. In all other cases, the signal may be caught
 * by any thread.
 */
typedef struct
{
    int             signal;
    std::string     signame;
    sighandler_t    function;
} fds_signal_handler_t;

static fds_signal_handler_t fds_signal_table[] =
{
/*  signal ID           signal name     handler function
    -------------       -------------   ----------------    */
    {SIGABRT,           "SIGABRT",      FdsProcess::fds_catch_signal},
    {SIGBUS,            "SIGBUS",       FdsProcess::fds_catch_signal},
    {SIGCHLD,           "SIGCHLD",      SIG_DFL}, // Seems to be used by gtest in one way and PM in another.
    {SIGFPE,            "SIGFPE",       FdsProcess::fds_catch_signal},
    {SIGHUP,            "SIGHUP",       SIG_IGN},
    {SIGILL,            "SIGILL",       FdsProcess::fds_catch_signal},
    {SIGINT,            "SIGINT",       FdsProcess::fds_catch_signal},
    {SIGPROF,           "SIGPROF",      FdsProcess::fds_catch_signal},
    {SIGSEGV,           "SIGSEGV",      FdsProcess::fds_catch_signal},
    {SIGSYS,            "SIGSYS",       FdsProcess::fds_catch_signal},
    {SIGTERM,           "SIGTERM",      SIG_DFL}, // Handeled by signal handling thread.
    {SIGTRAP,           "SIGTRAP",      FdsProcess::fds_catch_signal},
    {SIGUSR1,           "SIGUSR1",      SIG_IGN},
    {SIGUSR2,           "SIGUSR2",      SIG_IGN},
    {SIGXCPU,           "SIGXCPU",      FdsProcess::fds_catch_signal},
    {SIGXFSZ,           "SIGXFSZ",      FdsProcess::fds_catch_signal},
};

/* Processwide globals from fds_process.h */
// TODO(Rao): Ideally we shouldn't have globals (g_fdslog maybe an exception).  As we slowly
// migrate towards not having globals, these should go away.

FdsProcess *g_fdsprocess                    = NULL;
fds_log *g_fdslog                           = NULL;
boost::shared_ptr<FdsCountersMgr> g_cntrs_mgr;
const FdsRootDir                 *g_fdsroot;

void init_process_globals(const std::string &log_name)
{
    fds_log *temp_log = new fds_log(log_name, "logs");
    init_process_globals(temp_log);
}

void init_process_globals(fds_log *log)
{
    fds_verify(g_fdslog == nullptr);
    g_fdslog = log;
    g_cntrs_mgr.reset(new FdsCountersMgr(net::get_my_hostname()+".unknown"));
}

void destroy_process_globals() {
    if (g_fdslog) {
        delete g_fdslog;
        g_fdslog = nullptr;
    }
}

CommonModuleProviderIf* getModuleProvider() {
    return g_fdsprocess;
}

/**
* @brief Constructor.  Keep it as bare shell.  Do the initialization work
* in init()
*/
FdsProcess::FdsProcess()
{
    mod_vectors_ = nullptr;
    proc_root = nullptr;
    proc_thrp = nullptr;
}

/**
* @brief Constructor.  Keep it as bare shell.  Do the initialization work
* in init()
*
* @param argc
* @param argv[]
* @param def_cfg_file
* @param base_path
* @param mod_vec
*/
FdsProcess::FdsProcess(int argc, char *argv[],
                       const std::string &def_cfg_file,
                       const std::string &base_path, Module **mod_vec)
    : FdsProcess(argc, argv, def_cfg_file, base_path, "", mod_vec)
{
}

/**
* @brief Constructor.  Keep it as bare shell.  Do the initialization work
* in init()
*
* @param argc
* @param argv[]
* @param def_cfg_file
* @param base_path
* @param def_log_file
* @param mod_vec
*/
FdsProcess::FdsProcess(int argc, char *argv[],
                       const std::string &def_cfg_file,
                       const std::string &base_path,
                       const std::string &def_log_file, fds::Module **mod_vec)
{
    init(argc, argv, def_cfg_file, base_path, def_log_file, mod_vec);
}

void FdsProcess::init(int argc, char *argv[],
                      const std::string &def_cfg_file,
                      const std::string &base_path,
                      const std::string &def_log_file, fds::Module **mod_vec)
{
    std::string  fdsroot, cfgfile;

    fds_verify(g_fdsprocess == NULL);

    proc_thrp    = NULL;
    proc_id = argv[0];
    
    /* Initialize process wide globals */
    g_fdsprocess = this;
    /* Set up the signal handler.  We should do this before creating any threads */
    setup_sig_handler();

    /* setup atexit handler */
    setupAtExitHandler();

    /* Setup module vectors and config */
    mod_vectors_ = new ModuleVector(argc, argv, mod_vec);
    fdsroot      = mod_vectors_->get_sys_params()->fds_root;
    proc_root    = new FdsRootDir(fdsroot);

    if (def_cfg_file != "") {
        cfgfile = proc_root->dir_fds_etc() + def_cfg_file;
        /* Check if config file exists */
        struct stat buf;
        if (stat(cfgfile.c_str(), &buf) == -1) {
        	// LOGGER isn't ready at this point yet.
        	std::cerr << "Configuration file " << cfgfile << " not found. Exiting."
        			<< std::endl;
        	exit(ERR_DISK_READ_FAILED);
        }

        setup_config(argc, argv, cfgfile, base_path);

        /* We should be able to ID ourself now. And we need to do this
         * before we use macro SERVICE_NAME_FROM_ID and before we create
         * the logger (which uses that macro to ID the log).
         */
        if (conf_helper_.exists("id")) {
            proc_id = conf_helper_.get<std::string>("id");
        }

        syslog(LOG_NOTICE, "FDS service %s started.",
               SERVICE_NAME_FROM_ID((g_fdsprocess != nullptr) ? g_fdsprocess->getProcId().c_str() : "unknown"));

        /*
         * Create a global logger.  Logger is created here because we need the file
         * name from config
         */
        if (def_log_file == "") {
            g_fdslog = new fds_log(proc_root->dir_fds_logs() +
                                   conf_helper_.get<std::string>("logfile"),
                                   proc_root->dir_fds_logs());
        } else {
            g_fdslog = new fds_log(proc_root->dir_fds_logs() + def_log_file,
                                   proc_root->dir_fds_logs());
        }

        /* Process wide counters setup */
        setup_cntrs_mgr(net::get_my_hostname() + "."  + proc_id);
        properties.set("hostname", net::get_my_hostname());
        properties.set("build.version",util::strformat("%s-%s",versDate,versRev));
        properties.set("build.os", machineArch);
#ifdef DEBUG
        properties.set("build.mode", "debug");
#else
        properties.set("build.mode", "release");
#endif
        // get the build date
        auto pos = strstr(buildStrTmpl,"Date:");
        if (pos) {
            auto stpos = strstr(pos,"<");
            auto endpos = strstr(pos,">");
            if (stpos && endpos) {
                properties.set("build.date", std::string(stpos+1, endpos-stpos-1));
            }
        }

        /* Process wide fault injection */
        fiu_init(0);

        /* Process wide timer service */
        setup_timer_service();

        /* if graphite is enabled, setup graphite task to dump counters */
        if (conf_helper_.get<bool>("enable_graphite",false)) {
            /* NOTE: Timer service will be setup as well */
            setup_graphite();
        }
        /* If threadpool option is specified, create one. */
        if (conf_helper_.exists("threadpool")) {
            int num_thr = conf_helper_.get<int>("threadpool.num_threads", 10);
            bool use_lfthreadpool = conf_helper_.get<bool>("threadpool.use_lftp");
            proc_thrp   = new fds_threadpool("ProcessThreadpool", num_thr, use_lfthreadpool);
            proc_thrp->enableThreadpoolCheck(timer_servicePtr_.get(),
                                             std::chrono::seconds(30));
        }
    } else {
        g_fdslog  = new fds_log(def_log_file, proc_root->dir_fds_logs());
    }
    /* Set the logger level */
    g_fdslog->setSeverityFilter(
        fds_log::getLevelFromName(conf_helper_.get<std::string>("log_severity","NORMAL")));

    /* Adding a timer task to periodically flush all buffered log data to files */
    timer_servicePtr_->scheduledFunctionRepeated(std::chrono::seconds(10),
                                                 []() { g_fdslog->flush(); });

    const libconfig::Setting& fdsSettings = conf_helper_.get_fds_config()->getConfig().getRoot();
    LOGNORMAL << "Configurations as modified by the command line:";
    log_config(fdsSettings);

    /* detect the core file size limit and print to log */
    struct rlimit crlim;
    int ret = getrlimit(RLIMIT_CORE, &crlim);
    if (-1 == ret) {
        LOGERROR << "getrlimit(RLIMIT_CORE) failed with errno " << errno;
    } else {
        LOGNOTIFY << "current rlimit(RLIMIT_CORE): soft limit " << crlim.rlim_cur
                  << ", hard limit " << crlim.rlim_max;
        if ((crlim.rlim_cur != RLIM_INFINITY) ||
            (crlim.rlim_max != RLIM_INFINITY)) {

            struct rlimit newcrlim;
            newcrlim.rlim_cur = RLIM_INFINITY;
            newcrlim.rlim_max = RLIM_INFINITY;
            int ret = setrlimit(RLIMIT_CORE, &newcrlim);
            if (-1 == ret) {
                LOGERROR << "setrlimit(RLIMIT_CORE) failed with errno " << errno;
            } else {
                LOGNOTIFY << "rlimit(RLIMIT_CORE) is set to infinity";
            }
        }
    }

}

FdsProcess::~FdsProcess()
{
    /* Kill timer service */
    if (timer_servicePtr_) {
        timer_servicePtr_->destroy();
    }

    /* Terminate signal handling thread */
    {
        std::unique_lock<std::mutex> lock(sig_tid_stop_mutex_);
        sig_tid_stop_cv_.wait(lock, []{return FdsProcess::sig_tid_ready;}); // Make sure it's ready to be terminated.
        lock.unlock();

        if (sig_tid_) {
            int rc = pthread_kill(*sig_tid_, SIGTERM);
            if (0 != rc) {
                LOGERROR << "thread kill returned error:" << rc;
            } else {
                rc = pthread_join(*sig_tid_, NULL);
                if ( 0 != rc) LOGERROR << "thread join returned error : " << rc;
            }
        }
    }

    /* cleanup process wide globals */
    g_fdsprocess = nullptr;

    if (proc_thrp) delete proc_thrp;
    if (proc_root) delete proc_root;
    if (mod_vectors_) delete mod_vectors_;
}

void FdsProcess::proc_pre_startup() {}
void FdsProcess::proc_pre_service() {}

/**
 * The main method to coordinate everything in proper sequence.
 */
int FdsProcess::main()
{
    int ret;

    start_modules();
    /* Run the main loop. */
    ret = run();

    std::call_once(mod_shutdown_invoked_,
                    &FdsProcess::shutdown_modules,
                    this);

    return ret;
}

void FdsProcess::stop() {
    std::call_once(mod_shutdown_invoked_,
                   &FdsProcess::shutdown_modules,
                   this);
}

/**
* @brief Runs mod_init() and mod_startup() on all the modules.
*/
void FdsProcess::start_modules() {
    mod_vectors_->mod_init_modules();

    /* The process should have all objects allocated in proper place. */
    proc_pre_startup();

    /* The false flags runs module appended by the pre_startup() call. */
    mod_vectors_->mod_init_modules(false);

    /* Do FDS process startup sequence. */
    mod_vectors_->mod_startup_modules();
    mod_vectors_->mod_startup_modules(false);

    /*  Star to run the main process. */
    proc_pre_service();
    mod_vectors_->mod_start_services();
    mod_vectors_->mod_start_services(false);

    mod_vectors_->mod_run_locksteps();
}

/**
* @brief Runs shutdown() on all the modules
*/
void FdsProcess::shutdown_modules() {
    shutdownGate_.open();
    /* Do FDS shutdown sequence. */
    mod_vectors_->mod_stop_services();
    mod_vectors_->mod_shutdown_locksteps();
    mod_vectors_->mod_shutdown();
}

void FdsProcess::setup_config(int argc, char *argv[],
                       const std::string &default_config_file,
                       const std::string &base_path)
{
    boost::shared_ptr<FdsConfig> config(new FdsConfig(default_config_file,
                                                      argc, argv));
    conf_helper_.init(config, base_path);
}

void FdsProcess::log_config(const libconfig::Setting& root)
{
    if (root.isGroup()) {
        for (auto i = 0; i < root.getLength(); ++i) {
            log_config(root[i]);
        }
    } else {
        if (root.isAggregate()) {
            LOGWARN << "Unexpected aggregate in configuration: " << root.getPath();
        } else {
            const libconfig::Config& config = conf_helper_.get_fds_config()->getConfig();
            if (root.getType() == libconfig::Setting::TypeString) {
                LOGNORMAL << root.getPath() << " = " << "\"" << config.lookup(root.getPath()).c_str() << "\"";
            } else if (root.getType() == libconfig::Setting::TypeBoolean) {
                bool value = config.lookup(root.getPath());
                LOGNORMAL << root.getPath() << " = " << value;
            } else if (root.getType() == libconfig::Setting::TypeFloat) {
                float value = config.lookup(root.getPath());
                LOGNORMAL << root.getPath() << " = " << value;
            } else if ((root.getType() == libconfig::Setting::TypeInt) ||
                       (root.getType() == libconfig::Setting::TypeInt64)) {
                int64_t value = conf_helper_.get_abs<int64_t>(root.getPath());
                LOGNORMAL << root.getPath() << " = " << value;
            } else {
                LOGWARN << "Unexpected value type.";
            }
        }
    }
}

FdsConfigAccessor FdsProcess::get_conf_helper() const {
    return conf_helper_;
}

boost::shared_ptr<FdsConfig> FdsProcess::get_fds_config() const
{
    return conf_helper_.get_fds_config();
}

boost::shared_ptr<FdsCountersMgr> FdsProcess::get_cntrs_mgr() const {
    return cntrs_mgrPtr_;
}

FdsTimerPtr FdsProcess::getTimer() const
{
    return timer_servicePtr_;
}

void
FdsProcess::fds_catch_signal(int sig) {
    static int timesCalledBefore = -1; // Protects against recursion. Probably should be synchronized.

    /*
     * Unblock the signal that just occurred.  Otherwise, we can't
     * handle recursive signals.
     */
    sigset_t ctrl_c_sigs;
    sigemptyset(&ctrl_c_sigs);
    sigaddset(&ctrl_c_sigs, sig);
    int rc = pthread_sigmask(SIG_UNBLOCK, &ctrl_c_sigs, 0);
    fds_assert(rc == 0);

    /*
     * Let recursive signals fall through to the default handler,
     * and let our call to abort fall through and create a core file.
     */
    if (signal(sig, SIG_DFL) == SIG_ERR) {
        fds_assert("Unable to install signal handler.");
    }
    if (signal(SIGABRT, SIG_DFL) == SIG_ERR) {
        fds_assert("Unable to install signal handler.");
    }

    const char* signalNotificationTmplt = "FDS service %s caught signal ";
    char signalNotification[64];
    snprintf(signalNotification, sizeof(signalNotification), signalNotificationTmplt,
             SERVICE_NAME_FROM_ID((g_fdsprocess != nullptr) ? g_fdsprocess->getProcId().c_str() : "unknown"));

    /* Find our signal for reporting. */
    std::string sigName(": unknown");
    for (auto sigp : fds_signal_table)
    {
        if (sigp.signal == sig)
        {
            sigName = ": " + sigp.signame;
            break;
        }
    }

    GLOGNOTIFY << signalNotification << sig << sigName;
    g_fdslog->flush();
    syslog(LOG_NOTICE, "%s%d %s", signalNotification, sig, sigName.c_str());

    /*
    * If we receive a SIGTERM or SIGINT, then gracefully shutdown
    * the server process.
    */
    if ((sig == SIGTERM) || (sig == SIGINT)) {
        const char* normalSignOffTmplt = "FDS service %s exiting normally.";
        char normalSignOff[64];
        snprintf(normalSignOff, sizeof(normalSignOff), normalSignOffTmplt,
                 SERVICE_NAME_FROM_ID((g_fdsprocess != nullptr) ? g_fdsprocess->getProcId().c_str() : "unknown"));

        if (g_fdsprocess) {
            g_fdsprocess->interrupt_cb(sig);
        }

        GLOGNOTIFY << normalSignOff;
        g_fdslog->flush();
        syslog(LOG_NOTICE, "%s", normalSignOff);

        exit(EXIT_SUCCESS);
    }

    const char* abnormalSignOffTmplt = "FDS service %s exiting abnormally.";
    char abnormalSignOff[64];
    snprintf(abnormalSignOff, sizeof(abnormalSignOff), abnormalSignOffTmplt,
             SERVICE_NAME_FROM_ID((g_fdsprocess != nullptr) ? g_fdsprocess->getProcId().c_str() : "unknown"));

    GLOGERROR << abnormalSignOff;
    g_fdslog->flush();
    syslog(LOG_ALERT, "%s", abnormalSignOff);

    /*
     * Protect against recursion.  Since the most likely cause is
     * seg faults due to the interrupt_cb() or the code to dump
     * the stack, the first time we recurse, we skip this code.
     * If the problem is in another place, we will recurse again.
     * The second time we recurse we quickly call abort.
     */
    ++timesCalledBefore;
    if (timesCalledBefore >= 2) {
        abort();
    }

    if (timesCalledBefore == 0) {
        util::print_stacktrace();
    }

    g_fdslog->flush();

    abort(); // Produce a core file.
    exit(EXIT_FAILURE); // For completness (helps searches), but not reached.
}

/*
 * Handler for signals blocked on all other threads.
 *
 * param should point to a sigset_t of blocked signals
 */
void*
FdsProcess::sig_handler(void* param)
{
    sigset_t blocked_sigs;
    sigemptyset(&blocked_sigs);

    sigorset(&blocked_sigs, &blocked_sigs, static_cast<sigset_t*>(param));

    /**
     * We're about as "started" as we can be before we must let folks know.
     */
    {
        /**
         * Grabbing the lock here ensures that the thread that started us
         * (whom we expect to have grabbed this lock before starting us)
         * is in fact waiting for us to notify it to continue before we
         * actually send the notification.
         */
        std::lock_guard<std::mutex> lock(sig_tid_start_mutex_);
        FdsProcess::sig_tid_start_cv_.notify_all();
    }

    while (true)
    {
        int signum = 0;
        int rc = sigwait(&blocked_sigs, &signum);
        if ((rc == 0) || (rc == EINTR))
        {
            /*
             * RE: EINTR: Probably caught some unblocked signal.
             */
            fds_catch_signal(signum);
            continue;
        } else {
            fds_assert("sigwait failed");
        }
    }
    return (nullptr);
}

/**
 * We will block SIGTERM in all threads and listen for it in our
 * signal handling thread. All other signals will be handled
 * by the thread that receives them.
 */
void FdsProcess::setup_sig_handler()
{
    sigset_t ctrl_c_sigs;
    sigemptyset(&ctrl_c_sigs);

    /*
     * NOTE: We won't block SIGHUP and SIGINT because when we do
     * sigwait() on these signals with the process running under gdb, they are
     * delivered to the process instead of gdb.
     * See https://bugzilla.kernel.org/show_bug.cgi?id=9039
     */
    sigaddset(&ctrl_c_sigs, SIGTERM);
    int rc = pthread_sigmask(SIG_BLOCK, &ctrl_c_sigs, 0);
    fds_assert(rc == 0);

    // Joinable thread for handling signals blocked by this and other threads.
    {
        std::unique_lock<std::mutex> lock(sig_tid_start_mutex_);

        sig_tid_.reset(new pthread_t);
        rc = pthread_create(sig_tid_.get(), 0, FdsProcess::sig_handler, static_cast<void *>(&(ctrl_c_sigs)));
        fds_assert(rc == 0);

        /**
         * Block until the sig handler thread is ready.
         */
        sig_tid_start_cv_.wait(lock);
        lock.unlock();
    }

    /**
     * Let the dtor know it can proceed.
     *
     * Normally, the dtor is not running at the same time as this,
     * the ctor. But it can happen in contrived cases such as unit
     * testing. Without some coordination in that case, the dtor
     * will try to cleanup the signal handling thread before it
     * is started.
     */
    {
        std::lock_guard<std::mutex> lock(FdsProcess::sig_tid_stop_mutex_);
        FdsProcess::sig_tid_ready = true;
        FdsProcess::sig_tid_stop_cv_.notify_all();
    }

    /**
     * For each signal of interest, install the handler.
     * However, if the current handler is not the default,
     * then restore the current handler. We run into problems,
     * for example, when the OM c++ library is driven by the JVM
     * which uses certain signals (notably SIGSEGV) for its
     * own purposes.
     *
     * TODO(Greg): I thought the non-default signal handler restoration described
     * above would take care of the JVM issues, but apparently not. Unless
     * and until someone is able to come up with a solution for having
     * the OM library manage its signals without interferring with the
     * JVM, we'll leave OM signal handling alone.
     *
     * Depending upon where we are in initializing and how the OM is started,
     * the corresponding "process ID" may take on one of several forms.
     */
    if ((g_fdsprocess->getProcId() != "om") &&
        (g_fdsprocess->getProcId().find("orchMgr") == std::string::npos) &&
        (g_fdsprocess->getProcId().find("java") == std::string::npos)) {
        __sighandler_t currentHandler;
        for (auto sigp : fds_signal_table) {
            if (sigp.signal == SIGCHLD) {
                /**
                 * We won't mess with SIGCHLD. While PM relies upon it
                 * to notice stopped child processes such as DM, SM, and AM,
                 * apparently gtest uses it in some fashion as well.
                 */
                continue;
            } else {
                if ((currentHandler = signal(sigp.signal, sigp.function)) == SIG_ERR) {
                    fds_assert("Unable to install signal handler.");
                } else if (currentHandler != SIG_DFL) {
                    if (signal(sigp.signal, currentHandler) == SIG_ERR) {
                        fds_assert("Failed to restore default signal handler.");
                    } else {
                        // Our logging is not set up yet.
                        char currentHandlerBuf[32];
                        snprintf(currentHandlerBuf, 32, "%p", currentHandler);
                        syslog(LOG_NOTICE, "FDS service %s restored signal handler %s for signal %s.",
                               SERVICE_NAME_FROM_ID(
                                       (g_fdsprocess != nullptr) ? g_fdsprocess->getProcId().c_str() : "unknown"),
                               (currentHandler == SIG_ERR) ? "SIG_ERR" :
                               (currentHandler == SIG_IGN) ? "SIG_IGN" : currentHandlerBuf,
                               sigp.signame.c_str());
                    }
                }
            }
        }
    } else {
        // Our logging is not set up yet.
        syslog(LOG_NOTICE, "FDS service %s left with default signal handling.",
               SERVICE_NAME_FROM_ID((g_fdsprocess != nullptr) ? g_fdsprocess->getProcId().c_str() : "unknown"));
    }
}

/**
 * Called at normal process termination, either via exit(3) or via return from the program's main().
 *
 * Functions registered using atexit() (and on_exit(3)) are not called if a process terminates abnormally
 * because of the delivery of a signal.
 *
 * When a child process is created via fork(2), it inherits its parent's registrations.
 *
 */
void
FdsProcess::atExitHandler()
{
    if (g_fdslog != nullptr) {
        g_fdslog->flush();
    }
}

void
FdsProcess::setupAtExitHandler()
{
    atexit(FdsProcess::atExitHandler);
}

void FdsProcess::setup_cntrs_mgr(const std::string &mgr_id)
{
    fds_verify(cntrs_mgrPtr_.get() == NULL);
    cntrs_mgrPtr_.reset(new FdsCountersMgr(mgr_id));
    g_cntrs_mgr = cntrs_mgrPtr_;
    new ResourceUsageCounter(g_cntrs_mgr->get_default_counters());
}

void FdsProcess::setup_timer_service()
{
    if (!timer_servicePtr_) {
        timer_servicePtr_.reset(new FdsTimer("ProcessWideTimer"));
    }
}

void FdsProcess::setup_graphite()
{
    std::string ip = conf_helper_.get<std::string>("graphite.ip");
    int port = conf_helper_.get<int>("graphite.port");

    graphitePtr_.reset(new GraphiteClient(ip, port,
                                          timer_servicePtr_, cntrs_mgrPtr_));
    int period = conf_helper_.get<int>("graphite.period");
    graphitePtr_->start(period);
    FDS_PLOG(g_fdslog) << "Set up graphite.  ip: " << ip << " port: " << port
        << " period: " << period;
}

void FdsProcess::interrupt_cb(int signum)
{
    std::call_once(mod_shutdown_invoked_,
                    &FdsProcess::shutdown_modules,
                    this);
}

void
FdsProcess::closeAllFDs()
{
    /* close all file descriptors.  however, redirect std file descriptors
     * to /dev/null.
     */
    int devNullFd = open("/dev/null", O_RDWR);
    for (int i = getdtablesize(); i >= 0 ; --i) {
        int ret;
        /* redirect std io to /dev/null */
        if ((i == STDIN_FILENO) || (i == STDOUT_FILENO) || (i == STDERR_FILENO)) {
            /* flush before re-directing file descriptors.  flush all of them same
             * time, because really don't need to convert fileno to filep for
             * stdio (yeah.. laziness).
             */
            fflush(stdin);
            fflush(stdout);
            fflush(stderr);
            ret = dup2(i, devNullFd);
#if 0
            if (-1 == ret) {
                // xxx: Not sure if we can log here as we are closing all fds in this loop.
                // we should log to syslog
                GLOGERROR << "Error on redirecting stdio to /dev/null: errno " << errno;
            }
#endif
        } else {
            ret = close(i);
            /* intentionally ignoring return value. some file descriptor may not be
             * open for closing.  not all entries in the dtable is populated.
             */
        }
    }
}

void FdsProcess::checkAndDaemonize(int argc, char *argv[]) {
    bool makeDaemon = true;
    volatile bool gdb = false;
    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]).find("--foreground") != std::string::npos) {
            makeDaemon = false;
        }
        if (std::string(argv[i]).find("--gdb") != std::string::npos) {
            gdb = true;
        }
    }

    /* When --gdb is set, wait until gdb flag is unset from inside a gdb session */
    while (gdb) {
        sleep(1);
    }

    if (makeDaemon) {
        closeAllFDs();
        daemonize();
    }
}

void FdsProcess::daemonize() {
    // adapted from http://www.enderunix.org/docs/eng/daemon.php

    if (getppid() == 1) {
        /* already a daemon */
        return;
    }

    /* fork a process */
    int childpid = fork();
    if (childpid < 0) {
        /* fork failed */
        printf("error forking for daemonize : %d", errno);
        exit(1);
    }
    if (childpid > 0) {
        /* parent process */
        printf("forked successfully : child pid : %d", childpid);
        exit(0);
    }

    /* child process */

    /* obtain a new process group */
    setsid();

    // umask(027); /* set newly created file permissions */
    // ignore tty signals
    signal(SIGTSTP, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGHUP , SIG_IGN);
}

util::Properties* FdsProcess::getProperties() {
    return &properties;
}

fds_log* HasLogger::GetLog() const {
    // if neither the class logger nor the global logger are initialized,
    // something is wrong with the init sequence.  Make sure logging is
    // properly initialized before attempting to access the log
    fds_verify(logptr != nullptr || g_fdslog != nullptr);

    if (logptr == NULL && g_fdslog) return g_fdslog;

    // Don't default the log initialization.  This could result in problems
    // if not handled properly by callers, but I think that is preferable to
    // returning a default that is wrong.
    return logptr;
}

fds_log* HasLogger::SetLog(fds_log* logptr) const {
    fds_log* oldlogptr = this->logptr;
    this->logptr = logptr;
    return oldlogptr;
}

// get the global logger
fds_log* GetLog() {
    if (g_fdslog) return g_fdslog;
    // this is a fallback.
    // if the process did not explicity init ..
    init_process_globals("fds");
    return g_fdslog;
}

}  // namespace fds
