/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_AM_ENGINE_XDI_SERVER_H_
#define SOURCE_INCLUDE_AM_ENGINE_XDI_SERVER_H_

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

class XdiIf : public xdi::AmShimIf {
    void createVolume(const std::string& domainName,
                      const std::string& volumeName,
                      const xdi::VolumeDescriptor& volumeDescriptor) {
    }
    void createVolume(boost::shared_ptr<std::string>& domainName,  // NOLINT
                      boost::shared_ptr<std::string>& volumeName,
                      boost::shared_ptr<xdi::VolumeDescriptor>& volumeDescriptor) {
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
    void listVolumes(std::vector<std::string> & _return,
                     const std::string& domainName) {
    }
    void listVolumes(std::vector<std::string> & _return,
                     boost::shared_ptr<std::string>& domainName) {
    }
    void volumeContents(std::vector<std::string> & _return,
                        const std::string& domainName,
                        const std::string& volumeName) {
    }
    void volumeContents(std::vector<std::string> & _return,
                        boost::shared_ptr<std::string>& domainName,
                        boost::shared_ptr<std::string>& volumeName) {
    }
    void statBlob(xdi::BlobDescriptor& _return,  // NOLINT
                  const std::string& domainName,
                  const std::string& volumeName) {
    }
    void statBlob(xdi::BlobDescriptor& _return,  // NOLINT
                  boost::shared_ptr<std::string>& domainName,
                  boost::shared_ptr<std::string>& volumeName) {
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
    void updateMetadata(const std::string& domainName,
                        const std::string& volumeName,
                        const std::string& blobName,
                        const std::map<std::string, std::string> & metadata) {
    }
    void updateMetadata(boost::shared_ptr<std::string>& domainName,  // NOLINT
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
                    const int64_t offset) {
    }
    void updateBlob(boost::shared_ptr<std::string>& domainName,  // NOLINT
                    boost::shared_ptr<std::string>& volumeName,
                    boost::shared_ptr<std::string>& blobName,
                    boost::shared_ptr<std::string>& bytes,
                    boost::shared_ptr<int32_t>& length,
                    boost::shared_ptr<int64_t>& offset) {
    }
};

/**
 * RPC-based server for XDI. Exposes XDI interface via
 * RPC-endpoints.
 */
class XdiServer : public Module {
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
    explicit XdiServer(const std::string &name);
    virtual ~XdiServer() {
    }

    int  mod_init(SysParams const *const param);
    void mod_startup();
    void mod_shutdown();

    virtual void init_server(FDS_NativeAPI *api);
    virtual void deinit_server();
};

extern XdiServer gl_XdiServer;

}  // namespace fds

#endif  // SOURCE_INCLUDE_AM_ENGINE_XDI_SERVER_H_
