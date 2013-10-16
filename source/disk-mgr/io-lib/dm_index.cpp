#include <iostream>
#include <fds_assert.h>
#include <fds_module.h>
#include <disk-mgr/dm_io.h>
#include <persistentdata.h>

using namespace std;

namespace diskio {

DataIndexModule   dataIndexMod("dataIndex mod");

// \DataIndexModule
// ----------------
//
DataIndexModule::DataIndexModule(char const *const name)
    : Module(name)
{
}

DataIndexModule::~DataIndexModule()
{
}

void
DataIndexModule::mod_init(fds::SysParams const *const param)
{
    Module::mod_init(param);
    cout << "DataIndex init..." << endl;
}

void
DataIndexModule::mod_startup()
{
    Module::mod_startup();
    cout << "DataIndex startup..." << endl;
}

void
DataIndexModule::mod_shutdown()
{
}

// \DataIndexProxy
// ---------------
//
DataIndexProxy::DataIndexProxy(int nr_queue, int max_depth)
    : idx_queue(nr_queue, max_depth)
{
}

DataIndexProxy::~DataIndexProxy()
{
}

// \disk_index_put
// ---------------
//
void
DataIndexProxy::disk_index_put(IndexRequest &req, meta_obj_map_t *value)
{
    fds_verify(obj_map_has_init_val(value) == false);

    if (obj_id_is_valid(&req.idx_oid) == true) {
        // save <oid, obj_map> pair.
    } else {
        fds_verify(vadr_is_valid(req.idx_vio.vol_adr) == true);
        // save <vio, obj_map> pair.
    }
    req.req_complete();
}

void
DataIndexProxy::disk_index_put(IndexRequest &req, meta_vol_adr_t *value)
{
    fds_verify(vadr_is_valid(value) == true);
    fds_verify(obj_id_is_valid(&req.idx_oid) == true);

    // save <oid, vio> pair.
    req.req_complete();
}

void
DataIndexProxy::disk_index_put(IndexRequest &req, meta_obj_id_t *value)
{
    fds_verify(obj_id_is_valid(value) == true);
    fds_verify(vadr_is_valid(req.idx_vio.vol_adr) == true);

    // save <vio, oid> pair.
    req.req_complete();
}

// \disk_index_get
// ---------------
//
void
DataIndexProxy::disk_index_get(IndexRequest &req, meta_obj_map_t *value)
{
    fds_verify(value != nullptr);

    if (obj_id_is_valid(&req.idx_oid) == true) {
        // put <oid, obj_map> to value.
    } else {
    }
    req.req_complete();
}

void
DataIndexProxy::disk_index_get(IndexRequest &req, meta_vol_adr_t *value)
{
    fds_verify(value != nullptr);
    fds_verify(obj_id_is_valid(&req.idx_oid) == true);

    req.req_complete();
}

void
DataIndexProxy::disk_index_get(IndexRequest &req, meta_obj_id_t *value)
{
    fds_verify(value != nullptr);
    fds_verify(vadr_is_valid(req.idx_vio.vol_adr) == true);

    req.req_complete();
}

// \disk_index_update
// ------------------
//
void
DataIndexProxy::disk_index_update(IndexRequest &req, meta_obj_map_t *map)
{
    disk_index_put(req, map);
}

// \disk_index_commit
// ------------------
//
void
DataIndexProxy::disk_index_commit(IndexRequest &req)
{
    req.req_complete();
}

// \disk_index_remove
// ------------------
//
void
DataIndexProxy::disk_index_remove(IndexRequest &req)
{
}

// \disk_index_inc_ref
// -------------------
//
void
DataIndexProxy::disk_index_inc_ref(IndexRequest &req)
{
}

// \disk_index_dec_ref
// -------------------
//
void
DataIndexProxy::disk_index_dec_ref(IndexRequest &req)
{
}

// \DataIndexLDb
// -------------
//
DataIndexLDb::DataIndexLDb(const char *base)
    : idx_queue(2, 1000)
{
}

DataIndexLDb::~DataIndexLDb()
{
}

} // namespace diskio
