/* Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_NET_RPCFUNC_H_
#define SOURCE_INCLUDE_NET_RPCFUNC_H_

#include <functional>
#include <boost/shared_ptr.hpp>

#include <fdsp/fds_service_types.h>
#include <net/net-service.h>


namespace fds {

/**
 * Interface for rpc function wrapper
 */
class RpcFuncIf {
 public:
    virtual void invoke() = 0;
    virtual void setHeader(const fpi::AsyncHdr &hdr) = 0;

 protected:
};
typedef boost::shared_ptr<RpcFuncIf> RpcFuncPtr;

/**
 * Rpc function wrapper implementation for a function that takes one argument.
 * The rpc arugment must have header as a member.
 */
template <class ServiceT, class MemFuncT, class Arg1T>
class RpcFunc1 : public RpcFuncIf {
 public:
    RpcFunc1(MemFuncT f, Arg1T a1) {
        f_ = f;
        a1_ = a1;
    }
    virtual void invoke() override {
        EpSvcHandle::pointer ep = NetMgr::ep_mgr_singleton()->\
            svc_lookup(a1_->header.msg_dst_uuid, 0 , 0);
        auto client = ep->svc_rpc<ServiceT>();
        (client.get()->*f_)(a1_);
    }
    virtual void setHeader(const fpi::AsyncHdr &hdr) {
    }
    MemFuncT f_;
    Arg1T a1_;
};

#if 0
template <class ServiceT, class ArgT>
class RpcFunc1 : public RpcFuncIf {
 public:
    RpcFunc1(std::function<void(boost::shared_ptr<ArgT> arg)> rpc,
            boost::shared_ptr<ArgT> arg)
    : rpc_(rpc), rpcArg_(arg)
    {
    }

    virtual void invoke() override {
        EpSvcHandle::pointer ep = NetMgr::ep_mgr_singleton()->\
            svc_lookup(rpcArg_->header.msg_dst_uuid, 0 , 0);
        auto client = ep->svc_rpc<ServiceT>();
        rpc_(client.get(), rpcArg_);
    }
    // TODO(Rao): Remove
    virtual void invoke2() {
        ServiceT s;
        rpc_(s, rpcArg_);
    }

    virtual void setHeader(const fpi::AsyncHdr &hdr) {
        rpcArg_->header = hdr;
    }

 protected:
    std::function<void(boost::shared_ptr<ArgT> arg)> rpc_;
    boost::shared_ptr<ArgT> rpcArg_;
};

template <class ReturnT, class ServiceT, class ArgT>
class RpcRetFunc1 : public  RpcFunc1<ServiceT, ArgT> {
};
#endif

}  // namespace fds

#endif  // SOURCE_INCLUDE_NET_RPCFUNC_H_
