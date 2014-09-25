/**
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <unistd.h>
#include <set>
#include <string>

#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>

#include <util/Log.h>
#include <fds_assert.h>
#include <fds_process.h>
#include <ObjectId.h>
#include <FdsRandom.h>
#include <object-store/SmTokenPlacement.h>

namespace fds {

int run_test(fds_uint32_t ssd_count,
             fds_uint32_t hdd_count) {
    ObjectLocationTable::ptr olt(new ObjectLocationTable());
    std::set<fds_uint16_t> hdds;
    std::set<fds_uint16_t> ssds;
    fds_uint16_t disk_id = 1;
    for (fds_uint32_t i = 0; i < hdd_count; i++) {
        hdds.insert(disk_id);
        disk_id++;
    }
    for (fds_uint32_t i = 0; i < ssd_count; i++) {
        ssds.insert(disk_id);
        disk_id++;
    }

    // compute sm token placement
    SmTokenPlacement::compute(hdds, ssds, olt);
    std::cout << "Initial computation - " << *olt << std::endl;

    // test serialize
    std::string buffer;
    olt->getSerialized(buffer);

    ObjectLocationTable::ptr olt2(new ObjectLocationTable());
    olt2->loadSerialized(buffer);
    fds_verify(*olt == *olt2);

    // test comparison :)
    olt2->setDiskId(0, diskio::diskTier, disk_id+1);
    fds_verify(!(*olt == *olt2));

    return 0;
}

}  // namespace fds

int main(int argc, char * argv[]) {
    int ret = 0;
    std::cout << "unit test " << __FILE__ << " started." << std::endl;
    std::cout << "HDDs and SSDs" << std::endl;
    ret = fds::run_test(2, 12);
    std::cout << "HDDs and no SSDs" << std::endl;
    ret = fds::run_test(0, 12);
    std::cout << "SSDs and no HDDs" << std::endl;
    ret = fds::run_test(2, 0);
    std::cout << "no HDDs and no SSDs" << std::endl;
    ret = fds::run_test(0, 0);
    std::cout << "unit test " << __FILE__ << " finished." << std::endl;
    return 0;
}

