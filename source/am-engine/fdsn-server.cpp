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
#include <am-engine/handlers/handlermappings.h>
#include <am-engine/handlers/responsehandler.h>

namespace fds {

/// Global singleton server object
FdsnServer gl_FdsnServer("Global FDSN Server");

/**
 * FDSN interface server class. Provides handlers for each
 * interface function.
 */
class FdsnIf : public apis::AmServiceIf {
  private:
    /// Pointer to AM's data API
    FDS_NativeAPI::ptr am_api;

  public:
    explicit FdsnIf(FDS_NativeAPI::ptr api)
            : am_api(api) {
    }

    typedef boost::shared_ptr<FdsnIf> ptr;

    void createVolume(const std::string& domainName,
                      const std::string& volumeName,
                      const apis::VolumePolicy& volumePolicy) {
    }

    void createVolume(boost::shared_ptr<std::string>& domainName,
                      boost::shared_ptr<std::string>& volumeName,
                      boost::shared_ptr<apis::VolumePolicy>& volumePolicy) {
        BucketContext bucket_ctx("host", *volumeName, "accessid", "secretkey");
        fds_uint32_t fakeMaxObjSize = 2 * 1024 * 1024;

        if ((volumePolicy->maxObjectSizeInBytes % fakeMaxObjSize) != 0) {
            apis::ApiException fdsE;
            fdsE.errorCode = apis::BAD_REQUEST;
            throw fdsE;
        }

        // The CreateBucket is synchronous...though still uses
        // the callback mechanism. The callback will throw
        // an exception if we get an error
        SimpleResponseHandler handler(__func__);
        am_api->CreateBucket(&bucket_ctx, CannedAclPrivate,
                             NULL, fn_ResponseHandler, &handler);
        handler.wait();
        handler.process();
    }

    void deleteVolume(const std::string& domainName,
                      const std::string& volumeName) {
    }

    void deleteVolume(boost::shared_ptr<std::string>& domainName,
                      boost::shared_ptr<std::string>& volumeName) {
        BucketContext bucket_ctx("host", *volumeName, "accessid", "secretkey");

        // The DeleteBucket is synchronous...though still uses
        // the callback mechanism. The callback will throw
        // an exception if we get an error
        SimpleResponseHandler handler(__func__);
        am_api->DeleteBucket(&bucket_ctx, NULL, fn_ResponseHandler, &handler);
        handler.wait();
        handler.process();
    }

    void statVolume(apis::VolumeDescriptor& _return,
                    const std::string& domainName,
                    const std::string& volumeName) {
    }

    void statVolume(apis::VolumeDescriptor& _return,
                    boost::shared_ptr<std::string>& domainName,
                    boost::shared_ptr<std::string>& volumeName) {
        BucketContext bucket_ctx("host", *volumeName, "accessid", "secretkey");
        BucketStatsResponseHandler handler(_return);
        am_api->GetBucketStats(&bucket_ctx, fn_BucketStatsHandler, &handler);
        handler.wait();
        handler.process();
    }

    void listVolumes(std::vector<apis::VolumeDescriptor> & _return,
                     const std::string& domainName) {
    }

    void listVolumes(std::vector<apis::VolumeDescriptor> & _return,
                     boost::shared_ptr<std::string>& domainName) {
    }

    void volumeStatus(apis::VolumeStatus& _return,
                      const std::string& domainName,
                      const std::string& volumeName) {
    }

    void volumeStatus(apis::VolumeStatus& _return,
                      boost::shared_ptr<std::string>& domainName,
                      boost::shared_ptr<std::string>& volumeName) {
    }

    void volumeContents(std::vector<apis::BlobDescriptor> & _return,
                        const std::string& domainName,
                        const std::string& volumeName,
                        const int32_t count,
                        const int64_t offset) {
    }

    void volumeContents(std::vector<apis::BlobDescriptor> & _return,
                        boost::shared_ptr<std::string>& domainName,
                        boost::shared_ptr<std::string>& volumeName,
                        boost::shared_ptr<int32_t>& count,
                        boost::shared_ptr<int64_t>& offset) {
    }

    void statBlob(apis::BlobDescriptor& _return,
                  const std::string& domainName,
                  const std::string& volumeName,
                  const std::string& blobName) {
    }

    void statBlob(apis::BlobDescriptor& _return,
                  boost::shared_ptr<std::string>& domainName,
                  boost::shared_ptr<std::string>& volumeName,
                  boost::shared_ptr<std::string>& blobName) {
        StatBlobResponseHandler* handler = new StatBlobResponseHandler();
        handler->blobDescriptor = &_return;

        native::StatBlobCallbackPtr cb(handler);
        am_api->StatBlob(*volumeName, *blobName, cb);
        LOGDEBUG << "waiting for statblob";
        handler->wait();
        LOGDEBUG << "processing statblob";
        handler->process();
    }

    void getBlob(std::string& _return,
                 const std::string& domainName,
                 const std::string& volumeName,
                 const std::string& blobName,
                 const int32_t length,
                 const apis::ObjectOffset& offset) {
    }

