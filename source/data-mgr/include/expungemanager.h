/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
#include <lib/Catalog.h>
#include <string>
#ifndef SOURCE_DATA_MGR_INCLUDE_EXPUNGEMANAGER_H_
#define SOURCE_DATA_MGR_INCLUDE_EXPUNGEMANAGER_H_

namespace fds {
struct DataMgr;

/**
 * Maintains the expunge count of an obj in a vol 
 */
struct ExpungeDB {
    ExpungeDB();
    uint32_t increment(fds_volid_t volId, const ObjectID &objId);
    uint32_t getExpungeCount(fds_volid_t volId, const ObjectID &objId);
    void discard(fds_volid_t volId, const ObjectID &objId);
    ~ExpungeDB();
  protected:
    std::string getKey(fds_volid_t volId, const ObjectID &objId);
  private:
    Catalog db;
};

struct ExpungeManager {
    explicit ExpungeManager(DataMgr* dm);
    Error expunge(fds_volid_t volId, const std::vector<ObjectID>& vecObjIds, bool force=false);
    Error expunge(fds_volid_t volId, const ObjectID& objId, bool force=false);
    
    // request to/response from SMs
    Error sendDeleteRequest(fds_volid_t volId, const ObjectID &objId);
    void  onDeleteResponse(fds_uint64_t dltVersion,
                           SHPTR<concurrency::TaskStatus> taskStatus,
                           QuorumSvcRequest* svcReq,
                           const Error& error,
                           boost::shared_ptr<std::string> payload);
    void threadTask(fds_volid_t volId, ObjectID objId, bool fFromSnapshot=false);

  private:
    DataMgr* dm;
    SHPTR<ExpungeDB> expungeDB;
};

}  // namespace fds

#endif  // SOURCE_DATA_MGR_INCLUDE_EXPUNGEMANAGER_H_
