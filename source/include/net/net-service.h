/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_NET_NET_SERVICE_H_
#define SOURCE_INCLUDE_NET_NET_SERVICE_H_

#include <fds_module.h>
#include <fds_typedefs.h>
#include <shared/fds-constants.h>
#include <fdsp/fds_service_types.h>
#include <boost/intrusive_ptr.hpp>

namespace fds {

class EpOps;
class NetMgr;

struct EpAttr
{
};

#define EP_CAST_PTR(T, ptr)  static_cast<T::pointer>(get_pointer(ptr))

class EpOps
{
  public:
    typedef boost::intrusive_ptr<EpOps> pointer;
    typedef boost::intrusive_ptr<const EpOps> const_ptr;

    EpOps();
    virtual ~EpOps();

    virtual void ep_connected() = 0;

  private:
    mutable boost::atomic<int>       ep_refcnt;

    friend void intrusive_ptr_add_ref(const EpOps *x) {
        x->ep_refcnt.fetch_add(1, boost::memory_order_relaxed);
    }
    friend void intrusive_ptr_release(const EpOps *x) {
        if (x->ep_refcnt.fetch_sub(1, boost::memory_order_release) == 1) {
            boost::atomic_thread_fence(boost::memory_order_acquire);
            delete x;
        }
    }
};

class EpSvc
{
  public:
    typedef boost::intrusive_ptr<EpSvc> pointer;
    typedef boost::intrusive_ptr<const EpSvc> const_ptr;

    EpSvc();
    virtual ~EpSvc();

  private:
    mutable boost::atomic<int>       ep_refcnt;

    friend void intrusive_ptr_add_ref(const EpSvc *x) {
        x->ep_refcnt.fetch_add(1, boost::memory_order_relaxed);
    }
    friend void intrusive_ptr_release(const EpSvc *x) {
        if (x->ep_refcnt.fetch_sub(1, boost::memory_order_release) == 1) {
            boost::atomic_thread_fence(boost::memory_order_acquire);
            delete x;
        }
    }
};

template <class EndPointIf>
class EndPoint : public EpSvc
{
  public:
    typedef boost::intrusive_ptr<EndPoint<EndPointIf>> pointer;
    typedef boost::intrusive_ptr<const EndPoint<EndPointIf>> const_ptr;

    virtual ~EndPoint();
    EndPoint(const fpi::SvcID              &mine,
             const fpi::SvcID              &peer,
             boost::shared_ptr<EndPointIf>  snd_if,
             boost::shared_ptr<EndPointIf>  rcv_if,
             EpOps::pointer                 ops,
             const EpAttr                  &attr);

    static inline EndPoint<EndPointIf>::pointer ep_cast_ptr(EpSvc::pointer ptr) {
        return static_cast<EndPoint<EndPointIf> *>(get_pointer(ptr));
    }

    // Service registration & lookup.
    //
    virtual void           ep_bind_service(const fpi::SvcID &id, EpSvc::pointer svc);
    virtual EpSvc::pointer ep_unbind_service(const ResourceUUID &svc);
    virtual EpSvc::pointer ep_lookup_service(const ResourceUUID &svc_uuid);
    virtual EpSvc::pointer ep_lookup_service(const char *name);

    // Synchronous send.
    boost::shared_ptr<EndPointIf> ep_sync_rpc();

    // Async message passing.
    //

    // Endpoint attributes.
    virtual void ep_get_attr(EpAttr *attr) const;
    virtual void ep_set_attr(const EpAttr *attr);

  protected:
    friend class NetMgr;

    boost::shared_ptr<EndPointIf>  ep_send;
    boost::shared_ptr<EndPointIf>  ep_recv;
    EpOps::pointer                 ep_ops;
    EpAttr                         ep_attr;
    NetMgr                        *ep_mgr;
};

class NetMgr : public Module
{
  public:
    explicit NetMgr(const char *name);
    virtual ~NetMgr();

    // Module methods.
    //
    virtual int  mod_init(SysParams const *const p);
    virtual void mod_startup();
    virtual void mod_shutdown();

    // Endpoint registration and lookup.
    //
    virtual void           ep_register(EpSvc::pointer ep);
    virtual void           ep_unregister(const NodeUuid &uuid);
    virtual EpSvc::pointer ep_lookup(const NodeUuid &peer);
    virtual EpSvc::pointer ep_lookup(const char *peer_name);
};

}  // namespace fds
#endif  // SOURCE_INCLUDE_NET_NET_SERVICE_H_
