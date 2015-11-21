/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_AMVOLUMETABLE_H_
#define SOURCE_ACCESS_MGR_INCLUDE_AMVOLUMETABLE_H_

#include <string>
#include <mutex>
#include <unordered_map>
#include <vector>

#include <fds_types.h>
#include "fds_timer.h"
#include "AmDataProvider.h"
#include <qos_ctrl.h>
#include <blob/BlobTypes.h>
#include <fds_qos.h>
#include <util/Log.h>
#include <concurrency/RwLock.h>
#include "fds_module_provider.h"

namespace fds {

/* Forward declarations */
struct AmVolume;
class CommonModuleProviderIf;

struct AmVolumeTable :
    public HasLogger,
    public AmDataProvider,
    public HasModuleProvider
{
    using volume_ptr_type = std::shared_ptr<AmVolume>;

    /// Use logger that passed in to the constructor
    AmVolumeTable(AmDataProvider* prev, CommonModuleProviderIf *modProvider, fds_log *parent_log);
    AmVolumeTable(AmVolumeTable const& rhs) = delete;
    AmVolumeTable& operator=(AmVolumeTable const& rhs) = delete;
    ~AmVolumeTable();
    
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


    /**
     * These are the Volume specific DataProvider routines.
     * Everything else is pass-thru.
     */
    void start() override;
    void stop() override;
    Error modifyVolumePolicy(fds_volid_t const vol_uuid, const VolumeDesc& vdesc) override;
    void registerVolume(VolumeDesc const& volDesc) override;
    Error removeVolume(VolumeDesc const& volDesc) override;
    void openVolume(AmRequest *amReq) override;
    void setVolumeMetadata(AmRequest *amReq) override;
    void volumeContents(AmRequest *amReq) override;
    void startBlobTx(AmRequest *amReq) override;
    void commitBlobTx(AmRequest *amReq) override;
    void statBlob(AmRequest *amReq) override;
    void setBlobMetadata(AmRequest *amReq) override;
    void deleteBlob(AmRequest *amReq) override;
    void renameBlob(AmRequest *amReq) override;
    void getBlob(AmRequest *amReq) override;
    void putBlob(AmRequest *amReq) override;
    void putBlobOnce(AmRequest *amReq) override;

 protected:

    /**
     * These are the response we actually care about seeing the results of
     */
    void openVolumeCb(AmRequest *amReq, const Error error) override;
    void statVolumeCb(AmRequest *amReq, const Error error) override;

  private:
    /// volume uuid -> AmVolume map
    std::unordered_map<fds_volid_t, volume_ptr_type> volume_map;

    /// Protects volume_map
    mutable fds_rwlock map_rwlock;

    /// Timer for token renewal
    boost::shared_ptr<FdsTimer> token_timer;

    std::chrono::duration<fds_uint32_t> vol_tok_renewal_freq {30};

    volume_ptr_type ensureReadable(AmRequest *amReq) const;
    volume_ptr_type ensureWritable(AmRequest *amReq) const;

    void renewToken(const fds_volid_t vol_id);
    void renewTokenCb(AmRequest *amReq, const Error& error);
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_AMVOLUMETABLE_H_
