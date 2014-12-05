/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <string>
#include <fdsn-server.h>
#include <util/fds_stat.h>
#include <am-platform.h>
#include <net/net-service.h>
#include <AccessMgr.h>

#include <google/profiler.h>
#include <fds_timer.h>

namespace fds {

class StartProfiler : public FdsTimerTask
{
    public:
    explicit StartProfiler(std::string fname, int duration, FdsTimer &fds_timer, std::mutex& m) :
                                                                FdsTimerTask(fds_timer),
                                                                _fname(fname),
                                                                _mutex(m),
                                                                _id(0),
                                                                _duration(duration) {}
    virtual void runTimerTask() {
        std::string fname = _fname + "." + std::to_string(_id);
        _mutex.lock();
        ProfilerStart(fname.c_str());
        _id++;
        _mutex.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(_duration * 1000));
        _mutex.lock();
        ProfilerStop();
        _mutex.unlock();
    }
    std::string _fname;
    std::mutex &_mutex;
    int _id;
    int _duration;
};

class AMMain : public PlatformProcess
{
  public:
    virtual ~AMMain() {}
    AMMain(int argc, char **argv,
               Platform *platform, Module **mod_vec)
        : PlatformProcess(argc, argv, "fds.am.", "am.log", platform, mod_vec) {}

    void proc_pre_startup() override
    {
        int    argc;
        char **argv;

        PlatformProcess::proc_pre_startup();

        argv = mod_vectors_->mod_argv(&argc);
        am = AccessMgr::unique_ptr(new AccessMgr("AMMain AM Module",
                                                 this));
        proc_add_module(am.get());
        Module *lckstp[] = { am.get(), NULL };
        proc_assign_locksteps(lckstp);

        // FdsConfigAccessor conf = get_conf_helper();

        // profilerTimerTask.reset(new StartProfiler(
        //             conf.get<std::string>("testing.google_profiler_filename"),
        //             conf.get<int>("testing.google_profiler_duration"),
        //             timer,
        //             profiler_mutex));
    }
    int run() override {
        // FdsConfigAccessor conf = get_conf_helper();
        // if (conf.get<bool>("testing.google_profiler_enable"))
        //     timer.scheduleRepeated(
        //         profilerTimerTask,
        //         std::chrono::milliseconds(
        //             conf.get<int>("testing.google_profiler_period") * 1000));
        am->run();
        return 0;
    }

  private:
    AccessMgr::unique_ptr am;

    // Profiler
    FdsTimer timer;
    std::mutex profiler_mutex;
    boost::shared_ptr<FdsTimerTask> profilerTimerTask;
};

}  // namespace fds

int main(int argc, char **argv)
{
    // FdsTimer timer;
    // std::mutex profiler_mutex;
    // boost::shared_ptr<FdsTimerTask> start(new StartProfiler(timer, profiler_mutex));
    // timer.scheduleRepeated(start, std::chrono::milliseconds(30000));

    fds::Module *am_mod_vec[] = {
        &fds::gl_fds_stat,
        &fds::gl_AmPlatform,
        &fds::gl_NetService,
        nullptr
    };

    fds::AMMain amMain(argc, argv, &fds::gl_AmPlatform, am_mod_vec);
    return amMain.main();
}
