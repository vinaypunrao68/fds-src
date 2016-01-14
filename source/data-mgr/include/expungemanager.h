/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
#ifndef SOURCE_DATA_MGR_INCLUDE_EXPUNGEMANAGER_H_
#define SOURCE_DATA_MGR_INCLUDE_EXPUNGEMANAGER_H_

// Standard includes.
#include <string>

// Internal includes.
#include "lib/Catalog.h"
#include "fds_types.h"
#include "fds_volume.h"

namespace fds {
struct DataMgr;

/**
 * Maintains the no.of of times a object has been deleted. Once the object
 * is not present in any snapshot, the expunge manager sends that many
 * delete requests to the SM.
 */
struct ExpungeDB {
    explicit ExpungeDB(const FdsRootDir* root);
    uint32_t increment(fds_volid_t volId, const ObjectID &objId);
    uint32_t decrement(fds_volid_t volId, const ObjectID &objId);
    uint32_t getExpungeCount(fds_volid_t volId, const ObjectID &objId);
    void discard(fds_volid_t volId, const ObjectID &objId);
    ~ExpungeDB();
  protected:
    std::string getKey(fds_volid_t volId, const ObjectID &objId);
  private:
    Catalog db;
};

/**
 * The Expunge Manager, queues an expunge request & later checks whether the
 * delete needs to be counted and when neccessary send the request to SM
 */
struct ExpungeManager {
    explicit ExpungeManager(DataMgr* dm);
    Error expunge(fds_volid_t volId, const std::vector<ObjectID>& vecObjIds, bool force=false);
    Error expunge(fds_volid_t volId, const ObjectID& objId, bool force=false);
    
    // request to/response from SMs
    Error sendDeleteRequest(fds_volid_t volId, const ObjectID &objId);
    void  onDeleteResponse(fds_uint64_t dltVersion,
                           SHPTR<concurrency::TaskStatus> taskStatus,
                           MultiPrimarySvcRequest* svcReq,
                           const Error& error,
                           boost::shared_ptr<std::string> payload);
    void threadTask(fds_volid_t volId, ObjectID objId, bool fFromSnapshot=false);

  private:
    DataMgr* dm;
    SHPTR<ExpungeDB> expungeDB;
    static const std::hash<fds_volid_t> volIdHash;
    static const ObjectHash objHash;
    struct VolObjHashType {
        size_t operator()(const fds_volid_t volId, const ObjectID &objId) const {
            return volIdHash(volId) + objHash(objId);
        }
    } VolObjHash;
    
    std::unique_ptr<SynchronizedTaskExecutor<size_t>> serialExecutor;
};

}  // namespace fds

#endif  // SOURCE_DATA_MGR_INCLUDE_EXPUNGEMANAGER_H_
