/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <map>
#include <unordered_map>
#include <utility>
#include <string>
#include <vector>

#include <FdsCrypto.h>
#include <fds_uuid.h>
#include <concurrency/Mutex.h>
#include <am-engine/fdsn-server.h>

namespace fds {

/// Global singleton server object
FdsnServer gl_FdsnServer("Global FDSN Server");

static int
fdsn_updblob_cbfn(void *reqContext, fds_uint64_t bufferSize, fds_off_t offset,
                  char *buffer, void *callbackData, FDSN_Status status,
                  ErrorDetails* errDetails) {
    LOGCRITICAL << "Got a update blob callback!";

    gl_FdsnServer.notifyCallback(0);

    if (status != FDSN_StatusOK) {
        xdi::XdiException fdsE;
        throw fdsE;
    }

    return 0;
}

/*
static FDSN_Status
fdsn_getblob_cbfn(BucketContextPtr bucket_ctx,
                 void *req, fds_uint64_t bufSize, fds_off_t offset,
                 const char *buf, fds_uint64_t blobSize,
                 const std::string &blobEtag, void *cbData,
                 FDSN_Status status, ErrorDetails *errdetails) {
}
*/

static void
fdsn_crtvol_cbfn(FDSN_Status status, const ErrorDetails *err, void *arg) {
    LOGCRITICAL << "Got a create volumes callback!";

    if (status != FDSN_StatusOK) {
        xdi::XdiException fdsE;
        throw fdsE;
    }
}

static void
fdsn_delvol_cbfn(FDSN_Status status, const ErrorDetails *err, void *arg) {
    LOGDEBUG << "status:" << status << ":" << static_cast<int>(status);
    if (status != FDSN_StatusOK) {
        xdi::XdiException fdsE;
        throw fdsE;
    }
}

/**
 * Tracks the progress of a FDSN transaction
 */
/*
class FdsnTransCtx {
  public:
    typedef boost::shared_ptr<FdsnTransCtx> Ptr;

  private:

};
*/

/**
 * Tracks the progress of a single FDSN request
 */
class FdsnReqCtx {
  public:
    typedef hash::Md5 EtagGenerator;
    typedef boost::shared_ptr<FdsnReqCtx> Ptr;

  private:
    fds_uint64_t    fdsnReqId;
    FdsConditionPtr fdsnCv;
    fds_uint32_t    fdsnAckCount;
    FDSN_Status     fdsnStatus;
    EtagGenerator   fdsnEtag;

  public:
    explicit FdsnReqCtx(fds_uint64_t id)
            : fdsnReqId(id),
              fdsnCv(new fds_condition()),
              fdsnAckCount(0),
              fdsnStatus(FDSN_StatusOK) {
    }

    void updateEtag(const char *input, size_t length) {
        fdsnEtag.update(reinterpret_cast<const byte *>(input),
                        length);
    }
    void finishEtag(byte *digest) {
        fdsnEtag.final(digest);
    }
    fds_bool_t isDone() const {
        if (fdsnAckCount == 0) {
            return false;
        }
        // TODO(Andrew): For now, just expect a
        // single response...
        fds_verify(fdsnAckCount == 1);
        return true;
    }
    void receivedAck() {
        fdsnAckCount++;
    }
    FdsConditionPtr getCv() {
        return fdsnCv;
    }
};

/**
 * FDSN interface server class. Provides handlers for each
 * interface function.
 */
class FdsnIf : public xdi::AmShimIf {
  private:
    /// Pointer to AM's data API
    FDS_NativeAPI::ptr am_api;
    /// Mutex that manages shared access
    /// to the server
    fds_mutex          fdsnMtx;
    /// Generates new request ids - atomic for now...
    fds_atomic_ullong  fdsnReqCount;
    typedef std::unordered_map<fds_uint64_t,
                               FdsnReqCtx::Ptr> FdsnReqMap;
    /// Synchronizes outstanding req/resp contexts
    FdsnReqMap         fdsnReqMap;

  public:
    explicit FdsnIf(FDS_NativeAPI::ptr api)
            : am_api(api),
              fdsnReqCount(0) {
    }
    typedef boost::shared_ptr<FdsnIf> ptr;

    /**
     * Notifies a waiting thread that a callback
     * has been received for a specific request ID
     */
    void notifyCallback(fds_uint64_t reqId) {
        fds_scoped_lock slock(fdsnMtx);
        fds_verify(fdsnReqMap.count(reqId) > 0);
        FdsnReqCtx::Ptr fdsnCtx = fdsnReqMap[reqId];
        fdsnCtx->receivedAck();
        fdsnCtx->getCv()->notify_one();
    }

