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

#include "smchk.h"

// WE LOVE GLOBALS
fds::SmDiskMap::ptr smDiskMap = fds::SmDiskMap::ptr(
        new fds::SmDiskMap("SMchk"));

fds::ObjectDataStore::unique_ptr smObjStore = fds::ObjectDataStore::unique_ptr(
        new fds::ObjectDataStore("SMchk"));

namespace fds {
    SMChk::SMChk(int argc, char * argv[], const std::string &config,
            const std::string &basePath, Module * vec[]):
            FdsProcess(argc, argv, config, basePath, "smchk.log", vec), error_count(0) {
        // Get from command line args
        namespace po = boost::program_options;
        po::options_description desc("Run SM checker");

        desc.add_options()
                ("help,h", "Print this help message")
                ("sm-count,c", po::value<int>(),
                        "Name of the volume to check.");

        po::variables_map vm;
        po::parsed_options parsed =
                po::command_line_parser(argc, argv).options(desc).allow_unregistered().run();
        po::store(parsed, vm);
        po::notify(vm);

        if (vm.count("help")) {
            std::cout << desc << "\n";
            exit(0);
        }

        if (vm.count("sm-count") == 0) {
            std::cout << desc << "\n";
            exit(-1);
        }

        sm_count = vm["sm-count"].as<int>();

        // ------------------------------------------------ //


        smMdDb = fds::ObjectMetadataDb::unique_ptr(
                new fds::ObjectMetadataDb());
    }

    int SMChk::run() {
        fds_uint32_t cols = (sm_count < 4) ? sm_count : 4;
        DLT* dlt = new DLT(10, cols, 1, true);
        SmUtUtils::populateDlt(dlt, sm_count);
        GLOGDEBUG << "Using DLT: " << *dlt;
        smDiskMap->handleNewDlt(dlt);

        // Meta db
        smMdDb->setNumBitsPerToken(16);
        smMdDb->openMetadataDb(smDiskMap);

        // we don't need dlt anymore
        delete dlt;

        full_consistency_check();

        return 0;
    }

    SmTokenSet SMChk::getSmTokens() {
        return smDiskMap->getSmTokens();
    }

    void SMChk::list_path_by_token() {
        SmTokenSet smToks = getSmTokens();
        for (SmTokenSet::const_iterator cit = smToks.cbegin();
             cit != smToks.cend();
             ++cit) {
            std::cout << "Token " << *cit << "\ndisk path: "
                    << smDiskMap->getDiskPath(*cit, diskio::diskTier)
                    << "\n";
        }
    }

    void SMChk::list_token_by_path() {
        std::unordered_map<std::string, std::vector<fds_token_id>> rev_map;

        SmTokenSet smToks = getSmTokens();
        for (SmTokenSet::const_iterator cit = smToks.cbegin();
             cit != smToks.cend(); ++cit) {
            std::string path(smDiskMap->getDiskPath(*cit, diskio::diskTier));
            rev_map[path].push_back(*cit);
        }
        std::cout << "HIT HERE";
        for (auto entry : rev_map) {
            std::cout << "PATH: " << entry.first << "\n";
            for (auto val : entry.second) {
                std::cout << "SM Token: " << val << "\n";
            }
        }
    }

#if 0
    void list_metadata() {
        ObjMetaData omd;
        leveldb::DB* ldb;
        leveldb::ReadOptions options;
        smMdDb->snapshot(smTokId, ldb, options);
        leveldb::Iterator* it = ldb->NewIterator(options);

        GLOGDEBUG << "Reading metadata db for SM token " << smTokId;
        for (it->SeekToFirst(); it->Valid(); it->Next()) {
            ObjectID id(it->key().ToString());

            // Get object

            // Do hashing
            // Compare hash to objID
            // Alert or continue

            omd.deserializeFrom(it->value());
            GLOGDEBUG << "Snap: " << id << " " << omd;
        }


        delete it;
        ldb->ReleaseSnapshot(options.snapshot);
    }
#endif

    bool SMChk::full_consistency_check() {
        // Token stuffs
        SmTokenSet all_toks = getSmTokens();

        boost::shared_ptr<ObjMetaData> omd;
        leveldb::ReadOptions options;

        for (auto token : all_toks) {
            leveldb::DB *ldb;
            leveldb::Iterator *it;

            GLOGNORMAL << "Reading metadata db for SM token " << token << "\n";
            smMdDb->snapshot(token, ldb, options);
            it = ldb->NewIterator(options);
            for (it->SeekToFirst(); it->Valid(); it->Next()) {
                const ObjectID id(it->key().ToString());

                omd = boost::shared_ptr<ObjMetaData>(new ObjMetaData());
                omd->deserializeFrom(it->value());

                fds_uint32_t obj_size = omd->getObjSize();

                GLOGDEBUG << *omd << "\n" << "Disk Path: "
                            << smDiskMap->getDiskPath(token, diskio::diskTier);

                // Get object
                Error err(ERR_OK);
                boost::shared_ptr<const std::string> ptr
                        = smObjStore->getObjectData(invalid_vol_id, id, omd, err);

                // Get hash of data
                ObjectID hashId = ObjIdGen::genObjectId((*ptr).c_str(),
                        static_cast<size_t>(obj_size));

                GLOGDEBUG << "Hasher found: " << hashId
                            << " and objId was: " << id << "\n";

                // Compare hash to objID
                if (hashId != id) {
                    std::cout << "An error was found with " << id << "\n";
                    GLOGNORMAL << id << " corrupt!\n";
                    error_count++;
                }
                // omd will auto delete when it goes out of scope
            }
            ldb->ReleaseSnapshot(options.snapshot);
            // Delete ldb and it
            delete it;
            delete ldb;
        }
        GLOGNORMAL << error_count << " errors were found.\n";
        if (error_count > 0) {
            std::cout << "WARNING: " << error_count << " errors were found.\n";
            return false;
        }
        std::cout << "SUCCESS! All objects checked; no errors found.\n";
        return true;
    }
}  // namespace fds

int
main(int argc, char** argv) {
    fds::Module *vec[] = {
            &diskio::gl_dataIOMod,
            smDiskMap.get(),
            smObjStore.get(),
            nullptr
    };
    fds::SMChk smChk(argc, argv, "sm_ut.conf",
            "fds.diskmap_ut.", vec);
    smChk.main();
    return 0;
}
