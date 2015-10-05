/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#include <fds_assert.h>
#include <fds_process.h>
#include <object-store/SmDiskMap.h>
#include <object-store/ObjectMetaDb.h>
#include <object-store/ObjectMetadataStore.h>
#include <object-store/ObjectStore.h>
#include <vector>
#include <string>
#include <boost/program_options.hpp>

#include <SMCheckDriver.h>
#include <SMCheck.h>

namespace fds {

SMCheckDriver::SMCheckDriver(int argc, char *argv[],
                             const std::string &config,
                             const std::string &basePath, Module *vec[],
                             SmDiskMap::ptr smDiskMap,
                             ObjectDataStore::ptr smObjStore)
    : FdsProcess(argc, argv, config, basePath, "smchk.log", vec),
      checker(nullptr),
      smDiskMap(smDiskMap),
      smObjStore(smObjStore)
{
    // Create the metadata db
    smMdDb = fds::ObjectMetadataDb::ptr(
            new fds::ObjectMetadataDb());
    // Get from command line args
    namespace po = boost::program_options;
    po::options_description desc("Run SM checker");
    desc.add_options()
            ("help,h", "Print this help message")
            ("verbose,v", "Verbose output")
            ("full-check,f",
                    po::bool_switch()->default_value(false),
                    "Run full consistency check")
            ("list-metadata,p",
                    po::bool_switch()->default_value(false),
                    "Print all metadata.")
            ("check-active-only,a",
                    po::bool_switch()->default_value(false),
                    "check only active metadata when running full-check or list metadata")
            ("check-ownership,o",
                    po::bool_switch()->default_value(false),
                    "check SM token ownership when running full-check")
            ("list-tokens",
                    po::bool_switch()->default_value(false),
                    "Print all tokens by path.")
            ("list-paths",
                    po::bool_switch()->default_value(false),
                    "Print paths by token.")
            ("gc,g",
                    po::bool_switch()->default_value(false),
                    "Calculate and print bytes reclaimable by garbage collector.");

    po::variables_map vm;
    po::parsed_options parsed =
            po::command_line_parser(argc, argv).options(desc).allow_unregistered().run();
    po::store(parsed, vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << "\n";
        exit(0);
    }

    if (vm["full-check"].as<bool>()) {
        cmd = RunFunc::FULL_CHECK;
    } else if (vm["list-tokens"].as<bool>()) {
        cmd = RunFunc::PRINT_TOK_BY_PATH;
    } else if (vm["list-paths"].as<bool>()) {
        cmd = RunFunc::PRINT_PATH_BY_TOK;
    } else if (vm["list-metadata"].as<bool>()) {
        cmd = RunFunc::PRINT_MD;
    } else if (vm["gc"].as<bool>()) {
        cmd = RunFunc::CALC_BYTES_RECLAIMABLE;
    } else {
        cmd = RunFunc::FULL_CHECK;
    }
    ownershipCheck = vm["check-ownership"].as<bool>();
    checkOnlyActive = vm["check-active-only"].as<bool>();
    verbose = vm.count("verbose");
}

int SMCheckDriver::run() {
    checker = new SMCheckOffline(smDiskMap, smObjStore, smMdDb, verbose);
    bool success = false;
    switch (cmd) {
        case RunFunc::FULL_CHECK:
            success = checker->full_consistency_check(ownershipCheck, checkOnlyActive);
            break;
        case RunFunc::PRINT_MD:
            checker->list_metadata(checkOnlyActive);
            success = true;
            break;
        case RunFunc::PRINT_PATH_BY_TOK:
            checker->list_path_by_token();
            success = true;
            break;
        case RunFunc::PRINT_TOK_BY_PATH:
            checker->list_token_by_path();
            success = true;
            break;
        case RunFunc::CALC_BYTES_RECLAIMABLE:
            checker->bytes_reclaimable();
            success = true;
            break;
        default:
            success = checker->full_consistency_check(ownershipCheck, checkOnlyActive);
            break;
    }
    return (success ? 0 : 1);
}
}  // namespace fds

int
main(int argc, char** argv) {
    fds::SmDiskMap::ptr smDiskMap = fds::SmDiskMap::ptr(
            new fds::SmDiskMap("SMchk"));

    fds::ObjectDataStore::ptr smObjStore = fds::ObjectDataStore::ptr(
            new fds::ObjectDataStore("SMchk", NULL));

    fds::Module *vec[] = {
            //&diskio::gl_dataIOMod,
            smDiskMap.get(),
            smObjStore.get(),
            nullptr
    };

    std::vector<char*> new_argv;
    for (int i = 0; i < argc; ++i) {
        new_argv.push_back(argv[i]);
    }
    new_argv.push_back("--fds.sm.enable_graphite=false");

    argc = new_argv.size();

    fds::SMCheckDriver smChk(argc, &new_argv[0], "platform.conf", "fds.sm.", vec, smDiskMap, smObjStore);
    int ret = smChk.main();
    return ret;
}
