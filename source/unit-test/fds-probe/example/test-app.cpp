/*
 * Copyright 2014 by Formations Data System, Inc.
 */
#include <unistd.h>
#include <string>
#include <iostream>
#include <platform/platform-lib.h>

namespace fds {

class TestProg : public FdsService
{
  public:
    TestProg(int argc, char *argv[], const std::string &log, Module **vec)
        : FdsService(argc, argv, log, vec) {}

    int run() override {
        while (1) {
            std::cout << "your code here" << std::endl;
            sleep(2);
        }
        return 0;
    }
};

}  // namespace fds

int
main(int argc, char *argv[])
{
    static fds::Module *mod[] = {
        NULL
    };
    fds::TestProg test(argc, argv, "test.log", mod);
    return test.main();
}

