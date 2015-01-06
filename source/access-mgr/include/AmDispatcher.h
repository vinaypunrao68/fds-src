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

/* Forward declaarations */
class MockSvcHandler;

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
                 DLTManagerPtr _dltMgr,
                 DMTManagerPtr _dmtMgr);
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
     * Dispatches a get volume metadata request.
     */
    void dispatchGetVolumeMetadata(AmRequest *amReq);

    /**
     * Callback for get volume metadata responses.
     */
    void getVolumeMetadataCb(AmRequest* amReq,
                             FailoverSvcRequest* svcReq,
                             const Error& error,
                             boost::shared_ptr<std::string> payload);

    /**
     * Aborts a blob transaction request.
     */
    void dispatchAbortBlobTx(AmRequest *amReq);

    /**
     * Dipatches a start blob transaction request.
     */
    void dispatchStartBlobTx(AmRequest *amReq);

    /**
     * Callback for start blob transaction responses.
     */
    void startBlobTxCb(AmRequest* amReq,
                       QuorumSvcRequest* svcReq,
                       const Error& error,
                       boost::shared_ptr<std::string> payload);

    /**
     * Dispatches a commit blob transaction request.
     */
    void dispatchCommitBlobTx(AmRequest *amReq);

    /**
     * Callback for commit blob transaction responses.
     */
    void commitBlobTxCb(AmRequest* amReq,
                       QuorumSvcRequest* svcReq,
                       const Error& error,
                       boost::shared_ptr<std::string> payload);

    /**
     * Dispatches an update catalog request.
     */
    void dispatchUpdateCatalog(AmRequest *amReq);

    /**
     * Dispatches an update catalog once request.
     */
    void dispatchUpdateCatalogOnce(AmRequest *amReq);

    /**
     * Dipatches a put object request.
     */
    void dispatchPutObject(AmRequest *amReq);

    /**
     * Dipatches a get object request.
     */
    void dispatchGetObject(AmRequest *amReq);

    /**
     * Dispatches a delete blob transaction request.
     */
    void dispatchDeleteBlob(AmRequest *amReq);

    /**
     * Dipatches a query catalog request.
     */
    void dispatchQueryCatalog(AmRequest *amReq);

    /**
     * Dispatches a stat blob transaction request.
     */
    void dispatchStatBlob(AmRequest *amReq);

    /**
     * Dispatches a set metadata on blob transaction request.
     */
    void dispatchSetBlobMetadata(AmRequest *amReq);

    /**
     * Dispatches a volume contents (list bucket) transaction request.
     */
    void dispatchVolumeContents(AmRequest *amReq);

  private:
    /// Shared ptrs to the DLT and DMT managers used
    /// for deciding who to dispatch to
    DLTManagerPtr dltMgr;
    DMTManagerPtr dmtMgr;

    /**
     * Callback for delete blob responses.
     */
    void abortBlobTxCb(AmRequest *amReq,
                       QuorumSvcRequest* svcReq,
                       const Error& error,
                       boost::shared_ptr<std::string> payload);

    /**
     * Callback for delete blob responses.
     */
    void deleteBlobCb(AmRequest *amReq,
                      QuorumSvcRequest* svcReq,
                      const Error& error,
                      boost::shared_ptr<std::string> payload);

    /**
     * Callback for get blob responses.
     */
    void getObjectCb(AmRequest* amReq,
                     FailoverSvcRequest* svcReq,
                     const Error& error,
                     boost::shared_ptr<std::string> payload);

    /**
     * Callback for catalog query responses.
     */
    void getQueryCatalogCb(AmRequest* amReq,
                           FailoverSvcRequest* svcReq,
                           const Error& error,
                           boost::shared_ptr<std::string> payload);

    /**
     * Callback for set metadata on blob responses.
     */
    void setBlobMetadataCb(AmRequest *amReq,
                           QuorumSvcRequest* svcReq,
                           const Error& error,
                           boost::shared_ptr<std::string> payload);

    /**
     * Callback for stat blob responses.
     */
    void statBlobCb(AmRequest *amReq,
                    FailoverSvcRequest* svcReq,
                    const Error& error,
                    boost::shared_ptr<std::string> payload);

    /**
     * Callback for update blob responses.
     */
    void updateCatalogCb(AmRequest* amReq,
                         QuorumSvcRequest* svcReq,
                         const Error& error,
                         boost::shared_ptr<std::string> payload);

    /**
     * Callback for put object responses.
     */
    void putObjectCb(AmRequest* amReq,
                     QuorumSvcRequest* svcReq,
                     const Error& error,
                     boost::shared_ptr<std::string> payload);

    /**
     * Callback for stat blob responses.
     */
    void volumeContentsCb(AmRequest *amReq,
                          FailoverSvcRequest* svcReq,
                          const Error& error,
                          boost::shared_ptr<std::string> payload);



    boost::shared_ptr<MockSvcHandler> mockHandler_;
    uint64_t mockTimeoutUs_  = 200;
    bool mockTimeoutEnabled_ = false;
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_AMDISPATCHER_H_
