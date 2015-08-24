/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef SOURCE_LIB_CATALOG_H_
#define SOURCE_LIB_CATALOG_H_

// Standard includes.
#include <memory>
#include <string>
#include <cstdint>
#include <cstdio>

#include "../include/catalogKeys/CatalogKey.h"
// Internal includes.
#include "fds_types.h"
#include "fds_error.h"
#include "leveldb/db.h"
#include "leveldb/env.h"
#include "leveldb/filter_policy.h"
#include "leveldb/write_batch.h"
#include "leveldb/copy_env.h"

namespace fds {

struct CatalogException : std::runtime_error {
	   explicit CatalogException (const std::string& what_arg) : std::runtime_error(what_arg) {}
};

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
    std::unique_ptr<leveldb::DB> db;

    /*
     * leveldb file system interface
     */
    std::unique_ptr<leveldb::CopyEnv> env;

    /*
     * Database options. These are not expected
     * to change.
     */
    leveldb::Options      options;       /**< LevelDB options */
    leveldb::WriteOptions write_options; /**< LevelDB write options */
    leveldb::ReadOptions  read_options;  /**< LevelDB read options */

    std::unique_ptr<leveldb::FilterPolicy const> filter_policy;

    static const std::string empty;

  public:
    static const fds_uint32_t WRITE_BUFFER_SIZE;
    static const fds_uint32_t CACHE_SIZE;

    /** Constructor */
    Catalog(const std::string& _file, fds_uint32_t writeBufferSize = WRITE_BUFFER_SIZE,
            fds_uint32_t cacheSize = CACHE_SIZE, const std::string& logDirName = empty,
            const std::string& logFilePrefix = empty, fds_uint32_t maxLogFiles = 0,
            leveldb::Comparator * cmp = 0);

    ~Catalog();

    /** Uses the underlying leveldb iterator */
    typedef leveldb::Iterator catalog_iterator_t;

    /** a NULL value means the associated actions should happen on live data */
    typedef const leveldb::Snapshot* MemSnap;

    /** Gets catalog iterator for a given snapshot
     * @param[in] m the snapshot to use, NULL for latest data
     * @return Pointer to catalog iterator
     */
    std::unique_ptr<catalog_iterator_t> NewIterator(MemSnap m = NULL) {
        leveldb::ReadOptions ro{read_options};

        ro.snapshot = m;

        return std::unique_ptr<catalog_iterator_t>(db->NewIterator(ro));
    }

    inline const leveldb::Options & GetOptions() const {
        return options;
    }

    inline leveldb::WriteOptions & GetWriteOptions() {
        return write_options;
    }

    fds::Error Update(const CatalogKey& key, const leveldb::Slice& val);
    fds::Error Update(CatWriteBatch* batch);
    fds::Error Query(const CatalogKey& key, std::string* val, MemSnap m = NULL);
    fds::Error Delete(const CatalogKey& key);

    /**
     * Wrapper for leveldb::GetSnapshot(), ReleaseSnapshot()
     */

    /*
     * all MemSnap Gets and Releases should be pass-by-reference, so any dangling
     * pointers are zeroed by ReleaseSnapshot
     */
    void GetSnapshot(MemSnap &m) {
        m = db->GetSnapshot();
    }

    /*
     * after this call, m must not be used
     */
    void ReleaseSnapshot(MemSnap &m) {
        db->ReleaseSnapshot(m);
        m = NULL;
    }

    fds::Error DbSnap(const std::string& fileName);

    inline void clearLogRotate() {
        fds_assert(env);
        env->logRotate() = false;
    }

    std::string GetFile() const;
  };

}  // namespace fds

#endif  // SOURCE_LIB_CATALOG_H_
