/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_ACCESS_MGR_INCLUDE_AMDISPATCHER_H_
#define SOURCE_ACCESS_MGR_INCLUDE_AMDISPATCHER_H_

#include <deque>
#include <mutex>
#include <string>
#include <tuple>
#include "AmDataProvider.h"
#include <net/SvcRequest.h>
#include "concurrency/RwLock.h"

namespace fds {

/* Forward declarations */
struct StartBlobTxReq;
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
struct AmDispatcher :
    public AmDataProvider,
    public HasModuleProvider
{
    /**
     * The dispatcher takes a shared ptr to the DMT manager
     * which it uses when deciding who to dispatch to. The
     * DMT manager is still owned and updated to omClient.
     * TODO(Andrew): Make the dispatcher own this piece or
     * iterface with platform lib.
     */
    AmDispatcher(AmDataProvider* prev, CommonModuleProviderIf *modProvider);
    AmDispatcher(AmDispatcher const&)               = delete;
    AmDispatcher& operator=(AmDispatcher const&)    = delete;
    AmDispatcher(AmDispatcher &&)                   = delete;
    AmDispatcher& operator=(AmDispatcher &&)        = delete;
    ~AmDispatcher();

    /**
     * These are the Volume specific DataProvider routines.
     * Everything else is pass-thru.
     */
    Error registerVolume(VolumeDesc const& volDesc) override { return ERR_OK; }
    Error removeVolume(VolumeDesc const& volDesc) override;
    void start() override;
    Error retrieveVolDesc(std::string const& volume_name) override;
    void openVolume(AmRequest * amReq) override;
    void closeVolume(fds_volid_t vol_id, int64_t token) override;
    Error updateDlt(bool dlt_type, std::string& dlt_data, FDS_Table::callback_type const& cb) override;
    Error updateDmt(bool dmt_type, std::string& dmt_data, FDS_Table::callback_type const& cb) override;
    Error getDMT() override;
    Error getDLT() override;
    void statVolume(AmRequest * amReq) override;
    void setVolumeMetadata(AmRequest * amReq) override;
    void getVolumeMetadata(AmRequest * amReq) override;
    void abortBlobTx(AmRequest * amReq) override;
    void startBlobTx(AmRequest * amReq) override;
    void commitBlobTx(AmRequest * amReq) override;
    void putBlob(AmRequest * amReq) override;
    void putBlobOnce(AmRequest * amReq) override;
    void getObject(AmRequest * amReq) override;
    void deleteBlob(AmRequest * amReq) override;
    void getOffsets(AmRequest * amReq) override;
    void statBlob(AmRequest * amReq) override;
    void renameBlob(AmRequest * amReq) override;
    void setBlobMetadata(AmRequest * amReq) override;
    void volumeContents(AmRequest * amReq) override;

  private:

    /**
     * set flag for network available.
     */
     bool noNetwork {false};

    /**
     * FEATURE TOGGLE: Safe PutBlobOnce
     * Wed 19 Aug 2015 10:56:46 AM MDT
     */
    bool safe_atomic_write { false };

    /**
     * Shared ptrs to the DLT and DMT managers used
     * for deciding who to dispatch to.
     */
    boost::shared_ptr<DLTManager> dltMgr;
    boost::shared_ptr<DMTManager> dmtMgr;

    mutable VolumeDispatchTable dispatchTable;

    using dmt_ver_count_type = std::tuple<fds_uint64_t, size_t, std::deque<StartBlobTxReq*>>;
    using blob_id_type = std::pair<fds_volid_t, std::string>;
    using tx_map_barrier_type = std::map<blob_id_type, dmt_ver_count_type>;

    std::mutex tx_map_lock;
    tx_map_barrier_type tx_map_barrier;

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

    void _startBlobTx(AmRequest *amReq);

    void openVolumeCb(AmRequest* amReq,
                              MultiPrimarySvcRequest* svcReq,
                              const Error& error,
                              boost::shared_ptr<std::string> payload);


    /**
     * Callback for set volume metadata responses.
     */
    void setVolumeMetadataCb(AmRequest* amReq,
                             MultiPrimarySvcRequest* svcReq,
                             const Error& error,
                             boost::shared_ptr<std::string> payload);
    /**
     * Callback for get volume metadata responses.
     */
    void getVolumeMetadataCb(AmRequest* amReq,
                             FailoverSvcRequest* svcReq,
                             const Error& error,
                             boost::shared_ptr<std::string> payload);
    /**
     * Callback for start blob transaction responses.
     */
    void startBlobTxCb(AmRequest* amReq,
                       MultiPrimarySvcRequest* svcReq,
                       const Error& error,
                       boost::shared_ptr<std::string> payload);
    /**
     * Callback for commit blob transaction responses.
     */
    void commitBlobTxCb(AmRequest* amReq,
                       MultiPrimarySvcRequest* svcReq,
                       const Error& error,
                       boost::shared_ptr<std::string> payload);

    /**
     * Dispatches a rename blob request.
     */
    void renameBlobCb(AmRequest *amReq,
                      MultiPrimarySvcRequest* svcReq,
                      const Error& error,
                      boost::shared_ptr<std::string> payload);


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
     * Callback for stat volume responses.
     */
    void statVolumeCb(AmRequest* amReq,
                      FailoverSvcRequest* svcReq,
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
    void putBlobOnceCb(AmRequest* amReq,
                       MultiPrimarySvcRequest* svcReq,
                       const Error& error,
                       boost::shared_ptr<std::string> payload);
    void putBlobCb(AmRequest* amReq,
                   MultiPrimarySvcRequest* svcReq,
                   const Error& error,
                   boost::shared_ptr<std::string> payload);

    /**
     * Dipatches a put object request.
     */
    void putObject(AmRequest * amReq);

    /**
     * Callback for put object responses.
     */
    void dispatchObjectCb(AmRequest* amReq,
                          MultiPrimarySvcRequest* svcReq,
                          const Error& error,
                          boost::shared_ptr<std::string> payload);
    void putObjectCb(AmRequest* amReq, Error const error);

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
     * Drains any pending start tx requests into the svc layer with updated
     * dmt version
     */
    void releaseTx(blob_id_type const& blob_id);

    /**
     * Sets the configured request serialization.
     */
    void setSerialization(AmRequest* amReq, boost::shared_ptr<SvcRequestIf> svcReq);
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_AMDISPATCHER_H_
