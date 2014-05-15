
#include <arpa/inet.h>

#include <apis/ConfigurationService.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>

#include <fds_typedefs.h>
#include <util/Log.h>
#include <OmResources.h>
#include <convert.h>
using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using namespace  ::fds::apis;

namespace fds {
class OrchMgr;
}


class ConfigurationServiceHandler : virtual public ConfigurationServiceIf {
    fds::OrchMgr* om;
  public:
    ConfigurationServiceHandler(fds::OrchMgr* om) : om(om) {
    }

    void apiException(std::string message, ErrorCode code=INTERNAL_SERVER_ERROR) {
        LOGERROR << "exception: " << message;
        ApiException e;
        e.message = message;
        e.errorCode    = code;
        throw e;
    }

    void checkDomainStatus() {
        OM_NodeDomainMod *domain = OM_NodeDomainMod::om_local_domain();
        if (!domain->om_local_domain_up()) {
            apiException("local domain not up");
        }
    }

    // Don't do anything here. This stub is just to keep cpp compiler happy
    void createVolume(const std::string& domainName, const std::string& volumeName, const VolumePolicy& volumePolicy) {}
    void deleteVolume(const std::string& domainName, const std::string& volumeName) {}
    void statVolume(VolumeDescriptor& _return, const std::string& domainName, const std::string& volumeName) {}
    void listVolumes(std::vector<VolumeDescriptor> & _return, const std::string& domainName) {}

    void createVolume(boost::shared_ptr<std::string>& domainName,
                      boost::shared_ptr<std::string>& volumeName,
                      boost::shared_ptr<VolumePolicy>& volumePolicy) {

        LOGNOTIFY << " domain: " << *domainName
                  << " volume: " << *volumeName;

        checkDomainStatus();

        OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
        VolumeContainer::pointer volContainer =local->om_vol_mgr();
        fds::Error err = volContainer->getVolumeStatus(*volumeName);
        if (err == ERR_OK) apiException("volume already exists");

        fpi::FDSP_MsgHdrTypePtr header;
        fpi::FDSP_CreateVolTypePtr request;
        convert::getFDSPCreateVolRequest(header, request, *domainName, *volumeName, *volumePolicy);
        err = volContainer->om_create_vol(header, request, false);
        if (err != ERR_OK) apiException("error creating volume");
    }

    void deleteVolume(boost::shared_ptr<std::string>& domainName,
                      boost::shared_ptr<std::string>& volumeName) {

        checkDomainStatus();

        OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
        VolumeContainer::pointer volContainer =local->om_vol_mgr();
        fds::Error err = volContainer->getVolumeStatus(*volumeName);

        if (err != ERR_OK) apiException("volume does NOT exist");

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
        fds::Error err = volContainer->getVolumeStatus(*volumeName);
        if (err != ERR_OK) apiException("volume NOT found");

        VolumeInfo::pointer  vol = volContainer->get_volume(*volumeName);

        volDescriptor.name = *volumeName;
        volDescriptor.policy.maxObjectSizeInBytes=2*1024*1024;
    }

    void listVolumes(std::vector<VolumeDescriptor> & _return, boost::shared_ptr<std::string>& domainName) {

        checkDomainStatus();

        OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
        VolumeContainer::pointer volContainer = local->om_vol_mgr();

        volContainer->vol_up_foreach<std::vector<VolumeDescriptor> &>(_return, [] (std::vector<VolumeDescriptor> &vec , fds::VolumeInfo::pointer vol) {
                VolumeDescriptor volDescriptor;
                volDescriptor.name = vol->vol_get_name();
                vec.push_back(volDescriptor);
            }
            );
    }
};

namespace fds {

std::thread* runConfigService(OrchMgr* om) {
    int port = 9090;
    LOGNORMAL << "about to start config service @ " << port;
    boost::shared_ptr<ConfigurationServiceHandler> handler(new ConfigurationServiceHandler(om));
    boost::shared_ptr<TProcessor> processor(new ConfigurationServiceProcessor(handler));
    boost::shared_ptr<TServerTransport> serverTransport(new TServerSocket(port));
    boost::shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
    boost::shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());

    TThreadedServer* server=new TThreadedServer(processor, serverTransport, transportFactory, protocolFactory);
    return new std::thread ( [server] {
            LOGNOTIFY << "starting config service";
            server->serve();
            LOGCRITICAL << "stopping ... config service";
        });
}

}  // namespace fds
