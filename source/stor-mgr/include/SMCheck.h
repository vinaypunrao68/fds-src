/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_STOR_MGR_INCLUDE_SMCHECK_H_
#define SOURCE_STOR_MGR_INCLUDE_SMCHECK_H_

#include <persistent-layer/dm_io.h>
#include <SmIo.h>


namespace fds {

// Forward Declaration
class ObjectStore;
class SmDiskMap;
class ObjectMetadataDb;
class DLT;

enum RunFunc {
    FULL_CHECK = 0,
    PRINT_MD,
    PRINT_TOK_BY_PATH,
    PRINT_PATH_BY_TOK,
    CALC_BYTES_RECLAIMABLE,
    TOKEN_CHECK
};

typedef boost::shared_ptr<ObjMetaData> MdPtr;

// TODO(Sean):
// This really needs to be restructure with base SMCheck class and SMCheckOnline and SMCheckOffline
// classes.
// off-line smcheck: directly calls the ObjectStore, so SM cannot run at the time of running SM.
// on-line smcheck: snapshot request is a QoS request, and per SM token consistency check is done as a callback.
// But, for now, just deal with messy setup.  Come back later to clean it up.  Made an attempt, but took
// too long without a desired success.
class SMCheckOffline {
// friend class MetadataIterator;
  public:
    // Off-line smcheck constructor.
    explicit SMCheckOffline(SmDiskMap::ptr smDiskMap,
                            ObjectDataStore::ptr smObjStore,
                            ObjectMetadataDb::ptr smMdDb,
                            bool verboseMsg);

    ~SMCheckOffline() { delete curDLT; }

    // off-line smcheck ifaces
    void list_path_by_token();
    void list_token_by_path();
    void list_metadata(bool checkOnlyActive);
    bool full_consistency_check(bool checkOwnership, bool checkOnlyActive);
    int  bytes_reclaimable();
    bool consistency_check(fds_token_id tokId); // test a single token

    // common smcheck ifaces
    bool checkObjectOwnership(const ObjectID& objId);

  protected:
    // data needed by off-line smcheck
    RunFunc cmd;
    SmDiskMap::ptr smDiskMap;
    ObjectDataStore::ptr smObjStore;
    ObjectMetadataDb::ptr smMdDb;
    bool verbose;

    // Methods
    SmTokenSet getSmTokens();
    ObjectID hash_data(boost::shared_ptr<const std::string> dataPtr, fds_uint32_t obj_size);

    // Common for
    DLT *curDLT;
    NodeUuid smUuid;


    // Iterator used by off-line smcheck ifaces
    class MetadataIterator {
      public:
        explicit MetadataIterator(SMCheckOffline * instance);
        ~MetadataIterator() {}
        bool end();
        void start();
        MdPtr value();
        std::string key();
        void next();
      private:
        SMCheckOffline *smchk;

        SmTokenSet all_toks;
        SmTokenSet::iterator token_it;

        leveldb::ReadOptions options;
        leveldb::DB *ldb;
        leveldb::Iterator *ldb_it;

        MdPtr omd;
    };  // MetadataIterator
};  // SMCheckOffline



// This is a online version of smchecker.
class SMCheckOnline {
  public:
    SMCheckOnline(SmIoReqHandler *datastore, SmDiskMap::ptr diskmap);
    ~SMCheckOnline();

    Error startIntegrityCheck(SmTokenSet tgtDltTokens);

    void SMCheckSnapshotCB(const Error& error,
                           SmIoSnapshotObjectDB* snapReq,
                           leveldb::ReadOptions& options,
                           leveldb::DB* db);

    void updateDLT(const DLT *latestDLT);

    void getStats(fpi::CtrlNotifySMCheckStatusRespPtr resp);

    bool setActive();

    void setInactive();

    inline bool getActiveStatus() {
        return std::atomic_load(&SMChkActive);
    }


  private:
    // A simple boolean to indicate state of the checker.
    // If we ever got to multiple requests, this can be converted to
    // a atomic counter, where is count represents number of outstanding
    // requests.
    std::atomic<bool> SMChkActive;

    // Target DLT tokens list
    std::set<fds_token_id> targetDLTTokens;

    void resetStats();
    bool checkObjectOwnership(const ObjectID& objId);

    // Macking the atomic, in case we want to SM check in parallel with
    // multiple threads
    // total number of corruption detected.
    std::atomic<int64_t> numCorruptions;
    // total number of SM token ownership mismatch.
    std::atomic<int64_t> numOwnershipMismatches;
    // total number of active objects.
    std::atomic<int64_t> numActiveObjects;

    // progress of number of tokens examined.
    int64_t totalNumTokens;
    std::atomic<int64_t> totalNumTokensVerified;

    SmIoReqHandler *dataStore;
    SmDiskMap::ptr diskMap;

    // snapshot request structure
    SmIoSnapshotObjectDB snapRequest;

    // set of all tokens beloning to the SM.
    SmTokenSet allTokens;
    // in-memory persistent iterator.  iterator is incremented in different
    // thread context, so we need to save them here.
    SmTokenSet::iterator iterTokens;

    // Latest cloned DLT
    DLT *latestClosedDLT;
    // UUID of the SM service.
    NodeUuid SMCheckUuid;
};  // SMCheckOnline

}  // namespace fds
#endif  // SOURCE_STOR_MGR_INCLUDE_SMCHECK_H_