    void createVolume(const std::string& domainName,
                      const std::string& volumeName,
                      const xdi::VolumePolicy& volumePolicy) {
    }
    void createVolume(boost::shared_ptr<std::string>& domainName,  // NOLINT
                      boost::shared_ptr<std::string>& volumeName,
                      boost::shared_ptr<xdi::VolumePolicy>& volumePolicy) {
        BucketContext bucket_ctx("host", *volumeName, "accessid", "secretkey");

        // The CreateBucket is synchronous...though still uses
        // the callback mechanism. The callback will throw
        // an exception if we get an error
        am_api->CreateBucket(&bucket_ctx, CannedAclPrivate,
                             NULL, fdsn_crtvol_cbfn, this);
    }
    void deleteVolume(const std::string& domainName,
                      const std::string& volumeName) {
    }
    void deleteVolume(boost::shared_ptr<std::string>& domainName,  // NOLINT
                      boost::shared_ptr<std::string>& volumeName) {
        BucketContext bucket_ctx("host", *volumeName, "accessid", "secretkey");

        // The DeleteBucket is synchronous...though still uses
        // the callback mechanism. The callback will throw
        // an exception if we get an error
        am_api->DeleteBucket(&bucket_ctx, NULL, fdsn_delvol_cbfn, this);
    }
    void statVolume(xdi::VolumeDescriptor& _return,  // NOLINT
                    const std::string& domainName,
                    const std::string& volumeName) {
    }
    void statVolume(xdi::VolumeDescriptor& _return,  // NOLINT
                    boost::shared_ptr<std::string>& domainName,
                    boost::shared_ptr<std::string>& volumeName) {
    }
    void listVolumes(std::vector<xdi::VolumeDescriptor> & _return,
                     const std::string& domainName) {
    }
    void listVolumes(std::vector<xdi::VolumeDescriptor> & _return,
                     boost::shared_ptr<std::string>& domainName) {
    }
    void volumeStatus(xdi::VolumeStatus& _return,  // NOLINT
                      const std::string& domainName,
                      const std::string& volumeName) {
    }
    void volumeStatus(xdi::VolumeStatus& _return,  // NOLINT
                      boost::shared_ptr<std::string>& domainName,
                      boost::shared_ptr<std::string>& volumeName) {
    }
    void volumeContents(std::vector<xdi::BlobDescriptor> & _return,
                        const std::string& domainName,
                        const std::string& volumeName,
                        const int32_t count,
                        const int64_t offset) {
    }
    void volumeContents(std::vector<xdi::BlobDescriptor> & _return,
                        boost::shared_ptr<std::string>& domainName,
                        boost::shared_ptr<std::string>& volumeName,
                        boost::shared_ptr<int32_t>& count,
                        boost::shared_ptr<int64_t>& offset) {
    }
    void statBlob(xdi::BlobDescriptor& _return,  // NOLINT
                  const std::string& domainName,
                  const std::string& volumeName,
                  const std::string& blobName) {
    }
    void statBlob(xdi::BlobDescriptor& _return,  // NOLINT
                  boost::shared_ptr<std::string>& domainName,
                  boost::shared_ptr<std::string>& volumeName,
                  boost::shared_ptr<std::string>& blobName) {
    }
    void getBlob(std::string& _return,  // NOLINT
                 const std::string& domainName,
                 const std::string& volumeName,
                 const std::string& blobName,
                 const int32_t length,
                 const int64_t offset) {
    }
    void getBlob(std::string& _return,  // NOLINT
                 boost::shared_ptr<std::string>& domainName,
                 boost::shared_ptr<std::string>& volumeName,
                 boost::shared_ptr<std::string>& blobName,
                 boost::shared_ptr<int32_t>& length,
                 boost::shared_ptr<int64_t>& offset) {
    }
    void startBlobTx(xdi::Uuid& _return,  // NOLINT
                     const std::string& domainName,
                     const std::string& volumeName,
                     const std::string& blobName) {
    }
    void startBlobTx(xdi::Uuid& _return,  // NOLINT
                     boost::shared_ptr<std::string>& domainName,
                     boost::shared_ptr<std::string>& volumeName,
                     boost::shared_ptr<std::string>& blobName) {
        _return.high = fds_get_uuid64(*volumeName);
        _return.low  = fds_get_uuid64(*blobName);
    }
    void updateMetadata(const std::string& domainName,
                        const std::string& volumeName,
                        const std::string& blobName,
                        const xdi::Uuid& txUuid,
                        const std::map<std::string, std::string> & metadata) {
    }
    void updateMetadata(boost::shared_ptr<std::string>& domainName,  // NOLINT
                        boost::shared_ptr<std::string>& volumeName,
                        boost::shared_ptr<std::string>& blobName,
                        boost::shared_ptr<xdi::Uuid>& txUuid,
                        boost::shared_ptr<
                            std::map<std::string, std::string> >& metadata) {
    }
    void updateBlob(const std::string& domainName,
                    const std::string& volumeName,
                    const std::string& blobName,
                    const xdi::Uuid& txUuid,
                    const std::string& bytes,
                    const int32_t length,
                    const int64_t offset) {
    }
    void updateBlob(boost::shared_ptr<std::string>& domainName,  // NOLINT
                    boost::shared_ptr<std::string>& volumeName,
                    boost::shared_ptr<std::string>& blobName,
                    boost::shared_ptr<xdi::Uuid>& txUuid,
                    boost::shared_ptr<std::string>& bytes,
                    boost::shared_ptr<int32_t>& length,
                    boost::shared_ptr<int64_t>& offset) {
        BucketContext bucket_ctx("host", *volumeName, "accessid", "secretkey");

        fds_verify(*length >= 0);
        fds_verify(*offset >= 0);

        fds_scoped_lock slock(fdsnMtx);

        // Set async request/response handler
        fds_uint64_t reqId = fdsnReqCount.fetch_add(1);
        fds_verify(fdsnReqMap.count(reqId) == 0);
        FdsnReqCtx::Ptr fdsnCtx = FdsnReqCtx::Ptr(new FdsnReqCtx(reqId));
        fdsnReqMap[reqId] = fdsnCtx;

        // Setup put properties
        // TODO(Andrew): Since we don't currently handle the
        // transactions, always set a put properties and etag.
        // TODO(Andrew): Actually do the etag
        PutPropertiesPtr putProps;
        putProps.reset(new PutProperties());

        // Calculate the etag
        byte etagDigest[FdsnReqCtx::EtagGenerator::numDigestBytes];
        fdsnCtx->updateEtag(bytes->c_str(),
                           *length);
        fdsnCtx->finishEtag(etagDigest);
        // Get a hex string for the etag
        putProps->md5 = ObjectID::ToHex(etagDigest,
                                        FdsnReqCtx::EtagGenerator::
                                        numDigestBytes);

        // Do async putobject
        // TODO(Andrew): The error path callback maybe called
        // in THIS thread's context...need to fix or handle that.
        // TODO(Andrew): Do we need to request context anymore?
        fds_bool_t lastBuf = true;
        am_api->PutObject(&bucket_ctx,
                          *volumeName,
                          putProps,
                          NULL,  // Not passing any context for the callback
                          const_cast<char *>(bytes->c_str()),
                          *offset,
                          *length,
                          lastBuf,
                          fdsn_updblob_cbfn,
                          static_cast<void *>(this));

        // Wait for a signal from the callback thread
        while (fdsnCtx->isDone() == false) {
            LOGDEBUG << "Update blob " << *blobName
                     << " and waiting with lock";
            fdsnCtx->getCv()->wait(slock.boost());
            LOGDEBUG << "Update blob " << *blobName << " and awake!";
        }
        LOGDEBUG << "Update blob " << *blobName << " and done!";
        fdsnReqMap.erase(reqId);
    }
    void commit(const xdi::Uuid& txId) {
    }
    void commit(boost::shared_ptr<xdi::Uuid>& txId) {  // NOLINT
    }
    void deleteBlob(const std::string& domainName,
                    const std::string& volumeName,
                    const std::string& blobName) {
    }
    void deleteBlob(boost::shared_ptr<std::string>& domainName,  // NOLINT
                    boost::shared_ptr<std::string>& volumeName,
                    boost::shared_ptr<std::string>& blobName) {
    }
};

FdsnServer::FdsnServer(const std::string &name)
        : Module(name.c_str()),
          port(9988) {
    serverTransport.reset(new xdi_att::TServerSocket(port));
    transportFactory.reset(new xdi_att::TBufferedTransportFactory());
    protocolFactory.reset(new xdi_atp::TBinaryProtocolFactory());


    // server_->setServerEventHandler(event_handler_);
}

/**
 * Module initialization
 */
int
FdsnServer::mod_init(SysParams const *const p) {
    Module::mod_init(p);
    return 0;
}

/**
 * Module startup
 */
void
FdsnServer::mod_startup() {
}

/**
 * Module shutdown
 */
void
FdsnServer::mod_shutdown() {
}

/**
 * Initializes the server component
 */
void
FdsnServer::init_server(FDS_NativeAPI::ptr api) {
    fds_verify(api != NULL);
    am_api = api;

    // We init here becuse we need the apiobject to
    // init the processor and server
    fdsnInterface.reset(new FdsnIf(am_api));
    processor.reset(new xdi::AmShimProcessor(
        fdsnInterface));
    // event_handler_.reset(new ServerEventHandler(*this));
    server.reset(new xdi_ats::TThreadedServer(processor,
                                              serverTransport,
                                              transportFactory,
                                              protocolFactory));

    try {
        LOGNORMAL << "Starting the FDSN server...";
        listen_thread.reset(new boost::thread(&xdi_ats::TThreadedServer::serve,
                                              server.get()));
      } catch(const xdi_att::TTransportException& e) {
          LOGERROR << "unable to start FDSN server : " << e.what();
          fds_panic("Unable to start FDSN server...bailing out");
      }
}

void
FdsnServer::deinit_server() {
    fds_verify(listen_thread != NULL);
    listen_thread->join();
}

void
FdsnServer::notifyCallback(fds_uint64_t reqId) {
    fdsnInterface->notifyCallback(reqId);
}
}  // namespace fds
