/*
 * Copyright 2016 Formation Data Systems, Inc.
 */

#include <CatalogScanner.h>

namespace fds {

CatalogScanner::CatalogScanner(Catalog &_c,
                               fds_threadpool *_tp,
                               unsigned _batchSize,
                               ForEachBatchCb _batchCb,
                               ScannerCb _doneCb)
       : catalog(_c),
         threadpool(_tp),
         batchSize(_batchSize),
         batchCb(_batchCb),
         doneCb(_doneCb)
{}

} // namespace fds
