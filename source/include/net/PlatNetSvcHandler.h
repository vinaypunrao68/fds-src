/* Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_NET_PLATNETSVCHANDLER_H_
#define SOURCE_INCLUDE_NET_PLATNETSVCHANDLER_H_

#include <string>
#include <map>
#include <fdsp/fds_service_types.h>
#include <fdsp/PlatNetSvc.h>
#include <net/BaseAsyncSvcHandler.h>

namespace fpi = FDS_ProtocolInterface;

namespace fds {
class PlatNetSvcHandler : virtual public fpi::PlatNetSvcIf, public BaseAsyncSvcHandler
{
 public:
    PlatNetSvcHandler();
    virtual ~PlatNetSvcHandler();
    virtual void allUuidBinding(const fpi::UuidBindMsg& mine);
    virtual void allUuidBinding(boost::shared_ptr<fpi::UuidBindMsg>& mine);  // NOLINT

    virtual void notifyNodeActive(const fpi::FDSP_ActivateNodeType &info);
    virtual void notifyNodeActive(boost::shared_ptr<fpi::FDSP_ActivateNodeType> &);

    virtual void notifyNodeInfo(std::vector<fpi::NodeInfoMsg> & _return,  // NOLINT
                        const fpi::NodeInfoMsg& info, const bool bcast);
    virtual void notifyNodeInfo(std::vector<fpi::NodeInfoMsg> & _return,  // NOLINT
                        boost::shared_ptr<fpi::NodeInfoMsg>& info,  // NOLINT
                        boost::shared_ptr<bool>& bcast);

    virtual void getDomainNodes(fpi::DomainNodes &ret, const fpi::DomainNodes &d);
    virtual void getDomainNodes(fpi::DomainNodes &ret,
                        boost::shared_ptr<fpi::DomainNodes> &d);

    virtual fpi::ServiceStatus getStatus(const int32_t nullarg);
    virtual void getCounters(std::map<std::string, int64_t> & _return,
                             const std::string& id);
    virtual void resetCounters(const std::string& id);
    virtual void setConfigVal(const std::string& id, const int64_t val);
    void setFlag(const std::string& id, const int64_t value);
    int64_t getFlag(const std::string& id);
    void getFlags(std::map<std::string, int64_t> & _return, const int32_t nullarg);  // NOLINT
    bool setFault(const std::string& command) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
        return false;
    }

    virtual fpi::ServiceStatus getStatus(boost::shared_ptr<int32_t>& nullarg);  // NOLINT
    virtual void getCounters(std::map<std::string, int64_t> & _return,
                             boost::shared_ptr<std::string>& id);
    virtual void resetCounters(boost::shared_ptr<std::string>& id);
    virtual void setConfigVal(boost::shared_ptr<std::string>& id,  // NOLINT
                              boost::shared_ptr<int64_t>& val);
    void setFlag(boost::shared_ptr<std::string>& id, boost::shared_ptr<int64_t>& value);  // NOLINT
    int64_t getFlag(boost::shared_ptr<std::string>& id);  // NOLINT
    void getFlags(std::map<std::string, int64_t> & _return, boost::shared_ptr<int32_t>& nullarg);  // NOLINT
    bool setFault(boost::shared_ptr<std::string>& command);
};
}  // namespace fds
#endif  // SOURCE_INCLUDE_NET_PLATNETSVCHANDLER_H_
