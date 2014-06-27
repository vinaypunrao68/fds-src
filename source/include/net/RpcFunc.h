/* Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_NET_RPCFUNC_H_
#define SOURCE_INCLUDE_NET_RPCFUNC_H_

#include <net/net-service.h>

#define NET_SVC_RPC_CALL(eph, rpc, rpc_fn, ...)                                        \
    do {                                                                               \
        bool __retry = false;                                                          \
        do {                                                                           \
            try {                                                                      \
                rpc->rpc_fn(__VA_ARGS__);                                              \
                __retry = false;                                                       \
            } catch(...) {                                                             \
                eph->ep_handle_net_error();                                            \
                if (eph->ep_reconnect() == EP_ST_CONNECTED) {                          \
                    __retry = true;                                                    \
                } else {                                                               \
                    const bo::shared_ptr<tt::TSocket> sk = eph->ep_debug_sock();       \
                    GLOGDEBUG << "Rpc fails " << sk->getHost() << ":" << sk->getPort(); \
                } \
            }                                                                          \
        } while (__retry == true);                                                     \
    } while (0)

/**
* @brief private wrapper for invoking rpc function
*
* @param ServiceT
* @param hdr
* @param func
* @param arg1
*
* @return 
*/
#define INVOKE_RPC_INTERNAL(ServiceT, hdr, func, arg1) \
    EpSvcHandle::pointer ep; \
    NetMgr::ep_mgr_singleton()->\
        svc_get_handle<ServiceT>(hdr.msg_dst_uuid, &ep, 0 , 0); \
    if (ep == nullptr) { \
        throw std::runtime_error("Null endpoint"); \
    } \
    auto client = ep->svc_rpc<ServiceT>(); \
    if (!client) { \
        throw std::runtime_error("Null client"); \
    } \
    client->func(arg1)

/**
* @brief Wrapper macro fro creating an rpc function
*
* @param ServiceT service type class
* @param func Function to create the wrapper.  Function must be a method in ServiceT class.
* @param arg1 argument to the function.  It must contain a member with name "hdr"
*
* @return std::function<void(arg1 type)>
*/
#define CREATE_RPC(ServiceT, func, arg1) \
    [arg1] (const fpi::AsyncHdr &hdr) mutable { \
        arg1.hdr = hdr; \
        INVOKE_RPC_INTERNAL(ServiceT, hdr, func, arg1); \
    }

/**
 * @see CREATE_RPC() macro.  Use this macro if arg1sptr is a shared pointer
 */
#define CREATE_RPC_SPTR(ServiceT, func, arg1sptr) \
    [arg1sptr] (const fpi::AsyncHdr &hdr) mutable { \
        arg1sptr->hdr = hdr;                     \
        INVOKE_RPC_INTERNAL(ServiceT, hdr, func, arg1sptr); \
    }

namespace fds {

typedef std::function<void (const fpi::AsyncHdr&)> RpcFunc;

#if 0
/**
 * Interface for rpc function wrapper
 */
class RpcFuncIf {
 public:
    virtual void invoke() = 0;
    virtual void setHeader(const fpi::AsyncHdr &hdr) = 0;

 protected:
};
// typedef boost::shared_ptr<RpcFuncIf> RpcFuncPtr;
typedef std::function<void (const fpi::AsyncHdr&)> RpcFunc;

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
        EpSvcHandle::pointer ep;
        NetMgr::ep_mgr_singleton()->\
            svc_get_handle<ServiceT>(a1_->hdr.msg_dst_uuid, &ep, 0 , 0);

        if (ep == nullptr) {
            throw std::runtime_error("Null endpoint");
        }

        auto client = ep->svc_rpc<ServiceT>();
        if (!client) {
            throw std::runtime_error("Null client");
        }
        (client.get()->*f_)(a1_);
    }
    virtual void setHeader(const fpi::AsyncHdr &hdr) {
        a1_->hdr = hdr;
    }
    MemFuncT f_;
    Arg1T a1_;
};

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
