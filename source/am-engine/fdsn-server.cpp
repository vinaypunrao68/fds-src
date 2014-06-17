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
#include <fds_process.h>
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

    /// Uturn test all AM service APIs
    fds_bool_t testUturnAll;
    /// Uturn test start tx API
    fds_bool_t testUturnStartTx;
    /// Uturn test update blob API
    fds_bool_t testUturnUpdateBlob;
    /// Uturn test update metadata API
    fds_bool_t testUturnUpdateMeta;

  public:
    explicit FdsnIf(FDS_NativeAPI::ptr api)
            : am_api(api) {
        FdsConfigAccessor conf(g_fdsprocess->get_conf_helper());
        testUturnAll = conf.get_abs<bool>("fds.am.testing.uturn_amserv_all");
        if (testUturnAll == true) {
            LOGDEBUG << "Enabling uturn testing for all AM service APIs";
        }
        testUturnStartTx = conf.get_abs<bool>("fds.am.testing.uturn_amserv_starttx");
        if (testUturnStartTx == true) {
            LOGDEBUG << "Enabling uturn testing for AM service start tx API";
        }
        testUturnUpdateBlob = conf.get_abs<bool>("fds.am.testing.uturn_amserv_updateblob");
        if (testUturnUpdateBlob == true) {
            LOGDEBUG << "Enabling uturn testing for AM service update blob API";
        }
        testUturnUpdateMeta = conf.get_abs<bool>("fds.am.testing.uturn_amserv_updatemeta");
        if (testUturnUpdateMeta == true) {
            LOGDEBUG << "Enabling uturn testing for AM service update metadata API";
        }
    }

    typedef boost::shared_ptr<FdsnIf> ptr;

    void attachVolume(const std::string& domainName,
                      const std::string& volumeName) {
    }

    void attachVolume(boost::shared_ptr<std::string>& domainName,
                      boost::shared_ptr<std::string>& volumeName) {
        AttachVolumeResponseHandler::ptr handler(
            new AttachVolumeResponseHandler());

        am_api->attachVolume(*volumeName,
                             SHARED_DYN_CAST(Callback, handler));

        handler->wait();
        handler->process();
    }

    void volumeStatus(apis::VolumeStatus& _return,
                      const std::string& domainName,
                      const std::string& volumeName) {
    }

    void volumeStatus(apis::VolumeStatus& _return,
                      boost::shared_ptr<std::string>& domainName,
                      boost::shared_ptr<std::string>& volumeName) {
        LOGDEBUG << "volumeStatus for vol:" << *volumeName;
        StatVolumeResponseHandler::ptr handler(new StatVolumeResponseHandler(_return));
        am_api->GetVolumeMetaData(*volumeName, SHARED_DYN_CAST(Callback, handler));
        handler->wait();
        handler->process();
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
        BucketContext *bucket_ctxt = new BucketContext("host",
                                                       *volumeName,
                                                       "accessid",
                                                       "secretkey");
        ListBucketResponseHandler handler(_return);

        am_api->GetBucket(bucket_ctxt,
                          "", "", "", *count,
                          NULL,
                          fn_ListBucketHandler, &handler);
        handler.wait();
        handler.process();
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
        StatBlobResponseHandler::ptr handler(
            new StatBlobResponseHandler(_return));

        am_api->StatBlob(*volumeName,
                         *blobName,
                         SHARED_DYN_CAST(Callback, handler));

        handler->wait();
        handler->process();
    }

    void startBlobTx(apis::TxDescriptor& _return,
                  const std::string& domainName,
                  const std::string& volumeName,
                  const std::string& blobName) {
    }

    void startBlobTx(apis::TxDescriptor& _return,
                     boost::shared_ptr<std::string>& domainName,
                     boost::shared_ptr<std::string>& volumeName,
                     boost::shared_ptr<std::string>& blobName) {
        if ((testUturnAll == true) ||
            (testUturnStartTx == true)) {
            LOGDEBUG << "Uturn testing start blob tx";
            return;
        }
        StartBlobTxResponseHandler::ptr handler(
            new StartBlobTxResponseHandler(_return));

        am_api->StartBlobTx(*volumeName, *blobName, SHARED_DYN_CAST(Callback, handler));

        handler->wait();
        handler->process();
    }

    void commitBlobTx(const apis::TxDescriptor& txDesc) {
    }

    void commitBlobTx(boost::shared_ptr<apis::TxDescriptor>& txDesc) {
    }

    void abortBlobTx(const apis::TxDescriptor& txDesc) {
    }

    void abortBlobTx(boost::shared_ptr<apis::TxDescriptor>& txDesc) {
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

        fds_verify(*length >= 0);
        fds_verify(objectOffset->value >= 0);

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
                          static_cast<fds_uint64_t>(objectOffset->value),
                          *length,
                          buf,
                          *length,  // We always allocate buf of the requested size
                          NULL,  // Not passing a context right now
                          fn_GetObjectHandler,
                          static_cast<void *>(&getHandler));

        // Wait for a signal from the callback thread
        getHandler.wait();

        if (getHandler.status != ERR_OK) {
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
                        const apis::TxDescriptor& txDesc,
                        const std::map<std::string, std::string> & metadata) {
    }

    void updateMetadata(boost::shared_ptr<std::string>& domainName,
                        boost::shared_ptr<std::string>& volumeName,
                        boost::shared_ptr<std::string>& blobName,
                        boost::shared_ptr<apis::TxDescriptor>& txDesc,
                        boost::shared_ptr< std::map<std::string, std::string> >& metadata) {
        if ((testUturnAll == true) ||
            (testUturnUpdateMeta == true)) {
            LOGDEBUG << "Uturn testing update metadata";
            return;
        }
        SimpleResponseHandler::ptr handler(new SimpleResponseHandler(__func__));
        boost::shared_ptr<fpi::FDSP_MetaDataList> metaDataList(new fpi::FDSP_MetaDataList());
        LOGDEBUG << "received updateMetadata cmd";
        fpi::FDSP_MetaDataPair metaPair;
        for (auto const & meta : *metadata) {
            LOGDEBUG << meta.first << ":" << meta.second;
            metaPair.key = meta.first;
            metaPair.value = meta.second;
            metaDataList->push_back(metaPair);
        }

        am_api->setBlobMetaData(*volumeName, *blobName, metaDataList,
                                SHARED_DYN_CAST(Callback, handler));
        handler->wait();
        LOGDEBUG << "set meta data returned";
        handler->process();
    }

    void updateBlob(const std::string& domainName,
                    const std::string& volumeName,
                    const std::string& blobName,
                    const apis::TxDescriptor& txDesc,
                    const std::string& bytes,
                    const int32_t length,
                    const apis::ObjectOffset& objectOffset,
                    const bool isLast) {
    }

    void updateBlob(boost::shared_ptr<std::string>& domainName,
                    boost::shared_ptr<std::string>& volumeName,
                    boost::shared_ptr<std::string>& blobName,
                    boost::shared_ptr<apis::TxDescriptor>& txDesc,
                    boost::shared_ptr<std::string>& bytes,
                    boost::shared_ptr<int32_t>& length,
                    boost::shared_ptr<apis::ObjectOffset>& objectOffset,
                    boost::shared_ptr<bool>& isLast) {
        if ((testUturnAll == true) ||
            (testUturnUpdateBlob == true)) {
            LOGDEBUG << "Uturn testing update blob";
            return;
        }

        BucketContext bucket_ctx("host", *volumeName, "accessid", "secretkey");

        fds_verify(*length >= 0);
        fds_verify(objectOffset->value >= 0);

        // Create context handler
        PutObjectResponseHandler putHandler;

        // Setup the transcation descriptor
        BlobTxId::ptr blobTxDesc(new BlobTxId(
            txDesc->txId));

        // Do async putobject
        // TODO(Andrew): The error path callback maybe called
        // in THIS thread's context...need to fix or handle that.
        am_api->PutBlob(&bucket_ctx,
                        *blobName,
                        NULL,  // Not passing any put properties
                        NULL,  // Not passing any context for the callback
                        const_cast<char *>(bytes->c_str()),
                        static_cast<fds_uint64_t>(objectOffset->value),
                        *length,
                        blobTxDesc,
                        *isLast,
                        fn_PutObjectHandler,
                        static_cast<void *>(&putHandler));

        // Wait for a signal from the callback thread
        putHandler.wait();

        LOGDEBUG << "Finishing updateBlob for blob " << *blobName
                 << " at object offset " << objectOffset->value;

        // Throw an exception if we didn't get an OK response
        if (putHandler.status != ERR_OK) {
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

    // TODO(Andrew): Leave the server single threaded for now...
    threadManager = xdi_atc::ThreadManager::newSimpleThreadManager(1);
    threadFactory.reset(new xdi_atc::PosixThreadFactory());
    threadManager->threadFactory(threadFactory);
    threadManager->start();

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

    // server.reset(new xdi_ats::TNonblockingServer(
    //  processor, protocolFactory, 9988, threadManager));
    server.reset(new xdi_ats::TThreadedServer(processor,
                                              serverTransport,
                                              transportFactory,
                                              protocolFactory));

    try {
        LOGNORMAL << "Starting the FDSN server...";
        // listen_thread.reset(new boost::thread(&xdi_ats::TNonblockingServer::serve,
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
