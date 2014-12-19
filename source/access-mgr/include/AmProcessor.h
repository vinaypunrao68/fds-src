/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_ACCESS_MGR_INCLUDE_AMPROCESSOR_H_
#define SOURCE_ACCESS_MGR_INCLUDE_AMPROCESSOR_H_

#include <string>
#include <fds_module.h>
#include <FdsRandom.h>
#include <StorHvVolumes.h>
#include <StorHvQosCtrl.h>
#include <am-tx-mgr.h>
#include <AmDispatcher.h>
#include "AmRequest.h"

namespace fds {

/**
 * AM request processing layer. The processor handles state and
 * execution for AM requests.
 */
class AmProcessor : public Module, public boost::noncopyable {
  public:
    /**
     * The processor takes shared ptrs to a cache and tx manager.
     * TODO(Andrew): Remove the cache and tx from constructor
     * and make them owned by the processor. It's only this way
     * until we clean up the legacy path.
     * TODO(Andrew): Use a different structure than SHVolTable.
     */
    AmProcessor(const std::string &modName,
                AmDispatcher::shared_ptr _amDispatcher,
                StorHvQosCtrl     *_qosCtrl,
                StorHvVolumeTable *_volTable,
                AmTxManager::shared_ptr _amTxMgr,
                AmCache::shared_ptr _amCache);
    ~AmProcessor() {}
    typedef std::unique_ptr<AmProcessor> unique_ptr;

    /**
     * Module methods
     */
    int mod_init(SysParams const *const param)
    { Module::mod_init(param); return 0; }
    void mod_startup() {}
    void mod_shutdown() {}

    /**
     * Processes a get volume metadata request
     */
    void getVolumeMetadata(AmRequest *amReq);

    /**
     * Callback for a get volume metadata request
     */
    void getVolumeMetadataCb(AmRequest *amReq,
                             const Error &error);

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
     * Callback for catalog query request
     */
    void queryCatalogCb(AmRequest *amReq, const Error& error);

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

    void respond(AmRequest *amReq, const Error& error)
    { qosCtrl->markIODone(amReq); amReq->cb->call(error); }

  private:
    /// Raw pointer to QoS controller
    // TODO(Andrew): Move this to unique once it's owned here.
    StorHvQosCtrl *qosCtrl;

    /// Raw pointer to table of attached volumes
    // TODO(Andrew): Move this unique once it's owned here.
    // Also, probably want a simpler class structure
    StorHvVolumeTable *volTable;

    /// Shared ptr to the dispatcher layer
    // TODO(Andrew): Decide if AM or Process owns this and make unique.
    // I'm leaning towards this layer owning it.
    AmDispatcher::shared_ptr amDispatcher;

    /// Shared ptr to the transaction manager
    // TODO(Andrew): Move to unique once owned here.
    AmTxManager::shared_ptr txMgr;

    // Shared ptr to the data object cache
    // TODO(bszmyd): Tue 07 Oct 2014 08:43:26 PM MDT
    // Make this a unique pointer once owned here.
    AmCache::shared_ptr amCache;

    /// Unique ptr to a random num generator for tx IDs
    RandNumGenerator::unique_ptr randNumGen;
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_AMPROCESSOR_H_
