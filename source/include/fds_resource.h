/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_FDS_RESOURCE_H_
#define SOURCE_INCLUDE_FDS_RESOURCE_H_

#include <list>
#include <string>
#include <vector>
#include <ostream>
#include <unordered_map>

#include <fds_ptr.h>
#include <cpplist.h>
#include <shared/fds_types.h>
#include <concurrency/Mutex.h>
#include <fdsp/fds_service_types.h>
#include <fds_typedefs.h>

namespace fds {

// ----------------------------------------------------------------------------
// Resource UUID
// ----------------------------------------------------------------------------
class ResourceUUID
{
  public:
    ResourceUUID() : rs_uuid(0) {}
    ResourceUUID(fds_uint64_t uuid);  // NOLINT
    explicit ResourceUUID(const fds_uint8_t *raw);
    explicit ResourceUUID(const fpi::SvcUuid &uuid) : rs_uuid(uuid.svc_uuid) {}

    inline fds_uint64_t uuid_get_val() const {
        return rs_uuid;
    }
    inline void uuid_set_val(fds_uint64_t val) {
        rs_uuid = val;
    }
    inline void uuid_set_from_raw(const fds_uint8_t *raw) {
        const fds_uint64_t *ptr = reinterpret_cast<const fds_uint64_t *>(raw);
        rs_uuid = *ptr;
    }
    inline void uuid_set_to_raw(fds_uint8_t *raw) const {
        fds_uint64_t *ptr = reinterpret_cast<fds_uint64_t *>(raw);
        *ptr = rs_uuid;
    }
    inline void uuid_assign(fpi::SvcUuid *svc) const {
        svc->svc_uuid = rs_uuid;
    }
    inline void uuid_copy(fpi::SvcUuid &svc) {
        rs_uuid = svc.svc_uuid;
    }

    bool operator==(const ResourceUUID& rhs) const {
        return (this->rs_uuid == rhs.rs_uuid);
    }

    bool operator!=(const ResourceUUID& rhs) const {
        return !(*this == rhs);
    }

    bool operator<(const ResourceUUID& rhs) const {
        return rs_uuid < rhs.rs_uuid;
    }

    ResourceUUID& operator=(const ResourceUUID& rhs) {
        rs_uuid = rhs.rs_uuid;
        return *this;
    }

  protected:
    fds_uint64_t             rs_uuid;
};

extern ResourceUUID INVALID_RESOURCE_UUID;

class UuidHash {
  public:
    fds_uint64_t operator()(const ResourceUUID& rs) const {
        return rs.uuid_get_val();
    }
};

/**
 * ------------------------------------------------------------------------------------
 * Hash key of 2 uuids.
 * ------------------------------------------------------------------------------------
 */
struct UuidDouble
{
    fds_uint64_t             src_uuid;
    fds_uint64_t             dst_uuid;

    bool operator == (const UuidDouble &rhs) const {
        return ((this->src_uuid == rhs.src_uuid) && (this->dst_uuid == rhs.dst_uuid));
    }
    bool operator != (const UuidDouble &rhs) const {
        return !(*this == rhs);
    }
};

struct UuidDoubleHash {
    fds_uint64_t operator()(const UuidDouble &v) const {
        return v.src_uuid + v.dst_uuid;
    }
};

const int RS_NAME_MAX           = 64;
const int RS_DEFAULT_ELEM_CNT   = 1024;

class RsContainer;

// ----------------------------------------------------------------------------
// Generic Resource Object
// ----------------------------------------------------------------------------
class Resource
{
  public:
    typedef boost::intrusive_ptr<Resource> pointer;
    typedef boost::intrusive_ptr<const Resource> const_ptr;

    inline char const *const  rs_get_name() const { return rs_name; }
    inline const ResourceUUID rs_get_uuid() const { return rs_uuid; }
    inline fds_uint32_t       rs_my_index() { return rs_index; }
    inline fds_mutex *        rs_mutex() { return &rs_mtx; }

