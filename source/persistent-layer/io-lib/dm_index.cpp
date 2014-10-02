/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <iostream>
#include <fds_assert.h>
#include <fds_module.h>
#include <persistent-layer/dm_io.h>
#include <persistent-layer/persistentdata.h>


namespace diskio {

DataIndexModule          dataIndexMod("dataIndex mod");

static DataIndexLDb      *sgt_oidIndex;
static DataIndexLDb      *sgt_vioIndex;
static DataIndexProxy    *sgt_oidProxy;

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

int
DataIndexModule::mod_init(fds::SysParams const *const param)
{
    Module::mod_init(param);
    std::cout << "DataIndex init..." << std::endl;

    sgt_oidProxy = new DataIndexProxy(1000);
    return 0;
}

void
DataIndexModule::mod_startup()
{
    Module::mod_startup();
    std::cout << "DataIndex startup..." << std::endl;
}

void
DataIndexModule::mod_shutdown()
{
}

// \DataIndexProxy
// ---------------
//
DataIndexProxy::DataIndexProxy(int max_depth)
    : idx_queue(1, max_depth)
{
}

DataIndexProxy::~DataIndexProxy()
{
}

DataIndexProxy &
DataIndexProxy::disk_index_singleton()
{
    return *sgt_oidProxy;
}

// \disk_index_put
// ---------------
//
void
DataIndexProxy::disk_index_put(IndexRequest *req, meta_obj_map_t *value)
{
    meta_obj_id_t const *const oid = req->req_get_oid();

    fds_verify(obj_map_has_init_val(value) == false);
    if (obj_id_is_valid(oid) == true) {
        // save <oid, obj_map> pair.
    } else {
        meta_vol_io_t const *const vio = req->req_get_vio();

        // save <vio, obj_map> pair.
    }
    req->req_complete();
}

void
DataIndexProxy::disk_index_put(IndexRequest *req, meta_vol_adr_t *value)
{
    meta_obj_id_t const *const oid = req->req_get_oid();

    fds_verify(vadr_is_valid(value) == true);
    fds_verify(obj_id_is_valid(oid) == true);

    // save <oid, vio> pair.
    req->req_complete();
}

void
DataIndexProxy::disk_index_put(IndexRequest *req, meta_obj_id_t *value)
{
    meta_vol_io_t const *const vio = req->req_get_vio();

    fds_verify(obj_id_is_valid(value) == true);

    // save <vio, oid> pair.
    req->req_complete();
}

// \disk_index_get
// ---------------
//
void
DataIndexProxy::disk_index_get(IndexRequest *req, meta_obj_map_t *value)
{
    meta_obj_id_t const *const oid = req->req_get_oid();

    fds_verify(value != nullptr);
    if (obj_id_is_valid(oid) == true) {
        // put <oid, obj_map> to value.
    } else {
    }
    req->req_complete();
}

void
DataIndexProxy::disk_index_get(IndexRequest *req, meta_vol_adr_t *value)
{
    meta_obj_id_t const *const oid = req->req_get_oid();

    fds_verify(value != nullptr);
    fds_verify(obj_id_is_valid(oid) == true);

    req->req_complete();
}

void
DataIndexProxy::disk_index_get(IndexRequest *req, meta_obj_id_t *value)
{
    meta_vol_io_t const *const vio = req->req_get_vio();

    fds_verify(value != nullptr);

    req->req_complete();
}

// \disk_index_update
// ------------------
//
void
DataIndexProxy::disk_index_update(IndexRequest *req, meta_obj_map_t *map)
{
    disk_index_put(req, map);
}

// \disk_index_commit
// ------------------
//
void
DataIndexProxy::disk_index_commit(IndexRequest *req)
{
    req->req_complete();
}

// \disk_index_remove
// ------------------
//
void
DataIndexProxy::disk_index_remove(IndexRequest *req)
{
}

// \disk_index_inc_ref
// -------------------
//
void
DataIndexProxy::disk_index_inc_ref(IndexRequest *req)
{
}

// \disk_index_dec_ref
// -------------------
//
void
DataIndexProxy::disk_index_dec_ref(IndexRequest *req)
{
}

// \DataIndexLDb
// -------------
//
DataIndexLDb::DataIndexLDb(const char *base) : idx_queue(2, 1000)
{
}

DataIndexLDb::~DataIndexLDb()
{
}

}  // namespace diskio
