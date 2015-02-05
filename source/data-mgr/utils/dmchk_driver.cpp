/*
 * Copyright 2015 by Formation Data Systems, Inc.
 */

#include <boost/program_options.hpp>
#include <dmchk.h>

namespace po = boost::program_options;

int main(int argc, char* argv[]) {
    fds_volid_t volumeUuid = 0;

    // process command line options
    po::options_description desc("\nDM checker command line options");
    desc.add_options()
            ("help,h", "help/ usage message")
            ("list-blobs,l",
             po::value<fds_volid_t>(&volumeUuid)->default_value(0),
             "list blob contents for given volume");

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(desc).allow_unregistered().run(), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return 1;
    }

    Module *dmchkVec[] = {
        nullptr
    };
    DmChecker dmchk(argc, argv, "platform.conf", "fds.dm.", dmchkVec, "DM checker driver", volumeUuid);

    if (volumeUuid > 0)
        dmchk.listBlobs();

    return 0;
}
