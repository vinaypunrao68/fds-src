/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <cstdlib>
#include <hash/MurmurHash3.h>

#include <iostream>  // NOLINT(*)
#include <unordered_map>
#include <vector>
#include <string>
#include <list>
#include <atomic>

#include <util/Log.h>
#include <fds_assert.h>
#include <fds_types.h>
#include "ObjRank.h"

namespace fds {

class ObjRankUnitTest {
 private:
  std::list<std::string>  unit_tests;
  fds_log *test_log;

  fds_uint32_t rank_table_size;
  fds_uint32_t vols_count;

  std::unordered_map<ObjectID, std::string, ObjectHash> added_objs;

  /*
   * Helper functions.
   */

  /*
   * Unit test funtions
   */
  fds_int32_t basic_ranking() {

    FDS_PLOG(test_log) << "Starting test: basic_ranking()";
    ObjectRankEngine* rank_eng = new ObjectRankEngine("sm", rank_table_size, test_log);
    if (!rank_eng) {
      FDS_PLOG(test_log) << "basic_ranking: failed to create ObjectRankEngine";
      return 1;
    }    

    /* lets have 10 volumes, priorities 1 to 10 */
    FDS_Volume vols[vols_count];
    for (int i = 1; i <= vols_count; i++)
      {
	vols[i-1].voldesc = new VolumeDesc(std::string("vol_") + std::to_string(i), i);
	(vols[i-1].voldesc)->iops_min = 0;
	(vols[i-1].voldesc)->relativePrio = i;
      }

    /* lets insert # objects > rank_table_size by 20 */
    ObjectID oid;
    int vol_ix = 0;
    fds_uint32_t rank = 0;
    for (fds_uint32_t i = 1; i < (rank_table_size + 20); ++i)
      {
	oid = ObjectID(i, i*i);

	/* will round-robin volumes */
	rank = rank_eng->rankAndInsertObject(oid, *(vols[vol_ix].voldesc));
	FDS_PLOG(test_log) << "object " << i << " volume (prio=" << (vols[vol_ix].voldesc)->relativePrio << ") rank=" << rank;

	vol_ix = (vol_ix + 1) % vols_count;
      }

    
    FDS_PLOG(test_log) << "Ending test: basic_ranking()";
    return 0;
  }

 public:
  /*
   * The non-const refernce is OK.
   */
  explicit ObjRankUnitTest(fds_uint32_t _rank_table_size,
			   fds_uint32_t _num_volumes)  // NOLINT(*)
    : rank_table_size(_rank_table_size),
      vols_count(_num_volumes)
  {
    test_log   = new fds_log("rank_test", "logs");

    unit_tests.push_back("basic_ranking");
  }


  ~ObjRankUnitTest() {
    delete test_log;
  }

  fds_log* GetLogPtr() {
    return test_log;
  }

  fds_int32_t Run(const std::string& testname) {
    fds_int32_t result = 0;
    std::cout << "Running unit test \"" << testname << "\"" << std::endl;

    if (testname == "basic_ranking") {
      result = basic_ranking();
    } else {
      std::cout << "Unknown unit test " << testname << std::endl;
    }

    if (result == 0) {
      std::cout << "Unit test \"" << testname << "\" PASSED"  << std::endl;
    } else {
      std::cout << "Unit test \"" << testname << "\" FAILED" << std::endl;
    }
    std::cout << std::endl;

    return result;
  }

  fds_uint32_t Run() {
    fds_int32_t result = 0;
    for (std::list<std::string>::iterator it = unit_tests.begin();
         it != unit_tests.end();
         ++it) {
      result = Run(*it);
      if (result != 0) {
        std::cout << "Unit test FAILED" << std::endl;
        break;
      }
    }

    if (result == 0) {
      std::cout << "Unit test PASSED" << std::endl;
    }

    return result;
  }
};

}  // namespace fds

int main(int argc, char* argv[]) {
  fds::ObjRankUnitTest unit_test(100, 10);

  return unit_test.Run();
}
