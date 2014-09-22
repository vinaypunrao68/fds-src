/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_PERSISTENT_LAYER_DM_IO_H_
#define SOURCE_INCLUDE_PERSISTENT_LAYER_DM_IO_H_

#include <string>
#include <vector>
#include <gtest/gtest_prod.h>

#include <cpplist.h>
#include <fds_error.h>
#include <fds_types.h>
#include <fds_module.h>
#include <fds_request.h>
#include <persistent_layer/dm_service.h>
#include <persistent_layer/dm_metadata.h>

namespace diskio {

class DataIO;
class DataIOFunc;
class PersisDataIO;
class FilePersisDataIO;
class DataIOModule;
class DataIndexModule;
class DataIndexProxy;

enum DataTier {
    diskTier  = 0,
    flashTier = 1,
    maxTier
};

struct DiskStat {
    fds_uint64_t dsk_tot_size;      // total size in bytes
    fds_uint64_t dsk_avail_size;    // in bytes
    fds_uint64_t dsk_reclaim_size;  // in bytes
};

struct TokenStat {
    fds::fds_token_id tkn_id;
    fds_uint64_t      tkn_tot_size;      // total size of disk files in bytes
    fds_uint64_t      tkn_reclaim_size;  // total reclaimable space in bytes
};

// ---------------------------------------------------------------------------
// IO Request to meta-data index.
// ---------------------------------------------------------------------------
class IndexRequest : public fdsio::Request
{
  protected:
    meta_obj_id_t          idx_oid;
    meta_vol_io_t          idx_vio;
    meta_obj_map_t         idx_vmap;
    obj_phy_loc_t          idx_phy_loc;
    IndexRequest           *idx_peer;

  public:
    IndexRequest(meta_obj_id_t const &oid,
                 bool                block);
    IndexRequest(meta_vol_io_t const &vio,
                 bool                block);
    IndexRequest(meta_obj_id_t const &oid,
                 meta_vol_io_t const &vio,
                 bool                block);

    virtual void req_abort();
    virtual void req_submit();
    virtual bool req_wait_cond();
    virtual void req_set_wakeup_cond();

    inline meta_obj_id_t const *const req_get_oid() const { return &idx_oid; }
    inline meta_vol_io_t const *const req_get_vio() const { return &idx_vio; }
    inline meta_obj_map_t *req_get_vmap() { return &idx_vmap; }
    inline obj_phy_loc_t *req_get_phy_loc() { return &idx_phy_loc; }
    inline void set_phy_loc(obj_phy_loc_t *pl) {
        memcpy(&idx_phy_loc, pl, sizeof(obj_phy_loc_t));
    }
    inline void req_set_peer(IndexRequest *peer) { idx_peer = peer; }
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
                fds::ObjectBuf      *buf,
                bool                block);
    // Same as above constructor w/ tier
    DiskRequest(meta_vol_io_t       &vio,
                meta_obj_id_t       &oid,
                fds::ObjectBuf      *buf,
                bool                block,
                DataTier            tier);

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
                fds::ObjectBuf      *buf,
                bool                block);
    // Same as above constructor w/ tier
    DiskRequest(meta_vol_io_t       &vio,
                meta_obj_id_t       &oid,
                meta_obj_id_t       *old_oid,
                meta_vol_io_t       *new_vol,
                fds::ObjectBuf      *buf,
                bool                block,
                DataTier            tier);

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

    inline DataTier getTier() const { return datTier; }
    inline fds::ObjectBuf const *const req_obj_buf() { return dat_buf; }
    inline fds::ObjectBuf *req_obj_rd_buf() { return dat_buf; }

  protected:
    fds::ObjectBuf           *dat_buf;
    meta_vol_io_t            dat_new_vol;
    meta_obj_id_t            dat_old_oid;
    DataTier                 datTier;
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

enum { io_all_write = 0, io_all_read = 100 };
enum { io_tier_fast = 0, io_tier_slow = 10 };

class DataIOFuncParams
{
  public:
    // \data_io_func
    // -------------
    // Return the IO function for short term next time_delta from now
    // (e.g. 1 day).  TODO(Vy): change time_delta to the right type.
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
    // TODO(Vy): change to the right type, unit.
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
    static DataIndexProxy &disk_index_singleton();

