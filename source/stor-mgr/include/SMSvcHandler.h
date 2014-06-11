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
class SmIoReadObjectdata;

class SMSvcHandler : virtual public SMSvcIf, public PlatNetSvcHandler {
 public:
    SMSvcHandler();

    void getObject(const fpi::GetObjectMsg& getObjMsg) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }


    void getObject(boost::shared_ptr<fpi::GetObjectMsg>& getObjMsg);

    void getObjectCb(boost::shared_ptr<fpi::GetObjectMsg>& getObjMsg,
                     const Error &err,
                     SmIoReadObjectdata *read_data);
};

}  // namespace fds

#endif  // SOURCE_STOR_MGR_INCLUDE_SMSVCHANDLER_H_
