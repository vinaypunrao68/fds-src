/*
 * Copyright 2013-2015 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_ORCH_MGR_INCLUDE_OM_SVC_HANDLER_H_
#define SOURCE_ORCH_MGR_INCLUDE_OM_SVC_HANDLER_H_

#include <OmResources.h>
#include <fdsp/OMSvc.h>
#include <fdsp/om_api_types.h>
#include <fdsp/svc_types_types.h>
#include <net/PlatNetSvcHandler.h>
#include <util/EventTracker.h>

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

      int mod_init(SysParams const *const param) override;
      /**
       * Stub Overrides from OMSvcIf
       */
      virtual void registerService(const fpi::SvcInfo& svcInfo) override {
          // Don't do anything here. This stub is just to keep cpp compiler happy
      }

      virtual void getDLT( ::FDS_ProtocolInterface::CtrlNotifyDLTUpdate& _return, const int64_t nullarg) override {
        // Don't do anything here. This stub is just to keep cpp compiler happy
      }

      virtual void getDMT( ::FDS_ProtocolInterface::CtrlNotifyDMTUpdate& _return, const int64_t nullarg) override {
        // Don't do anything here. This stub is just to keep cpp compiler happy
        return;
      }

      virtual void getAllVolumeDescriptors(fpi::GetAllVolumeDescriptors& _return, const int64_t nullarg) override {
    	  // Don't do anything here. This stub is just to keep cpp compiler happy
    	  return;
      }

      void getSvcInfo(fpi::SvcInfo &_return,
                      const  fpi::SvcUuid& svcUuid) override {
          // Don't do anything here. This stub is just to keep cpp compiler happy
      }

      /**
       * RPC overrides from fpi::OMSvcIf
       */
      virtual void registerService(boost::shared_ptr<fpi::SvcInfo>& svcInfo) override;
      virtual void getDLT( ::FDS_ProtocolInterface::CtrlNotifyDLTUpdate& _return, boost::shared_ptr<int64_t>& nullarg) override;
      virtual void getDMT( ::FDS_ProtocolInterface::CtrlNotifyDMTUpdate& _return, boost::shared_ptr<int64_t>& nullarg) override;
      virtual void getAllVolumeDescriptors(fpi::GetAllVolumeDescriptors& _return, boost::shared_ptr<int64_t>& nullarg) override;
      virtual void getSvcInfo(fpi::SvcInfo & _return,
                              boost::shared_ptr< fpi::SvcUuid>& svcUuid) override;

      /**
       * Async message handlers
       */
      void om_node_svc_info(boost::shared_ptr<fpi::AsyncHdr>    &hdr,
                            boost::shared_ptr<fpi::NodeSvcInfo> &svc);

      void om_node_info(boost::shared_ptr<fpi::AsyncHdr>    &hdr,
                        boost::shared_ptr<fpi::NodeInfoMsg> &node);

      virtual void
      getVolumeDescriptor(boost::shared_ptr<fpi::AsyncHdr>         &hdr,
                   boost::shared_ptr<fpi::GetVolumeDescriptor> &msg);

      virtual void
      SvcEvent(boost::shared_ptr<fpi::AsyncHdr>         &hdr,
                   boost::shared_ptr<fpi::CtrlSvcEvent> &msg);

      void AbortTokenMigration(boost::shared_ptr<fpi::AsyncHdr> &hdr,
                               boost::shared_ptr<fpi::CtrlTokenMigrationAbort> &msg);

      void setConfigDB(kvstore::ConfigDB* configDB);

      void notifyServiceRestart(boost::shared_ptr<fpi::AsyncHdr> &hdr,
          boost::shared_ptr<fpi::NotifyHealthReport> &msg);

      void heartbeatCheck(boost::shared_ptr<fpi::AsyncHdr>& hdr,
                          boost::shared_ptr<fpi::HeartbeatMessage>& msg);

      void svcStateChangeResp(boost::shared_ptr<fpi::AsyncHdr>& hdr,
                              boost::shared_ptr<fpi::SvcStateChangeResp>& msg);
    protected:
      OM_NodeDomainMod         *om_mod;
      EventTracker<NodeUuid, Error, UuidHash, ErrorHash> event_tracker;

      std::pair<int64_t, int32_t> lastHeardResp;

    private:
      void init_svc_event_handlers();
      void healthReportUnreachable( fpi::FDSP_MgrIdType &svc_type,
                                    boost::shared_ptr<fpi::NotifyHealthReport> &msg);
      void healthReportUnexpectedExit(fpi::FDSP_MgrIdType &comp_type,
          boost::shared_ptr<fpi::NotifyHealthReport> &msg);
      void healthReportRunning( boost::shared_ptr<fpi::NotifyHealthReport> &msg );
      void healthReportError(fpi::FDSP_MgrIdType &svc_type,
                             boost::shared_ptr<fpi::NotifyHealthReport> &msg);

      kvstore::ConfigDB* configDB = NULL;
  };

}  // namespace fds
#endif  // SOURCE_ORCH_MGR_INCLUDE_OM_SVC_HANDLER_H_
