/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_ACCESS_MGR_INCLUDE_AMPROCESSOR_H_
#define SOURCE_ACCESS_MGR_INCLUDE_AMPROCESSOR_H_

#include <string>
#include <fds_module.h>
#include <StorHvVolumes.h>
#include <StorHvQosCtrl.h>
#include <AmDispatcher.h>
#include "AmRequest.h"

namespace fds {

/**
 * Forward declarations
 */
struct AmTxManager;
struct RandNumGenerator;

/**
 * AM request processing layer. The processor handles state and
 * execution for AM requests.
 */
class AmProcessor : public Module {
  public:
    /**
     * The processor takes a shared ptr to a tx manager.
     * TODO(Andrew): Use a different structure than SHVolTable.
     */
    AmProcessor(const std::string &modName,
                AmDispatcher::shared_ptr _amDispatcher,
                std::shared_ptr<StorHvQosCtrl> _qosCtrl,
                std::shared_ptr<StorHvVolumeTable> _volTable);
    AmProcessor(AmProcessor const&) = delete;
    AmProcessor& operator=(AmProcessor const&) = delete;
    ~AmProcessor();

    typedef std::unique_ptr<AmProcessor> unique_ptr;

    /**
     * Module methods
     */
    int mod_init(SysParams const *const param)
    { Module::mod_init(param); return 0; }
    void mod_startup() {}
    void mod_shutdown() {}

    /**
     * Create object/metadata/offset caches for the given volume
     */
    Error addVolume(const VolumeDesc& volDesc);

    /**
     * Processes a stat volume request
     */
    void statVolume(AmRequest *amReq);

    /**
     * Callback for a stat volume request
     */
    void statVolumeCb(AmRequest *amReq,
                      const Error &error);

    /**
     * Processes a set volume metadata request
     */
    void setVolumeMetadata(AmRequest *amReq);

    /**
     * Processes a abort blob transaction
     */
    void abortBlobTx(AmRequest *amReq);

    /**
     * Callback for abort blob transaction
     */
    void abortBlobTxCb(AmRequest *amReq,
                       const Error &error);

    /**
     * Processes a start blob transaction
     */
    void startBlobTx(AmRequest *amReq);

    /**
     * Callback for start blob transaction
     */
    void startBlobTxCb(AmRequest *amReq,
                       const Error &error);

    /**
     * Processes a put blob request
     */
    void putBlob(AmRequest *amReq);

    /**
     * Callback for get blob request
     */
    void putBlobCb(AmRequest *amReq, const Error& error);

    /**
     * Processes a get blob request
     */
    void getBlob(AmRequest *amReq);

    /**
     * Callback for catalog query request
     */
    void queryCatalogCb(AmRequest *amReq, const Error& error);

    /**
     * Callback for get blob request
     */
    void getBlobCb(AmRequest *amReq, const Error& error);

    /**
     * Processes a delete blob request
     */
    void deleteBlob(AmRequest *amReq);

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
     * Processes a commit blob transaction
     */
    void commitBlobTx(AmRequest *amReq);

    /**
     * Callback for commit blob transaction
     */
    void commitBlobTxCb(AmRequest *amReq, const Error& error);

    /**
     * Generic callback for a few responses
     */
    void respond_and_delete(AmRequest *amReq, const Error& error)
    { respond(amReq, error); delete amReq; }

    void respond(AmRequest *amReq, const Error& error);

  private:

    /**
     * Return pointer to volume iff volume is not a snapshot
     */
    StorHvVolumeTable::volume_ptr_type getNoSnapshotVolume(AmRequest* amReq);

    /// Raw pointer to QoS controller
    // TODO(Andrew): Move this to unique once it's owned here.
    std::shared_ptr<StorHvQosCtrl> qosCtrl;

    /// Raw pointer to table of attached volumes
    // TODO(Andrew): Move this unique once it's owned here.
    // Also, probably want a simpler class structure
    std::shared_ptr<StorHvVolumeTable> volTable;

    /// Shared ptr to the dispatcher layer
    // TODO(Andrew): Decide if AM or Process owns this and make unique.
    // I'm leaning towards this layer owning it.
    AmDispatcher::shared_ptr amDispatcher;

    /// Shared ptr to the transaction manager
    std::unique_ptr<AmTxManager> txMgr;

    /// Unique ptr to a random num generator for tx IDs
    std::unique_ptr<RandNumGenerator> randNumGen;
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_AMPROCESSOR_H_
