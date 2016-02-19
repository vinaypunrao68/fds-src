/*
 *  Copyright 2016 Formation Data Systems, Inc.
 */

#include <VolumeChecker.h>

int main(int argc, char **argv) {
    fds::VolumeChecker checker(argc, argv, false);
    checker.run();
    return checker.main();
}
