/* Copyright 2015 Formation Data Systems, Inc.
 */
#include <net/PlatNetSvcHandler.h>
#include <checker/DmChecker.h>

int main(int argc, char* argv[]) {
    fds::DMOfflineCheckerEnv env(argc, argv);
    env.main();
    fds::DMChecker checker(&env);
    auto ret = checker.run();
    std::cout << "Total mismaches found: " << ret << std::endl;
    return ret == 0 ? 0 : -1;
}
