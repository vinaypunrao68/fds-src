/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <string>

#include "data_mgr/VolumeMeta.h"

namespace fds {

VolumeMeta::VolumeMeta()
    : vol_uuid(0), vcat(NULL), tcat(NULL) {
}

VolumeMeta::VolumeMeta(const std::string& _name,
                       fds_uint64_t _uuid)
    : vol_name(_name),
      vol_uuid(_uuid) {


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
  Record key((const char *)&vol_offset,
             sizeof(vol_offset));
  Record val(oid.ToString());
  err = vcat->Update(key, val);

  return err;
}

}  // namespace fds
