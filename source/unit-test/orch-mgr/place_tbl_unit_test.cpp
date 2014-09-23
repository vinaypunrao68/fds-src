/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <iostream> // NOLINT(*)
#include <string>
#include <vector>

#include <fds_dmt.h>

int main(int argc, char *argv[]) {
    fds::Error err(fds::ERR_OK);
    fds_uint32_t width = 4;
    fds_uint32_t depth = 4;
    fds_uint32_t cols = pow(2,width);
    fds_uint64_t version = 1;

    fds::DMTManagerPtr dmt_mgr(new fds::DMTManager());
    fds::DMT* dmt = new fds::DMT(width, depth, version);
    std::cout << *dmt << std::endl;

    err = dmt->verify();
    if (err != fds::Error(fds::ERR_INVALID_DMT)) {
        std::cout << "Expected invalid DMT error" << std::endl;
        return -1;
    }

    for (fds_uint32_t j = 0; j < cols; ++j) {
        fds::DmtColumn col(depth);
        for (fds_uint32_t i = 0; i < depth; ++i) {
            col.set(i, i+1);
        }
        dmt->setNodeGroup(j, col);
    }

    std::cout << *dmt << std::endl;
    err = dmt->verify();
    if (!err.ok()) {
        std::cout << "Invalid DMT" << std::endl;
        return -1;
    }

    // set first column with same uuid
    fds::DmtColumn col(depth);
    for (fds_uint32_t i = 0; i < depth; ++i) {
        col.set(i, 10);
    }
    dmt->setNodeGroup(0, col);

    std::cout << *dmt << std::endl;

    // should return error
    err = dmt->verify();
    if (err != fds::Error(fds::ERR_INVALID_DMT)) {
        std::cout << "Expected invalid dmt error" << std::endl;
        return -1;
    }

    // fix first column
    fds::DmtColumn col2(depth);
    for (fds_uint32_t i = 0; i < depth; ++i) {
        col2.set(i, 10+i);
    }
    dmt->setNodeGroup(0, col2);

    // add to DMT manager as target DMT
    err = dmt_mgr->add(dmt, fds::DMT_TARGET);
    if (!err.ok()) {
        std::cout << "Failed to add to DMT manager " << err;
        return -1;
    }

    std::cout << *dmt_mgr << std::endl;

    // set target DMT as commited
    dmt_mgr->commitDMT();
    std::cout << *dmt_mgr << std::endl;

    // add another dmt
    fds::DMT* dmt2 = new fds::DMT(width, depth, version+1);
    for (fds_uint32_t j = 0; j < cols; ++j) {
        fds::DmtColumn col(depth);
        for (fds_uint32_t i = 0; i < depth; ++i) {
            col.set(i, i+20);
        }
        dmt2->setNodeGroup(j, col);
    }
    err = dmt_mgr->add(dmt2, fds::DMT_TARGET);
    if (!err.ok()) {
        std::cout << "Failed to add to DMT manager " << err;
        return -1;
    }

    std::cout << *dmt_mgr << std::endl;

    dmt_mgr->commitDMT();

    // add one more
    fds::DMT* dmt3 = new fds::DMT(width, depth, version+2);
    for (fds_uint32_t j = 0; j < cols; ++j) {
        fds::DmtColumn col(depth);
        for (fds_uint32_t i = 0; i < depth; ++i) {
            col.set(i, i+30);
        }
        dmt3->setNodeGroup(j, col);
    }
    err = dmt_mgr->add(dmt3, fds::DMT_TARGET);
    if (!err.ok()) {
        std::cout << "Failed to add to DMT manager " << err;
        return -1;
    }
    std::cout << *dmt_mgr << std::endl;

    // get one column
    fds::DmtColumnPtr pcol = dmt_mgr->getCommittedNodeGroup(0x85739);
    std::cout << "Primary node for volume is "
              << std::hex << pcol->get(0) << std::dec << std::endl;

    return 0;
}
