#ifndef OBJECT_STORE_MOCK_H
#define OBJECT_STORE_MOCK_H

#include <unordered_map>
#include <thread>
#include <fdsp/FDSP_types.h>
#include <hash/MurmurHash3.h>

#include <iostream>  // NOLINT(*)
#include <unordered_map>
#include <vector>
#include <string>
#include <set>
#include <atomic>
#include <boost/shared_ptr.hpp>

#include <util/Log.h>
#include <fds_assert.h>
#include <fdsp/FDSP_types.h>
#include <fdsp/FDSP_DataPathReq.h>
#include <fdsp/FDSP_DataPathResp.h>
#include <concurrency/Mutex.h>
#include <fds-probe/fds_probe.h>
#include <StorMgrVolumes.h>
#include <dlt.h>

using namespace FDS_ProtocolInterface;  // NOLINT
namespace fds {

class MObjStore : public SmIoReqHandler {
 public:
    MObjStore()
     : lock_("MObjStoreMutex"),
       dlt_(8, 3, 1)
    {

    }
    ~MObjStore() {

    }
    void putObject(const std::string &data)
    {
        fds_mutex::scoped_lock l(lock_);
        ObjectID oid;
        MurmurHash3_x64_128(data.c_str(),
                data.size() + 1,
                0,
                &oid);

        token_db_[dlt_.getToken(oid)].insert(oid);
        object_db_[oid] = data;
    }

    bool getObject(const ObjectID &oid, std::string &data) {

         fds_mutex::scoped_lock l(lock_);
         auto itr = object_db_.find(oid);
         if (itr == object_db_.end()) {
             return false;
         }
         data = itr->second;
         return true;
    }

    std::vector<fds_token_id> getTokens() {
        std::vector<fds_token_id> tokens;
        fds_mutex::scoped_lock l(lock_);
        for (auto itr : token_db_) {
            tokens.push_back(itr.first);
        }
        return tokens;
    }

    int totalObjectCnt() {
        fds_mutex::scoped_lock l(lock_);
        return object_db_.size();
    }

    int totalObjectCntAllTokens() {
        fds_mutex::scoped_lock l(lock_);
        int cnt = 0;
        for (auto itr : token_db_) {
            cnt += itr.second.size();
        }
        return cnt;
    }

    void putTokenObjectsInternal(SmIoReq* ioReq)
    {
        Error err(ERR_OK);
        fds_mutex::scoped_lock l(lock_);

        SmIoPutTokObjectsReq *putTokReq = static_cast<SmIoPutTokObjectsReq*>(putTokReq);
        FDSP_MigrateObjectList &objList = putTokReq->obj_list;

        for (auto obj : objList) {
            ObjectID objId(obj.meta_data.object_id.hash_high,
                    obj.meta_data.object_id.hash_low);

            object_db_[objId] = obj.data;
            token_db_[putTokReq->token_id].insert(objId);
        }
        putTokReq->response_cb(err, putTokReq);
    }

    void getTokenObjectsInternal(SmIoReq* ioReq)
    {
        Error err(ERR_OK);
        fds_mutex::scoped_lock l(lock_);

        SmIoGetTokObjectsReq *getTokReq = static_cast<SmIoGetTokObjectsReq*>(ioReq);
        fds_token_id token_id = getTokReq->token_id;
        auto obj_ids = token_db_[token_id];
        for (auto obj_id : obj_ids) {
            std::string obj_data = object_db_[obj_id];
            FDSP_MigrateObjectData mig_obj;
            mig_obj.meta_data.token_id = token_id;
            mig_obj.meta_data.object_id.hash_high = obj_id.GetHigh();
            mig_obj.meta_data.object_id.hash_low = obj_id.GetLow();
            mig_obj.meta_data.obj_len = obj_data.size();
            mig_obj.data = obj_data;
            getTokReq->obj_list.push_back(mig_obj);
        }
        getTokReq->itr.objId = NullObjectID;

        getTokReq->response_cb(err, getTokReq);
    }

    virtual Error enqueueMsg(fds_volid_t volId, SmIoReq* io)
    {
        switch (io->io_type) {
        case FDS_SM_WRITE_TOKEN_OBJECTS:
            std::thread(&MObjStore::putTokenObjectsInternal, this, io);
            break;
        case FDS_SM_READ_TOKEN_OBJECTS:
            std::thread(&MObjStore::getTokenObjectsInternal, this, io);
            break;
        default:
            fds_assert(!"Unknown message");
            break;
        }
        return ERR_OK;
    }

 private:
    fds_mutex  lock_;
    DLT dlt_;
    std::unordered_map<ObjectID, std::string, ObjectHash> object_db_;
    std::unordered_map<fds_token_id, std::set<ObjectID, ObjectLess> > token_db_;
};

typedef boost::shared_ptr<MObjStore> MObjStorePtr;
}   // namespace fds
#endif
