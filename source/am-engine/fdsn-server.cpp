/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <map>
#include <unordered_map>
#include <utility>
#include <string>
#include <vector>

#include <fds_uuid.h>
#include <concurrency/Mutex.h>
#include <am-engine/fdsn-server.h>

namespace fds {

/// Global singleton server object
FdsnServer gl_FdsnServer("Global FDSN Server");

static void
fdsnsrv_putbucket_cbfn(FDSN_Status status, const ErrorDetails *err, void *arg)
{
    LOGCRITICAL << "Got a callback!";

    if (status == FDSN_StatusOK) {
        xdi::FdsException fdsE;
        throw fdsE;
    }
}

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
    /// Generates new requests - atomic for now...
    fds_atomic_ullong  fdsnReqCount;
    typedef boost::shared_ptr<fds_condition> fds_condition_ptr;
    typedef std::pair<fds_condition_ptr,
                      fds_uint32_t> FdsnReqPair;
    typedef std::unordered_map<fds_uint64_t,
                               FdsnReqPair> FdsnReqMap;
    /// Synchronizes outstanding req/resp pairs
    FdsnReqMap         fdsnReqMap;

  public:
    explicit FdsnIf(FDS_NativeAPI::ptr api)
            : am_api(api),
              fdsnReqCount(0) {
    }
    typedef boost::shared_ptr<FdsnIf> ptr;

    void notifyCallback(fds_uint64_t reqId) {
        fds_scoped_lock slock(fdsnMtx);
        fds_verify(fdsnReqMap.count(reqId) > 0);
        fdsnReqMap[reqId].second++;

        fdsnReqMap[reqId].first->notify_one();
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
                             NULL, fdsnsrv_putbucket_cbfn, this);
    }
    void deleteVolume(const std::string& domainName,
                      const std::string& volumeName) {
    }
    void deleteVolume(boost::shared_ptr<std::string>& domainName,  // NOLINT
                      boost::shared_ptr<std::string>& volumeName) {
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
        // Commented out for now...will use soon
        /*
        BucketContext bucket_ctx("host", *volumeName, "accessid", "secretkey");

        LOGCRITICAL << "Creating vol and getting lock";
        fds_scoped_lock slock(fdsnMtx);
        LOGCRITICAL << "Creating vol and got lock";
        fds_uint64_t reqId = fdsnReqCount.fetch_add(1);
        fds_verify(fdsnReqMap.count(reqId) == 0);
        fds_condition_ptr cv(new fds_condition());
        fdsnReqMap[reqId] = FdsnReqPair(cv, 0);
        am_api->CreateBucket(&bucket_ctx, CannedAclPrivate,
                             NULL, fdsnsrv_putbucket_cbfn, this);

        while (fdsnReqMap[reqId].second == 0) {
            LOGCRITICAL << "Creating vol and waiting with lock";
            cv->wait(slock.boost());
        }
        LOGCRITICAL << "Creating vol and awake!";
        fdsnReqMap.erase(reqId);

        LOGCRITICAL << "Returning RPC";
        */
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