    // \disk_index_put
    // ---------------
    // Given the key in request structure, put different types of values to
    // the index database.
    //
    void disk_index_put(IndexRequest *req, meta_obj_map_t *value);
    void disk_index_put(IndexRequest *req, meta_vol_adr_t *value);
    void disk_index_put(IndexRequest *req, meta_obj_id_t *value);

    // \disk_index_get
    // ---------------
    // Given the key in request structure, get different types of values from
    // the index database.
    //
    void disk_index_get(IndexRequest *req, meta_obj_map_t *value);
    void disk_index_get(IndexRequest *req, meta_vol_adr_t *value);
    void disk_index_get(IndexRequest *req, meta_obj_id_t *value);

    // \disk_index_update
    // ------------------
    // Given the key in request structure, update the physical location
    // mapping.  The caller needs to call the commit method to save
    // the key to persistent storage.
    //
    void disk_index_update(IndexRequest *req, meta_obj_map_t *map);
    void disk_index_commit(IndexRequest *req);

    // \disk_index_remove
    // ------------------
    // Remove all records associated with the key in the request structure.
    //
    void disk_index_remove(IndexRequest *req);

    // \disk_index_inc/dec_ref
    // -----------------------
    // Inc or dec refcount associated with the key in the request structure.
    // The update to map an object ID to new volume that has reference to
    // it is done in separate index update transaction.
    //
    void disk_index_inc_ref(IndexRequest *req);
    void disk_index_dec_ref(IndexRequest *req);

  private:
    friend class DataIndexModule;
    explicit DataIndexProxy(int max_depth);
    ~DataIndexProxy();

    fdsio::RequestQueue      idx_queue;
};

// ---------------------------------------------------------------------------
// Data IO interface
//
// Save data associated with an object and its index to persistent storage.
// The request's life cycle follows the same model defined by generic Request
// object.
// ---------------------------------------------------------------------------
class DataIO
{
  public:
    static DataIO &disk_singleton();

    // Units for block IO to persistent data store.
    //
    static inline fds_uint32_t disk_io_blk_shift() {
        return fds::DmQuery::dm_blk_shift;
    }
    static inline fds_uint32_t disk_io_blk_size() {
        return fds::DmQuery::dm_blk_size;
    }
    static inline fds_uint32_t disk_io_blk_mask() {
        return fds::DmQuery::dm_blk_mask;
    }
    // Round up the given byte to the next block size.
    //
    static inline fds_blk_t disk_io_round_up_blk(fds_uint64_t byte)
    {
        fds_uint32_t shft = fds::DmQuery::dm_blk_shift;
        fds_uint32_t mask = fds::DmQuery::dm_blk_mask;
        return (byte >> shft) + ((byte & mask) == 0 ? 0 : 1);
    }

    // \disk_read
    // ----------
    // Read data to the buffer specified by oid or vio from the request.
    // The call can be blocking or non-blocking.
    // - Non-blocking: do other works, methods in the request obj will
    //   be called in this order: req_submit(), req_complete().
    // - Blocking: call req_wait() to block util the request is done.
    //
    fds::Error disk_read(DiskRequest *req);

    // \disk_write
    // -----------
    // Write data to the buffer specified by oid or vio from the request.
    // Similar to the read method, the call can be blocking or non-blocking.
    //
    fds::Error disk_write(DiskRequest *req);

    // \disk_remap_obj
    // ---------------
    // Remap the object ID with current vio address to the new vio address.
    // Used to snapshot data about to be overwritten to a snapshot volume.
    //
    void disk_remap_obj(meta_obj_id_t const *const obj_id,
                        meta_vol_io_t const *const cur_vio,
                        meta_vol_io_t const *const new_vio);

    // \disk_destroy_vol
    // -----------------
    // Notify disk mgr to destroy the volume and reclaim spaces that it
    // consumed.
    //
    void disk_destroy_vol(meta_vol_adr_t *vol);

    // \disk_loc_path_info
    // -------------------
    // Return the real physical path matching the location id.
    //
    void disk_loc_path_info(fds_uint16_t loc_id, std::string *path);

    /**
     * Notify that object was deleted to update stats for garbage collection
     */
    void disk_delete_obj(meta_obj_id_t const *const oid, fds_uint32_t obj_size,
                         obj_phy_loc_t* loc);

