/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_STOR_MGR_INCLUDE_SMSVCHANDLER_H_
#define SOURCE_STOR_MGR_INCLUDE_SMSVCHANDLER_H_

#include <fdsp/fds_service_types.h>
#include <net/PlatNetSvcHandler.h>
#include <fdsp/SMSvc.h>

namespace fds {

/* Forward declarations */
class SmIoGetObjectReq;
class SmIoPutObjectReq;
class SmIoDeleteObjectReq;
class SmIoAddObjRefReq;

class SMSvcHandler : virtual public fpi::SMSvcIf, public PlatNetSvcHandler {
 public:
    SMSvcHandler();

    void getObject(const fpi::AsyncHdr& asyncHdr,
                   const fpi::GetObjectMsg& getObjMsg) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }
    void putObject(const fpi::AsyncHdr& asyncHdr,
                   const fpi::PutObjectMsg& putObjMsg) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }

    void getObject(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                   boost::shared_ptr<fpi::GetObjectMsg>& getObjMsg);
    void getObjectCb(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                     const Error &err,
                     SmIoGetObjectReq *read_data);
    void mockGetCb(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr);

    void putObject(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                   boost::shared_ptr<fpi::PutObjectMsg>& putObjMsg);
    void putObjectCb(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                     const Error &err,
                     SmIoPutObjectReq *put_req);
    void mockPutCb(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr);

    void deleteObject(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                      boost::shared_ptr<fpi::DeleteObjectMsg>& expObjMsg);
    void deleteObjectCb(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                        const Error &err,
                        SmIoDeleteObjectReq *del_req);
    virtual void
    notifySvcChange(boost::shared_ptr<fpi::AsyncHdr>    &hdr,
                    boost::shared_ptr<fpi::NodeSvcInfo> &msg);

    virtual void
    NotifyAddVol(boost::shared_ptr<fpi::AsyncHdr>         &hdr,
                 boost::shared_ptr<fpi::CtrlNotifyVolAdd> &vol_msg);

    virtual void
    NotifyRmVol(boost::shared_ptr<fpi::AsyncHdr>            &hdr,
                boost::shared_ptr<fpi::CtrlNotifyVolRemove> &vol_msg);

    virtual void
    NotifyModVol(boost::shared_ptr<fpi::AsyncHdr>         &hdr,
                 boost::shared_ptr<fpi::CtrlNotifyVolMod> &vol_msg);

    virtual void
    NotifyScavenger(boost::shared_ptr<fpi::AsyncHdr>         &hdr,
                 boost::shared_ptr<fpi::CtrlNotifyScavenger> &vol_msg);

    void queryScavengerStatus(boost::shared_ptr<fpi::AsyncHdr>    &hdr,
            boost::shared_ptr<fpi::CtrlQueryScavengerStatus> &query_msg);

    void queryScavengerProgress(boost::shared_ptr<fpi::AsyncHdr> &hdr,
            boost::shared_ptr<fpi::CtrlQueryScavengerProgress> &query_msg);

    void setScavengerPolicy(boost::shared_ptr<fpi::AsyncHdr> &hdr,
            boost::shared_ptr<fpi::CtrlSetScavengerPolicy> &policy_msg);

    void queryScavengerPolicy(boost::shared_ptr<fpi::AsyncHdr> &hdr,
            boost::shared_ptr<fpi::CtrlQueryScavengerPolicy> &policy_msg);

    void queryScrubberStatus(boost::shared_ptr<fpi::AsyncHdr> &hdr,
            boost::shared_ptr<fpi::CtrlQueryScrubberStatus> &scrub_msg);

    void setScrubberStatus(boost::shared_ptr<fpi::AsyncHdr> &hdr,
            boost::shared_ptr<fpi::CtrlSetScrubberStatus> &scrub_msg);

    virtual void
    NotifyDLTUpdate(boost::shared_ptr<fpi::AsyncHdr>            &hdr,
                    boost::shared_ptr<fpi::CtrlNotifyDLTUpdate> &dlt);
    virtual void
    TierPolicy(boost::shared_ptr<fpi::AsyncHdr>       &hdr,
               boost::shared_ptr<fpi::CtrlTierPolicy> &msg);

    virtual void
    TierPolicyAudit(boost::shared_ptr<fpi::AsyncHdr>            &hdr,
                    boost::shared_ptr<fpi::CtrlTierPolicyAudit> &msg);

    void addObjectRef(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                      boost::shared_ptr<fpi::AddObjectRefMsg>& addObjRefMsg);
    void addObjectRefCb(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                        const Error &err,
                        SmIoAddObjRefReq *addObjRefReq);
};

}  // namespace fds

#endif  // SOURCE_STOR_MGR_INCLUDE_SMSVCHANDLER_H_
