/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#ifndef __STOR_HV_VOLS_H_
#define __STOR_HV_VOLS_H_

#include <lib/Ice-3.5.0/cpp/include/Ice/Ice.h>
#include <lib/Ice-3.5.0/cpp/include/IceUtil/IceUtil.h>

#include <unordered_map>

#include "fdsp/FDSP.h"
#include "include/fds_err.h"
#include "include/fds_types.h"
#include "include/fds_volume.h"
#include "util/Log.h"
#include "util/concurrency/RwLock.h"
#include "VolumeCatalogCache.h"
#include "StorHvJournal.h"


/* defaults */
#define FDS_DEFAULT_VOL_UUID 1


/*
 * Forward declaration of SH control class
 */
class StorHvCtrl;

namespace fds {

class StorHvVolume : public FDS_Volume
{
public:
  StorHvVolume(fds_volid_t vol_uuid, StorHvCtrl *sh_ctrl, fds_log *parent_log);
  ~StorHvVolume();

public: /* data*/

  StorHvJournal *journal_tbl;
  VolumeCatalogCache *vol_catalog_cache;

  /* Reference to parent SH instance */
  StorHvCtrl *parent_sh;
};

class StorHvVolumeTable
{
 public:
  /* A logger is created if not passed in */
  StorHvVolumeTable(StorHvCtrl *sh_ctrl);
  /* Use logger that passed in to the constructor */
  StorHvVolumeTable(StorHvCtrl *sh_ctrl, fds_log *parent_log);
  ~StorHvVolumeTable();

  Error registerVolume(fds_volid_t vol_uuid);
  StorHvVolume* getVolume(fds_volid_t vol_uuid);
  
 private: /* data */

  /* volume uuid -> StorHvVolume map */
  std::unordered_map<fds_volid_t, StorHvVolume*> volume_map;   

  /* Protects volume_map */
  fds_rwlock map_rwlock;
  
  /* Reference to parent SH instance */
  StorHvCtrl *parent_sh;

  /*
   * Pointer to logger to use 
   */
  fds_log *vt_log;
  fds_bool_t created_log;
};

} // namespace fds

#endif // __STOR_HV_VOLS_H_
