/*
 * Copyright 2016 Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_CATALOGSCANNER_H_
#define SOURCE_INCLUDE_CATALOGSCANNER_H_

#include <concurrency/ThreadPool.h>

// Forward declarations
namespace leveldb {
    class Slice;
} // namespace leveldb

/**
 * This is an iterative scanner given a specific levelDB Catalog.
 * It scans and returns things in slices, and the users can implement
 * their own callbacks/handlers to handle what to do with the results.
 * The scanner executes things within a QoS context and per batch size
 * so that it doesn't block any QoS for too long.
 */
namespace fds {

// Forward declaration
class Catalog;

using CatalogKVPair = std::pair<leveldb::Slice, leveldb::Slice>;

class CatalogScanner {
public:
    enum progress {
        CS_INIT,
        CS_WORKING,
        CS_DONE,
        CS_ERROR
    };

    using ForEachBatchCb = std::function<void (std::list<CatalogKVPair> &batchSlice)>;
    using ScannerCb = std::function<void (progress &scannerProgress)>;

    CatalogScanner(Catalog &_c,
                   fds_threadpool *_threadpool,
                   const unsigned _batchSize,
                   ForEachBatchCb _batchCb,
                   ScannerCb _doneCb);
    CatalogScanner(Catalog &_c,
                   fds_threadpool *_threadpool,
                   const unsigned _batchSize,
                   ForEachBatchCb _batchCb,
                   const ScannerCb _doneCb,
                   const uint64_t _affinity);
    ~CatalogScanner() = default;

    // Calls to have the scanner start and go off to do work
    void start();

    // Does the iteration and fills up batchSlice
    void doTableWalk();

    // Get current status
    inline progress getProgress() {
        return progressTracking;
    }

private:
    // Internal reference to the catalog
    Catalog &catalog;

    // Internal reference to threadpool.
    fds_threadpool *threadpool;

    // Batch size
    const unsigned batchSize;

    // Calls when each batch is done
    ForEachBatchCb batchCb;

    // Calls when the whole scanner is done or errored out
    ScannerCb doneCb;

    // Tracks the current progress
    progress progressTracking;

    typedef leveldb::Iterator catalog_iterator_t;
    std::unique_ptr<catalog_iterator_t> iterator;

    // In case users want to schedule with affinity
    const uint64_t affinity;
    const bool affinitySet;
};
} // namespace fds

#endif // SOURCE_INCLUDE_CATALOGSCANNER_H_
