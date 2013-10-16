#include <unistd.h>
#include <fds_module.h>
#include <disk-mgr/dm_io.h>

int main(int argc, char **argv)
{
    fds::Module *io_test_vec[] = {
        &diskio::dataIOMod,
        nullptr
    };
    fds::ModuleVector  io_test(argc, argv);

    io_test.mod_register(io_test_vec);
    io_test.mod_execute();
    while (1) {
        sleep(1);
        io_test.mod_timer_fn();
    }
}
