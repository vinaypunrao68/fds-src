/*
 * Copyright 2013-2015 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_ORCH_MGR_INCLUDE_OM_SVC_HANDLER_H_
#define SOURCE_ORCH_MGR_INCLUDE_OM_SVC_HANDLER_H_

#include <OmResources.h>
#include <fdsp/om_service_types.h>
#include <fdsp/OMSvc.h>
#include <net/PlatNetSvcHandler.h>
#include <OmEventTracker.h>

/* Forward declarations */
namespace FDS_ProtocolInterface {
    class PlatNetSvcProcessor;
    using PlatNetSvcProcessorPtr = boost::shared_ptr<PlatNetSvcProcessor>;
    class OMSvcIf;
}  // namespace FDS_ProtocolInterface
namespace fpi = FDS_ProtocolInterface;

namespace fds {

class OmSvcHandler : virtual public fpi::OMSvcIf, public PlatNetSvcHandler
{
  public:
    explicit OmSvcHandler(CommonModuleProviderIf *provider);
    virtual ~OmSvcHandler();

    int mod_init(SysParams const *const param);
    /**
     * Stub Overrides from OMSvcIf
     */
    virtual void registerService(const fpi::SvcInfo& svcInfo) override {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }
    virtual void getSvcMap(std::vector<fpi::SvcInfo> & _return, const int64_t nullarg) override {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }
    virtual void setServiceProperty(const  fpi::SvcUuid& svcUuid,
                                    const std::string& key,
                                    const std::string& value) override {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }
    virtual void getServicePropery(std::string& _return,
                           const  fpi::SvcUuid& svcUuid,
                           const std::string& key) override {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }
    virtual void setServiceProperties(const fpi::SvcUuid& svcUuid,
                              const std::map<std::string, std::string> & props) override {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }
    void getServiceProperties(std::map<std::string, std::string> & _return,
                              const  fpi::SvcUuid& svcUuid) override {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }

    /**
     * RPC overrides from fpi::OMSvcIf
     */
    virtual void registerService(boost::shared_ptr<fpi::SvcInfo>& svcInfo);
    virtual void getSvcMap(std::vector<fpi::SvcInfo> & _return,
                           boost::shared_ptr<int64_t>& nullarg);
    virtual void setServiceProperty(boost::shared_ptr< fpi::SvcUuid>& svcUuid,
                            boost::shared_ptr<std::string>& key,
                            boost::shared_ptr<std::string>& value) override;
    virtual void getServicePropery(std::string& _return,
                           boost::shared_ptr< fpi::SvcUuid>& svcUuid,
                           boost::shared_ptr<std::string>& key) override;
    virtual void setServiceProperties(boost::shared_ptr< fpi::SvcUuid>& svcUuid,
                    boost::shared_ptr<std::map<std::string, std::string> >& props) override;
    virtual void getServiceProperties(std::map<std::string, std::string> & _return,
                                      boost::shared_ptr< fpi::SvcUuid>& svcUuid) override;


    /**
     * Async message handlers
     */
    virtual void getSvcMap(boost::shared_ptr<fpi::AsyncHdr> &hdr,
                           boost::shared_ptr<fpi::GetSvcMapMsg> &msg);
    void om_node_svc_info(boost::shared_ptr<fpi::AsyncHdr>    &hdr,
                          boost::shared_ptr<fpi::NodeSvcInfo> &svc);

    void om_node_info(boost::shared_ptr<fpi::AsyncHdr>    &hdr,
                      boost::shared_ptr<fpi::NodeInfoMsg> &node);

    virtual void
    TestBucket(boost::shared_ptr<fpi::AsyncHdr>         &hdr,
                 boost::shared_ptr<fpi::CtrlTestBucket> &msg);

    virtual void
    CreateBucket(boost::shared_ptr<fpi::AsyncHdr>         &hdr,
                 boost::shared_ptr<fpi::CtrlCreateBucket> &msg);

    virtual void
    DeleteBucket(boost::shared_ptr<fpi::AsyncHdr>         &hdr,
                 boost::shared_ptr<fpi::CtrlDeleteBucket> &msg);

    virtual void
    ModifyBucket(boost::shared_ptr<fpi::AsyncHdr>         &hdr,
                 boost::shared_ptr<fpi::CtrlModifyBucket> &msg);

    virtual void
    SvcEvent(boost::shared_ptr<fpi::AsyncHdr>         &hdr,
                 boost::shared_ptr<fpi::CtrlSvcEvent> &msg);

    void AbortTokenMigration(boost::shared_ptr<fpi::AsyncHdr> &hdr,
                             boost::shared_ptr<fpi::CtrlTokenMigrationAbort> &msg);

    void setConfigDB(kvstore::ConfigDB* configDB);
    
  protected:
    OM_NodeDomainMod         *om_mod;
    EventTracker<NodeUuid, Error, UuidHash, ErrorHash> event_tracker;

  private:
    void init_svc_event_handlers();
    
    kvstore::ConfigDB* configDB = NULL;
    
};

}  // namespace fds
#endif  // SOURCE_ORCH_MGR_INCLUDE_OM_SVC_HANDLER_H_
