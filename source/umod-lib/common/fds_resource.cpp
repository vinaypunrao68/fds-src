/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <fds_assert.h>
#include <fds_resource.h>
#include <fdsp_utils.h>
#include <util/stringutils.h>
#include <string>
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
Resource::pointer RsContainer::rs_alloc_new(const ResourceUUID &uuid) {
    Resource *rs = rs_new(uuid);

    rs->rs_index = INVALID_INDEX;
    return rs;
}

// rs_register
// -----------
//
bool RsContainer::rs_register(Resource::pointer rs) {
    FDSGUARD(rs_mtx);
    bool ret = rs_register_mtx(rs);
    return ret;
}

// rs_register_mtx
// ---------------
// Same as the call above except the caller must hold the mutex.
//
bool RsContainer::rs_register_mtx(Resource::pointer rs) {
    fds_uint32_t i;

    fds_verify(rs->rs_uuid.uuid_get_val() != 0);
    fds_verify(rs->rs_name[0] != '\0');
    rs->rs_name[RS_NAME_MAX - 1] = '\0';

    std::string lowername = util::strlower(rs->rs_name);

    // check if the resource has state ..
    // if it has state then add the resource only if the previous
    // resource has not been deleted or marked for delete

    auto iter = rs_name_map.find(lowername.c_str());
    if (iter != rs_name_map.end()) {
        HasState* state = dynamic_cast<HasState*>(iter->second.get()); //NOLINT
        if (state) {
            if (state->isStateLoading()
                || state->isStateCreated()
                || state->isStateActive()
                || state->getState() == fpi::ResourceState::Unknown
                ) {
                GLOGWARN << "an active resource already exists with the same name.. "
                         << " name:" << iter->second->rs_get_name()
                         << " uuid:" << iter->second->rs_get_uuid()
                         << " state:" << state->getStateName();
                return false;
            } else {
                GLOGDEBUG << "will overide another resource with the same name.. "
                          << " name:" << iter->second->rs_get_name()
                          << " uuid:" << iter->second->rs_get_uuid()
                          << " state:" << state->getStateName();
            }
        }
    }
    rs_name_map[lowername.c_str()] = rs;
    rs_uuid_map[rs->rs_uuid] = rs;

    for (i = 0; i < rs_cur_idx; i++) {
        if (rs_array[i] == rs) {
            GLOGWARN << "resource already exists... ";
            return false;
        }
        if (rs_array[i] == NULL) {
            // Found an empty slot, use it.
            rs_array[i] = rs;
            rs->rs_index = i;
            return true;
        }
        fds_verify(rs_array[i] != rs);
    }
    fds_verify(rs_array[rs_cur_idx] == NULL);
    rs_array[rs_cur_idx] = rs;
    rs->rs_index = rs_cur_idx;
    rs_cur_idx++;
    return true;
}

// rs_unregister
// -------------
//
bool RsContainer::rs_unregister(Resource::pointer rs) {
    FDSGUARD(rs_mtx);
    bool ret = rs_unregister_mtx(rs);
    return ret;
}

// rs_unregister_mtx
// -----------------
// Same as the call above except the caller must hold the mutex.
//
bool RsContainer::rs_unregister_mtx(Resource::pointer rs) {
    fds_verify(rs->rs_uuid.uuid_get_val() != 0);
    fds_verify(rs->rs_name[0] != '\0');
    fds_verify(rs_array[rs->rs_index] == rs);

    rs_uuid_map.erase(rs->rs_uuid);

    std::string lowername = util::strlower(rs->rs_name);
    auto iter = rs_name_map.find(lowername.c_str());
    if (iter != rs_name_map.end()) {
        if (iter->second == rs) {
            rs_name_map.erase(iter);
            return true;
        } else {
            GLOGWARN << "resource [" << iter->second->rs_uuid.uuid_get_val()
                     << "] exists with name : " << rs->rs_name
                     << " which is diff from uuid : " << rs->rs_uuid.uuid_get_val();
        }
    }
    return false;
}

// rs_get_resource
// ---------------
Resource::pointer
RsContainer::rs_get_resource(const ResourceUUID &uuid)
{
    auto iter = rs_uuid_map.find(uuid);
    if (iter != rs_uuid_map.end()) {
        return iter->second;
    }
    return NULL;
}

Resource::pointer
RsContainer::rs_get_resource(const char *name)
{
    std::string lowername = util::strlower(name);
    auto iter = rs_name_map.find(lowername.c_str());
    if (iter != rs_name_map.end()) {
        return iter->second;
    }
    return NULL;
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
    FDSGUARD(rs_mtx);
    for (auto it = cbegin(); it != cend(); it++) {
        if (*it != NULL) {
            ret++;
            out->push_back(*it);
        }
    }
    fds_verify(ret <= rs_cur_idx);

    return ret;
}

// rs_free_resource
// ----------------
//
bool RsContainer::rs_free_resource(Resource::pointer rs) {
    if (rs->rs_index != INVALID_INDEX) {
        fds_verify(rs_array[rs->rs_index] == rs);
        rs_array[rs->rs_index] = NULL;
        rs->rs_index = INVALID_INDEX;
        return true;
    }
    // Don't need to do anything more.  The ptr will be freed when its refcnt is 0.
    return false;
}

// rs_foreach
// ----------
//
void
RsContainer::rs_foreach(ResourceIter *iter)
{
    FDSGUARD(rs_mtx);
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

//  --- States
std::string HasState::getStateName() const {
    auto iter = fpi::_ResourceState_VALUES_TO_NAMES.find(getState());
    if (iter != fpi::_ResourceState_VALUES_TO_NAMES.end()) {
        return iter->second;
    }
    return "Unknown";
}

bool HasState::isStateLoading() const {
    return getState() == fpi::ResourceState::Loading;
}

bool HasState::isStateCreated() const {
    return getState() == fpi::ResourceState::Created;
}

bool HasState::isStateActive() const {
    return getState() == fpi::ResourceState::Active;
}

bool HasState::isStateOffline() const {
    return getState() == fpi::ResourceState::Offline;
}

bool HasState::isStateMarkedForDeletion() const {
    return getState() == fpi::ResourceState::MarkedForDeletion;
}

bool HasState::isStateDeleted() const {
    return getState() == fpi::ResourceState::Deleted;
}

//  ----------

QueryMgr::QueryMgr()
{
}

QueryMgr::~QueryMgr()
{
}

}  // namespace fds
