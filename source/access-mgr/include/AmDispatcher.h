/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_ACCESS_MGR_INCLUDE_AMDISPATCHER_H_
#define SOURCE_ACCESS_MGR_INCLUDE_AMDISPATCHER_H_

#include <mutex>
#include <string>
#include <fds_volume.h>
#include <net/SvcRequest.h>
#include "AmRequest.h"
#include "concurrency/RwLock.h"

namespace fds {

/* Forward declaarations */
class MockSvcHandler;
struct DLT;

struct VolumeDispatchTable {
    using entry_type = std::pair<std::mutex, fds_uint64_t>;
    using key_type = fds_volid_t;
    using map_type = std::unordered_map<key_type, entry_type>;

    VolumeDispatchTable() = default;
    VolumeDispatchTable(VolumeDispatchTable const& rhs) = delete;
    VolumeDispatchTable& operator=(VolumeDispatchTable const& rhs) = delete;
    ~VolumeDispatchTable() = default;

    /**
     * Set the sequence id of a request to the next value and return a unique
     * lock preventing any other request that would update the volume from
     * being dispatched until this has entered SvcLayer
     */
    std::unique_lock<std::mutex> getAndLockVolumeSequence(fds_volid_t const vol_id, int64_t& seq_id) {
        SCOPEDREAD(table_lock);
        auto it = lock_table.find(vol_id);
        fds_assert(lock_table.end() != it);
        std::unique_lock<std::mutex> g(it->second.first);
        seq_id = ++it->second.second;
        return g;
    }

    /**
     * Either register a new sequence id for dispatching updates or update an
     * existing id if newer.
     */
    void registerVolumeSequence(fds_volid_t const vol_id, fds_uint64_t const seq_id) {
        SCOPEDWRITE(table_lock);
        auto it = lock_table.find(vol_id);
        if (lock_table.end() != it) {
            auto& curr_seq_id = it->second.second;
            curr_seq_id = std::max(curr_seq_id, seq_id);
        } else {
            lock_table[vol_id].second = seq_id;
        }
    }

 private:
    map_type lock_table;
    fds_rwlock table_lock;
};

/**
 * AM FDSP request dispatcher and reciever. The dispatcher
 * does the work to send and receive AM network messages over
 * the service layer.
 */
struct AmDispatcher : HasModuleProvider
{
    /**
     * The dispatcher takes a shared ptr to the DMT manager
     * which it uses when deciding who to dispatch to. The
     * DMT manager is still owned and updated to omClient.
     * TODO(Andrew): Make the dispatcher own this piece or
     * iterface with platform lib.
     */
    explicit AmDispatcher(CommonModuleProviderIf *modProvider);
    AmDispatcher(AmDispatcher const&)               = delete;
    AmDispatcher& operator=(AmDispatcher const&)    = delete;
    AmDispatcher(AmDispatcher &&)                   = delete;
    AmDispatcher& operator=(AmDispatcher &&)        = delete;
    ~AmDispatcher();

    /**
     * Initialize the OM client, and retrieve Dlt/Dmt managers
     */
    void start();

    /**
     * Dlt/Dmt updates
     */
    Error updateDlt(bool dlt_type, std::string& dlt_data, FDS_Table::callback_type const& cb);
    Error updateDmt(bool dmt_type, std::string& dmt_data, FDS_Table::callback_type const& cb);
    /**
     * Uses the OM Client to fetch the DMT and DLT, and update the AM's own versions.
     */
    Error getDMT();
    Error getDLT();

    /**
     * Dispatches a test volume request to OM.
     */
    Error attachVolume(std::string const& volume_name);
    void dispatchAttachVolume(AmRequest *amReq);

    /**
     * Dispatches an open volume request to DM.
     */
    void dispatchOpenVolume(AmRequest *amReq);
    void openVolumeCb(AmRequest* amReq,
                              MultiPrimarySvcRequest* svcReq,
                              const Error& error,
                              boost::shared_ptr<std::string> payload) const;

    /**
     * Dispatches an open volume request to DM.
     */
    Error dispatchCloseVolume(fds_volid_t vol_id, fds_int64_t token);

