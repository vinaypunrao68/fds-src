/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <fds_assert.h>
#include <fds_resource.h>
#include <fdsp_utils.h>

namespace fds {

// ------------------------------------------------------------------------------------
// Resource types
// ------------------------------------------------------------------------------------
const fds_uint32_t RsType::rs_func_mask[UUID_TYPE_MAX] =
{
    /* fpi::FDSD_PLATFORM    */ RS_IN_NODE_INV,
    /* fpi::FDSP_STOR_MGR    */ RS_IN_NODE_INV,
    /* fpi::FDSP_DATA_MGR    */ RS_IN_NODE_INV,
    /* fpi::FDSP_STOR_HVISOR */ RS_IN_NODE_INV,
    /* fpi::FDSP_ORCH_MGR    */ RS_IN_NODE_INV,
    0
};

// ------------------------------------------------------------------------------------
// Resource UUID
// ------------------------------------------------------------------------------------
const fds_uint32_t INVALID_INDEX = 0xffffffff;

ResourceUUID::ResourceUUID(fds_uint64_t val) : rs_uuid(val) {}
ResourceUUID::ResourceUUID(const fpi::SvcUuid &uuid) : rs_uuid(uuid.svc_uuid) {}
ResourceUUID::ResourceUUID(const fds_uint8_t *raw)
{
    const fds_uint64_t *ptr = reinterpret_cast<const fds_uint64_t *>(raw);
    rs_uuid = *ptr;
}

fpi::SvcUuid ResourceUUID::toSvcUuid() const {
    fpi::SvcUuid s;
    return assign(s, *this);
}

std::ostream& operator<< (std::ostream& os, const fds::ResourceUUID& uuid) {
    os << uuid.uuid_get_val();
    return os;
}

ResourceUUID INVALID_RESOURCE_UUID;

RsContainer::RsContainer()
    : rs_array(RS_DEFAULT_ELEM_CNT), rs_cur_idx(0),
      rs_mtx("rs-container"), rs_refcnt(0) {}

RsContainer::~RsContainer() {}

// rs_alloc_new
// ------------
//
Resource::pointer
RsContainer::rs_alloc_new(const ResourceUUID &uuid)
{
    Resource *rs = rs_new(uuid);

    rs->rs_index = INVALID_INDEX;
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
    fds_uint32_t i;

    fds_verify(rs->rs_uuid.uuid_get_val() != 0);
    fds_verify(rs->rs_name[0] != '\0');
    rs->rs_name[RS_NAME_MAX - 1] = '\0';

    rs_uuid_map[rs->rs_uuid] = rs;
    rs_name_map[rs->rs_name] = rs;

    for (i = 0; i < rs_cur_idx; i++) {
        if (rs_array[i] == rs) {
            return;
        }
        if (rs_array[i] == NULL) {
            // Found an empty slot, use it.
            rs_array[i] = rs;
            rs->rs_index = i;
            return;
        }
        fds_verify(rs_array[i] != rs);
    }
    fds_verify(rs_array[rs_cur_idx] == NULL);
    rs_array[rs_cur_idx] = rs;
    rs->rs_index = rs_cur_idx;
    rs_cur_idx++;
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

    rs_name_map.erase(rs->rs_name);
    rs_uuid_map.erase(rs->rs_uuid);
}

// rs_get_resource
// ---------------
Resource::pointer
RsContainer::rs_get_resource(const ResourceUUID &uuid)
{
    return rs_uuid_map[uuid];
}

Resource::pointer
RsContainer::rs_get_resource(const char *name)
{
    return rs_name_map[name];
}

// rs_container_snapshot
// ---------------------
// Take a snapshot of all elemnts in the array.
//
int
RsContainer::rs_container_snapshot(RsArray *out)
{
    uint ret;

    ret = 0;
    rs_mtx.lock();
    for (auto it = cbegin(); it != cend(); it++) {
        if (*it != NULL) {
            ret++;
            out->push_back(*it);
        }
    }
    fds_verify(ret <= rs_cur_idx);
    rs_mtx.unlock();

    return ret;
}

// rs_free_resource
// ----------------
//
void
RsContainer::rs_free_resource(Resource::pointer rs)
{
    if (rs->rs_index != INVALID_INDEX) {
        rs_mtx.lock();
        fds_verify(rs_array[rs->rs_index] == rs);
        rs_array[rs->rs_index] = NULL;
        rs->rs_index = INVALID_INDEX;
        rs_mtx.unlock();
    }
    // Don't need to do anything more.  The ptr will be freed when its refcnt is 0.
}

// rs_foreach
// ----------
//
void
RsContainer::rs_foreach(ResourceIter *iter)
{
    for (fds_uint32_t i = 0; i < rs_cur_idx; i++) {
        Resource::pointer cur = rs_array[i];
        if (rs_array[i] != NULL) {
            /* The refcnt already set by the cur var. */
            iter->rs_iter_cnt++;
            if (iter->rs_iter_fn(cur) == false) {
                break;
            }
        }
    }
}

QueryMgr::QueryMgr()
{
}

QueryMgr::~QueryMgr()
{
}

}  // namespace fds
