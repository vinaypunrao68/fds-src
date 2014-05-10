/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <OmVolumePlacement.h>

namespace fds {

/******* RoundRobin algorithm implementation ******/

Error RRAlgorithm::updateDMT(const DMTPtr& curDmt,
                             DMT* newDMT) {
    Error err(ERR_OK);

    return err;
}

/******* RoundRobinDynamic algorithm inmplementation ******/

Error DynamicRRAlgorithm::updateDMT(const DMTPtr& curDmt,
                                    DMT* newDMT) {
    Error err(ERR_OK);

    return err;
}

}  // namespace fds
