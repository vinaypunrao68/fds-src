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
#include <StorHvisorNet.h>

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
    /// Uturn test commit tx API
    fds_bool_t testUturnCommitTx;
    /// Uturn test abort tx API
    fds_bool_t testUturnAbortTx;
    /// Uturn test stat blob API
    fds_bool_t testUturnStatBlob;
    /// Uturn test get blob API
    fds_bool_t testUturnGetBlob;

    std::atomic_ullong      io_log_counter;
    fds_uint64_t            io_log_interval;

  public:
    explicit FdsnIf(FDS_NativeAPI::ptr api)
            : am_api(api),
              io_log_counter(0),
              io_log_interval(10) {
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
        testUturnCommitTx = conf.get_abs<bool>("fds.am.testing.uturn_amserv_committx");
        if (testUturnCommitTx == true) {
            LOGDEBUG << "Enabling uturn testing for AM service commit tx API";
        }
        testUturnAbortTx = conf.get_abs<bool>("fds.am.testing.uturn_amserv_aborttx");
        if (testUturnAbortTx == true) {
            LOGDEBUG << "Enabling uturn testing for AM service abort tx API";
        }
        testUturnStatBlob = conf.get_abs<bool>("fds.am.testing.uturn_amserv_statblob");
        if (testUturnStatBlob == true) {
            LOGDEBUG << "Enabling uturn testing for AM service stat blob API";
        }
        testUturnGetBlob = conf.get_abs<bool>("fds.am.testing.uturn_amserv_getblob");
        if (testUturnGetBlob == true) {
            LOGDEBUG << "Enabling uturn testing for AM service get blob API";
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
        ListBucketResponseHandler::ptr handler(new ListBucketResponseHandler(_return));
        STORHANDLER(GetBucketHandler, fds::FDS_LIST_BUCKET)->
                handleRequest(bucket_ctxt,
                              *offset, *count,
                              SHARED_DYN_CAST(Callback, handler));
        /*
        am_api->GetBucket(bucket_ctxt,
                          "", "", "", *count,
                          NULL,
                          fn_ListBucketHandler, &handler);
        */
        handler->wait();
        handler->process();
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
        if ((testUturnAll == true) ||
            (testUturnStatBlob == true)) {
            LOGDEBUG << "Uturn testing stat blob";
            _return.name = *blobName;
            _return.byteCount = 0;
            return;
        }

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
                  const std::string& blobName,
                  const fds_int32_t blobMode) {
    }

    void startBlobTx(apis::TxDescriptor& _return,
                     boost::shared_ptr<std::string>& domainName,
                     boost::shared_ptr<std::string>& volumeName,
                     boost::shared_ptr<std::string>& blobName,
                     boost::shared_ptr<fds_int32_t>& blobMode) {
        if ((testUturnAll == true) ||
            (testUturnStartTx == true)) {
            LOGDEBUG << "Uturn testing start blob tx";
            return;
        }

        StartBlobTxResponseHandler::ptr handler(
            new StartBlobTxResponseHandler(_return));

        am_api->StartBlobTx(*volumeName, *blobName, *blobMode, SHARED_DYN_CAST(Callback, handler));

        handler->wait();
        handler->process();
    }

    void commitBlobTx(const std::string& domainName,
                  const std::string& volumeName,
                  const std::string& blobName,
                  const apis::TxDescriptor& txDesc) {
    }

    void commitBlobTx(boost::shared_ptr<std::string>& domainName,
                     boost::shared_ptr<std::string>& volumeName,
                     boost::shared_ptr<std::string>& blobName,
                     boost::shared_ptr<apis::TxDescriptor>& txDesc) {
        if ((testUturnAll == true) ||
            (testUturnCommitTx == true)) {
            LOGDEBUG << "Uturn testing commit blob tx";
            return;
        }

        // Setup the transcation descriptor
        BlobTxId::ptr blobTxDesc(new BlobTxId(
            txDesc->txId));

        SimpleResponseHandler::ptr handler(new SimpleResponseHandler(__func__));

        am_api->CommitBlobTx(*volumeName, *blobName, blobTxDesc,
                SHARED_DYN_CAST(Callback, handler));

        handler->wait();

        if (handler->error != ERR_OK) {
            apis::ApiException fdsE;
            if (handler->error == ERR_BLOB_NOT_FOUND) {
                fdsE.errorCode = apis::MISSING_RESOURCE;
            } else {
                fdsE.errorCode = apis::BAD_REQUEST;
            }
            throw fdsE;
        }
    }

    void abortBlobTx(const std::string& domainName,
                  const std::string& volumeName,
                  const std::string& blobName,
                  const apis::TxDescriptor& txDesc) {
    }

    void abortBlobTx(boost::shared_ptr<std::string>& domainName,
                     boost::shared_ptr<std::string>& volumeName,
                     boost::shared_ptr<std::string>& blobName,
                     boost::shared_ptr<apis::TxDescriptor>& txDesc) {
        if ((testUturnAll == true) ||
            (testUturnAbortTx == true)) {
            LOGDEBUG << "Uturn testing abort blob tx";
            return;
        }

        SimpleResponseHandler::ptr handler(new SimpleResponseHandler(__func__));

        // Setup the transcation descriptor
        BlobTxId::ptr blobTxDesc(new BlobTxId(
            txDesc->txId));

        am_api->AbortBlobTx(*volumeName, *blobName, blobTxDesc,
                              SHARED_DYN_CAST(Callback, handler));

        handler->wait();
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
        if ((testUturnAll == true) ||
            (testUturnGetBlob == true)) {
            LOGDEBUG << "Uturn testing get blob";
            return;
        }

        BucketContextPtr bucket_ctx(
            new BucketContext("host", *volumeName, "accessid", "secretkey"));

        fds_verify(*length >= 0);
        fds_verify(objectOffset->value >= 0);

        // Get a buffer of the requested size
        // TODO(Andrew): This should be a shared pointer
        // as we pass it around a lot
        // TODO(Andrew): We should just resize the return string
        // rather than doing a reassign at the end
        char *buf = new char[*length];

        // Create request context
        // Set the pointer we want filled in the handler
        // TODO(Andrew): Get the correctly sized pointer directly
        // from the return string so we can avoid one extra copy.
        GetObjectResponseHandler::ptr getHandler(new GetObjectResponseHandler(buf));

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
                          SHARED_DYN_CAST(Callback, getHandler));

        // Wait for a signal from the callback thread
        getHandler->wait();

        if (getHandler->error != ERR_OK) {
            apis::ApiException fdsE;
            if (getHandler->error == ERR_BLOB_OFFSET_INVALID) {
                fdsE.errorCode = apis::MISSING_RESOURCE;
            } else {
                fdsE.errorCode = apis::BAD_REQUEST;
            }
            delete[] buf;
            throw fdsE;
        }
        _return.assign(getHandler->returnBuffer,
                       getHandler->returnSize);

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
        // Setup the transcation descriptor
        BlobTxId::ptr blobTxDesc(new BlobTxId(
            txDesc->txId));

        am_api->setBlobMetaData(*volumeName, *blobName, blobTxDesc, metaDataList,
                                SHARED_DYN_CAST(Callback, handler));
        handler->wait();
        LOGDEBUG << "set meta data returned";
        handler->process();
    }

    void updateBlobOnce(const std::string& domainName,
                        const std::string& volumeName,
                        const std::string& blobName,
                        const fds_int32_t blobMode,
                        const std::string& bytes,
                        const int32_t length,
                        const apis::ObjectOffset& objectOffset,
                        const std::map<std::string, std::string>& metadata) {
    }

    void updateBlobOnce(boost::shared_ptr<std::string>& domainName,
                        boost::shared_ptr<std::string>& volumeName,
                        boost::shared_ptr<std::string>& blobName,
                        boost::shared_ptr<fds_int32_t>& blobMode,
                        boost::shared_ptr<std::string>& bytes,
                        boost::shared_ptr<int32_t>& length,
                        boost::shared_ptr<apis::ObjectOffset>& objectOffset,
                        boost::shared_ptr< std::map<std::string, std::string> >& metadata) {
        if ((testUturnAll == true) ||
            (testUturnUpdateBlob == true)) {
            LOGDEBUG << "Uturn testing update blob";
            return;
        }

        fds_verify(*length >= 0);
        fds_verify(objectOffset->value >= 0);

        // Create context handler
        PutObjectResponseHandler putHandler;

        // Do async putobject
        // TODO(Andrew): The error path callback maybe called
        // in THIS thread's context...need to fix or handle that.
        am_api->PutBlobOnce(*volumeName,
                            *blobName,
                            const_cast<char *>(bytes->c_str()),
                            static_cast<fds_uint64_t>(objectOffset->value),
                            *length,
                            *blobMode,
                            metadata,
                            fn_PutObjectHandler,
                            static_cast<void *>(&putHandler));

        // Wait for a signal from the callback thread
        putHandler.wait();

        io_log_counter++;
        if ((io_log_counter % io_log_interval) == 0) {
            LOGNORMAL << "Finishing updateBlobOnce for blob " << *blobName
                      << " at object offset " << objectOffset->value
                      << ", log_counter = " << io_log_counter;
        }
        LOGDEBUG << "Finishing updateBlobOnce for blob " << *blobName
                 << " at object offset " << objectOffset->value
                 << " status:" << putHandler.status;

        // Throw an exception if we didn't get an OK response
        if (putHandler.status != ERR_OK) {
            apis::ApiException fdsE;
            throw fdsE;
        }
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

        io_log_counter++;
        if ((io_log_counter % io_log_interval) == 0) {
            LOGNORMAL << "Finishing updateBlob for blob " << *blobName
                      << " at object offset " << objectOffset->value
                      << ", log_counter = " << io_log_counter;
        }
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
        apis::TxDescriptor txnId;
        boost::shared_ptr<fds_int32_t> bmode(new fds_int32_t(0));
        startBlobTx(txnId, domainName, volumeName, blobName, bmode);
        BlobTxId::ptr blobTxId(new BlobTxId(txnId.txId));

        SimpleResponseHandler::ptr handler(new SimpleResponseHandler(__func__));
        STORHANDLER(DeleteBlobHandler, fds::FDS_DELETE_BLOB)->
                handleRequest(*volumeName, *blobName, blobTxId,
                              SHARED_DYN_CAST(Callback, handler));
        handler->wait();
        boost::shared_ptr<apis::TxDescriptor> txnPtr(new apis::TxDescriptor());
        txnPtr->txId = txnId.txId;
        try {
            handler->process();
        } catch(apis::ApiException& e) {
            LOGDEBUG << "delete failed with exception : " << e.what();
            try {
                abortBlobTx(domainName, volumeName, blobName, txnPtr);
            } catch(apis::ApiException& e) {
                LOGERROR << "exception @ abortBlobTx : " << e.what();
            }
            throw e;
        }
        commitBlobTx(domainName, volumeName, blobName, txnPtr);
    }
};

FdsnServer::FdsnServer(const std::string &name)
        : Module(name.c_str()),
          port(9988),
          numFdsnThreads(10) {
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
    FdsConfigAccessor conf(g_fdsprocess->get_fds_config(), "fds.am.");
    numFdsnThreads = conf.get<fds_uint32_t>("fdsn_server_threads");

    threadManager = xdi_atc::ThreadManager::newSimpleThreadManager(numFdsnThreads);
    threadFactory.reset(new xdi_atc::PosixThreadFactory());
    threadManager->threadFactory(threadFactory);
    threadManager->start();
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

    nbServer.reset(new xdi_ats::TNonblockingServer(
        processor, protocolFactory, port, threadManager));
    // server.reset(new xdi_ats::TThreadedServer(processor,
    //                                       serverTransport,
    //                                        transportFactory,
    //                                        protocolFactory));

    try {
        LOGNORMAL << "Starting the FDSN server with " << numFdsnThreads
                  << " threads...";
        listen_thread.reset(new boost::thread(&xdi_ats::TNonblockingServer::serve,
                                              nbServer.get()));
        // listen_thread.reset(new boost::thread(&xdi_ats::TThreadedServer::serve,
        //                                   server.get()));
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