    /**
     * Dispatches a stat volume request.
     */
    void dispatchStatVolume(AmRequest *amReq);

    /**
     * Callback for stat volume responses.
     */
    void statVolumeCb(AmRequest* amReq,
                      FailoverSvcRequest* svcReq,
                      const Error& error,
                      boost::shared_ptr<std::string> payload);

    /**
     * Dispatches a set volume metadata request.
     */
    void dispatchSetVolumeMetadata(AmRequest *amReq);

    /**
     * Callback for set volume metadata responses.
     */
    void setVolumeMetadataCb(AmRequest* amReq,
                             MultiPrimarySvcRequest* svcReq,
                             const Error& error,
                             boost::shared_ptr<std::string> payload);

    /**
     * Dispatches a get volume metadata request.
     */
    void dispatchGetVolumeMetadata(AmRequest *amReq);

    /**
     * Callback for get volume metadata responses.
     */
    void getVolumeMetadataCb(AmRequest* amReq,
                             FailoverSvcRequest* svcReq,
                             const Error& error,
                             boost::shared_ptr<std::string> payload);

    /**
     * Aborts a blob transaction request.
     */
    void dispatchAbortBlobTx(AmRequest *amReq);

    /**
     * Dipatches a start blob transaction request.
     */
    void dispatchStartBlobTx(AmRequest *amReq);

    /**
     * Callback for start blob transaction responses.
     */
    void startBlobTxCb(AmRequest* amReq,
                       MultiPrimarySvcRequest* svcReq,
                       const Error& error,
                       boost::shared_ptr<std::string> payload);

    /**
     * Dispatches a commit blob transaction request.
     */
    void dispatchCommitBlobTx(AmRequest *amReq);

    /**
     * Callback for commit blob transaction responses.
     */
    void commitBlobTxCb(AmRequest* amReq,
                       MultiPrimarySvcRequest* svcReq,
                       const Error& error,
                       boost::shared_ptr<std::string> payload);

    /**
     * Dispatches an update catalog request.
     */
    void dispatchUpdateCatalog(AmRequest *amReq);

    /**
     * Dispatches an update catalog once request.
     */
    void dispatchUpdateCatalogOnce(AmRequest *amReq);

    /**
     * Dipatches a put object request.
     */
    void dispatchPutObject(AmRequest *amReq);

    /**
     * Dipatches a get object request.
     */
    void dispatchGetObject(AmRequest *amReq);

    /**
     * Dispatches a delete blob transaction request.
     */
    void dispatchDeleteBlob(AmRequest *amReq);

    /**
     * Dipatches a query catalog request.
     */
    void dispatchQueryCatalog(AmRequest *amReq);

    /**
     * Dispatches a stat blob transaction request.
     */
    void dispatchStatBlob(AmRequest *amReq);

    /**
     * Dispatches a rename blob request.
     */
    void dispatchRenameBlob(AmRequest *amReq);

    /**
     * Dispatches a rename blob request.
     */
    void renameBlobCb(AmRequest *amReq,
                      MultiPrimarySvcRequest* svcReq,
                      const Error& error,
                      boost::shared_ptr<std::string> payload);

    /**
     * Dispatches a set metadata on blob transaction request.
     */
    void dispatchSetBlobMetadata(AmRequest *amReq);

    /**
     * Dispatches a volume contents (list bucket) transaction request.
     */
    void dispatchVolumeContents(AmRequest *amReq);

    bool getNoNetwork() const {
        return noNetwork;
    }

  private:

    /**
     * set flag for network available.
     */
     bool noNetwork {false};

    /**
     * Shared ptrs to the DLT and DMT managers used
     * for deciding who to dispatch to.
     */
    boost::shared_ptr<DLTManager> dltMgr;
    boost::shared_ptr<DMTManager> dmtMgr;

    mutable VolumeDispatchTable dispatchTable;

