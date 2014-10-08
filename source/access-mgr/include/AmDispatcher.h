/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_ACCESS_MGR_INCLUDE_AMDISPATCHER_H_
#define SOURCE_ACCESS_MGR_INCLUDE_AMDISPATCHER_H_

#include <string>
#include <fds_module.h>
#include <AmCache.h>
#include <StorHvVolumes.h>
#include <net/SvcRequest.h>

namespace fds {

/**
 * AM FDSP request dispatcher and reciever. The dispatcher
 * does the work to send and receive AM network messages over
 * the service layer. The layer is stateless and does not
 * internal locking.
 */
class AmDispatcher : public Module, public boost::noncopyable {
  public:
    /**
     * The dispatcher takes a shared ptr to the DMT manager
     * which it uses when deciding who to dispatch to. The
     * DMT manager is still owned and updated to omClient.
     * TODO(Andrew): Make the dispatcher own this piece or
     * iterface with platform lib.
     */
    AmDispatcher(const std::string &modName,
                 OMgrClient *_om_client);
    ~AmDispatcher();
    typedef std::unique_ptr<AmDispatcher> unique_ptr;
    typedef boost::shared_ptr<AmDispatcher> shared_ptr;

    /**
     * Module methods
     */
    int mod_init(SysParams const *const param);
    void mod_startup();
    void mod_shutdown();

    /**
     * Dipatches a start blob transaction request.
     */
    void dispatchStartBlobTx(AmQosReq *qosReq);

    /**
     * Callback for start blob transaction responses.
     */
    void startBlobTxCb(AmQosReq* qosReq,
                       QuorumSvcRequest* svcReq,
                       const Error& error,
                       boost::shared_ptr<std::string> payload);

    /**
     * Dipatches a start blob transaction request.
     */
    void dispatchGetBlob(AmQosReq *qosReq);

    void dispatchQueryCatalog(AmQosReq *qosReq);

    /**
     * Callback for start blob transaction responses.
     */
//    void getBlobCb(AmQosReq* qosReq,
//                   QuorumSvcRequest* svcReq,
//                   const Error& error,
//                   boost::shared_ptr<std::string> payload);

  private:
    /// Shared ptr to the DMT manager used for deciding who
    /// to dispatch to
    DMTManagerPtr dmtMgr;

    /// Raw pointer to orchestration manager
    // TODO(bszmyd): Tue 07 Oct 2014 07:22:55 PM MDT
    // Make this unique once complete
    OMgrClient  *om_client;

    /// Uturn test all network requests
    fds_bool_t uturnAll;

    void getBlobCb(fds::AmQosReq* qosReq,
                   FailoverSvcRequest* svcReq,
                   const Error& error,
                   boost::shared_ptr<std::string> payload);

    void getBlobQueryCatalogCb(fds::AmQosReq* qosReq,
                               FailoverSvcRequest* svcReq,
                               const Error& error,
                               boost::shared_ptr<std::string> payload);
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_AMDISPATCHER_H_
