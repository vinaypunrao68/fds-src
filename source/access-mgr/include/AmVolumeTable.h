/*
 * Copyright 2013-2016 Formation Data Systems, Inc.
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
struct EPSvcRequest;
struct FdsTimerTask;
struct WaitQueue;

struct AmVolumeTable :
    public HasLogger,
    public AmDataProvider
{
    using volume_ptr_type = std::shared_ptr<AmVolume>;

    /// Use logger that passed in to the constructor
    AmVolumeTable(AmDataProvider* prev, size_t const max_thrds, fds_log *parent_log);
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
    Error modifyVolumePolicy(const VolumeDesc& vdesc) override;
    void registerVolume(VolumeDesc const& volDesc) override;
    void removeVolume(VolumeDesc const& volDesc) override;
    void openVolume(AmRequest *amReq) override;
    void closeVolume(AmRequest *amReq) override;
    void statVolume(AmRequest *amReq) override          { read(amReq, &AmDataProvider::statVolume);}
    void setVolumeMetadata(AmRequest *amReq) override   {write(amReq, &AmDataProvider::setVolumeMetadata);}
    void getVolumeMetadata(AmRequest *amReq) override   { read(amReq, &AmDataProvider::getVolumeMetadata);}
    void volumeContents(AmRequest *amReq) override      { read(amReq, &AmDataProvider::volumeContents);}
    void startBlobTx(AmRequest *amReq) override         {write(amReq, &AmDataProvider::startBlobTx);}
    void commitBlobTx(AmRequest *amReq) override        {write(amReq, &AmDataProvider::commitBlobTx);}
    void statBlob(AmRequest *amReq) override            { read(amReq, &AmDataProvider::statBlob);}
    void setBlobMetadata(AmRequest *amReq) override     {write(amReq, &AmDataProvider::setBlobMetadata);}
    void deleteBlob(AmRequest *amReq) override          {write(amReq, &AmDataProvider::deleteBlob);}
    void renameBlob(AmRequest *amReq) override          {write(amReq, &AmDataProvider::renameBlob);}
    void getBlob(AmRequest *amReq) override             { read(amReq, &AmDataProvider::getBlob);}
    void putBlob(AmRequest *amReq) override             {write(amReq, &AmDataProvider::putBlob);}
    void putBlobOnce(AmRequest *amReq) override         {write(amReq, &AmDataProvider::putBlobOnce);}

    // Renews volume leases
    void renewTokens();

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

    boost::shared_ptr<FdsTimerTask> renewal_task;

    std::unique_ptr<WaitQueue> read_queue;
    std::unique_ptr<WaitQueue> write_queue;

    /// Timer for token renewal
    boost::shared_ptr<FdsTimer> token_timer;

    std::chrono::duration<fds_uint32_t> vol_tok_renewal_freq {30};

    /**
     * FEATURE TOGGLE: "VolumeGroup" support
     * Fri Jan 15 10:25:00 2016
     */
    bool volume_grouping_support {false};

    void lookupVolume(std::string const volume_name);
    void lookupVolumeCb(std::string const volume_name,
                        EPSvcRequest* svcReq,
                        const Error& error,
                        boost::shared_ptr<std::string> payload);

    /**
     * Returns volume if found in volume map.
     * if volume does not exist, returns 'nullptr'
     */
    volume_ptr_type getVolume(const std::string& vol_name) const;
    volume_ptr_type getVolume(fds_volid_t const vol_uuid) const;

    void read(AmRequest *amReq, void (AmDataProvider::*func)(AmRequest*));
    void write(AmRequest *amReq, void (AmDataProvider::*func)(AmRequest*));
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_AMVOLUMETABLE_H_