    /**
     * Notify about start garbage collection for given token id 'tok_id'
     * and tier. If many disks contain this token, then token file on each
     * disk will be compacted. All new writes will be re-routed to the
     * appropriate (new) token file.
     */
    void notify_start_gc(fds::fds_token_id tok_id,
                         fds_uint16_t loc_id,
                         DataTier tier);

    /**
     * Notify about end of garbage collection for a given token id
     * 'tok_id' and tier.
     */
    fds::Error notify_end_gc(fds::fds_token_id tok_id,
                             fds_uint16_t loc_id,
                             DataTier tier);


    /**
     * Returns true if a given location is a shadow file
     */
    fds_bool_t is_shadow_location(obj_phy_loc_t* loc,
                                  fds::fds_token_id tok_id);

  private:
    friend class DataIOModule;

    DataIO();
    ~DataIO();
};

// ---------------------------------------------------------------------------
// Persistent Data IO Module
// ---------------------------------------------------------------------------
class tokenFileDb;
typedef std::unordered_map<int, PersisDataIO *>      DataIOMap;

class DataIOModule : public fds::Module
{
  protected:
    fds_uint32_t             io_ssd_total;
    fds_uint32_t             io_ssd_curr;
    fds_uint32_t             io_hdd_total;
    fds_uint32_t             io_hdd_curr;

    fds_uint16_t            *io_ssd_diskid;
    fds_uint16_t            *io_hdd_diskid;
    fds::fds_mutex           io_mtx;

    tokenFileDb             *io_token_db;

  public:
    explicit DataIOModule(char const *const name);
    ~DataIOModule();

    virtual int  mod_init(fds::SysParams const *const param);
    virtual void mod_startup();
    virtual void mod_shutdown();

    virtual void disk_mount_ssd_path(const char *path, fds_uint16_t disk_id);
    virtual void disk_mount_hdd_path(const char *path, fds_uint16_t disk_id);

    /**
     * Use this API to get the object handler when we know the disk_id from physical
     * location.  The disk_id is 16-bit value mapping to 64-bit uuid retrieved from
     * the supperblock.
     */
    virtual PersisDataIO *
    disk_hdd_disk(DataTier tier, fds_uint16_t disk_id, fds_uint32_t file_id,
                  meta_obj_id_t const *const token_val);

    /**
     * Use this API to write the object with token_val to a disk location.  The object
     * handler will put the disk_id, disk_offset to the physical location which will
     * be saved to meta-data used with the API above.
     */
    virtual PersisDataIO *
    disk_hdd_io(DataTier tier, fds_uint32_t file_id, meta_obj_id_t const *const token);

    /**
     * Returns the root path to access the disk for a given SM token
     */
    virtual const char *disk_path(fds::fds_token_id tok_id, DataTier tier);

    /**
     * @return 'ret_stat' aggregated statistics for a given disk 'disk_id' on 'tier'
     */
    virtual fds::Error
    get_disk_stats(DataTier tier, fds_uint16_t disk_id, DiskStat* ret_stat);

    /**
     * @return an array of per disk/token stats in 'ret_tok_stats'
     */
    virtual void get_disk_token_stats(DataTier tier, fds_uint16_t disk_id,
                                      std::vector<TokenStat>* ret_tok_stats);

    /**
     * Notify about start garbage collection for given token id 'tok_id'
     * and tier. If many disks contain this token, then token file on each
     * disk will be compacted. All new writes will be re-routed to the
     * appropriate (new) token file.
     */
    void notify_start_gc(fds::fds_token_id tok_id,
                         fds_uint16_t loc_id,
                         DataTier tier);

    /**
     * Notify about end of garbage collection for a given token id
     * 'tok_id' and tier.
     */
    fds::Error notify_end_gc(fds::fds_token_id tok_id,
                             fds_uint16_t loc_id,
                             DataTier tier);

    /**
     * Returns true if a given location is a shadow file
     */
    fds_bool_t is_shadow_location(obj_phy_loc_t* loc,
                                  fds::fds_token_id tok_id);
};

extern DataIOModule          gl_dataIOMod;
}  // namespace diskio
#endif  // SOURCE_INCLUDE_PERSISTENT_LAYER_DM_IO_H_
