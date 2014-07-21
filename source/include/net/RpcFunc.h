/* Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_NET_RPCFUNC_H_
#define SOURCE_INCLUDE_NET_RPCFUNC_H_

#include <string>
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
* @param ServiceT - service type
* @param func - rpc function
* @param hdr - AsyncHdr
* @param ... arguments list for func
*
* @return 
*/
#define INVOKE_RPC_INTERNAL(ServiceT, func, hdr, ...) \
    auto ep = NetMgr::ep_mgr_singleton()->svc_get_handle<ServiceT>(hdr.msg_dst_uuid, 0 , 0); \
    if (!ep) { \
        throw std::runtime_error("Null client"); \
    } \
    fds_verify(hdr.msg_type_id != fpi::UnknownMsgTypeId); \
    ep->svc_rpc<ServiceT>()->func(hdr, __VA_ARGS__)

/**
* @brief Wrapper macro for creating service layer rpc function
*
* @param ServiceT service type class
* @param func Function to create the wrapper.  Function must be a method in ServiceT class.
* @param arg1 argument to the function.
*
* @return std::function<void(const fpi::AsyncHdr &)>
*/
#define CREATE_RPC(ServiceT, func, arg1sptr) \
    [arg1sptr] (const fpi::AsyncHdr &hdr) mutable { \
        INVOKE_RPC_INTERNAL(ServiceT, func, hdr, *arg1sptr); \
    }

/**
 * @see CREATE_RPC() macro.  
 * Use this macro for creating an rpc for sending network messages
 * TODO(Rao): When we switch to completely message based this macro isn't needed.
 */
#define CREATE_NET_REQUEST_RPC() \
    [this] (const fpi::AsyncHdr &hdr) mutable { \
        const_cast<fpi::AsyncHdr&>(hdr).msg_type_id = msgTypeId_; \
        INVOKE_RPC_INTERNAL(fpi::BaseAsyncSvcClient, asyncReqt, hdr, *payloadBuf_); \
        GLOGDEBUG << fds::logString(hdr) << " sent payload size: " << payloadBuf_->size(); \
    }

namespace fds {

typedef std::function<void (const fpi::AsyncHdr&)> RpcFunc;

}  // namespace fds

#endif  // SOURCE_INCLUDE_NET_RPCFUNC_H_
