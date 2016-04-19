/*
 * Copyright 2014 By Formation Data Systems, Inc.
 */
#ifndef SOURCE_ACCESS_MGR_INCLUDE_AMSVCHANDLER_H_
#define SOURCE_ACCESS_MGR_INCLUDE_AMSVCHANDLER_H_

#include <unordered_map>
#include <set>

#include <net/PlatNetSvcHandler.h>
#include <fds_typedefs.h>
#include <fdsp/AMSvc.h>

/* Forward declarations */
namespace FDS_ProtocolInterface {
class AMSvcClient;
class AMSvcProcessor;
struct CtrlNotifyDMTUpdate;
}


namespace fds {

class AmProcessor;

class AMSvcHandler :  virtual public fpi::AMSvcIf, virtual public PlatNetSvcHandler
{
    std::shared_ptr<AmProcessor> amProcessor;
  public:
    AMSvcHandler(CommonModuleProviderIf *provider, std::shared_ptr<AmProcessor> processor);
    virtual ~AMSvcHandler();

    /**
     * Message handlers
     */
    virtual void
    notifySvcChange(boost::shared_ptr<fpi::AsyncHdr>    &hdr,
                    boost::shared_ptr<fpi::NodeSvcInfo> &msg);

    virtual void
    SetThrottleLevel(boost::shared_ptr<fpi::AsyncHdr>           &hdr,
                     boost::shared_ptr<fpi::CtrlNotifyThrottle> &msg);

    virtual void
    QoSControl(boost::shared_ptr<fpi::AsyncHdr>           &hdr,
                     boost::shared_ptr<fpi::CtrlNotifyQoSControl> &msg);

    virtual void
    NotifyModVol(boost::shared_ptr<fpi::AsyncHdr>         &hdr,
                 boost::shared_ptr<fpi::CtrlNotifyVolMod> &vol_msg);

    virtual void
    AddVol(boost::shared_ptr<fpi::AsyncHdr>         &hdr,
              boost::shared_ptr<fpi::CtrlNotifyVolAdd> &vol_msg);

    virtual void
    RemoveVol(boost::shared_ptr<fpi::AsyncHdr>            &hdr,
              boost::shared_ptr<fpi::CtrlNotifyVolRemove> &vol_msg);

    virtual void
    NotifyDMTUpdate(boost::shared_ptr<fpi::AsyncHdr>            &hdr,
                    boost::shared_ptr<fpi::CtrlNotifyDMTUpdate> &msg);

    void
    NotifyDMTUpdateCb(boost::shared_ptr<fpi::AsyncHdr>            hdr,
                      boost::shared_ptr<fpi::CtrlNotifyDMTUpdate> msg,
                      const Error                                 &err);

    virtual void
    NotifyDLTUpdate(boost::shared_ptr<fpi::AsyncHdr>            &hdr,
                    boost::shared_ptr<fpi::CtrlNotifyDLTUpdate> &msg);

    void
    NotifyDLTUpdateCb(boost::shared_ptr<fpi::AsyncHdr>            hdr,
                      boost::shared_ptr<fpi::CtrlNotifyDLTUpdate> dlt,
                      const Error                                 &err);

    virtual void
    prepareForShutdownMsgRespCb(boost::shared_ptr<fpi::AsyncHdr> &hdr,
               boost::shared_ptr<fpi::PrepareForShutdownMsg>  &shutdownMsg);

    virtual void
    shutdownAM(boost::shared_ptr<fpi::AsyncHdr>               &hdr,
               boost::shared_ptr<fpi::PrepareForShutdownMsg>  &shutdownMsg);

    virtual void
    addToVolumeGroup(fpi::AsyncHdrPtr& asyncHdr,
                         fpi::AddToVolumeGroupCtrlMsgPtr& addMsg);

    virtual void
    switchCoordinator(boost::shared_ptr<fpi::AsyncHdr>&           hdr,
                      fpi::SwitchCoordinatorMsgPtr&               msg);
    virtual void
    offlineVolumeGroup(fpi::AsyncHdrPtr& asyncHdr,
                       fpi::DbgOfflineVolumeGroupMsgPtr& offlineMsg);

  private:
    // This mutex protects the _pendingFlushes map
    std::mutex                                                                                         _flush_map_lock;
    std::unordered_map<std::string, boost::shared_ptr<std::set<boost::shared_ptr<fpi::AsyncHdr>>>>     _pendingFlushes;

    void addPendingFlush(std::string const& volName, boost::shared_ptr<fpi::AsyncHdr>& hdr);
    void _flushCb(std::string const& volName, Error const& err);
    void flushCb(std::string const& volName, Error const& err);
    void completeFlush(boost::shared_ptr<fpi::AsyncHdr> const& hdr, Error const& err);
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_AMSVCHANDLER_H_
