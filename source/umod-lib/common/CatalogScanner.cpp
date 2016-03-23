/*
 * Copyright 2016 Formation Data Systems, Inc.
 */

#include <Catalog.h>
#include <CatalogScanner.h>

namespace fds {

#define SCHEDULE_TASK(task) \
    if (affinitySet) { \
        threadpool->scheduleWithAffinity(affinity, task); \
    } else { \
        threadpool->schedule(task); \
    }


CatalogScanner::CatalogScanner(Catalog &_c,
                               fds_threadpool *_tp,
                               unsigned _batchSize,
                               ForEachBatchCb _batchCb,
                               ScannerCb _doneCb)
       : catalog(_c),
         threadpool(_tp),
         batchSize(_batchSize),
         batchCb(_batchCb),
         doneCb(_doneCb),
         progressTracking(CS_INIT),
         affinity(0),
         affinitySet(false)
{}

CatalogScanner::CatalogScanner(Catalog &_c,
                               fds_threadpool *_tp,
                               unsigned _batchSize,
                               ForEachBatchCb _batchCb,
                               ScannerCb _doneCb,
                               uint64_t _affinity)
       : catalog(_c),
         threadpool(_tp),
         batchSize(_batchSize),
         batchCb(_batchCb),
         doneCb(_doneCb),
         progressTracking(CS_INIT),
         affinity(_affinity),
         affinitySet(true)
{}

void
CatalogScanner::start() {
    // Do some checks
    if ((threadpool == nullptr) ||
        (batchCb == nullptr) ||
        (doneCb == nullptr)) {
        progressTracking = CS_ERROR;
        doneCb(progressTracking);
        return;
    } else {
        progressTracking = CS_WORKING;
    }

    // Passes checks. Let's get a new iterator.
    iterator = (std::move(catalog.NewIterator()));
    iterator->SeekToFirst();

    auto nextStep = [this](){ doTableWalk(); };
    SCHEDULE_TASK(nextStep);
}

void
CatalogScanner::doTableWalk() {
    // Each batch to be passed as part of batchCb.
    // Gets cleared up at the beginning of every batch processing.
    std::list<CatalogKVPair> batch;
    unsigned curBatchCnt(0);

    while (iterator->Valid() && (curBatchCnt < batchSize)) {
        CatalogKVPair kvPair(iterator->key(), iterator->value());
        batch.push_back(kvPair);

        iterator->Next();
        curBatchCnt++;
    }

    batchCb(batch);

    if (iterator->Valid()) {
        // still working. Do another batch next cycle.
        auto nextStep = [this](){ doTableWalk(); };
        SCHEDULE_TASK(nextStep);
    } else {
        // We are done with the scan. Pass whatever we have for the last batch
        progressTracking = CS_DONE;
        doneCb(progressTracking);
    }
}

} // namespace fds
