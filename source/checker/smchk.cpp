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

namespace fds {
    SMChk::SMChk(int sm_count, SmDiskMap::ptr smDiskMap,
            ObjectDataStore::ptr smObjStore, ObjectMetadataDb::ptr smMdDb):
            sm_count(sm_count),
            smDiskMap(smDiskMap),
            smObjStore(smObjStore),
            smMdDb(smMdDb) {
        fds_uint32_t cols = (sm_count < 4) ? sm_count : 4;
        DLT* dlt = new DLT(8, cols, 1, true);
        SmUtUtils::populateDlt(dlt, sm_count);
        GLOGDEBUG << "Using DLT: " << *dlt;
        smDiskMap->handleNewDlt(dlt);

        // Open the data store
        smObjStore->openDataStore(smDiskMap);

        // Meta db
        smMdDb->openMetadataDb(smDiskMap);

        // we don't need dlt anymore
        delete dlt;
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
        for (auto entry : rev_map) {
            std::cout << "PATH: " << entry.first << "\n";
            for (auto val : entry.second) {
                std::cout << "SM Token: " << val << "\n";
            }
        }
    }

    void SMChk::list_metadata() {
        MetadataIterator md_iter(this);
        for (md_iter.start(); !md_iter.end(); md_iter.next()) {
            std::cout << *(md_iter.value()) << "\n";
        }
    }

    int SMChk::bytes_reclaimable() {
        int bytes = 0;
        MetadataIterator md_it(this);
        for (md_it.start(); !md_it.end(); md_it.next()) {
            boost::shared_ptr<ObjMetaData> omd = md_it.value();
            if (omd->getRefCnt() == 0) {
                bytes += omd->getObjSize();
            }
        }
        std::cout << "Found " << bytes << " bytes reclaimable.\n";
        return bytes;
    }

    bool SMChk::full_consistency_check() {
        int error_count = 0;
        int objs_count = 0;
        MetadataIterator md_it(this);
        for (md_it.start(); !md_it.end(); md_it.next()) {
            const ObjectID id(md_it.key());
            boost::shared_ptr<ObjMetaData> omd = md_it.value();
            fds_uint32_t obj_size = omd->getObjSize();

            // Get object
            Error err(ERR_OK);
            boost::shared_ptr<const std::string> dataPtr
                    = smObjStore->getObjectData(invalid_vol_id, id, omd, err);

            // Get hash of data
            ObjectID hashId = ObjIdGen::genObjectId((*dataPtr).c_str(),
                    static_cast<size_t>(obj_size));

            GLOGDEBUG << "Hasher found: " << hashId
                        << " and objId was: " << id << "\n";

            // Compare hash to objID
            if (hashId != id) {
                std::cout << "An error was found with " << id << "\n";
                GLOGNORMAL << id << " corrupt!\n";
                error_count++;
            }
            // Increment the number of objects we've checked
            objs_count++;
            // omd will auto delete when it goes out of scope
        }
        GLOGNORMAL << objs_count << " objects checked, " << error_count << " errors were found.\n";
        if (error_count > 0) {
            std::cout << "WARNING: " << error_count << " errors were found. "
                    << objs_count << " objects were checked. \n";
            return false;
        }
        std::cout << "SUCCESS! " << objs_count << " objects checked; no errors found.\n";
        return true;
    }

    // ---------------------- MetadataIterator -------------------------------- //

    SMChk::MetadataIterator::MetadataIterator(SMChk * instance) {
        smchk = instance;
        ldb = nullptr;
        ldb_it = nullptr;
        all_toks = smchk->getSmTokens();
    }

    void SMChk::MetadataIterator::start() {
        token_it = all_toks.begin();
        GLOGNORMAL << "Reading metadata db for SM token " << *token_it << "\n";
        smchk->smMdDb->snapshot(*token_it, ldb, options);
        ldb_it = ldb->NewIterator(options);
        ldb_it->SeekToFirst();
        if (!ldb_it->Valid()) {
            next();
        }
    }

    bool SMChk::MetadataIterator::end() {
        if (token_it == all_toks.end()) {
            if (!ldb_it->Valid()) {
                return true;
            }
        }
        return false;
    }

    void SMChk::MetadataIterator::next() {
        // If we're currently valid, lets try to call next
        if (ldb_it->Valid()) {
            ldb_it->Next();
        }
        // If we're no longer valid (or never were, keep moving until we are)
        while (!ldb_it->Valid()) {
            // Moving to next was invalid so we must increase token iterator
            ++token_it;
            if (end()) { return; }

            delete ldb_it;
            delete ldb;

            // Create new from new token
            GLOGNORMAL << "Reading metadata db for SM token " << *token_it << "\n";
            smchk->smMdDb->snapshot(*token_it, ldb, options);
            ldb_it = ldb->NewIterator(options);
            ldb_it->SeekToFirst();
        }
    }

    std::string SMChk::MetadataIterator::key() {
        return ldb_it->key().ToString();
    }

    boost::shared_ptr<ObjMetaData> SMChk::MetadataIterator::value() {
        if (!end()) {
            // We should have a valid levelDB / ldb_it
            omd = boost::shared_ptr<ObjMetaData>(new ObjMetaData());
            omd->deserializeFrom(ldb_it->value());
        }
        return omd;
    }



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
            new fds::ObjectDataStore("SMchk"));

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
