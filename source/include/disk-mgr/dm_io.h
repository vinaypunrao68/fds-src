#ifndef INCLUDE_DISK_MGR_DM_IO_H_
#define INCLUDE_DISK_MGR_DM_IO_H_

#include <cpplist.h>
#include <fds_types.h>
#include <fds_module.h>
#include <fds_request.h>
#include <disk-mgr/dm_metadata.h>

namespace diskio {

// ---------------------------------------------------------------------------
// IO Request to meta-data index.
// ---------------------------------------------------------------------------
class IndexRequest : public fdsio::Request
{
  public:
    IndexRequest(meta_obj_id_t       &oid,
                 bool                block);
    IndexRequest(meta_vol_io_t       &vio,
                 bool                block);
    IndexRequest(meta_obj_id_t       &oid,
                 meta_vol_io_t       &vio,
                 bool                block);

    virtual void req_abort();
    virtual void req_submit();
    virtual bool req_wait_cond();
    virtual void req_set_wakeup_cond();

    meta_obj_id_t          idx_oid;
    meta_vol_io_t          idx_vio;
    meta_obj_map_t         idx_vmap;
};

// ---------------------------------------------------------------------------
// IO Request to read/write data to a disk location.
// ---------------------------------------------------------------------------
class DiskRequest : public IndexRequest
{
  public:
    // \DiskRequest
    // ------------
    // Allocate request to do read/write to back-end storage.  Pass this obj
    // to DiskIO's read/write methods.  The request follows Request's object
    // life cycle.
    //
    // @param vio (i) : valid value if the key is { vol_uuid, vol_offset }.
    // @param oid (i) : valid value if the key is { object id hash }.
    // @param buf (i/o) : input for read, output for write.
    //
    DiskRequest(meta_vol_io_t       &vio,
                meta_obj_id_t       &oid,
                fds::ObjectBuf      &buf,
                bool                block);

    // \DiskRequest
    // ------------
    // Use this constructor when overwriting an old content and want to save
    // it in a snapshot volume or mark it for garbage collector to delete.
    //
    // @param old_oid (i) : nullptr if don't have the old obj id;
    //    otherwise, must have valid obj id values in the index.
    // @param new_vol (i) : nullptr if don't have any snapshot to host
    //    the old data, old data will be marked for deletion.  Otherwise, must
    //    have valid { vol_uuid, vol_offset } values.
    //
    DiskRequest(meta_vol_io_t       &vio,
                meta_obj_id_t       &oid,
                meta_obj_id_t       *old_oid,
                meta_vol_io_t       *new_vol,
                fds::ObjectBuf      &buf,
                bool                block);

    ~DiskRequest();

    // \req_has_remap_oid
    // ------------------
    // Check if the requester has old object ID to remap to new volume or
    // delete out of the index.  Used when we receive new data that overwrites
    // an old oid and we want to remap the old oid to a new snapshot volume.
    //
    // @param old_oid (o) : the old object ID to remap to new vol.
    // @param new_vol (o) : nullptr if want to delete the old_oid.
    // @return true if the request has old_oid to remap or delete.
    //
    bool req_has_remap_oid(meta_obj_id_t *old_oid, meta_vol_io_t *new_vol);

    virtual void req_submit() = 0;
    virtual void req_complete() = 0;

  protected:
    fds::ObjectBuf           &dat_buf;
    meta_vol_io_t            dat_new_vol;
    meta_obj_id_t            dat_old_oid;
};

// ---------------------------------------------------------------------------
// Generic plug-in IO function.
//
// The IO function attemps to classify data as a function of time during its
// life cycle.
//    F(data, t) = [0...100]
// Where F(data, t0) = 0       -> 100% write, 0% read.
//       F(data, ti) = [0-100] -> attemp to characterize IO demand schedule.
//       F(data, tn) = 100     -> 0% write, 100% read, archiving data.
//
// The output helps DataIO object to put data to the correct bucket that's
// optimized for the current age during its life cycle.
//
// The long term vision for this function is to incorporate the knowledge of
// application working set, content characteristics, and offset properties to
// derrive IO workload for a given time of data's lifecycle.
// ---------------------------------------------------------------------------
class DataIO;
class DataIOFunc;

enum { io_all_write = 0, io_all_read = 100 };
enum { io_tier_fast = 0, io_tier_slow = 10 };

class DataIOFuncParams
{
  public:
    // \data_io_func
    // -------------
    // Return the IO function for short term next time_delta from now
    // (e.g. 1 day).  TODO: change time_delta to the right type.
    //
    virtual int
    data_io_func(fds_uint32_t time_delta, int *tier);

  private:
    DataIOFuncParams();
    ~DataIOFuncParams();
    friend class DataIOFunc;

    int          io_func;
    int          io_tier;
};

class DataIOFunc
{
  public:

    // \data_iofn_singleton
    // --------------------
    // Return the singleton obj to use with DataIO.
    //
    static DataIOFunc &data_iofn_singleton();

