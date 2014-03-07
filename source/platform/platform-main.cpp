/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <stdlib.h>
#include <platform.h>

PlatformProcess::PlatformProcess(int argc, char **argv, Module **vec)
    : FdsProcess(argc, argv, "platform.conf", "fds.plat.", "platform.log", vec) {}

void
PlatformProcess::run()
{
    while (1) {
        sleep(1);
    }
}

int main(int argc, char **argv)
{
    fds::Module *plat_vec[] = {
        &fds::gl_NodePlatform,
        NULL
    };
    PlatformProcess plat(argc, argv, plat_vec);

    plat.setup();
    plat.run();
    return 0;
}
