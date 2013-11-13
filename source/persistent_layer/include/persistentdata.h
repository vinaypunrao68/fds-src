/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef INCLUDE_PERISTENT_DATA_H_
#define INCLUDE_PERISTENT_DATA_H_

#include <string>
#include <persistent_layer/dm_io.h>
#include <concurrency/Mutex.h>

namespace diskio {

class DataIndexModule : public fds::Module
{
  public:
    DataIndexModule(char const *const name);
    ~DataIndexModule();

    virtual int  mod_init(fds::SysParams const *const param);
    virtual void mod_startup();
    virtual void mod_shutdown();
};

class DataDiscoveryModule : public fds::Module
{
  public:
    DataDiscoveryModule(char const *const name);
    ~DataDiscoveryModule();

    virtual int  mod_init(fds::SysParams const *const param);
    virtual void mod_startup();
    virtual void mod_shutdown();

    char const *const disk_hdd_path(int route_idx);
    char const *const disk_ssd_path(int route_idx);

  private:
    void parse_device_dir(const std::string &path, DataTier tier);
    bool disk_detect_label(std::string &path, DataTier tier);
    void disk_make_label(std::string &path, DataTier tier, int diskno);

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
    DataIndexLDb(const char *base);
    ~DataIndexLDb();

  private:
    fdsio::RequestQueue      idx_queue;
};

class PersisDataIO
{
  public:
    const int pd_ioq_rd_pending = 0;
    const int pd_ioq_wr_pending = 1;

    virtual void disk_read(DiskRequest *req);
    virtual void disk_write(DiskRequest *req);

    virtual void disk_do_read(DiskRequest *req) = 0;
    virtual void disk_do_write(DiskRequest *req) = 0;

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
    void disk_do_read(DiskRequest *req);
    void disk_do_write(DiskRequest *req);

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

} // namespace diskio

#endif /* INCLUDE_PERISTENT_DATA_H_ */
