/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_PERSISTENT_LAYER_PERSISTENTDATA_H_
#define SOURCE_INCLUDE_PERSISTENT_LAYER_PERSISTENTDATA_H_

#include <string>
#include <unordered_map>
#include <set>
#include <persistent-layer/dm_io.h>
#include <concurrency/Mutex.h>
#include <fds_error.h>

namespace diskio {

class DataIndexModule : public fds::Module
{
  public:
    explicit DataIndexModule(char const *const name);
    ~DataIndexModule();

    virtual int  mod_init(fds::SysParams const *const param);
    virtual void mod_startup();
    virtual void mod_shutdown();
};

class DataDiscoveryModule;
class DataDiscIter : public fds::FdsCallback
{
  protected:
    friend class DataIOModule;
    friend class DataDiscoveryModule;

    DataIOModule            *cb_io;
    const std::string       *cb_root_dir;
    DataTier                 cb_tier;
    fds_uint16_t             cb_disk_id;

  public:
    explicit DataDiscIter(DataIOModule *io);
    virtual ~DataDiscIter();
    bool obj_callback();
};

typedef std::unordered_map<fds_uint16_t, std::string>   DiskLocMap;
typedef std::unordered_map<fds_uint16_t, fds_uint64_t>  DiskUuidMap;

class DataDiscoveryModule : public fds::Module
{
  public:
    explicit DataDiscoveryModule(char const *const name);
    ~DataDiscoveryModule();

    // Module methods.
    virtual int  mod_init(fds::SysParams const *const param);
    virtual void mod_startup();
    virtual void mod_shutdown();

    // Discovery methods.
    inline int  disk_hdd_discovered() { return pd_hdd_found; }
    inline int  disk_ssd_discovered() { return pd_ssd_found; }
    void get_hdd_ids(std::set<fds_uint16_t>* ret_disk_ids);
    void get_ssd_ids(std::set<fds_uint16_t>* ret_disk_ids);
    const char *disk_hdd_path(fds_uint16_t disk_id);
    const char *disk_ssd_path(fds_uint16_t disk_id);

    virtual void disk_hdd_iter(DataDiscIter *iter);
    virtual void disk_ssd_iter(DataDiscIter *iter);

  private:
    void disk_open_map();

    int                      pd_hdd_found;
    int                      pd_ssd_found;
    DiskLocMap               pd_hdd_map;
    DiskLocMap               pd_ssd_map;
    DiskUuidMap              pd_uuids;
};

extern DataIndexModule       dataIndexMod;
extern DataDiscoveryModule   dataDiscoveryMod;

// ----------------------------------------------------------------------------
//
class DataIndexLDb
{
  public:
    explicit DataIndexLDb(const char *base);
    ~DataIndexLDb();

  private:
    fdsio::RequestQueue      idx_queue;
};

class PersisDataIO
{
  public:
    const int pd_ioq_rd_pending = 0;
    const int pd_ioq_wr_pending = 1;

    virtual fds::Error  disk_read(DiskRequest *req);
    virtual fds::Error  disk_write(DiskRequest *req);
    virtual void        disk_delete(fds_uint32_t obj_size);

    virtual fds::Error  disk_do_read(DiskRequest *req) = 0;
    virtual fds::Error  disk_do_write(DiskRequest *req) = 0;
    virtual void        disk_do_delete(fds_uint32_t obj_size) = 0;

    virtual void disk_read_done(DiskRequest *req);
    virtual void disk_write_done(DiskRequest *req);

  protected:
    PersisDataIO();
    ~PersisDataIO();

    fdsio::RequestQueue      pd_queue;
};

class FilePersisDataIO : public PersisDataIO
{
  public:
    typedef std::shared_ptr<FilePersisDataIO> shared_ptr;
    fds::Error disk_do_read(DiskRequest *req);
    fds::Error disk_do_write(DiskRequest *req);

    /**
     * Does not do actual delete of the object from disk,
     * but records stats for late garbage collection
     */
    void disk_do_delete(fds_uint32_t obj_size);

    inline int disk_loc_id() { return fi_loc; }
    inline fds_uint16_t file_id() { return fi_id; }
    FilePersisDataIO(char const *const path, fds_uint16_t id, int loc);
    virtual ~FilePersisDataIO();

    /**
     * delete file; must be the last call before destroying this object
     */
    fds::Error delete_file();

    /**
     * getters for statistics
     */
    fds_uint64_t get_total_bytes() const;
    fds_uint64_t get_deleted_bytes() const;
    inline fds_uint64_t get_deleted_objects() const {
        return fi_del_objs; }

  private:
    friend class DataIOModule;

    fds::fds_mutex           fi_mutex;
    int                      fi_loc;
    int                      fi_fd;
    fds_uint16_t             fi_id;
    fds_int64_t              fi_cur_off;
    const std::string        fi_path;

    /**
     * statistics useful for automated garbage collection, etc.
     */
    fds_uint64_t fi_del_objs;
    fds_uint64_t fi_del_blks;
};

}  // namespace diskio

#endif  // SOURCE_INCLUDE_PERSISTENT_LAYER_PERSISTENTDATA_H_
