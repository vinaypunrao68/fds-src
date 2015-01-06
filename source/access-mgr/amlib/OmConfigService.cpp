/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <OmConfigService.h>
#include <fds_process.h>

#include <arpa/inet.h>
#include <thrift/concurrency/ThreadManager.h>
#include <thrift/concurrency/PosixThreadFactory.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TThreadPoolServer.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/server/TNonblockingServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/transport/TTransportUtils.h>

namespace xdi_at  = apache::thrift;
namespace xdi_att = apache::thrift::transport;
namespace xdi_atc = apache::thrift::concurrency;
namespace xdi_atp = apache::thrift::protocol;
namespace xdi_ats = apache::thrift::server;

namespace fds {

OmConfigApi::OmConfigApi()
        : omConfigPort(9090) {  // The port is hard coded in OM, not platform toggle
    FdsConfigAccessor conf(g_fdsprocess->get_fds_config(), "fds.am.");
    omConfigIp = conf.get<std::string>("om_ip");

    // Setup the client
    boost::shared_ptr<xdi_att::TTransport> respSock(
        boost::make_shared<xdi_att::TSocket>(
            omConfigIp, omConfigPort));
    boost::shared_ptr<xdi_att::TFramedTransport> respTrans(
        boost::make_shared<xdi_att::TFramedTransport>(respSock));
    boost::shared_ptr<xdi_atp::TProtocol> respProto(
        boost::make_shared<xdi_atp::TBinaryProtocol>(respTrans));
    omConfigClient = boost::make_shared<apis::ConfigurationServiceClient>(respProto);
    respSock->open();
}

OmConfigApi::~OmConfigApi() {
}

Error
OmConfigApi::statVolume(boost::shared_ptr<std::string> volumeName,
                        apis::VolumeDescriptor &volDesc) {
    try {
        std::string domain("Fake domain");
        omConfigClient->statVolume(volDesc, domain, *volumeName);
    } catch(apis::ApiException fdsE) {
        return ERR_NOT_FOUND;
    }
    return ERR_OK;
}

}  // namespace fds
