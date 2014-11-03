/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_CHECKER_SMCHK_H_
#define SOURCE_CHECKER_SMCHK_H_

class ObjectStore;
class SmDiskMap;
class ObjectMetadataDb;

namespace fds {

    typedef boost::shared_ptr<ObjMetaData> MdPtr;

    enum RunFunc {
        FULL_CHECK = 0,
        PRINT_MD,
        PRINT_TOK_BY_PATH,
        PRINT_PATH_BY_TOK,
        CALC_BYTES_RECLAIMABLE,
        TOKEN_CHECK
    };

    class SMChk {
    // friend class MetadataIterator;
    public:
        SMChk(int sm_count, SmDiskMap::ptr smDiskMap,
                ObjectDataStore::ptr smObjStore,
                ObjectMetadataDb::ptr smMdDb);
        SMChk(DLT* dlt, SmDiskMap::ptr smDiskMap,
                ObjectDataStore::ptr smObjStore,
                ObjectMetadataDb::ptr smMdDb);
        ~SMChk() {}

        void list_path_by_token();
        void list_token_by_path();
        void list_metadata();
        bool full_consistency_check();
        int  bytes_reclaimable();
        bool consistency_check(ObjectID obj_id);  // test a single object
        bool consistency_check(fds_token_id tokId); // test a single token


    protected:
        // Data
        int sm_count;
        RunFunc cmd;
        fds::SmDiskMap::ptr smDiskMap;
        fds::ObjectDataStore::ptr smObjStore;
        fds::ObjectMetadataDb::ptr smMdDb;
        // Methods
        SmTokenSet getSmTokens();
        ObjectID hash_data(boost::shared_ptr<const std::string> dataPtr, fds_uint32_t obj_size);

        class MetadataIterator {
        public:
            MetadataIterator(SMChk * instance);
            ~MetadataIterator() {}
            bool end();
            void start();
            MdPtr value();
            std::string key();
            void next();
        private:
            SMChk * smchk;

            SmTokenSet all_toks;
            SmTokenSet::iterator token_it;

            leveldb::ReadOptions options;
            leveldb::DB *ldb;
            leveldb::Iterator *ldb_it;

            MdPtr omd;
        };
    };

    class SMChkDriver : public FdsProcess {
    public:
        SMChkDriver(int argc, char * argv[], const std::string &config,
                const std::string &basePath, Module * vec[],
                fds::SmDiskMap::ptr smDiskMap,
                fds::ObjectDataStore::ptr smObjStore);
        ~SMChkDriver() {}

        int run() override;

    private:
        SMChk *checker;
        RunFunc cmd;
        fds::SmDiskMap::ptr smDiskMap;
        fds::ObjectDataStore::ptr smObjStore;
        fds::ObjectMetadataDb::ptr smMdDb;
        int sm_count;
    };
}  // namespace fds
#endif  // SOURCE_PLATFORM_INCLUDE_DISK_H_
