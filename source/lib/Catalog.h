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

#include <fds_types.h>
#include <fds_err.h>
#include <leveldb/db.h>
#include <leveldb/env.h>

namespace fds {

  /*
   * TODO: Just use leveldb's slice. Consider our own class
   * in the future.
   */
  typedef leveldb::Slice Record;

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
    fds::Error Query(const Record& key, std::string* val);

    /* delete catalog entry for 'key', 
     * not an error if 'key' is not in the database */
    fds::Error Delete(const Record& key);

    std::string GetFile() const;
  };

}  // namespace fds

#endif  // SOURCE_LIB_CATALOG_H_
