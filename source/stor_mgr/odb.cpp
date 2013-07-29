/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <assert.h>
#include <string>

#include "stor_mgr/odb.h"
#include "leveldb/db.h"
#include "leveldb/filter_policy.h"

namespace fds {
namespace osm {

#define WRITE_BUFFER_SIZE   50 * 1024 * 1024;
#define FILTER_BITS_PER_KEY 128  // Todo: Change this to the max size of DiskLoc

/** Constructs odb with filename.
 * 
 * @param filename (i) Name of file for backing storage.
 * 
 * @return ObjectDB object.
 */
ObjectDB::ObjectDB(const std::string& filename)
    : file(filename) {
  /*
   * Setup DB options
   */
  options.create_if_missing = 1;
  options.filter_policy     =
      leveldb::NewBloomFilterPolicy(FILTER_BITS_PER_KEY);
  options.write_buffer_size = WRITE_BUFFER_SIZE;

  write_options.sync = true;

  leveldb::Status status = leveldb::DB::Open(options, file, &db);
  /* Open has to succeed */
  assert(status.ok());

  histo_all.Clear();
  histo_put.Clear();
  histo_get.Clear();
}

/** Destructs odb.
 * @return None
 */
ObjectDB::~ObjectDB() {
  delete options.filter_policy;
  delete db;
}

/** Puts an object at a disk location.
 *
 * @param disk_location (i) Location to put obj.
 * @param obj_buf       (i) Object data.
 *
 * @return ERR_OK if successful, err otherwise.
 */
fds::Error ObjectDB::Put(const DiskLoc& disk_location,
                         const ObjectBuf& obj_buf) {
  fds::Error err(fds::ERR_OK);

  leveldb::Slice key((const char *)&disk_location, sizeof(disk_location));
  leveldb::Slice value(obj_buf.data.data(), obj_buf.size);

  timer_start();
  leveldb::Status status = db->Put(write_options, key, value);
  timer_stop();
  timer_update_put_histo();
  if (!status.ok()) {
    err = fds::Error(fds::ERR_DISK_WRITE_FAILED);
  }

  return err;
}

/** Gets an object from a disk location.
 *
 * @param disk_location (i) Location to get obj.
 * @param obj_buf       (o) Object data.
 *
 * @return ERR_OK if successful, err otherwise.
 */
fds::Error ObjectDB::Get(const DiskLoc& disk_location,
                         ObjectBuf& obj_buf) {
  fds::Error err(fds::ERR_OK);

  leveldb::Slice key((const char *)&disk_location, sizeof(disk_location));
  std::string value;

  timer_start();
  leveldb::Status status = db->Get(read_options, key, &value);
  timer_stop();
  timer_update_get_histo();
  if (!status.ok()) {
    err = fds::Error(fds::ERR_DISK_READ_FAILED);
  }

  obj_buf.data = value;

  return err;
}

}  // namespace osm
}  // namespace fds
