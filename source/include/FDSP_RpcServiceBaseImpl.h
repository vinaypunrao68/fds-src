/*
 * Copyright 2014 By Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_FDSP_RPC_SERVICE_BASE_H_
#define SOURCE_INCLUDE_FDSP_RPC_SERVICE_BASE_H_
#include <string>
#include <boost/shared_ptr.hpp>

#include <fdsp/FDSP_types.h>
#include <fdsp/FDSP_RpcService.h>

namespace fds {
class FdsCountersMgr;
class FdsConfig;
}

namespace FDS_ProtocolInterface {

class FDSP_RpcServiceBaseImpl : virtual public FDSP_RpcServiceIf {
public:
    virtual FDSP_RpcServiceStatus GetStatus(const int32_t nullarg);
    virtual FDSP_RpcServiceStatus GetStatus(boost::shared_ptr<int32_t>& nullarg);
    virtual void GetStats(std::map<std::string, int64_t> & _return,
            const std::string& id) = 0;
    virtual void GetStats(std::map<std::string, int64_t> & _return,
            boost::shared_ptr<std::string>& id) = 0;
    virtual void SetConfigVal(const std::string& id, const int64_t val);
    virtual void SetConfigVal(boost::shared_ptr<std::string>& id,
            boost::shared_ptr<int64_t>& val);

    virtual void RegisterCountersMgr(
            const boost::shared_ptr<fds::FdsCountersMgr> &cntrsMgr);
    virtual void RegisterFdsConfig(
            const boost::shared_ptr<fds::FdsConfig> &config);
protected:
    boost::shared_ptr<fds::FdsCountersMgr> cntrsMgr_;
    boost::shared_ptr<fds::FdsConfig> config_;
};
}  // namespace FDS_ProtocolInterface

#endif  // SOURCE_INCLUDE_FDSP_RPC_SERVICE_BASE_H_