    // \disk_io_func
    // -------------
    // Given volume offset, object id, or location map, return a number
    // between 0 to 100 to characterize IO workload at the current time
    // where:
    //    0 is 100% write, 0% read.
    //  100 is 0% write, 100% read.
    //
    // @param vio (i) : data location in volume.
    // @param oid (i) : content based data routing.
    // @param map (i) : physical location for data.
    // @param time_delta (i): short term function based on the time.
    //    TODO: change to the right type, unit.
    // @param iofn (o) : short term function (< 1 day) to give rough estimate
    //    about IO characteristic.
    //
    void data_io_func(meta_vol_adr_t   &vio,
                      meta_obj_id_t    &oid,
                      meta_obj_map_t   &map,
                      fds_uint64_t     time_delta,
                      DataIOFuncParams *iofn);
  private:
    friend class DataIO;
    DataIOFunc();
    ~DataIOFunc();
};

// ---------------------------------------------------------------------------
// Proxy to index meta-data
//
// Proxy to provide uniform interface for DataIO to access to meta-data index.
// All requests are non-blocking call.  The DataIndexProxy works with
// RequestQueue to manage pending request.
// ---------------------------------------------------------------------------
class DataIndexProxy
{
  public:
    DataIndexProxy(int nr_queue, int max_depth);
    ~DataIndexProxy();

    // \disk_index_put
    // ---------------
    // Given the key in request structure, put different types of values to
    // the index database.
    //
    void disk_index_put(IndexRequest &req, meta_obj_map_t &value);
    void disk_index_put(IndexRequest &req, meta_vol_adr_t &value);
    void disk_index_put(IndexRequest &req, meta_obj_id_t &value);

    // \disk_index_get
    // ---------------
    // Given the key in request structure, get different types of values from
    // the index database.
    //
    void disk_index_get(IndexRequest &req, meta_obj_map_t *value);
    void disk_index_get(IndexRequest &req, meta_vol_adr_t *value);
    void disk_index_get(IndexRequest &req, meta_obj_id_t *value);

    // \disk_index_update
    // ------------------
    // Given the key in request structure, update the physical location
    // mapping.  The caller needs to call the commit method to save
    // the key to persistent storage.
    //
    void disk_index_update(IndexRequest &req, meta_obj_map_t &map);
    void disk_index_commit(IndexRequest &req);

    // \disk_index_remove
    // ------------------
    // Remove all records associated with the key in the request structure.
    //
    void disk_index_remove(IndexRequest &req);

    // \disk_index_inc/dec_ref
    // -----------------------
    // Inc or dec refcount associated with the key in the request structure.
    // The update to map an object ID to new volume that has reference to
    // it is done in separate index update transaction.
    //
    void disk_index_inc_ref(IndexRequest &req);
    void disk_index_dec_ref(IndexRequest &req);

  private:
    fdsio::RequestQueue      idx_queue;
};

// ---------------------------------------------------------------------------
// Data IO interface
//
// Save data associated with an object and its index to persistent storage.
// The request's life cycle follows the same model defined by generic Request
// object.
// ---------------------------------------------------------------------------
extern "C" void disk_mgr_init(void);

class DataIO
{
  public:
    static DataIO &disk_singleton();

    // \disk_read
    // ----------
    // Read data to the buffer specified by oid or vio from the request.
    // The call can be blocking or non-blocking.
    // - Non-blocking: do other works, methods in the request obj will
    //   be called in this order: req_submit(), req_complete().
    // - Blocking: call req_wait() to block util the request is done.
    //
    void disk_read(DiskRequest &req);

    // \disk_write
    // -----------
    // Write data to the buffer specified by oid or vio from the request.
    // Similar to the read method, the call can be blocking or non-blocking.
    //
    void disk_write(DiskRequest &req);

    // \disk_remap_obj
    // ---------------
    // Remap the object ID with current vio address to the new vio address.
    // Used to snapshot data about to be overwritten to a snapshot volume.
    //
    void disk_remap_obj(meta_obj_id_t &obj_id,
                        meta_vol_io_t &cur_vio,
                        meta_vol_io_t &new_vio);

    // \disk_destroy_vol
    // -----------------
    // Notify disk mgr to destroy the volume and reclaim spaces that it
    // consumed.
    //
    void disk_destroy_vol(meta_vol_adr_t &vol);

    // \disk_loc_path_info
    // -------------------
    // Return the real physical path matching the location id.
    //
    void disk_loc_path_info(fds_uint16_t loc_id, std::string *path);

  private:
    friend void disk_mgr_init(void);
    DataIO(DataIndexProxy &idx,
           DataIOFunc     &iofn,
           int            nr_tier,
           int            max_depth);
    ~DataIO();

    DataIOFunc               &io_func;
    DataIndexProxy           &io_index;
};

// ---------------------------------------------------------------------------
// Persistent Data Module Initialization
//
// ---------------------------------------------------------------------------
class DataIOModule : public fds::Module
{
  public:
    DataIOModule(char const *const name);
    ~DataIOModule();

    virtual void mod_init(fds::SysParams const *const param);
    virtual void mod_startup();
    virtual void mod_shutdown();
};

extern DataIOModule          dataIOMod;

} // namespace diskio

#endif /* INCLUDE_DISK_MGR_DM_IO_H_ */
