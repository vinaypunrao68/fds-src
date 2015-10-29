/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <string>
#include <vector>
#include "AmAsyncResponseApi.h"
#include "connector/xdi/AmAsyncXdi.h"
#include <blob/BlobTypes.h>

#include <arpa/inet.h>
#include <thrift/concurrency/ThreadManager.h>
#include <thrift/concurrency/PosixThreadFactory.h>
#include <thrift/server/TThreadPoolServer.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/server/TNonblockingServer.h>

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TTransportUtils.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TBufferTransports.h>

namespace fds {

namespace xdi_att = apache::thrift::transport;
namespace xdi_atp = apache::thrift::protocol;

std::mutex AmAsyncXdiResponse::map_lock;
AmAsyncXdiResponse::client_map AmAsyncXdiResponse::clients;

AmAsyncXdiResponse::AmAsyncXdiResponse(std::string const& server_ip)
        : serverIp(server_ip),
          serverPort(9876),
          asyncRespClient(nullptr)
{ }

AmAsyncXdiResponse::~AmAsyncXdiResponse() {
    // See if we're the last connected to this client
    // if so we need to remove it from the client vector
    std::lock_guard<std::mutex> g(map_lock);
    if (asyncRespClient.use_count() <= 2) {
        std::string tcp_nexus(serverIp + ":" + std::to_string(serverPort));
        auto client_it = clients.find(tcp_nexus);
        if (client_it != clients.end())
            clients.erase(client_it);
        asyncRespClient.reset();
    }
}

void
AmAsyncXdiResponse::initiateClientConnect() {
    std::string tcp_nexus(serverIp + ":" + std::to_string(serverPort));
    // First see if we're already connected to this client
    // if so, reuse response path
    std::lock_guard<std::mutex> g(map_lock);
    auto client_it = clients.find(tcp_nexus);

    // Lookup current client for this nexus
    if (client_it != clients.end())
    {
        LOGTRACE << "Found response channel!";
        // If the client is different than ours, use that one
        if (asyncRespClient == client_it->second) {
            // Otherwise trash this one...it must be broken.
            clients.erase(client_it);
            asyncRespClient.reset();
        } else {
            asyncRespClient = client_it->second;
        }
    }

    if (!asyncRespClient) {
        LOGNORMAL << "Setting up response channel to: "
                  << "tcp://" << serverIp << ":" << serverPort;
        // Setup the async response client
        boost::shared_ptr<xdi_att::TTransport> respSock(
            boost::make_shared<xdi_att::TSocket>(
                serverIp, serverPort));
        boost::shared_ptr<xdi_att::TFramedTransport> respTrans(
            boost::make_shared<xdi_att::TFramedTransport>(respSock));
        boost::shared_ptr<xdi_atp::TProtocol> respProto(
            boost::make_shared<xdi_atp::TBinaryProtocol>(respTrans));
        asyncRespClient = std::make_shared<apis::AsyncXdiServiceResponseClient>(respProto);
        clients[tcp_nexus] = asyncRespClient;
        respSock->open();
    }
}

void
AmAsyncXdiResponse::handshakeComplete(boost::shared_ptr<apis::RequestId>& requestId,
                       boost::shared_ptr<int32_t>& port) {
    serverPort = *port;
    xdiClientCall(&client_type::handshakeComplete, requestId);
}

void
AmAsyncXdiResponse::attachVolumeResp(const fpi::ErrorCode &error,
                                     boost::shared_ptr<apis::RequestId>& requestId,
                                     boost::shared_ptr<VolumeDesc>& volDesc,
                                     boost::shared_ptr<fpi::VolumeAccessMode>& mode) {
    if (fpi::OK != error) {
        boost::shared_ptr<std::string> message(boost::make_shared<std::string>());
        auto code = boost::make_shared<fpi::ErrorCode>(error);
        xdiClientCall(&client_type::completeExceptionally, requestId, code, message);
    } else {
        xdiClientCall(&client_type::attachVolumeResponse, requestId, mode);
    }
}

void
AmAsyncXdiResponse::detachVolumeResp(const fpi::ErrorCode &error,
                                     boost::shared_ptr<apis::RequestId>& requestId) {
    // XXX(bszmyd): Mon 22 Jun 2015 08:59:38 AM MDT
    // Not implemented for Xdi
    return;
}

void
AmAsyncXdiResponse::startBlobTxResp(const fpi::ErrorCode &error,
                                    boost::shared_ptr<apis::RequestId>& requestId,
                                    boost::shared_ptr<apis::TxDescriptor>& txDesc) {
    if (fpi::OK != error) {
        boost::shared_ptr<std::string> message(boost::make_shared<std::string>());
        auto code = boost::make_shared<fpi::ErrorCode>(error);
        xdiClientCall(&client_type::completeExceptionally, requestId, code, message);
    } else {
        xdiClientCall(&client_type::startBlobTxResponse, requestId, txDesc);
    }
}

void
AmAsyncXdiResponse::abortBlobTxResp(const fpi::ErrorCode &error,
                                    boost::shared_ptr<apis::RequestId>& requestId) {
    if (fpi::OK != error) {
        boost::shared_ptr<std::string> message(boost::make_shared<std::string>());
        auto code = boost::make_shared<fpi::ErrorCode>(error);
        xdiClientCall(&client_type::completeExceptionally, requestId, code, message);
    } else {
        xdiClientCall(&client_type::abortBlobTxResponse, requestId);
    }
}

void
AmAsyncXdiResponse::commitBlobTxResp(const fpi::ErrorCode &error,
                                     boost::shared_ptr<apis::RequestId>& requestId) {
    if (fpi::OK != error) {
        boost::shared_ptr<std::string> message(boost::make_shared<std::string>());
        auto code = boost::make_shared<fpi::ErrorCode>(error);
        xdiClientCall(&client_type::completeExceptionally, requestId, code, message);
    } else {
        xdiClientCall(&client_type::commitBlobTxResponse, requestId);
    }
}

void
AmAsyncXdiResponse::updateBlobResp(const fpi::ErrorCode &error,
                                   boost::shared_ptr<apis::RequestId>& requestId) {
    if (fpi::OK != error) {
        boost::shared_ptr<std::string> message(boost::make_shared<std::string>());
        auto code = boost::make_shared<fpi::ErrorCode>(error);
        xdiClientCall(&client_type::completeExceptionally, requestId, code, message);
    } else {
        xdiClientCall(&client_type::updateBlobResponse, requestId);
    }
}

void
AmAsyncXdiResponse::updateBlobOnceResp(const fpi::ErrorCode &error,
                                       boost::shared_ptr<apis::RequestId>& requestId) {
    if (fpi::OK != error) {
        boost::shared_ptr<std::string> message(boost::make_shared<std::string>());
        auto code = boost::make_shared<fpi::ErrorCode>(error);
        xdiClientCall(&client_type::completeExceptionally, requestId, code, message);
    } else {
        xdiClientCall(&client_type::updateBlobOnceResponse, requestId);
    }
}

void
AmAsyncXdiResponse::updateMetadataResp(const fpi::ErrorCode &error,
                                       boost::shared_ptr<apis::RequestId>& requestId) {
    if (fpi::OK != error) {
        boost::shared_ptr<std::string> message(boost::make_shared<std::string>());
        auto code = boost::make_shared<fpi::ErrorCode>(error);
        xdiClientCall(&client_type::completeExceptionally, requestId, code, message);
    } else {
        xdiClientCall(&client_type::updateMetadataResponse, requestId);
    }
}

void
AmAsyncXdiResponse::renameBlobResp(const fpi::ErrorCode &error,
                                   boost::shared_ptr<apis::RequestId>& requestId,
                                   boost::shared_ptr<fpi::BlobDescriptor>& blobDesc) {
    if (fpi::OK != error) {
        boost::shared_ptr<std::string> message(boost::make_shared<std::string>());
        auto code = boost::make_shared<fpi::ErrorCode>(error);
        xdiClientCall(&client_type::completeExceptionally, requestId, code, message);
    } else {
        xdiClientCall(&client_type::renameBlobResponse, requestId, blobDesc);
    }
}

void
AmAsyncXdiResponse::deleteBlobResp(const fpi::ErrorCode &error,
                                   boost::shared_ptr<apis::RequestId>& requestId) {
    if (fpi::OK != error) {
        boost::shared_ptr<std::string> message(boost::make_shared<std::string>());
        auto code = boost::make_shared<fpi::ErrorCode>(error);
        xdiClientCall(&client_type::completeExceptionally, requestId, code, message);
    } else {
        xdiClientCall(&client_type::deleteBlobResponse, requestId);
    }
}

void
AmAsyncXdiResponse::statBlobResp(const fpi::ErrorCode &error,
                                 boost::shared_ptr<apis::RequestId>& requestId,
                                 boost::shared_ptr<fpi::BlobDescriptor>& blobDesc) {
    if (fpi::OK != error) {
        boost::shared_ptr<std::string> message(boost::make_shared<std::string>());
        auto code = boost::make_shared<fpi::ErrorCode>(error);
        xdiClientCall(&client_type::completeExceptionally, requestId, code, message);
    } else {
        xdiClientCall(&client_type::statBlobResponse, requestId, blobDesc);
    }
}

void
AmAsyncXdiResponse::volumeStatusResp(const fpi::ErrorCode &error,
                                     boost::shared_ptr<apis::RequestId>& requestId,
                                     boost::shared_ptr<apis::VolumeStatus>& volumeStatus) {
    if (fpi::OK != error) {
        boost::shared_ptr<std::string> message(boost::make_shared<std::string>());
        auto code = boost::make_shared<fpi::ErrorCode>(error);
        xdiClientCall(&client_type::completeExceptionally, requestId, code, message);
    } else {
        xdiClientCall(&client_type::volumeStatus, requestId, volumeStatus);
    }
}

void
AmAsyncXdiResponse::volumeContentsResp(
        const fpi::ErrorCode &error,
        boost::shared_ptr<apis::RequestId>& requestId,
        boost::shared_ptr<std::vector<fpi::BlobDescriptor>>& volContents,
        boost::shared_ptr<std::vector<std::string>>& skippedPrefixes) {
    if (fpi::OK != error) {
        boost::shared_ptr<std::string> message(boost::make_shared<std::string>());
        auto code = boost::make_shared<fpi::ErrorCode>(error);
        xdiClientCall(&client_type::completeExceptionally, requestId, code, message);
    } else {
        xdiClientCall(&client_type::volumeContents, requestId, volContents, skippedPrefixes);
    }
}

void
AmAsyncXdiResponse::setVolumeMetadataResp(const fpi::ErrorCode &error,
                                          boost::shared_ptr<apis::RequestId>& requestId) {
    if (fpi::OK != error) {
        boost::shared_ptr<std::string> message(boost::make_shared<std::string>());
        auto code = boost::make_shared<fpi::ErrorCode>(error);
        xdiClientCall(&client_type::completeExceptionally, requestId, code, message);
    } else {
        xdiClientCall(&client_type::setVolumeMetadataResponse, requestId);
    }
}

void
AmAsyncXdiResponse::getVolumeMetadataResp(const fpi::ErrorCode &error,
                                          boost::shared_ptr<apis::RequestId>& requestId,
                                          api_type::shared_meta_type& metadata) {
    if (fpi::OK != error) {
        boost::shared_ptr<std::string> message(boost::make_shared<std::string>());
        auto code = boost::make_shared<fpi::ErrorCode>(error);
        xdiClientCall(&client_type::completeExceptionally, requestId, code, message);
    } else {
        xdiClientCall(&client_type::getVolumeMetadataResponse, requestId, metadata);
    }
}

void
AmAsyncXdiResponse::getBlobResp(const fpi::ErrorCode &error,
                                boost::shared_ptr<apis::RequestId>& requestId,
                                const boost::shared_ptr<std::vector<boost::shared_ptr<std::string>>>& bufs,
                                int& length) {
    static auto empty_buffer = boost::make_shared<std::string>(0, 0x00);
    if (fpi::OK != error) {
        boost::shared_ptr<std::string> message(boost::make_shared<std::string>());
        auto code = boost::make_shared<fpi::ErrorCode>(error);
        xdiClientCall(&client_type::completeExceptionally, requestId, code, message);
    } else {
        // TODO(bszmyd): Mon 27 Apr 2015 06:17:05 PM MDT
        // When Xdi and AmProc support vectored reads, return the whole
        // vector, not just the front element. For now assume one object.
        // A nullptr (with ERR_OK), indicates a zero'd out object
        auto& buf = (bufs && !bufs->empty()) ? bufs->front() : empty_buffer;
        xdiClientCall(&client_type::getBlobResponse, requestId, buf);
    }
}

void
AmAsyncXdiResponse::getBlobWithMetaResp(const fpi::ErrorCode &error,
                                        boost::shared_ptr<apis::RequestId>& requestId,
                                        const boost::shared_ptr<std::vector<boost::shared_ptr<std::string>>>& bufs,
                                        int& length,
                                        boost::shared_ptr<fpi::BlobDescriptor>& blobDesc) {
    static auto empty_buffer = boost::make_shared<std::string>(0, 0x00);
    if (fpi::OK != error) {
        boost::shared_ptr<std::string> message(boost::make_shared<std::string>());
        auto code = boost::make_shared<fpi::ErrorCode>(error);
        xdiClientCall(&client_type::completeExceptionally, requestId, code, message);
    } else {
        // TODO(bszmyd): Mon 27 Apr 2015 06:17:05 PM MDT
        // When Xdi and AmProc support vectored reads, return the whole
        // vector, not just the front element. For now assume one object.
        // A nullptr (with ERR_OK), indicates a zero'd out object
        auto& buf = (bufs && !bufs->empty()) ? bufs->front() : empty_buffer;
        xdiClientCall(&client_type::getBlobWithMetaResponse, requestId, buf, blobDesc);
    }
}

boost::shared_ptr<fpi::BlobDescriptor>
transform_descriptor(boost::shared_ptr<fds::BlobDescriptor> descriptor) {
    auto retBlobDesc = boost::make_shared<fpi::BlobDescriptor>();
    retBlobDesc->name = descriptor->getBlobName();
    retBlobDesc->byteCount = descriptor->getBlobSize();

    for (const_kv_iterator it = descriptor->kvMetaBegin();
         it != descriptor->kvMetaEnd();
         ++it) {
        retBlobDesc->metadata[it->first] = it->second;
    }
    return retBlobDesc;
}
}  // namespace fds
