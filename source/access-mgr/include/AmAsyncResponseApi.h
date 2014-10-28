/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_ACCESS_MGR_INCLUDE_AMASYNCRESPONSEAPI_H_
#define SOURCE_ACCESS_MGR_INCLUDE_AMASYNCRESPONSEAPI_H_

#include <map>
#include <string>
#include <vector>
#include <fds_module.h>
#include <apis/AsyncAmServiceResponse.h>

namespace fds {

/**
 * AM's data API that is exposed to XDI. This interface is the
 * basic data API that XDI and connectors are programmed to. A
 * pure virtual interface is exposed so that the implementation
 * of the responses can be overloaded (e.g., respond to XDI, respond
 * to unit test, etc...).
 */
class AmAsyncResponseApi {
  public:
    typedef boost::shared_ptr<AmAsyncResponseApi> shared_ptr;

    virtual void startBlobTxResp(const Error &error,
                                 boost::shared_ptr<apis::RequestId>& requestId,
                                 boost::shared_ptr<apis::TxDescriptor>& txDesc) = 0;
    virtual void abortBlobTxResp(const Error &error,
                                 boost::shared_ptr<apis::RequestId>& requestId) = 0;
    virtual void commitBlobTxResp(const Error &error,
                                  boost::shared_ptr<apis::RequestId>& requestId) = 0;
    virtual void updateBlobOnceResp(const Error &error,
                                    boost::shared_ptr<apis::RequestId>& requestId) = 0;
};

class AmAsyncXdiResponse : public AmAsyncResponseApi {
  private:
    /// Thrift client to response to XDI
    boost::shared_ptr<apis::AsyncAmServiceResponseClient> asyncRespClient;

  public:
    AmAsyncXdiResponse();
    ~AmAsyncXdiResponse();
    typedef boost::shared_ptr<AmAsyncXdiResponse> shared_ptr;

    void startBlobTxResp(const Error &error,
                         boost::shared_ptr<apis::RequestId>& requestId,
                         boost::shared_ptr<apis::TxDescriptor>& txDesc);
    void abortBlobTxResp(const Error &error,
                         boost::shared_ptr<apis::RequestId>& requestId);
    void commitBlobTxResp(const Error &error,
                          boost::shared_ptr<apis::RequestId>& requestId);
    void updateBlobOnceResp(const Error &error,
                            boost::shared_ptr<apis::RequestId>& requestId);
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_AMASYNCRESPONSEAPI_H_
