/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <arpa/inet.h>

#include <apis/ConfigurationService.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>

#include <fds_typedefs.h>
#include <string>
#include <vector>
#include <util/Log.h>
#include <OmResources.h>
#include <convert.h>
using namespace ::apache::thrift;  //NOLINT
using namespace ::apache::thrift::protocol;  //NOLINT
using namespace ::apache::thrift::transport;  //NOLINT
using namespace ::apache::thrift::server;  //NOLINT

using namespace  ::apis;  //NOLINT

namespace fds {
class OrchMgr;

class ConfigurationServiceHandler : virtual public ConfigurationServiceIf {
    OrchMgr* om;

  public:
    explicit ConfigurationServiceHandler(OrchMgr* om) : om(om) {
    }

    void apiException(std::string message, ErrorCode code = INTERNAL_SERVER_ERROR) {
        LOGERROR << "exception: " << message;
        ApiException e;
        e.message = message;
        e.errorCode    = code;
        throw e;
    }

    void checkDomainStatus() {
        OM_NodeDomainMod *domain = OM_NodeDomainMod::om_local_domain();
        if (!domain->om_local_domain_up()) {
            apiException("local domain not up", SERVICE_NOT_READY);
        }
    }

    // stubs to keep cpp compiler happy - BEGIN
    void createVolume(const std::string& domainName, const std::string& volumeName, const VolumeSettings& volumeSettings) {}  //NOLINT
    void deleteVolume(const std::string& domainName, const std::string& volumeName) {}  //NOLINT
    void statVolume(VolumeDescriptor& _return, const std::string& domainName, const std::string& volumeName) {}  //NOLINT
    void listVolumes(std::vector<VolumeDescriptor> & _return, const std::string& domainName) {}  //NOLINT
    void registerStream(fpi::registration& _return, const std::string& url, const std::string& http_method, const std::vector<std::string> & volume_names, const int32_t sample_freq_seconds, const int32_t duration_seconds) {} //NOLINT
    void getStreamRegistrations(std::vector<fpi::registration> & _return, const int32_t thrift_sucks) {} //NOLINT
    void deregisterStream(const int32_t registration_id) {}

    // stubs to keep cpp compiler happy - END

    void createVolume(boost::shared_ptr<std::string>& domainName,
                      boost::shared_ptr<std::string>& volumeName,
                      boost::shared_ptr<VolumeSettings>& volumeSettings) {
        LOGNOTIFY << " domain: " << *domainName
                  << " volume: " << *volumeName;

        checkDomainStatus();

        OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
        VolumeContainer::pointer volContainer = local->om_vol_mgr();
        Error err = volContainer->getVolumeStatus(*volumeName);
        if (err == ERR_OK) apiException("volume already exists", RESOURCE_ALREADY_EXISTS);

        fpi::FDSP_MsgHdrTypePtr header;
        fpi::FDSP_CreateVolTypePtr request;
        convert::getFDSPCreateVolRequest(header, request,
                                         *domainName, *volumeName, *volumeSettings);
        err = volContainer->om_create_vol(header, request, false);
        if (err != ERR_OK) apiException("error creating volume");
    }

    void deleteVolume(boost::shared_ptr<std::string>& domainName,
                      boost::shared_ptr<std::string>& volumeName) {
        checkDomainStatus();

        OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
        VolumeContainer::pointer volContainer = local->om_vol_mgr();
        Error err = volContainer->getVolumeStatus(*volumeName);

        if (err != ERR_OK) apiException("volume does NOT exist", MISSING_RESOURCE);

        fpi::FDSP_MsgHdrTypePtr header;
        fpi::FDSP_DeleteVolTypePtr request;
        convert::getFDSPDeleteVolRequest(header, request, *domainName, *volumeName);
        err = volContainer->om_delete_vol(header, request);
    }

    void statVolume(VolumeDescriptor& volDescriptor,
                    boost::shared_ptr<std::string>& domainName,
                    boost::shared_ptr<std::string>& volumeName) {
        checkDomainStatus();
        OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
        VolumeContainer::pointer volContainer = local->om_vol_mgr();
        Error err = volContainer->getVolumeStatus(*volumeName);
        if (err != ERR_OK) apiException("volume NOT found", MISSING_RESOURCE);

        VolumeInfo::pointer  vol = volContainer->get_volume(*volumeName);

        convert::getVolumeDescriptor(volDescriptor, vol);
    }

    void listVolumes(std::vector<VolumeDescriptor> & _return,
                     boost::shared_ptr<std::string>& domainName) {
        checkDomainStatus();

        OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
        VolumeContainer::pointer volContainer = local->om_vol_mgr();

        volContainer->vol_up_foreach<std::vector<VolumeDescriptor> &>(_return, [] (std::vector<VolumeDescriptor> &vec, VolumeInfo::pointer vol) { //NOLINT
                VolumeDescriptor volDescriptor;
                convert::getVolumeDescriptor(volDescriptor, vol);
                vec.push_back(volDescriptor);
            });
    }

    void registerStream(fpi::registration& _return,
                      boost::shared_ptr<std::string>& url,
                      boost::shared_ptr<std::string>& http_method,
                      boost::shared_ptr<std::vector<std::string> >& volume_names,
                      boost::shared_ptr<int32_t>& sample_freq_seconds,
                      boost::shared_ptr<int32_t>& duration_seconds) {
    }

    void getStreamRegistrations(std::vector<fpi::registration> & _return, boost::shared_ptr<int32_t>& thrift_sucks) { //NOLINT
    }

    void deregisterStream(boost::shared_ptr<int32_t>& registration_id) {
    }
};

std::thread* runConfigService(OrchMgr* om) {
    int port = 9090;
    LOGNORMAL << "about to start config service @ " << port;
    boost::shared_ptr<ConfigurationServiceHandler> handler(new ConfigurationServiceHandler(om));  //NOLINT
    boost::shared_ptr<TProcessor> processor(new ConfigurationServiceProcessor(handler));  //NOLINT
    boost::shared_ptr<TServerTransport> serverTransport(new TServerSocket(port));  //NOLINT
    boost::shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());  //NOLINT
    boost::shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());  //NOLINT

    TThreadedServer* server = new TThreadedServer(processor,
                                                  serverTransport,
                                                  transportFactory, protocolFactory);
    return new std::thread ( [server] {
            LOGNOTIFY << "starting config service";
            server->serve();
            LOGCRITICAL << "stopping ... config service";
        });
}

}  // namespace fds
