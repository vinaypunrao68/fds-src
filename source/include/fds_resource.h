#ifndef INCLUDE_FDS_RESOURCE_H_
#define INCLUDE_FDS_RESOURCE_H_

#include <shared/fds_types.h>
#include <cpplist.h>

namespace fds {

class ResourceUUID
{
  public:
    ResourceUUID(fds_uint64_t uuid) : rs_uuid(uuid) {}

    fds_uint64_t             rs_uuid;
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

    ChainList                out_list;
};

class QueryMgr
{
  public:
    ~QueryMgr();
    QueryMgr();
};

} // namespace fds

#endif /* INCLUDE_FDS_RESOURCE_H_ */
