/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <string>

#include "data_mgr/VolumeMeta.h"

namespace fds {

/*
 * Currently the dm_log is NEEDED but set to NULL
 * when not passed in to the constructor. We should
 * build our own in class logger if we're not passed
 * one.
 */
/*
VolumeMeta::VolumeMeta()
  : vcat(NULL), tcat(NULL), dm_log(NULL) {

  vol_mtx = new fds_mutex("Volume Meta Mutex");
  vol_desc = new VolumeDesc(0);
}
*/

VolumeMeta::VolumeMeta(const std::string& _name,
                       fds_uint64_t _uuid)
    : dm_log(NULL) {

  vol_mtx = new fds_mutex("Volume Meta Mutex");
  vol_desc = new VolumeDesc(_name, _uuid);
  vcat = new VolumeCatalog(_name + "_vcat.ldb");
  tcat = new TimeCatalog(_name + "_tcat.ldb");
}

VolumeMeta::VolumeMeta(const std::string& _name,
                       fds_uint64_t _uuid,
                       fds_log* _dm_log)
    : VolumeMeta(_name, _uuid) {

  dm_log = _dm_log;
}

VolumeMeta::~VolumeMeta() {
  delete vcat;
  delete tcat;
  delete vol_desc;
  delete vol_mtx;
}

Error VolumeMeta::OpenTransaction(fds_uint32_t vol_offset,
                                  const ObjectID& oid) {
  Error err(ERR_OK);

  /*
   * TODO: Just put it in the vcat for now.
   */

  ObjectID test(oid.ToString());


  Record key((const char *)&vol_offset,
             sizeof(vol_offset));
  std::string oid_string = oid.ToString();
  Record val(oid_string);

  vol_mtx->lock();
  err = vcat->Update(key, val);
  vol_mtx->unlock();

  return err;
}

Error VolumeMeta::QueryVcat(fds_uint32_t vol_offset,
                            ObjectID *oid) {
  Error err(ERR_OK);

  Record key((const char *)&vol_offset,
             sizeof(vol_offset));

  /*
   * The query will allocate the record.
   * TODO: Don't have the query allocate
   * anything. That's not safe.
   */
  std::string val;

  vol_mtx->lock();
  err = vcat->Query(key, &val);
  vol_mtx->unlock();

  if (! err.ok()) {
    FDS_PLOG(dm_log) << "Failed to query for vol " << *vol_desc
                     << " offset " << vol_offset << " with err "
                     << err;
    return err;
  } else if (val.size() != oid->GetLen()) {
    /*
     * Expect the record to the be object sized.
     */
    FDS_PLOG(dm_log) << "Failed to query for vol " << *vol_desc
                     << " offset " << vol_offset << " with size "
                     << val.size() << " expected " << oid->GetLen();
    err = ERR_CAT_QUERY_FAILED;
    return err;
  }

  oid->SetId(val.c_str());

  return err;
}

}  // namespace fds
