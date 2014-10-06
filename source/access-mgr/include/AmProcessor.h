/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_ACCESS_MGR_INCLUDE_AMPROCESSOR_H_
#define SOURCE_ACCESS_MGR_INCLUDE_AMPROCESSOR_H_

#include <string>
#include <fds_module.h>
#include <StorHvVolumes.h>
#include <StorHvQosCtrl.h>
#include <am-tx-mgr.h>

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
                StorHvQosCtrl     *_qosCtrl,
                StorHvVolumeTable *_volTable,
                AmTxManager::shared_ptr _amTxMgr);
    ~AmProcessor();
    typedef std::unique_ptr<AmProcessor> unique_ptr;

    /**
     * Module methods
     */
    int mod_init(SysParams const *const param);
    void mod_startup();
    void mod_shutdown();

    /**
     * Processes a start blob transaction
     */
    void startBlobTx(AmQosReq *qosReq);

    /**
     * Callback for start blob transaction
     */
    void startBlobTxCb(AmQosReq* qosReq,
                       const Error& error);

  private:
    /// Raw pointer to QoS controller
    // TODO(Andrew): Move this to unique once it's owned here.
    StorHvQosCtrl *qosCtrl;

    /// Raw pointer to table of attached volumes
    // TODO(Andrew): Move this unique once it's owned here.
    // Also, probably want a simpler class structure
    StorHvVolumeTable *volTable;

    /// Shared ptr to the transaction manager
    // TODO(Andrew): Move to unique once owned here.
    AmTxManager::shared_ptr txMgr;
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_AMPROCESSOR_H_
