/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_AM_ENGINE_FDSN_SERVER_H_
#define SOURCE_INCLUDE_AM_ENGINE_FDSN_SERVER_H_

#include <string>
#include <vector>
#include <map>
#include <util/Log.h>
#include <fds_module.h>
#include <native_api.h>
#include <xdi/AmShim.h>
#include <concurrency/Thread.h>

#include <arpa/inet.h>
#include <thrift/concurrency/ThreadManager.h>
#include <thrift/concurrency/PosixThreadFactory.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TThreadPoolServer.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/transport/TTransportUtils.h>

namespace fds {

namespace xdi_at  = apache::thrift;
namespace xdi_att = apache::thrift::transport;
namespace xdi_atp = apache::thrift::protocol;
namespace xdi_ats = apache::thrift::server;

class FdsnIf : public xdi::AmShimIf {
    void createVolume(const std::string& domainName,
                      const std::string& volumeName,
                      const xdi::VolumePolicy& volumePolicy) {
    }
    void createVolume(boost::shared_ptr<std::string>& domainName,  // NOLINT
                      boost::shared_ptr<std::string>& volumeName,
                      boost::shared_ptr<xdi::VolumePolicy>& volumePolicy) {
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

/**
 * RPC-based server for FDSN. Exposes FDSN interface via
 * RPC-endpoints.
 */
class FdsnServer : public Module {
  private:
    FDS_NativeAPI            *eng_api;
    fds_uint32_t             port;

    /*
     * Thrift endpoint related
     */
    boost::shared_ptr<xdi_att::TServerTransport>  serverTransport;
    boost::shared_ptr<xdi_att::TTransportFactory> transportFactory;
    boost::shared_ptr<xdi_atp::TProtocolFactory>  protocolFactory;
    boost::shared_ptr<xdi::AmShimProcessor>       processor;
    boost::shared_ptr<xdi_ats::TThreadedServer>   server;
    // boost::shared_ptr<xdi::AmShimIf>  handler;

    boost::shared_ptr<boost::thread> listen_thread;

  public:
    explicit FdsnServer(const std::string &name);
    virtual ~FdsnServer() {
    }

    int  mod_init(SysParams const *const param);
    void mod_startup();
    void mod_shutdown();

    virtual void init_server(FDS_NativeAPI *api);
    virtual void deinit_server();
};

extern FdsnServer gl_FdsnServer;

}  // namespace fds

#endif  // SOURCE_INCLUDE_AM_ENGINE_FDSN_SERVER_H_
