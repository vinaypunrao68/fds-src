#ifndef INCLUDE_FDS_RESOURCE_H_
#define INCLUDE_FDS_RESOURCE_H_

#include <boost/atomic.hpp>
#include <boost/intrusive_ptr.hpp>
#include <shared/fds_types.h>
#include <cpplist.h>

namespace fds {

// ----------------------------------------------------------------------------
// Generic Resource Object
// ----------------------------------------------------------------------------
class Resource
{
  public:
    typedef boost::intrusive_ptr<Resource> pointer;
    typedef boost::intrusive_ptr<const Resource> const_ptr;
    Resource() : rs_refcnt(0) {}
    virtual ~Resource() {}

  private:
    mutable boost::atomic<int>  rs_refcnt;
    friend void intrusive_ptr_add_ref(const Resource  *x) {
        x->rs_refcnt.fetch_add(1, boost::memory_order_relaxed);
    }
    friend void intrusive_ptr_release(const Resource *x) {
        if (x->rs_refcnt.fetch_sub(1, boost::memory_order_release) == 1) {
            boost::atomic_thread_fence(boost::memory_order_acquire);
            delete x;
        }
    }
};

// ----------------------------------------------------------------------------
// Generic Resource Container
// ----------------------------------------------------------------------------
class RsContainer
{
  public:
    typedef boost::intrusive_ptr<RsContainer> pointer;
    typedef boost::intrusive_ptr<const RsContainer> const_ptr;
    RsContainer() : rs_refcnt(0) {}
    virtual ~RsContainer() {}

  private:
    mutable boost::atomic<int>  rs_refcnt;
    friend void intrusive_ptr_add_ref(const RsContainer *x) {
        x->rs_refcnt.fetch_add(1, boost::memory_order_relaxed);
    }
    friend void intrusive_ptr_release(const RsContainer *x) {
        if (x->rs_refcnt.fetch_sub(1, boost::memory_order_release) == 1) {
            boost::atomic_thread_fence(boost::memory_order_acquire);
            delete x;
        }
    }
};

// ----------------------------------------------------------------------------
// Resource UUID
// ----------------------------------------------------------------------------
class ResourceUUID
{
  public:
    ResourceUUID() : rs_uuid(0) {}
    ResourceUUID(fds_uint64_t uuid);

    inline fds_uint64_t uuid_get_val() const {
        return rs_uuid;
    }
    inline void uuid_set_val(fds_uint64_t val) {
        rs_uuid = val;
    }

    bool operator==(const ResourceUUID& rhs) const {
        return (this->rs_uuid == rhs.rs_uuid);
    }

    bool operator!=(const ResourceUUID& rhs) const {
        return !(*this == rhs);
    }

    ResourceUUID& operator=(const ResourceUUID& rhs) {
        rs_uuid = rhs.rs_uuid;
        return *this;
    }

  protected:
    fds_uint64_t             rs_uuid;
};

class UuidHash {
  public:
    fds_uint64_t operator()(const ResourceUUID& rs) const {
        return rs.uuid_get_val();
    }
};

// ----------------------------------------------------------------------------
// Generic Query list with embeded chain list.
// ----------------------------------------------------------------------------
class QueryMgr;

class QueryIn
{
};

template <class T>
class QueryOut
{
  public:
    QueryOut() : q_list() { q_list.chain_iter_init(&q_iter); }
    virtual ~QueryOut()
    {
        while (1) {
            T *elm = query_pop();
            if (elm != nullptr) {
                delete elm;
                continue;
            }
            break;
        }
    }
    inline void query_push(ChainLink *chain)
    {
        q_list.chain_add_back(chain);
    }
    inline T *query_pop()
    {
        ChainLink *elm = q_list.chain_front_elem();
        if (elm != nullptr) {
            if (elm->chain_link() == q_iter) {
                return q_list.chain_iter_rm_current<T>(&q_iter);
            }
            elm->chain_rm_init();
            return elm->chain_get_obj<T>();
        }
        return nullptr;
    }
    inline T *query_peek()
    {
        return q_list.chain_peek_front<T>();
    }
    /* ---------------------------------------------------------------------- */
    inline void query_iter_reset()
    {
        q_list.chain_iter_init(&q_iter);
    }
    inline bool query_iter_term()
    {
        return q_list.chain_iter_term(q_iter);
    }
    inline void query_iter_next()
    {
        return q_list.chain_iter_next(&q_iter);
    }
    inline T *query_iter_current()
    {
        return q_list.chain_iter_current<T>(q_iter);
    }
  private:
    friend class QueryMgr;

    ChainList                q_list;
    fds::ChainIter           q_iter;
};

class QueryMgr
{
  public:
    virtual ~QueryMgr();
    QueryMgr();
};

} // namespace fds

#endif /* INCLUDE_FDS_RESOURCE_H_ */
