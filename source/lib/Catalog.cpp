/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <string>

#include "lib/Catalog.h"
#include "lib/leveldb-1.12.0/include/leveldb/filter_policy.h"

namespace fds {

#define WRITE_BUFFER_SIZE   50 * 1024 * 1024;
#define FILTER_BITS_PER_KEY 128  // Todo: Change this something not arbitrary

Catalog::Catalog(const std::string& _file)
    : backing_file(_file) {
  /*
   * Setup DB options
   */
  options.create_if_missing = 1;
  options.filter_policy     =
      leveldb::NewBloomFilterPolicy(FILTER_BITS_PER_KEY);
  options.write_buffer_size = WRITE_BUFFER_SIZE;

  write_options.sync = true;

  leveldb::Status status = leveldb::DB::Open(options, backing_file, &db);
  /* Open has to succeed */
  assert(status.ok());
}

Catalog::~Catalog() {
  delete options.filter_policy;
  delete db;
}

Error
Catalog::Update(const Record& key, const Record& val) {
  Error err(ERR_OK);

  std::string test = val.ToString();

  leveldb::Status status = db->Put(write_options, key, val);
  if (!status.ok()) {
    err = Error(ERR_DISK_WRITE_FAILED);
  }

  return err;
}

Error
Catalog::Query(const Record& key, std::string* value) {
  Error err(ERR_OK);

  leveldb::Status status = db->Get(read_options, key, value);
  if (!status.ok()) {
    err = fds::Error(fds::ERR_DISK_READ_FAILED);
    return err;
  } 

  return err;
}

std::string
Catalog::GetFile() const {
  return backing_file;
}

}  // namespace fds
