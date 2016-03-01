#ifndef PROCESSHANDLE_HPP
#define PROCESSHANDLE_HPP

#include <thread>
#include <vector>
#include <util/stringutils.h>

namespace fds {
/**
* @brief Handle to control test processes
*
* @tparam T
*/
template <class T>
struct ProcessHandle
{
    std::thread         *thread{nullptr};
    T                   *proc{nullptr};
    std::vector<std::string>            args;
    bool    started;

    ProcessHandle() : started(false) {}
    ProcessHandle(const std::vector<std::string> &args)
    {
        start(args);
    }
    ProcessHandle(const std::string &processName,
                  const std::string &fds_root,
                  int64_t platformUuid,
                  int32_t platformPort)
    {
        start(processName, fds_root, platformUuid, platformPort);
    }
    ~ProcessHandle()
    {
        stop();
    }
    void start(const std::string &processName,
              const std::string &fds_root,
              int64_t platformUuid,
              int32_t platformPort)
    {
        args.push_back(processName);
        args.push_back(util::strformat("--fds-root=%s", fds_root.c_str()));
        args.push_back(util::strformat("--fds.pm.platform_uuid=%ld", platformUuid));
        args.push_back(util::strformat("--fds.pm.platform_port=%d", platformPort));
        start();
        started = true;
    }
    void start(const std::vector<std::string> &args)
    {
        this->args = args;
        start();
        started = true;
    }
    void start()
    {
        fds_assert(args.size() > 0);
        proc = new T((int) args.size(), (char**) &(args[0]), true);
        thread = new std::thread([&]() { proc->main(); });
        proc->getReadyWaiter().await();
        started = true;
    }
    void stop()
    {
        if (proc) {
            proc->stop();
            delete proc;
            proc = nullptr;
        }
        if (thread) {
            thread->join();
            delete thread;
            thread = nullptr;
        }
        started = false;
    }
    bool isRunning() {
        return started;
    }
};
}  // namespace fds

#endif          // PROCESSHANDLE_HPP
