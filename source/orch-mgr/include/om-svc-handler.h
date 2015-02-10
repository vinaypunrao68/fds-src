/*
 * Copyright 2013-2015 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_ORCH_MGR_INCLUDE_OM_SVC_HANDLER_H_
#define SOURCE_ORCH_MGR_INCLUDE_OM_SVC_HANDLER_H_

#include <OmResources.h>
#include <fdsp/fds_service_types.h>
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
    OmSvcHandler();
    virtual ~OmSvcHandler();

    void om_node_svc_info(boost::shared_ptr<fpi::AsyncHdr>    &hdr,
                          boost::shared_ptr<fpi::NodeSvcInfo> &svc);

    void om_node_info(boost::shared_ptr<fpi::AsyncHdr>    &hdr,
                      boost::shared_ptr<fpi::NodeInfoMsg> &node);

    virtual void
    TestBucket(boost::shared_ptr<fpi::AsyncHdr>         &hdr,
                 boost::shared_ptr<fpi::CtrlTestBucket> &msg);

    virtual void
    GetBucketStats(boost::shared_ptr<fpi::AsyncHdr>         &hdr,
                 boost::shared_ptr<fpi::CtrlGetBucketStats> &msg);

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
    PerfStats(boost::shared_ptr<fpi::AsyncHdr>         &hdr,
                 boost::shared_ptr<fpi::CtrlPerfStats> &msg);

    virtual void
    SvcEvent(boost::shared_ptr<fpi::AsyncHdr>         &hdr,
                 boost::shared_ptr<fpi::CtrlSvcEvent> &msg);

    virtual void
    GetSvcMap(boost::shared_ptr<fpi::AsyncHdr>         &hdr,
                 boost::shared_ptr<fpi::GetSvcMapMsg> &msg);

    /* Overrides from OMSvcIf */
    virtual void registerService(const SvcInfo& svcInfo) override {}
    virtual void getSvcMap(std::vector<SvcInfo> & _return, const int32_t nullarg) override {}
    virtual void registerService(boost::shared_ptr<SvcInfo>& svcInfo) override;
    virtual void getSvcMap(std::vector<SvcInfo> & _return, boost::shared_ptr<int32_t>& nullarg) override;

  protected:
    OM_NodeDomainMod         *om_mod;
    EventTracker<NodeUuid, Error, UuidHash, ErrorHash> event_tracker;

  private:
    void init_svc_event_handlers();
};

}  // namespace fds
#endif  // SOURCE_ORCH_MGR_INCLUDE_OM_SVC_HANDLER_H_
