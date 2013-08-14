/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

/*
 * Generic catalog super class. Provides basic
 * backing storage, update, and query.
 */

#ifndef SOURCE_LIB_CATALOG_H_
#define SOURCE_LIB_CATALOG_H_

#include <string>

#include "lib/fds_err.h"
#include "lib/leveldb-1.12.0/include/leveldb/db.h"
#include "lib/leveldb-1.12.0/include/leveldb/env.h"

namespace fds {

  class Record : public leveldb::Slice {
 private:
 public:
    Record(const char* record_data,
           fds_uint32_t record_size)
        : leveldb::Slice(record_data, record_size) {
    }
    Record(const std::string &_record)
        : leveldb::Slice(_record) {
    }

    ~Record() {
    }

    using leveldb::Slice::data;
    using leveldb::Slice::size;
    using leveldb::Slice::empty;
    using leveldb::Slice::clear;
  };

  class Catalog {
 private:
    /*
     * Backing file name
     */
    std::string backing_file;

    /*
     * Currently implements an LSM-tree.
     * We should make this more generic
     * and provide a generic interface for
     * when we change the backing DB.
     */
    leveldb::DB* db;

    /*
     * Database options. These are not expected
     * to change.
     */
    leveldb::Options      options;
    leveldb::WriteOptions write_options;
    leveldb::ReadOptions  read_options;

 public:
    explicit Catalog(const std::string& _file);
    ~Catalog();

    fds::Error Update(const Record& key, const Record& val);
    fds::Error Query(const Record& key, Record* val);
    std::string GetFile() const;
  };

}  // namespace fds

#endif  // SOURCE_LIB_CATALOG_H_
