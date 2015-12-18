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

  /**
   * @details
   * Templated so that dependency injection can be used to substitute
   * a fake data store for kvstore::ConfigDB. Enables component testing
   * with no inter-process dependencies.
   * If kvstore::ConfigDB implemented an interface, we would not need
   * to use a template here.
   */
  template<class DataStoreT>
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

      virtual void getSvcEndpoints(std::vector<fpi::FDSP_Node_Info_Type>& records,
          const ::FDS_ProtocolInterface::FDSP_MgrIdType svctype,
          const int32_t localDomainId) override {
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
      /**
       * @brief Supply service endpoint information to remote local domain
       *
       * @details
       * Consider a global domain with more than one local domain. The service layer
       * will need to route requests to services in a remote local domain (different
       * OM cluster). As of 1/1/2016, the SvcMap for an OM cluster does not list
       * services for remote local domains. Therefore getSvcMap() is unsuitable for
       * getting remote service endpoints.
       * <p/>
       * This interface provides a uniform way to get service endpoints for any 
       * local domain. It abstracts away making connections to the correct master
       * OM endpoint and caching of endpoint records.
       *
       * @param svctype Filter result according to the given service type
       * @param localDomainId Filter result according to given local domain Id
       *
       * @remark
       * Pushes significant business logic into service layer. Might result in
       * multiple requests to NIC. Instead, should we provide a client helper
       * that manages requests to the various OM clusters? 
       */
      virtual void getSvcEndpoints(std::vector<fpi::FDSP_Node_Info_Type>& _return,
          boost::shared_ptr< fpi::FDSP_MgrIdType>& svctype,
          boost::shared_ptr<int32_t>& localDomainId) override;

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

      void setConfigDB(DataStoreT* configDB);

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

      /**
       * @brief Supports look-up of global domain objects
       * @details
       * Templated so that unit tests can substitute fake/mock objects
       */
      DataStoreT* pConfigDB_;
  };

}  // namespace fds
#endif  // SOURCE_ORCH_MGR_INCLUDE_OM_SVC_HANDLER_H_
