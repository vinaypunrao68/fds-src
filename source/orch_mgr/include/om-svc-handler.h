/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_ORCH_MGR_INCLUDE_OM_SVC_HANDLER_H_
#define SOURCE_ORCH_MGR_INCLUDE_OM_SVC_HANDLER_H_

#include <OmResources.h>
#include <fdsp/fds_service_types.h>
#include <net/PlatNetSvcHandler.h>

namespace fds {

class OmSvcHandler : virtual public PlatNetSvcHandler
{
  public:
    OmSvcHandler();
    virtual ~OmSvcHandler();

    void om_node_svc_info(boost::shared_ptr<fpi::AsyncHdr>    &hdr,
                          boost::shared_ptr<fpi::NodeSvcInfo> &svc);

    void om_node_info(boost::shared_ptr<fpi::AsyncHdr>    &hdr,
                      boost::shared_ptr<fpi::NodeInfoMsg> &node);

    virtual void
    NotifyAddVol(boost::shared_ptr<fpi::AsyncHdr>         &hdr,
                 boost::shared_ptr<fpi::CtrlNotifyVolAdd> &vol_msg);

    virtual void
    NotifyRmVol(boost::shared_ptr<fpi::AsyncHdr>            &hdr,
                boost::shared_ptr<fpi::CtrlNotifyVolRemove> &vol_msg);

    virtual void
    NotifyModVol(boost::shared_ptr<fpi::AsyncHdr>         &hdr,
                 boost::shared_ptr<fpi::CtrlNotifyVolMod> &vol_msg);

  protected:
    OM_NodeDomainMod         *om_mod;
};

}  // namespace fds
#endif  // SOURCE_ORCH_MGR_INCLUDE_OM_SVC_HANDLER_H_
