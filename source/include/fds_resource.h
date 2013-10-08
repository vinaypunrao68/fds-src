#ifndef INCLUDE_FDS_RESOURCE_H_
#define INCLUDE_FDS_RESOURCE_H_

#include <shared/fds_types.h>
#include <shared/dlist.h>

namespace fds {
class RsList;

class ResourceUUID
{
  public:
    ResourceUUID(fds_uint64_t uuid) : rs_uuid(uuid) {}

    fds_uint64_t             rs_uuid;
};

template <class T>
class RsChain
{
  public:
    RsChain(T *obj) : rs_obj(obj) { dlist_init(&rs_link); }
    inline T *chain_obj(RsChain *lnk) { return (T *)lnk->rs_obj; }

  private:
    friend class RsList;

    T                      *rs_obj;
    dlist_t                 rs_link;
};

class RsList
{
  public:
    RsList() : rs_count(0), rs_iter(nullptr) {
        dlist_init(&rs_result);
    }
    ~RsList();

  private:
    int                      rs_count;
    dlist_t                  rs_result;
    dlist_t                 *rs_iter;
};

class QueryOut;
class QueryMgr;

class QueryIn
{
};

class QueryOut
{
  public:
    QueryOut();
    ~QueryOut();

  private:
    friend class QueryMgr;

    RsList                   out_list;
};

class QueryMgr
{
  public:
    ~QueryMgr();
    QueryMgr();
};

} // namespace fds

#endif /* INCLUDE_FDS_RESOURCE_H_ */
