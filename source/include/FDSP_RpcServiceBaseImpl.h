/*
 * Copyright 2014 By Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_FDSP_RPCSERVICEBASEIMPL_H_
#define SOURCE_INCLUDE_FDSP_RPCSERVICEBASEIMPL_H_
#include <string>
#include <map>
#include <boost/shared_ptr.hpp>

#include <fdsp/FDSP_types.h>
#include <fdsp/FDSP_RpcService.h>

namespace fds {
class FdsProcess;

}

namespace FDS_ProtocolInterface {

class FDSP_RpcServiceBaseImpl : virtual public FDSP_RpcServiceIf {
 public:
    virtual FDSP_RpcServiceStatus GetStatus(const int32_t nullarg);
    virtual FDSP_RpcServiceStatus GetStatus(boost::shared_ptr<int32_t>& nullarg);  // NOLINT
    virtual void GetStats(std::map<std::string, int64_t> & _return,
            const std::string& id);
    virtual void GetStats(std::map<std::string, int64_t> & _return,
            boost::shared_ptr<std::string>& id);
    virtual void SetConfigVal(const std::string& id, const int64_t val);
    virtual void SetConfigVal(boost::shared_ptr<std::string>& id,  // NOLINT
            boost::shared_ptr<int64_t>& val);

    void SetFdsProcess(fds::FdsProcess *p);
 protected:
    fds::FdsProcess *process_;
};
}  // namespace FDS_ProtocolInterface

#endif  // SOURCE_INCLUDE_FDSP_RPCSERVICEBASEIMPL_H_

