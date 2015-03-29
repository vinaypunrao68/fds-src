/*
 * Copyright 2013-2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_AMPROCESSOR_H_
#define SOURCE_ACCESS_MGR_INCLUDE_AMPROCESSOR_H_

#include <string>
#include <fds_module.h>
#include "fds_volume.h"
#include "fds_process.h"

namespace fds {

/**
 * Forward declarations
 */
struct AmDispatcher;
struct AmRequest;
struct AmTxManager;
struct AmVolume;
struct RandNumGenerator;

/**
 * AM request processing layer. The processor handles state and
 * execution for AM requests.
 */
class AmProcessor : public Module {
    using shutdown_cb_type = std::function<void(void)>;
  public:
    AmProcessor(const std::string &modName, shutdown_cb_type&& cb);
    AmProcessor(AmProcessor const&) = delete;
    AmProcessor& operator=(AmProcessor const&) = delete;
    ~AmProcessor();

    Error enqueueRequest(AmRequest* amReq);

    Error updateQoS(long int const* rate,
                    float const* throttle);

    /**
     * Module methods
     */
    int mod_init(SysParams const *const param)
    { Module::mod_init(param); return 0; }
    void mod_startup();
    void mod_shutdown() {}

    bool stop();

    /**
     * Create object/metadata/offset caches for the given volume
     */
    Error registerVolume(const VolumeDesc& volDesc);

    Error modifyVolumePolicy(fds_volid_t vol_uuid, const VolumeDesc& vdesc);

    /**
     * Remove object/metadata/offset caches for the given volume
     */
    Error removeVolume(const VolumeDesc& volDesc);

    /**
     * DMT/DLT update notifications
     */
    Error updateDlt(bool dlt_type, std::string& dlt_data, std::function<void (const Error&)> cb);
    Error updateDmt(bool dmt_type, std::string& dmt_data);

    bool isShuttingDown() const
    { return shut_down; }

  private:
    /// Unique ptr to the dispatcher layer
    std::unique_ptr<AmDispatcher> amDispatcher;

    /// Unique ptr to the transaction manager
    std::unique_ptr<AmTxManager> txMgr;

    /// Unique ptr to a random num generator for tx IDs
    std::unique_ptr<RandNumGenerator> randNumGen;

    shutdown_cb_type shutdown_cb;
    bool shut_down { false };

    void processBlobReq(AmRequest *amReq);

    std::shared_ptr<AmVolume> getVolume(AmRequest* amReq, bool const allow_snapshot=true);

    /**
     * Processes a abort blob transaction
     */
    void abortBlobTx(AmRequest *amReq);
    void abortBlobTxCb(AmRequest *amReq, const Error &error);

    /**
     * Processes a commit blob transaction
     */
    void commitBlobTx(AmRequest *amReq);
    void commitBlobTxCb(AmRequest *amReq, const Error& error);

    /**
     * Processes a delete blob request
     */
    void deleteBlob(AmRequest *amReq);

    /**
     * Processes a get blob request
     */
    void getBlob(AmRequest *amReq);
    void getBlobCb(AmRequest *amReq, const Error& error);

    /**
     * Processes a put blob request
     */
    void putBlob(AmRequest *amReq);
    void putBlobCb(AmRequest *amReq, const Error& error);

    /**
     * Processes a set volume metadata request
     */
    void setVolumeMetadata(AmRequest *amReq);

    /**
     * Processes a start blob transaction
     */
    void startBlobTx(AmRequest *amReq);
    void startBlobTxCb(AmRequest *amReq, const Error &error);

    /**
     * Processes a stat volume request
     */
    void statVolume(AmRequest *amReq);
    void statVolumeCb(AmRequest *amReq, const Error &error);

    /**
     * Callback for catalog query request
     */
    void queryCatalogCb(AmRequest *amReq, const Error& error);

    /**
     * Processes a set metadata on blob request
     */
    void setBlobMetadata(AmRequest *amReq);

    /**
     * Processes a stat blob request
     */
    void statBlob(AmRequest *amReq);
    void statBlobCb(AmRequest *amReq, const Error& error);

    /**
     * Processes a volumeContents (aka ListBucket) request
     */
    void volumeContents(AmRequest *amReq);

    /**
     * Generic callback for a few responses
     */
    inline void respond_and_delete(AmRequest *amReq, const Error& error);

    void respond(AmRequest *amReq, const Error& error);

};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_AMPROCESSOR_H_
