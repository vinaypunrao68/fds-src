/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_DATA_MGR_INCLUDE_DMSVCHANDLER_H_
#define SOURCE_DATA_MGR_INCLUDE_DMSVCHANDLER_H_

#include <fdsp/fds_service_types.h>
#include <net/BaseAsyncSvcHandler.h>
#include <fdsp/DMSvc.h>
// TODO(Rao): Don't include DataMgr here.  The only reason we include now is
// b/c dmCatReq is subclass in DataMgr and can't be forward declared
#include <DataMgr.h>

namespace fds {

class DMSvcHandler : virtual public DMSvcIf, public BaseAsyncSvcHandler {
 public:
    DMSvcHandler();

    void queryCatalogObject(const fpi::QueryCatalogMsg& queryMsg) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }
    void updateCatalog(const fpi::UpdateCatalogMsg& updcatMsg) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }


    void queryCatalogObject(boost::shared_ptr<fpi::QueryCatalogMsg>& queryMsg);
    void queryCatalogObjectCb(boost::shared_ptr<fpi::QueryCatalogMsg>& queryMsg,
                              const Error &e, dmCatReq *req, BlobNode *bnode);

    void updateCatalog(boost::shared_ptr<fpi::UpdateCatalogMsg>& updcatMsg);
    void updateCatalogCb(boost::shared_ptr<fpi::UpdateCatalogMsg>& updcatMsg,
                         const Error &e, DmIoUpdateCat *req);
};

}  // namespace fds

#endif  // SOURCE_DATA_MGR_INCLUDE_DMSVCHANDLER_H_
