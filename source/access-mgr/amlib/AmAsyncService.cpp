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

#include <AmAsyncService.h>
#include "AmAsyncDataApi.cxx"
#include <responsehandler.h>

namespace fds {

struct RequestApi
    : public fds::apis::AsyncAmServiceRequestIf,
      public AmAsyncDataApi<boost::shared_ptr<apis::RequestId>>  {
    typedef AmAsyncDataApi<boost::shared_ptr<apis::RequestId>> fds_api_type;
    explicit RequestApi(boost::shared_ptr<AmAsyncResponseApi<boost::shared_ptr<apis::RequestId>>> response_api):
        fds_api_type(response_api)
    {}

    void abortBlobTx(boost::shared_ptr<apis::RequestId>& requestId, boost::shared_ptr<std::string>& domainName, boost::shared_ptr<std::string>& volumeName, boost::shared_ptr<std::string>& blobName, boost::shared_ptr<apis::TxDescriptor>& txDesc)  // NOLINT
    { fds_api_type::abortBlobTx(requestId, domainName, volumeName, blobName, txDesc); }
    void attachVolume(boost::shared_ptr<apis::RequestId>& requestId, boost::shared_ptr<std::string>& domainName, boost::shared_ptr<std::string>& volumeName)  // NOLINT
    { fds_api_type::attachVolume(requestId, domainName, volumeName); }
    void commitBlobTx(boost::shared_ptr<apis::RequestId>& requestId, boost::shared_ptr<std::string>& domainName, boost::shared_ptr<std::string>& volumeName, boost::shared_ptr<std::string>& blobName, boost::shared_ptr<apis::TxDescriptor>& txDesc)  // NOLINT
    { fds_api_type::commitBlobTx(requestId, domainName, volumeName, blobName, txDesc); }
    void deleteBlob(boost::shared_ptr<apis::RequestId>& requestId, boost::shared_ptr<std::string>& domainName, boost::shared_ptr<std::string>& volumeName, boost::shared_ptr<std::string>& blobName, boost::shared_ptr<apis::TxDescriptor>& txDesc)  // NOLINT
    { fds_api_type::deleteBlob(requestId, domainName, volumeName, blobName, txDesc); }
    void getBlob(boost::shared_ptr<apis::RequestId>& requestId, boost::shared_ptr<std::string>& domainName, boost::shared_ptr<std::string>& volumeName, boost::shared_ptr<std::string>& blobName, boost::shared_ptr<int32_t>& length, boost::shared_ptr<apis::ObjectOffset>& offset)  // NOLINT
    { fds_api_type::getBlob(requestId, domainName, volumeName, blobName, length, offset); }
    void getBlobWithMeta(boost::shared_ptr<apis::RequestId>& requestId, boost::shared_ptr<std::string>& domainName, boost::shared_ptr<std::string>& volumeName, boost::shared_ptr<std::string>& blobName, boost::shared_ptr<int32_t>& length, boost::shared_ptr<apis::ObjectOffset>& offset)  // NOLINT
    { fds_api_type::getBlobWithMeta(requestId, domainName, volumeName, blobName, length, offset); }
    void handshakeStart(boost::shared_ptr<apis::RequestId>& requestId, boost::shared_ptr<int32_t>& portNumber)  // NOLINT
    {
        auto api = boost::dynamic_pointer_cast<AmAsyncXdiResponse>(responseApi);
        if (api)
            api->handshakeComplete(requestId, portNumber);
    }
    void startBlobTx(boost::shared_ptr<apis::RequestId>& requestId, boost::shared_ptr<std::string>& domainName, boost::shared_ptr<std::string>& volumeName, boost::shared_ptr<std::string>& blobName, boost::shared_ptr<int32_t>& blobMode)  // NOLINT
    { fds_api_type::startBlobTx(requestId, domainName, volumeName, blobName, blobMode); }
    void statBlob(boost::shared_ptr<apis::RequestId>& requestId, boost::shared_ptr<std::string>& domainName, boost::shared_ptr<std::string>& volumeName, boost::shared_ptr<std::string>& blobName)  // NOLINT
    { fds_api_type::statBlob(requestId, domainName, volumeName, blobName); }
    void updateBlob(boost::shared_ptr<apis::RequestId>& requestId, boost::shared_ptr<std::string>& domainName, boost::shared_ptr<std::string>& volumeName, boost::shared_ptr<std::string>& blobName, boost::shared_ptr<apis::TxDescriptor>& txDesc, boost::shared_ptr<std::string>& bytes, boost::shared_ptr<int32_t>& length, boost::shared_ptr<apis::ObjectOffset>& objectOffset, boost::shared_ptr<bool>& isLast)  // NOLINT
    { fds_api_type::updateBlob(requestId, domainName, volumeName, blobName, txDesc, bytes, length, objectOffset, isLast); }   // NOLINT
    void updateBlobOnce(boost::shared_ptr<apis::RequestId>& requestId, boost::shared_ptr<std::string>& domainName, boost::shared_ptr<std::string>& volumeName, boost::shared_ptr<std::string>& blobName, boost::shared_ptr<int32_t>& blobMode, boost::shared_ptr<std::string>& bytes, boost::shared_ptr<int32_t>& length, boost::shared_ptr<apis::ObjectOffset>& objectOffset, boost::shared_ptr<std::map<std::string, std::string> >& metadata)  // NOLINT
    { fds_api_type::updateBlobOnce(requestId, domainName, volumeName, blobName, blobMode, bytes, length, objectOffset, metadata); }   // NOLINT
    void updateMetadata(boost::shared_ptr<apis::RequestId>& requestId, boost::shared_ptr<std::string>& domainName, boost::shared_ptr<std::string>& volumeName, boost::shared_ptr<std::string>& blobName, boost::shared_ptr<apis::TxDescriptor>& txDesc, boost::shared_ptr<std::map<std::string, std::string> >& metadata)  // NOLINT
    { fds_api_type::updateMetadata(requestId, domainName, volumeName, blobName, txDesc, metadata); }
    void volumeContents(boost::shared_ptr<apis::RequestId>& requestId, boost::shared_ptr<std::string>& domainName, boost::shared_ptr<std::string>& volumeName, boost::shared_ptr<int32_t>& count, boost::shared_ptr<int64_t>& offset)  // NOLINT
    { fds_api_type::volumeContents(requestId, domainName, volumeName, count, offset); }
    void volumeStatus(boost::shared_ptr<apis::RequestId>& requestId, boost::shared_ptr<std::string>& domainName, boost::shared_ptr<std::string>& volumeName)  // NOLINT
    { fds_api_type::volumeStatus(requestId, domainName, volumeName); }

    void you_should_not_be_here()
    { fds_panic("You shouldn't be here."); }
    void abortBlobTx(const apis::RequestId& requestId, const std::string& domainName, const std::string& volumeName, const std::string& blobName, const apis::TxDescriptor& txDesc)  // NOLINT
    { you_should_not_be_here(); }
    void attachVolume(const apis::RequestId& requestId, const std::string& domainName, const std::string& volumeName)  // NOLINT
    { you_should_not_be_here(); }
    void commitBlobTx(const apis::RequestId& requestId, const std::string& domainName, const std::string& volumeName, const std::string& blobName, const apis::TxDescriptor& txDesc)  // NOLINT
    { you_should_not_be_here(); }
    void deleteBlob(const apis::RequestId& requestId, const std::string& domainName, const std::string& volumeName, const std::string& blobName, const apis::TxDescriptor& txDesc)  // NOLINT
    { you_should_not_be_here(); }
    void getBlob(const apis::RequestId& requestId, const std::string& domainName, const std::string& volumeName, const std::string& blobName, const int32_t length, const apis::ObjectOffset& offset)  // NOLINT
    { you_should_not_be_here(); }
    void getBlobWithMeta(const apis::RequestId& requestId, const std::string& domainName, const std::string& volumeName, const std::string& blobName, const int32_t length, const apis::ObjectOffset& offset)  // NOLINT
    { you_should_not_be_here(); }
    void handshakeStart(const apis::RequestId& requestId, const int32_t portNumber)
    { you_should_not_be_here(); }
    void startBlobTx(const apis::RequestId& requestId, const std::string& domainName, const std::string& volumeName, const std::string& blobName, const int32_t blobMode)  // NOLINT
    { you_should_not_be_here(); }
    void statBlob(const apis::RequestId& requestId, const std::string& domainName, const std::string& volumeName, const std::string& blobName)  // NOLINT
    { you_should_not_be_here(); }
    void updateMetadata(const apis::RequestId& requestId, const std::string& domainName, const std::string& volumeName, const std::string& blobName, const apis::TxDescriptor& txDesc, const std::map<std::string, std::string> & metadata)  // NOLINT
    { you_should_not_be_here(); }
    void updateBlob(const apis::RequestId& requestId, const std::string& domainName, const std::string& volumeName, const std::string& blobName, const apis::TxDescriptor& txDesc, const std::string& bytes, const int32_t length, const apis::ObjectOffset& objectOffset, const bool isLast)  // NOLINT
    { you_should_not_be_here(); }
    void updateBlobOnce(const apis::RequestId& requestId, const std::string& domainName, const std::string& volumeName, const std::string& blobName, const int32_t blobMode, const std::string& bytes, const int32_t length, const apis::ObjectOffset& objectOffset, const std::map<std::string, std::string> & metadata)  // NOLINT
    { you_should_not_be_here(); }
    void volumeContents(const apis::RequestId& requestId, const std::string& domainName, const std::string& volumeName, const int32_t count, const int64_t offset)  // NOLINT
    { you_should_not_be_here(); }
    void volumeStatus(const apis::RequestId& requestId, const std::string& domainName, const std::string& volumeName)  // NOLINT
    { you_should_not_be_here(); }
};


class AsyncAmServiceRequestIfCloneFactory
    : virtual public apis::AsyncAmServiceRequestIfFactory
{
    using request_if = apis::AsyncAmServiceRequestIf;
 public:
    AsyncAmServiceRequestIfCloneFactory() {}
    ~AsyncAmServiceRequestIfCloneFactory() {}

    request_if* getHandler(const xdi_at::TConnectionInfo& connInfo);
    void releaseHandler(request_if* handler);
};

AsyncAmServiceRequestIfCloneFactory::request_if*
AsyncAmServiceRequestIfCloneFactory::getHandler(const xdi_at::TConnectionInfo& connInfo) {  // NOLINT
    // Get the underlying transport's socket so we can see what the host IP
    // address was of the incoming client.
    boost::shared_ptr<xdi_att::TSocket> sock =
        boost::dynamic_pointer_cast<xdi_att::TSocket>(connInfo.transport);
    fds_assert(sock.get());
    return new RequestApi(boost::make_shared<AmAsyncXdiResponse>(sock->getPeerAddress()));
}

void
AsyncAmServiceRequestIfCloneFactory::releaseHandler(request_if* handler) {
    delete handler;
}

AsyncDataServer::AsyncDataServer(const std::string &name, fds_uint32_t instanceId)
: Module(name.c_str()),
    port(8899 + instanceId)
{
    serverTransport.reset(new xdi_att::TServerSocket(port));
    transportFactory.reset(new xdi_att::TFramedTransportFactory());
    protocolFactory.reset(new xdi_atp::TBinaryProtocolFactory());

    FdsConfigAccessor conf(g_fdsprocess->get_fds_config(), "fds.am.");
}

/**
 * Module initialization
 */
int
AsyncDataServer::mod_init(SysParams const *const p) {
    Module::mod_init(p);
    return 0;
}

/**
 * Module startup
 */
void
AsyncDataServer::mod_startup() {
}

/**
 * Module shutdown
 */
void
AsyncDataServer::mod_shutdown() {
}

/**
 * Initializes the server component
 */
void
AsyncDataServer::init_server() {
    // Setup API processor
    processorFactory.reset(new apis::AsyncAmServiceRequestProcessorFactory(
            boost::make_shared<AsyncAmServiceRequestIfCloneFactory>() ));

    // processor, protocolFactory, port, threadManager));
    ttServer.reset(new xdi_ats::TThreadedServer(processorFactory,
                                                serverTransport,
                                                transportFactory,
                                                protocolFactory));

    try {
        LOGNORMAL << "Starting the async data server at port " << port;
        listen_thread.reset(new boost::thread(&xdi_ats::TThreadedServer::serve,
                                              ttServer.get()));
    } catch(const xdi_att::TTransportException& e) {
        LOGERROR << "unable to start async data server : " << e.what();
        fds_panic("Unable to start async data server...bailing out");
    }
}

void
AsyncDataServer::deinit_server() {
    fds_verify(listen_thread != NULL);
    ttServer->stop();
    listen_thread->join();
    listen_thread.reset();
}
}  // namespace fds
