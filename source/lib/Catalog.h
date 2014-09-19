/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef SOURCE_LIB_CATALOG_H_
#define SOURCE_LIB_CATALOG_H_

#include <string>
#include <stdint.h>
#include <stdio.h>

#include <fds_types.h>
#include <fds_error.h>
#include <leveldb/db.h>
#include <leveldb/env.h>
#include "leveldb/write_batch.h"

namespace fds {

struct CopyDetails {
    CopyDetails(const std::string & src, const std::string & dest)
            : srcPath(src), destPath(dest) {}
    const std::string srcPath;
    const std::string destPath;
};

/**
 * Just use leveldb's slice. We should consider our
 * own class in the future.
 */
typedef leveldb::Slice Record;
typedef leveldb::WriteBatch CatWriteBatch;

/**
 * Generic catalog super class.
 * Provides basic backing storage, update, and query.
 */
class Catalog {
  private:
    /**
     * Backing file name
     */
    std::string backing_file;

    /** The backing db.
     * Currently implements an LSM-tree.
     * We should make this more generic
     * and provide a generic interface for
     * when we change the backing DB.
     */
    leveldb::DB* db;

    /*
     * leveldb file system interface 
     */
    leveldb::Env* env;

    /*
     * Database options. These are not expected
     * to change.
     */
    leveldb::Options      options;       /**< LevelDB options */
    leveldb::WriteOptions write_options; /**< LevelDB write options */
    leveldb::ReadOptions  read_options;  /**< LevelDB read options */

  public:
    /** Constructor */
    Catalog(const std::string& _file, fds_bool_t cat_flag = true);
    /** Default destructor */
    ~Catalog();

    typedef boost::shared_ptr<Catalog> ptr;
    typedef boost::shared_ptr<const Catalog> const_ptr;

    /** Uses the underlying leveldb iterator */
    typedef leveldb::Iterator catalog_iterator_t;
    /** Gets catalog iterator
     * @return Pointer to catalog iterator
     */
    catalog_iterator_t *NewIterator() { return db->NewIterator(read_options); }

    typedef leveldb::ReadOptions catalog_roptions_t;

    fds::Error Update(const Record& key, const Record& val);
    fds::Error Update(CatWriteBatch* batch);
    fds::Error Query(const Record& key, std::string* val);
    fds::Error Delete(const Record& key);

    /**
     * Wrapper for leveldb::GetSnapshot(), ReleaseSnapshot()
     */
    void GetSnapshot(catalog_roptions_t& opts) {
        opts.snapshot = db->GetSnapshot();
    }
    void ReleaseSnapshot(catalog_roptions_t opts) {
        db->ReleaseSnapshot(opts.snapshot);
    }
    catalog_iterator_t *NewIterator(catalog_roptions_t opts) {
        return db->NewIterator(opts);
    }

    bool DbEmpty();
    bool DbDelete();
    fds::Error DbSnap(const std::string& fileName);
    fds::Error QuerySnap(const std::string& _file, const Record& key, std::string* value);
    fds::Error QueryNew(const std::string& _file, const Record& key, std::string* value);

    std::string GetFile() const;
  };

}  // namespace fds

#endif  // SOURCE_LIB_CATALOG_H_
