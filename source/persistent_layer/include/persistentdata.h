/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef SOURCE_PERSISTENT_LAYER_INCLUDE_PERSISTENTDATA_H_
#define SOURCE_PERSISTENT_LAYER_INCLUDE_PERSISTENTDATA_H_

#include <string>
#include <persistent_layer/dm_io.h>
#include <concurrency/Mutex.h>
#include <fds_counters.h>

namespace fds {

class PMCounters : public FdsCounters
{
 public:
     PMCounters(const std::string &id, FdsCountersMgr *mgr)
        : FdsCounters(id, mgr),
          diskR_Err("diskR_Err", this),
          diskW_Err("diskW_Err", this)
     {
     }

     NumericCounter diskR_Err;
     NumericCounter diskW_Err;
};
}  // namespace fds

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

class DataDiscoveryModule : public fds::Module
{
  public:
    explicit DataDiscoveryModule(char const *const name);
    ~DataDiscoveryModule();

    virtual int  mod_init(fds::SysParams const *const param);
    virtual void mod_startup();
    virtual void mod_shutdown();

    char const *const disk_hdd_path(int route_idx);
    char const *const disk_ssd_path(int route_idx);

  private:
    void parse_device_dir(const std::string &path, DataTier tier);
    bool disk_detect_label(std::string *path, DataTier tier);
    void disk_make_label(std::string *path, DataTier tier, int diskno);

    int                      pd_hdd_count;
    int                      pd_ssd_count;
    int                      pd_hdd_cap_mb;
    int                      pd_ssd_cap_mb;
    int                      pd_hdd_found;
    int                      pd_ssd_found;
    std::string              *pd_hdd_raw;
    std::string              *pd_ssd_raw;
    std::string              *pd_hdd_labeled;
    std::string              *pd_ssd_labeled;
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

    virtual int disk_read(DiskRequest *req);
    virtual int disk_write(DiskRequest *req);

    virtual int disk_do_read(DiskRequest *req) = 0;
    virtual int disk_do_write(DiskRequest *req) = 0;

    virtual void disk_read_done(DiskRequest *req);
    virtual void disk_write_done(DiskRequest *req);


  protected:
    PersisDataIO();
    ~PersisDataIO();

    fds::PMCounters          pd_counters_;
    fdsio::RequestQueue      pd_queue;
};

class FilePersisDataIO : public PersisDataIO
{
  public:
    int disk_do_read(DiskRequest *req);
    int disk_do_write(DiskRequest *req);

    inline int disk_loc_id() { return fi_loc; }

  private:
    friend class DataIOModule;

    FilePersisDataIO(char const *const path, int loc);
    ~FilePersisDataIO();

    fds::fds_mutex           fi_mutex;
    int                      fi_loc;
    int                      fi_fd;
    fds_int64_t             fi_cur_off;
    char const *const        fi_path;
};

}  // namespace diskio

#endif  // SOURCE_PERSISTENT_LAYER_INCLUDE_PERSISTENTDATA_H_
