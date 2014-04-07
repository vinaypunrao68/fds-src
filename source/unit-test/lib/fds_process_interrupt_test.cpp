#include <iostream>
#include <thread>         // std::this_thread::sleep_for
#include <chrono>         // std::chrono::seconds
#include <string>
#include <vector>

#include <fds_process.h>

using namespace fds;

/**
 * This test spawns few child threads. After user presses ctrl+c, all the
 * threads should shutdown gracefully
 */
class Sm_process : public FdsProcess {
    public:
    Sm_process(int argc, char *argv[],
               const std::string &config, const std::string &base_path, Module **vec)
    : FdsProcess(argc, argv, config, base_path, vec)
    {
        join_done_ = false;
        done_ = false;
    }

    void child_run(int id) {
        std::cout << "Starting child " << id << std::endl;
        while (!done_) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        std::cout << "Ending child " << id << std::endl;
    }

    virtual int run() {
        for (int i = 0; i < 10; i++) {
            children_.push_back(std::thread(&Sm_process::child_run, this, i));
        }

        while (!join_done_) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        return 0;
    }

    virtual void interrupt_cb(int signum) {
        std::cout << "interrupted" << std::endl;
        done_ = true;
        for (uint i = 0; i < children_.size(); i++) {
            std::cout << "Join finished: " << i <<  std::endl;
            children_[i].join();
        }
        std::cout << "Join finished" << std::endl;
        exit(0);
    }

    private:
    bool done_;
    bool join_done_;
    std::vector<std::thread> children_;
};

int main(int argc, char *argv[]) {
    Sm_process p(argc, argv, "fds.conf", "fds.sm.", NULL);
    p.setup();
    p.run();
    std::cout << "Main finished" << std::endl;
    return 0;
}

