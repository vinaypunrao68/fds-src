/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_AMVOLUMETABLE_H_
#define SOURCE_ACCESS_MGR_INCLUDE_AMVOLUMETABLE_H_

#include <string>
#include <mutex>
#include <unordered_map>
#include <vector>

#include <fds_error.h>
#include <fds_types.h>
#include <fds_table.h>
#include "fds_timer.h"
#include <qos_ctrl.h>
#include <blob/BlobTypes.h>
#include <fds_qos.h>
#include <util/Log.h>
#include <concurrency/RwLock.h>

namespace fds {

/* Forward declarations */
struct AmTxManager;
struct AmQoSCtrl;
struct AmRequest;
struct AmVolume;
struct AmVolumeAccessToken;
struct WaitQueue;
class CommonModuleProviderIf;

struct AmVolumeTable : public HasLogger {
    using volume_ptr_type = std::shared_ptr<AmVolume>;

    /// Use logger that passed in to the constructor
    AmVolumeTable(CommonModuleProviderIf *modProvider, fds_log *parent_log);
    AmVolumeTable(AmVolumeTable const& rhs) = delete;
    AmVolumeTable& operator=(AmVolumeTable const& rhs) = delete;
    ~AmVolumeTable();
    
    /// Registers the callback we make to the transaction layer
    using processor_en_type = std::function<void(AmRequest*)>;
    using processor_cb_type = std::function<void(AmRequest*, Error const&)>;
    void init(processor_en_type enqueue_cb, processor_cb_type complete_cb);
    void stop();

    Error registerVolume(const VolumeDesc& volDesc);
    Error removeVolume(std::string const& volName, fds_volid_t const volId);
    Error removeVolume(const VolumeDesc& volDesc)
        { return removeVolume(volDesc.name, volDesc.volUUID); }

    /**
     * Returns NULL is volume does not exist
     */
    volume_ptr_type getVolume(fds_volid_t const vol_uuid) const;

    /**
     * Return volumes that we are currently attached to
     */
    std::vector<volume_ptr_type> getVolumes() const;

    /**
     * Returns volume uuid if found in volume map.
     * if volume does not exist, returns 'invalid_vol_id'
     */
    fds_volid_t getVolumeUUID(const std::string& vol_name) const;

    Error modifyVolumePolicy(fds_volid_t vol_uuid,
                             const VolumeDesc& vdesc);

    Error enqueueRequest(AmRequest* amReq);
    bool drained();

    Error updateQoS(long int const* rate,
                    float const* throttle);

    /** These are here as a pass-thru to tx manager until we have stackable
     * interfaces */
    Error attachVolume(std::string const& volume_name);
    void openVolume(AmRequest *amReq);
    void statVolume(AmRequest *amReq);
    void setVolumeMetadata(AmRequest *amReq);
    void getVolumeMetadata(AmRequest *amReq);
    void volumeContents(AmRequest *amReq);
    void startBlobTx(AmRequest *amReq);
    void commitBlobTx(AmRequest *amReq);
    void abortBlobTx(AmRequest *amReq);
    void statBlob(AmRequest *amReq);
    void setBlobMetadata(AmRequest *amReq);
    void deleteBlob(AmRequest *amReq);
    void renameBlob(AmRequest *amReq);
    void getBlob(AmRequest *amReq);
    void updateCatalog(AmRequest *amReq);
    Error updateDlt(bool dlt_type, std::string& dlt_data, FDS_Table::callback_type const& cb);
    Error updateDmt(bool dmt_type, std::string& dmt_data, FDS_Table::callback_type const& cb);
    Error getDMT();
    Error getDLT();

  private:
    /// volume uuid -> AmVolume map
    std::unordered_map<fds_volid_t, volume_ptr_type> volume_map;

    std::unique_ptr<WaitQueue> wait_queue;

    /// QoS Module
    std::unique_ptr<AmQoSCtrl> qos_ctrl;

    /// Unique ptr to the transaction manager
    std::unique_ptr<AmTxManager> txMgr;

    /// Response callback
    processor_cb_type processor_cb;

    /// Protects volume_map
    mutable fds_rwlock map_rwlock;

    /// Timer for token renewal
    FdsTimer token_timer;

    std::chrono::duration<fds_uint32_t> vol_tok_renewal_freq {30};

    /**
     * FEATURE TOGGLE: Single AM Enforcement
     * Wed 01 Apr 2015 01:52:55 PM PDT
     */
    bool volume_open_support { true };

    Error markIODone(AmRequest* amReq);

    bool ensureReadable(AmRequest *amReq) const;
    bool ensureWritable(AmRequest *amReq) const;

    void renewToken(const fds_volid_t vol_id);
    void attachVolumeCb(AmRequest *amReq, const Error& error);
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_AMVOLUMETABLE_H_