    void getBlob(std::string& _return,
                 boost::shared_ptr<std::string>& domainName,
                 boost::shared_ptr<std::string>& volumeName,
                 boost::shared_ptr<std::string>& blobName,
                 boost::shared_ptr<int32_t>& length,
                 boost::shared_ptr<apis::ObjectOffset>& objectOffset) {
        BucketContextPtr bucket_ctx(
            new BucketContext("host", *volumeName, "accessid", "secretkey"));

        // TODO(Andrew): Remove this hackey maxObjSize
        fds_uint64_t maxObjSize = 2 * 1024 * 1024;
        fds_uint64_t offset = objectOffset->value * maxObjSize;

        fds_verify(*length >= 0);
        fds_verify(offset >= 0);

        // Get a buffer of the requested size
        // TODO(Andrew): This should be a shared pointer
        // as we pass it around a lot
        char *buf = new char[*length];
        fds_verify(buf != NULL);

        // Create request context
        GetObjectResponseHandler getHandler;

        // Do async getobject
        // TODO(Andrew): The error path callback maybe called
        // in THIS thread's context...need to fix or handle that.
        // TODO(Andrew): Pass in the request context
        am_api->GetObject(bucket_ctx,
                          *blobName,
                          NULL,  // No get conditions
                          offset,
                          *length,
                          buf,
                          *length,  // We always allocate buf of the requested size
                          NULL,  // Not passing a context right now
                          fn_GetObjectHandler,
                          static_cast<void *>(&getHandler));

        // Wait for a signal from the callback thread
        getHandler.wait();

        if (getHandler.status != FDSN_StatusOK) {
            apis::ApiException fdsE;
            if (getHandler.status == FDSN_StatusEntityDoesNotExist) {
                fdsE.errorCode = apis::MISSING_RESOURCE;
            } else {
                fdsE.errorCode = apis::BAD_REQUEST;
            }
            throw fdsE;
        }
        _return.assign(buf, *length);

        delete[] buf;
    }

    void updateMetadata(const std::string& domainName,
                        const std::string& volumeName,
                        const std::string& blobName,
                        const std::map<std::string, std::string> & metadata) {
    }

    void updateMetadata(boost::shared_ptr<std::string>& domainName,
                        boost::shared_ptr<std::string>& volumeName,
                        boost::shared_ptr<std::string>& blobName,
                        boost::shared_ptr<
                            std::map<std::string, std::string> >& metadata) {
    }

    void updateBlob(const std::string& domainName,
                    const std::string& volumeName,
                    const std::string& blobName,
                    const std::string& bytes,
                    const int32_t length,
                    const apis::ObjectOffset& objectOffset,
                    const std::string& digest,
                    const bool isLast) {
    }

    void updateBlob(boost::shared_ptr<std::string>& domainName,
                    boost::shared_ptr<std::string>& volumeName,
                    boost::shared_ptr<std::string>& blobName,
                    boost::shared_ptr<std::string>& bytes,
                    boost::shared_ptr<int32_t>& length,
                    boost::shared_ptr<apis::ObjectOffset>& objectOffset,
                    boost::shared_ptr<std::string>& digest,
                    boost::shared_ptr<bool>& isLast) {
        BucketContext bucket_ctx("host", *volumeName, "accessid", "secretkey");

        // TODO(Andrew): Remove this hackey maxObjSize to force alignment
        fds_uint64_t maxObjSize = 2 * 1024 * 1024;
        fds_uint64_t offset = objectOffset->value * maxObjSize;

        fds_verify(*length >= 0);
        fds_verify(offset >= 0);

        // Create context handler
        PutObjectResponseHandler putHandler;

        // Setup put properties
        // TODO(Andrew): Since we don't currently handle the
        // transactions, always set a put properties and etag.
        PutPropertiesPtr putProps;
        putProps.reset(new PutProperties());
        putProps->md5 = ObjectID::ToHex(reinterpret_cast<const uint8_t *>(
            digest->c_str()),
                                        digest->size());

        // Do async putobject
        // TODO(Andrew): The error path callback maybe called
        // in THIS thread's context...need to fix or handle that.
        am_api->PutObject(&bucket_ctx,
                          *blobName,
                          putProps,
                          NULL,  // Not passing any context for the callback
                          const_cast<char *>(bytes->c_str()),
                          offset,
                          *length,
                          *isLast,
                          fn_PutObjectHandler,
                          static_cast<void *>(&putHandler));

        // Wait for a signal from the callback thread
        putHandler.wait();

        // Throw an exception if we didn't get an OK response
        if (putHandler.status != FDSN_StatusOK) {
            apis::ApiException fdsE;
            throw fdsE;
        }
    }

    void deleteBlob(const std::string& domainName,
                    const std::string& volumeName,
                    const std::string& blobName) {
    }

    void deleteBlob(boost::shared_ptr<std::string>& domainName,
                    boost::shared_ptr<std::string>& volumeName,
                    boost::shared_ptr<std::string>& blobName) {
        BucketContext bucket_ctx("host", *volumeName, "accessid", "secretkey");
        SimpleResponseHandler handler(__func__);
        am_api->DeleteObject(&bucket_ctx, *blobName, NULL, fn_ResponseHandler, &handler);
        handler.wait();
        handler.process();
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
    processor.reset(new apis::AmServiceProcessor(
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
}  // namespace fds
