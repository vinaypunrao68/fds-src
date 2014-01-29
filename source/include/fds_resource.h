#ifndef INCLUDE_FDS_RESOURCE_H_
#define INCLUDE_FDS_RESOURCE_H_

#include <shared/fds_types.h>
#include <cpplist.h>

namespace fds {

class Resource
{
  public:
    Resource() {}
    ~Resource() {}
};

class ResourceUUID
{
  public:
    ResourceUUID(fds_uint64_t uuid) : rs_uuid(uuid) {}

    fds_uint64_t             rs_uuid;
};

class QueryMgr;

class QueryIn
{
};

template <class T>
class QueryOut
{
  public:
    QueryOut() : q_list() { q_list.chain_iter_init(&q_iter); }
    ~QueryOut()
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
    ~QueryMgr();
    QueryMgr();
};

} // namespace fds

#endif /* INCLUDE_FDS_RESOURCE_H_ */
