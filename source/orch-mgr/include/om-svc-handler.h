/*
 * Copyright 2013-2015 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_ORCH_MGR_INCLUDE_OM_SVC_HANDLER_H_
#define SOURCE_ORCH_MGR_INCLUDE_OM_SVC_HANDLER_H_

#include <OmResources.h>
#include <fdsp/OMSvc.h>
#include <fdsp/om_api_types.h>
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
    void getSvcInfo(fpi::SvcInfo &_return,
                    const  fpi::SvcUuid& svcUuid) override {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }

    /**
     * RPC overrides from fpi::OMSvcIf
     */
    virtual void registerService(boost::shared_ptr<fpi::SvcInfo>& svcInfo) override;
    virtual void getSvcMap(std::vector<fpi::SvcInfo> & _return,
                           boost::shared_ptr<int64_t>& nullarg) override;
    virtual void getSvcInfo(fpi::SvcInfo & _return,
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
