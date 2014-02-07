/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <fds_assert.h>
#include <fds_resource.h>

namespace fds {

ResourceUUID::ResourceUUID(fds_uint64_t val)
    : rs_uuid(val)
{
}

RsContainer::RsContainer()
    : rs_refcnt(0), rs_mtx("rs-container"), rs_array(RS_DEFAULT_ELEM_CNT) {}

RsContainer::~RsContainer() {}

// rs_alloc_new
// ------------
//
Resource::pointer
RsContainer::rs_alloc_new()
{
    Resource *rs = rs_new();

    rs_mtx.lock();
    fds_verify(rs_array[rs_cur_idx] == NULL);

    rs_array[rs_cur_idx] = rs;
    rs->rs_index = rs_cur_idx;
    rs_cur_idx++;
    rs_mtx.unlock();

    return rs;
}

// rs_register
// -----------
//
void
RsContainer::rs_register(Resource::pointer rs)
{
    rs_mtx.lock();
    rs_register_mtx(rs);
    rs_mtx.unlock();
}

// rs_register_mtx
// ---------------
// Same as the call above except the caller must hold the mutex.
//
void
RsContainer::rs_register_mtx(Resource::pointer rs)
{
    fds_verify(rs->rs_uuid.uuid_get_val() != 0);
    fds_verify(rs->rs_name[0] != '\0');
    rs->rs_name[RS_NAME_MAX - 1] = '\0';

    rs_uuid_map[rs->rs_uuid] = rs;
    rs_name_map[rs->rs_name] = rs;
}

// rs_unregister
// -------------
//
void
RsContainer::rs_unregister(Resource::pointer rs)
{
    rs_mtx.lock();
    rs_unregister_mtx(rs);
    rs_mtx.unlock();
}

// rs_unregister_mtx
// -----------------
// Same as the call above except the caller must hold the mutex.
//
void
RsContainer::rs_unregister_mtx(Resource::pointer rs)
{
    fds_verify(rs->rs_uuid.uuid_get_val() != 0);
    fds_verify(rs->rs_name[0] != '\0');
    fds_verify(rs_array[rs->rs_index] == rs);

    // The resource can still be looked up by UUID until we destroy it.
    rs_name_map.erase(rs->rs_name);
}

// rs_get_resource
// ---------------
Resource::pointer
RsContainer::rs_get_resource(const ResourceUUID *uuid)
{
    return rs_uuid_map[*uuid];
}

Resource::pointer
RsContainer::rs_get_resource(char const *const name)
{
    return rs_name_map[name];
}

QueryMgr::QueryMgr()
{
}

QueryMgr::~QueryMgr()
{
}

}  // namespace fds
