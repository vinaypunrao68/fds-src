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
DataIndexProxy::disk_index_put(IndexRequest &req, meta_obj_map_t &value)
{
}

void
DataIndexProxy::disk_index_put(IndexRequest &req, meta_vol_adr_t &value)
{
}

void
DataIndexProxy::disk_index_put(IndexRequest &req, meta_obj_id_t &value)
{
}

// \disk_index_get
// ---------------
//
void
DataIndexProxy::disk_index_get(IndexRequest &req, meta_obj_map_t *value)
{
}

void
DataIndexProxy::disk_index_get(IndexRequest &req, meta_vol_adr_t *value)
{
}

void
DataIndexProxy::disk_index_get(IndexRequest &req, meta_obj_id_t *value)
{
}

// \disk_index_update
// ------------------
//
void
DataIndexProxy::disk_index_update(IndexRequest &req, meta_obj_map_t &map)
{
}

// \disk_index_commit
// ------------------
//
void
DataIndexProxy::disk_index_commit(IndexRequest &req)
{
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

} // namespace diskio
