#ifndef INCLUDE_PERISTENT_DATA_H_
#define INCLUDE_PERISTENT_DATA_H_

#include <disk-mgr/dm_io.h>
#include <concurrency/Mutex.h>

namespace diskio {

class DataIndexModule : public fds::Module
{
  public:
    DataIndexModule(char const *const name);
    ~DataIndexModule();

    virtual void mod_init(fds::SysParams const *const param);
    virtual void mod_startup();
    virtual void mod_shutdown();
};

extern DataIndexModule       dataIndexMod;

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

  private:
    friend class DataIOModule;

    FilePersisDataIO(const char *path);
    ~FilePersisDataIO();

    fds::fds_mutex  fi_mutex;
    int             fi_fd;
    fds_uint64_t    fi_cur_off;
    const char     *fi_path;
};

} // namespace diskio

#endif /* INCLUDE_PERISTENT_DATA_H_ */