    template<typename Msg>
    MultiPrimarySvcRequestPtr createMultiPrimaryRequest(fds_volid_t const& volId,
                                                        fds_uint64_t const dmt_ver,
                                                        boost::shared_ptr<Msg> const& payload,
                                                        MultiPrimarySvcRequestRespCb mpCb,
                                                        uint32_t timeout=0) const;
    template<typename Msg>
    MultiPrimarySvcRequestPtr createMultiPrimaryRequest(ObjectID const& objId,
                                                        DLT const* dlt,
                                                        boost::shared_ptr<Msg> const& payload,
                                                        MultiPrimarySvcRequestRespCb mpCb,
                                                        uint32_t timeout=0) const;
    template<typename Msg>
    FailoverSvcRequestPtr createFailoverRequest(fds_volid_t const& volId,
                                                fds_uint64_t const dmt_ver,
                                                boost::shared_ptr<Msg> const& payload,
                                                FailoverSvcRequestRespCb cb,
                                                uint32_t timeout=0) const;
    template<typename Msg>
    FailoverSvcRequestPtr createFailoverRequest(ObjectID const& objId,
                                                DLT const* dlt,
                                                boost::shared_ptr<Msg> const& payload,
                                                FailoverSvcRequestRespCb cb,
                                                uint32_t timeout=0) const;
    /**
     * Callback for delete blob responses.
     */
    void abortBlobTxCb(AmRequest *amReq,
                       MultiPrimarySvcRequest* svcReq,
                       const Error& error,
                       boost::shared_ptr<std::string> payload);

    /**
     * Callback for delete blob responses.
     */
    void deleteBlobCb(AmRequest *amReq,
                      MultiPrimarySvcRequest* svcReq,
                      const Error& error,
                      boost::shared_ptr<std::string> payload);

    /**
     * Callback for get blob responses.
     */
    void getObjectCb(AmRequest* amReq,
                     FailoverSvcRequest* svcReq,
                     const Error& error,
                     boost::shared_ptr<std::string> payload);

    /**
     * Callback for catalog query responses.
     */
    void getQueryCatalogCb(AmRequest* amReq,
                           FailoverSvcRequest* svcReq,
                           const Error& error,
                           boost::shared_ptr<std::string> payload);

    /**
     * Callback for catalog query error checks from service layer.
     */
    fds_bool_t missingBlobStatusCb(AmRequest* amReq,
                                   const Error& error,
                                   boost::shared_ptr<std::string> payload);

    /**
     * Callback for set metadata on blob responses.
     */
    void setBlobMetadataCb(AmRequest *amReq,
                           MultiPrimarySvcRequest* svcReq,
                           const Error& error,
                           boost::shared_ptr<std::string> payload);

    /**
     * Callback for stat blob responses.
     */
    void statBlobCb(AmRequest *amReq,
                    FailoverSvcRequest* svcReq,
                    const Error& error,
                    boost::shared_ptr<std::string> payload);

    /**
     * Callback for update blob responses.
     */
    void updateCatalogOnceCb(AmRequest* amReq,
                             MultiPrimarySvcRequest* svcReq,
                             const Error& error,
                             boost::shared_ptr<std::string> payload);
    void updateCatalogCb(AmRequest* amReq,
                         MultiPrimarySvcRequest* svcReq,
                         const Error& error,
                         boost::shared_ptr<std::string> payload);

    /**
     * Callback for put object responses.
     */
    void putObjectCb(AmRequest* amReq,
                     MultiPrimarySvcRequest* svcReq,
                     const Error& error,
                     boost::shared_ptr<std::string> payload);

    /**
     * Callback for stat blob responses.
     */
    void volumeContentsCb(AmRequest *amReq,
                          FailoverSvcRequest* svcReq,
                          const Error& error,
                          boost::shared_ptr<std::string> payload);

    /**
     * Configurable timeouts and defaults (ms)
     */
    uint32_t message_timeout_default { 0 };
    uint32_t message_timeout_io { 0 };

    /**
     * Number of primary replicas (right now DM/SM are identical)
     */
    uint32_t numPrimaries;

    boost::shared_ptr<MockSvcHandler> mockHandler_;
    uint64_t mockTimeoutUs_  = 200;

    /**
     * Sets the configured request serialization.
     */
    void setSerialization(AmRequest* amReq, boost::shared_ptr<SvcRequestIf> svcReq);
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_AMDISPATCHER_H_
