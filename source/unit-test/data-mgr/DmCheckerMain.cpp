/* Copyright 2015 Formation Data Systems, Inc.
 */
#include <DmChecker.h>

int main(int argc, char* argv[]) {
    fds::DMOfflineCheckerEnv env(argc, argv);
    fds::DMChecker checker(&env);
    auto ret = checker.run();
    return ret == 0 ? 0 : -1;
}
