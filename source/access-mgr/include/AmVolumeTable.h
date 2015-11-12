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
struct AmRequest;
struct AmVolume;
struct AmVolumeAccessToken;
class CommonModuleProviderIf;

struct AmVolumeTable : public HasLogger {
    using volume_ptr_type = std::shared_ptr<AmVolume>;

    /// Use logger that passed in to the constructor
    AmVolumeTable(CommonModuleProviderIf *modProvider, fds_log *parent_log);
    AmVolumeTable(AmVolumeTable const& rhs) = delete;
    AmVolumeTable& operator=(AmVolumeTable const& rhs) = delete;
    ~AmVolumeTable();
    
    /// Registers the callback we make to the transaction layer
    using processor_cb_type = std::function<void(AmRequest*, Error const&)>;
    void init(processor_cb_type const& complete_cb);
    void stop();

    Error registerVolume(VolumeDesc const& volDesc);
    Error removeVolume(fds_volid_t const volId);

    /**
     * Returns NULL is volume does not exist
     */
    volume_ptr_type getVolume(fds_volid_t const vol_uuid) const;

    /**
     * Returns volume if found in volume map.
     * if volume does not exist, returns 'nullptr'
     */
    volume_ptr_type getVolume(const std::string& vol_name) const;

    /**
     * Return volumes that we are currently attached to
     */
    std::vector<volume_ptr_type> getVolumes() const;

    Error modifyVolumePolicy(fds_volid_t vol_uuid, const VolumeDesc& vdesc);

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
    void putBlob(AmRequest *amReq);
    Error updateDlt(bool dlt_type, std::string& dlt_data, FDS_Table::callback_type const& cb);
    Error updateDmt(bool dmt_type, std::string& dmt_data, FDS_Table::callback_type const& cb);
    Error getDMT();
    Error getDLT();

  private:
    /// volume uuid -> AmVolume map
    std::unordered_map<fds_volid_t, volume_ptr_type> volume_map;

    /// Unique ptr to the transaction manager
    std::unique_ptr<AmTxManager> txMgr;

    /// Response callback
    processor_cb_type processor_cb;

    /// Protects volume_map
    mutable fds_rwlock map_rwlock;

    /// Timer for token renewal
    FdsTimer token_timer;

    std::chrono::duration<fds_uint32_t> vol_tok_renewal_freq {30};

    Error markIODone(AmRequest* amReq);

    bool ensureReadable(AmRequest *amReq) const;
    bool ensureWritable(AmRequest *amReq) const;

    void renewToken(const fds_volid_t vol_id);
    void renewTokenCb(AmRequest *amReq, const Error& error);
    void openVolumeCb(AmRequest *amReq, const Error& error);
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_AMVOLUMETABLE_H_
