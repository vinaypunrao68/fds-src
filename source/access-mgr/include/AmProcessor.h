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
    ~AmProcessor();
    typedef std::unique_ptr<AmProcessor> unique_ptr;

    /**
     * Module methods
     */
    int mod_init(SysParams const *const param);
    void mod_startup();
    void mod_shutdown();

    /**
     * Processes a get volume metadata request
     */
    void getVolumeMetadata(AmQosReq *qosReq);

    /**
     * Callback for a get volume metadata request
     */
    void getVolumeMetadataCb(AmQosReq *qosReq,
                             const Error &error);

    /**
     * Processes a abort blob transaction
     */
    void abortBlobTx(AmQosReq *qosReq);

    /**
     * Callback for abort blob transaction
     */
    void abortBlobTxCb(AmQosReq *qosReq,
                       const Error &error);

    /**
     * Processes a start blob transaction
     */
    void startBlobTx(AmQosReq *qosReq);

    /**
     * Callback for start blob transaction
     */
    void startBlobTxCb(AmQosReq *qosReq,
                       const Error &error);

    /**
     * Processes a put blob request
     */
    void putBlob(AmQosReq *qosReq);

    /**
     * Callback for get blob request
     */
    void putBlobCb(AmQosReq *qosReq, const Error& error);

    /**
     * Processes a get blob request
     */
    void getBlob(AmQosReq *qosReq);

    /**
     * Callback for get blob request
     */
    void getBlobCb(AmQosReq *qosReq, const Error& error);

    /**
     * Processes a delete blob request
     */
    void deleteBlob(AmQosReq *qosReq);
    void deleteBlobCb(AmQosReq *qosReq, const Error& error);

    /**
     * Processes a stat blob request
     */
    void statBlob(AmQosReq *qosReq);
    void statBlobCb(AmQosReq *qosReq, const Error& error);

    /**
     * Processes a volumeContents (aka ListBucket) request
     */
    void volumeContents(AmQosReq *qosReq);
    void volumeContentsCb(AmQosReq *qosReq, const Error& error);

    /**
     * Callback for catalog query request
     */
    void queryCatalogCb(AmQosReq *qosReq, const Error& error);

    /**
     * Processes a commit blob transaction
     */
    void commitBlobTx(AmQosReq *qosReq);

    /**
     * Callback for commit blob transaction
     */
    void commitBlobTxCb(AmQosReq *qosReq, const Error& error);

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
