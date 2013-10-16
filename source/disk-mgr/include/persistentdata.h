#ifndef INCLUDE_PERISTENT_DATA_H_
#define INCLUDE_PERISTENT_DATA_H_

#include <disk-mgr/dm_io.h>

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

class DataIndexLDb : DataIndexProxy
{
  public:
    DataIndexLDb();
    ~DataIndexLDb();

  private:
};

} // namespace diskio

#endif /* INCLUDE_PERISTENT_DATA_H_ */
