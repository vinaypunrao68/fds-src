/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_AMVOLUMETABLE_H_
#define SOURCE_ACCESS_MGR_INCLUDE_AMVOLUMETABLE_H_

#include <string>
#include <unordered_map>
#include <vector>

#include <fds_types.h>
#include "AmDataProvider.h"
#include <blob/BlobTypes.h>
#include <util/Log.h>
#include <concurrency/RwLock.h>
#include "fds_module_provider.h"

namespace fds {

/* Forward declarations */
struct AmVolume;
struct FdsTimerTask;
struct WaitQueue;
class CommonModuleProviderIf;

struct AmVolumeTable :
    public HasLogger,
    public AmDataProvider,
    public HasModuleProvider
{
    using volume_ptr_type = std::shared_ptr<AmVolume>;

    /// Use logger that passed in to the constructor
    AmVolumeTable(AmDataProvider* prev, size_t const max_thrds, CommonModuleProviderIf *modProvider, fds_log *parent_log);
    AmVolumeTable(AmVolumeTable const& rhs) = delete;
    AmVolumeTable& operator=(AmVolumeTable const& rhs) = delete;
    ~AmVolumeTable() override;
    
    /**
     * These are the Volume specific DataProvider routines.
     * Everything else is pass-thru.
     */
    bool done() override;
    void start() override;
    void stop() override;
    Error modifyVolumePolicy(fds_volid_t const vol_uuid, const VolumeDesc& vdesc) override;
    void lookupVolume(std::string const volume_name) override;
    void registerVolume(VolumeDesc const& volDesc) override;
    void removeVolume(VolumeDesc const& volDesc) override;
    void openVolume(AmRequest *amReq) override;
    void closeVolume(AmRequest *amReq) override;
    void statVolume(AmRequest *amReq) override;
    void setVolumeMetadata(AmRequest *amReq) override;
    void getVolumeMetadata(AmRequest *amReq) override;
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

    // Renews volume leases
    void renewTokens();

 protected:

    /**
     * These are the response we actually care about seeing the results of
     */
    void lookupVolumeCb(VolumeDesc const volDesc, Error const error) override;
    void openVolumeCb(AmRequest *amReq, const Error error) override;
    void statVolumeCb(AmRequest *amReq, const Error error) override;

  private:
    /// volume uuid -> AmVolume map
    std::unordered_map<fds_volid_t, volume_ptr_type> volume_map;

    /// Protects volume_map
    mutable fds_rwlock map_rwlock;

    boost::shared_ptr<FdsTimerTask> renewal_task;

    std::unique_ptr<WaitQueue> read_queue;
    std::unique_ptr<WaitQueue> write_queue;

    /// Timer for token renewal
    boost::shared_ptr<FdsTimer> token_timer;

    std::chrono::duration<fds_uint32_t> vol_tok_renewal_freq {30};

    /**
     * Returns volume if found in volume map.
     * if volume does not exist, returns 'nullptr'
     */
    volume_ptr_type getVolume(const std::string& vol_name) const;
    volume_ptr_type getVolume(fds_volid_t const vol_uuid) const;

    volume_ptr_type ensureReadable(AmRequest *amReq);
    volume_ptr_type ensureWritable(AmRequest *amReq);

    void renewTokenCb(AmRequest *amReq, const Error& error);
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_AMVOLUMETABLE_H_