    void setName(std::string name) {
        strncpy(rs_name, name.c_str(), RS_NAME_MAX);
        rs_name[RS_NAME_MAX - 1] = '\0';
    }

  protected:
    friend class RsContainer;
    ResourceUUID             rs_uuid;
    fds_uint32_t             rs_index;
    char                     rs_name[RS_NAME_MAX];
    fds_mutex                rs_mtx;

    virtual ~Resource() {}
    explicit Resource(const ResourceUUID &uuid)
        : rs_uuid(uuid), rs_mtx("rs-mtx"), rs_refcnt(0) {}

  private:
    INTRUSIVE_PTR_DEFS(Resource, rs_refcnt);
};

class ResourceIter
{
  public:
    ResourceIter() {}
    virtual ~ResourceIter() {}

    /**
     * Return true to continue the iteration loop; false to quit.
     */
    virtual bool rs_iter_fn(Resource::pointer curr) = 0;

  protected:
    friend class RsContainer;
    int          rs_iter_cnt;
};

typedef std::unordered_map<ResourceUUID, Resource::pointer, UuidHash> RsUuidMap;
typedef std::unordered_map<std::string, Resource::pointer> RsNameMap;
typedef std::list<Resource::pointer>   RsList;
typedef std::vector<Resource::pointer> RsArray;

// ----------------------------------------------------------------------------
// Generic Resource Container
// ----------------------------------------------------------------------------
class RsContainer
{
  public:
    typedef boost::intrusive_ptr<RsContainer> pointer;
    typedef boost::intrusive_ptr<const RsContainer> const_ptr;
    typedef RsArray::const_iterator const_iterator;

    RsContainer();
    virtual ~RsContainer();

    /**
     * Return the number of available elements in the container.
     */
    inline fds_uint32_t rs_available_elm() {
        return rs_cur_idx;
    }
    /**
     * Iterate through the array.  It's thread safe.
     */
    virtual int rs_container_snapshot(RsArray *out);

    /**
     * Iterator plugins.
     */
    virtual void rs_foreach(ResourceIter *iter);

    /**
     * Methods to allocate and reference the node.
     */
    virtual Resource::pointer rs_alloc_new(const ResourceUUID &uuid);
    virtual Resource::pointer rs_get_resource(const ResourceUUID &uuid);
    virtual Resource::pointer rs_get_resource(const char *name);
    virtual void rs_register(Resource::pointer rs);
    virtual void rs_register_mtx(Resource::pointer rs);
    virtual void rs_unregister(Resource::pointer rs);
    virtual void rs_unregister_mtx(Resource::pointer rs);
    virtual void rs_free_resource(Resource::pointer rs);

  protected:
    RsUuidMap                rs_uuid_map;
    RsNameMap                rs_name_map;
    RsArray                  rs_array;
    RsList                   rs_elem;
    fds_uint32_t             rs_cur_idx;
    fds_mutex                rs_mtx;

    /**
     * Factory method.
     */
    virtual Resource *rs_new(const ResourceUUID &uuid) = 0;

    /**
     * Disable the use of cbegin(), cend() by clients.
     */
    inline const_iterator cbegin() const {
        return rs_array.cbegin();
    }
    inline const_iterator cend() const {
        return rs_array.cbegin() + rs_cur_idx;
    }

  private:
    INTRUSIVE_PTR_DEFS(RsContainer, rs_refcnt);
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
    inline void query_push(ChainLink *chain) { q_list.chain_add_back(chain); }
    inline T *query_peek() { return q_list.chain_peek_front<T>(); }

    /* ---------------------------------------------------------------------- */
    inline void query_iter_reset() { q_list.chain_iter_init(&q_iter); }
    inline bool query_iter_term() { return q_list.chain_iter_term(q_iter); }
    inline void query_iter_next() { return q_list.chain_iter_next(&q_iter); }
    inline T *query_iter_current() { return q_list.chain_iter_current<T>(q_iter); }

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

std::ostream& operator<< (std::ostream& os, const fds::ResourceUUID& uuid);
}  // namespace fds

#endif  // SOURCE_INCLUDE_FDS_RESOURCE_H_
