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
VolumeMeta::VolumeMeta()
    : vol_uuid(0), vcat(NULL), tcat(NULL), dm_log(NULL) {
}

VolumeMeta::VolumeMeta(const std::string& _name,
                       fds_uint64_t _uuid)
    : vol_name(_name),
      vol_uuid(_uuid),
      dm_log(NULL) {

  vcat = new VolumeCatalog(vol_name + "_vcat.ldb");
  tcat = new TimeCatalog(vol_name + "_tcat.ldb");
}

VolumeMeta::VolumeMeta(const std::string& _name,
                       fds_uint64_t _uuid,
                       fds_log* _dm_log)
    : vol_name(_name),
      vol_uuid(_uuid),
      dm_log(_dm_log) {

  vcat = new VolumeCatalog(vol_name + "_vcat.ldb");
  tcat = new TimeCatalog(vol_name + "_tcat.ldb");
}

VolumeMeta::~VolumeMeta() {
  delete vcat;
  delete tcat;
}

Error VolumeMeta::OpenTransaction(fds_uint32_t vol_offset,
                                  const ObjectID& oid) {
  Error err(ERR_OK);

  /*
   * TODO: Just put it in the vcat for now.
   */

  FDS_PLOG(dm_log) << "Putting object " << oid;
  ObjectID test(oid.ToString());
  FDS_PLOG(dm_log) << "Converted object " << test;

  Record key((const char *)&vol_offset,
             sizeof(vol_offset));
  Record val(oid.ToString());
  err = vcat->Update(key, val);

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
  std::string test("WTF");
  Record val(test);
  err = vcat->Query(key, &val);
  if (! err.ok()) {
    FDS_PLOG(dm_log) << "Failed to query for vol offset "
                     << vol_offset << " with err " << err;
    return err;
  } else if (val.size() != oid->GetLen()) {
    /*
     * Expect the record to the be object sized.
     */
    FDS_PLOG(dm_log) << "Failed to query for vol offset "
                     << vol_offset << " with size " << val.size()
                     << " expected " << oid->GetLen();
    err = ERR_CAT_QUERY_FAILED;
    return err;
  }

  oid->SetId(val.data());

  return err;
}

}  // namespace fds
