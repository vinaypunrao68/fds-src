/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_STOR_MGR_INCLUDE_SMCHECK_H_
#define SOURCE_STOR_MGR_INCLUDE_SMCHECK_H_

class ObjectStore;
class SmDiskMap;
class ObjectMetadataDb;
class DLT;

namespace fds {

enum RunFunc {
    FULL_CHECK = 0,
    PRINT_MD,
    PRINT_ACTIVE_MD,
    PRINT_TOK_BY_PATH,
    PRINT_PATH_BY_TOK,
    CALC_BYTES_RECLAIMABLE,
    TOKEN_CHECK
};

typedef boost::shared_ptr<ObjMetaData> MdPtr;

class SMCheck {
// friend class MetadataIterator;
  public:
    SMCheck(SmDiskMap::ptr smDiskMap,
            ObjectDataStore::ptr smObjStore,
            ObjectMetadataDb::ptr smMdDb,
            bool verboseMsg);
    SMCheck(ObjectDataStore::ptr smObjStore,
            ObjectMetadataDb::ptr smMdDb);
    ~SMCheck() { delete curDLT; }

    void list_path_by_token();
    void list_token_by_path();
    void list_metadata();
    void list_active_metadata();
    bool full_consistency_check(bool checkOwnership, bool checkOnlyActive);
    int  bytes_reclaimable();
    bool consistency_check(ObjectID obj_id);  // test a single object
    bool consistency_check(fds_token_id tokId); // test a single token

    bool checkObjectOwnership(const ObjectID& objId);

  protected:
    // Data
    int sm_count;
    RunFunc cmd;
    SmDiskMap::ptr smDiskMap;
    ObjectDataStore::ptr smObjStore;
    ObjectMetadataDb::ptr smMdDb;
    // Methods
    SmTokenSet getSmTokens();
    ObjectID hash_data(boost::shared_ptr<const std::string> dataPtr, fds_uint32_t obj_size);

    DLT *curDLT;
    NodeUuid smUuid;

    bool verbose;

    class MetadataIterator {
      public:
        explicit MetadataIterator(SMCheck * instance);
        ~MetadataIterator() {}
        bool end();
        void start();
        MdPtr value();
        std::string key();
        void next();
      private:
        SMCheck *smchk;

        SmTokenSet all_toks;
        SmTokenSet::iterator token_it;

        leveldb::ReadOptions options;
        leveldb::DB *ldb;
        leveldb::Iterator *ldb_it;

        MdPtr omd;
    };
};
}  // namespace fds
#endif  // SOURCE_STOR_MGR_INCLUDE_SMCHECK_H_
