/*
 * Copyright 2016 Formation Data Systems, Inc.
 * A simple, C-Styled argument parser and usage helper to help
 * drive the main VolumeChecker process.
 */

#include <boost/program_options.hpp>
#include <VolumeChecker.h>
#include <unistd.h>
#include <TestUtils.h>
#include <string.h>
#include <stdio.h>
#include <fds_volume.h>

/* Globals to drive Volume Checker */
bool help_flag;
int pm_uuid = -1;
int pm_port = -1;
int min_argc = 0;
std::vector<fds::fds_volid_t> volumes;
namespace po = boost::program_options;

/**
 * Helper Functions
 */
void populateVolumeList(std::string &volumeList) {
    volumeList.clear();
    for (auto vol : volumes) {
        volumeList += std::to_string(vol.get());
        volumeList += ",";
    }
    // Erase the last ","
    auto it = volumeList.end();
    fds_assert(it != volumeList.begin());
    --it;
    volumeList.erase(it);
}

/**
 * Args to be sent are as follows:
 *         {"checker",
 *          roots[0],
 *          "--fds.pm.platform_uuid=<uuid>",
 *          "--fds.pm.platform_port=<port>",
 *          "-v=1,2,3
 *          }
 *
 *  internal_argc == 5 above + 1 for original prog name
 *
 */
void populateInternalArgs(int argc, char **argv, int *internal_argc, char ***internal_argv, std::string root) {

    fds_verify(internal_argc && internal_argv);

    char **new_argv = *internal_argv;
    int new_argc = *internal_argc;

    // see above for magic number here
    *internal_argc = 6;

    // one for NULL ptr
    *internal_argv = (char **)malloc(sizeof(char*) * (*internal_argc) + 1);
    (*internal_argv)[*internal_argc] = NULL;
    std::string currentArg;
    std::string volumeList;

    populateVolumeList(volumeList);

    for (int i = 0; i < *internal_argc; ++i) {
        if (i == 0) {
            currentArg.assign(argv[0]);
        } else if (i == 1) {
            currentArg = "checker";
        } else if (i == 2) {
            currentArg = root;
        } else if (i == 3) {
            currentArg = "--fds.pm.platform_uuid=" + std::to_string(pm_uuid);
        } else if (i == 4) {
            currentArg = "--fds.pm.platform_port=" + std::to_string(pm_port);
        } else if (i == 5) {
            currentArg = "-v=" + volumeList;
        } else {
            break;
        }
        (*internal_argv)[i] = (char *)malloc(sizeof(char) * currentArg.length() + 1);
        strcpy(((*internal_argv)[i]), currentArg.c_str());
    }
}

void parsePoArgs(int argc, char **argv) {
    /**
     * Process cmd line options
     * When adding new options here, please double check internal_argc in
     * populateInternalArgs as well.
     */
    po::options_description desc("\nVolume checker command line options");
    po::positional_options_description m_positional;
    desc.add_options()
            ("help,h", "help/ usage message")
            ("uuid,u", po::value<int>(&pm_uuid), "Current node PM's UUID in decimal")
            ("port,p", po::value<int>(&pm_port), "Current node PM's Port")
            ("volume,v", po::value<std::vector<uint64_t>>()->multitoken(), "Volume IDs to be checked");

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv)
              .options(desc)
//              .allow_unregistered()
              .positional(m_positional)
              .run(), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << std::endl;
        exit(0);
    }

    if (vm.count("volume")) {
        auto parsedIds = vm["volume"].as<std::vector<uint64_t> >();
        if (!parsedIds.empty()) {
            std::transform(parsedIds.begin(), parsedIds.end(), std::back_inserter(volumes),
                           [](uint64_t const& val) -> fds::fds_volid_t { return fds::fds_volid_t(val); });
        }
    } else {
        std::cout << desc << std::endl;
        exit(0);
    }

    if ((!vm.count("uuid")) || (!vm.count("port"))) {
        std::cout << desc << std::endl;
        exit(0);
    }
}

int main(int argc, char **argv) {

    char **internal_argv = NULL;
    int internal_argc = 0;

    parsePoArgs(argc, argv);

    // Set up internal args to pass to checker
    std::vector<std::string> roots;
    fds::TestUtils::populateRootDirectories(roots, 1);

    populateInternalArgs(argc, argv, &internal_argc, &internal_argv, roots[0]);

    fds::VolumeChecker checker(internal_argc, internal_argv, false);
    checker.run();
    return checker.main();
}
