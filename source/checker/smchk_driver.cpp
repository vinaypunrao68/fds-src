/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#include <fds_assert.h>
#include <fds_process.h>
#include <sm_ut_utils.h>
#include <object-store/SmDiskMap.h>
#include <object-store/ObjectMetaDb.h>
#include <object-store/ObjectMetadataStore.h>
#include <object-store/ObjectStore.h>
#include <vector>
#include <string>
#include <boost/program_options.hpp>

#include "smchk_driver.h"

namespace fds {

SMChkDriver::SMChkDriver(int argc, char *argv[],
        const std::string &config,
        const std::string &basePath, Module *vec[],
        fds::SmDiskMap::ptr smDiskMap,
        fds::ObjectDataStore::ptr smObjStore):
        FdsProcess(argc, argv, config, basePath, "smchk.log", vec),
        checker(nullptr),
        smDiskMap(smDiskMap),
        smObjStore(smObjStore) {
    // Create the metadata db
    smMdDb = fds::ObjectMetadataDb::ptr(
            new fds::ObjectMetadataDb());
    // Get from command line args
    namespace po = boost::program_options;
    po::options_description desc("Run SM checker");
    desc.add_options()
            ("help,h", "Print this help message")
            ("sm-count,c",
                    po::value<int>(&sm_count)->default_value(1),
                    "Number of active SMs.")
            ("full-check,f",
                    po::bool_switch()->default_value(false),
                    "Run full consistency check")
            ("list-tokens",
                    po::bool_switch()->default_value(false),
                    "Print all tokens by path.")
            ("list-paths",
                    po::bool_switch()->default_value(false),
                    "Print paths by token.")
            ("list-metadata,p",
                    po::bool_switch()->default_value(false),
                    "Print all metadata.")
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
    sm_count = vm["sm-count"].as<int>();
}

int SMChkDriver::run() {
    checker = new SMChk(sm_count, smDiskMap, smObjStore, smMdDb);
    switch (cmd) {
        case RunFunc::FULL_CHECK:
            checker->full_consistency_check();
            break;
        case RunFunc::PRINT_MD:
            checker->list_metadata();
            break;
        case RunFunc::PRINT_PATH_BY_TOK:
            checker->list_path_by_token();
            break;
        case RunFunc::PRINT_TOK_BY_PATH:
            checker->list_token_by_path();
            break;
        case RunFunc::CALC_BYTES_RECLAIMABLE:
            checker->bytes_reclaimable();
            break;
        default:
            checker->full_consistency_check();
            break;
    }
    return 0;
}
}  // namespace fds

int
main(int argc, char** argv) {
    fds::SmDiskMap::ptr smDiskMap = fds::SmDiskMap::ptr(
            new fds::SmDiskMap("SMchk"));

    fds::ObjectDataStore::ptr smObjStore = fds::ObjectDataStore::ptr(
            new fds::ObjectDataStore("SMchk", NULL));

    fds::Module *vec[] = {
            &diskio::gl_dataIOMod,
            smDiskMap.get(),
            smObjStore.get(),
            nullptr
    };

    fds::SMChkDriver smChk(argc, argv, "sm_ut.conf", "fds.diskmap_ut.",
            vec, smDiskMap, smObjStore);
    smChk.main();
    return 0;
}
