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

    /* lets insert # objects > rank_table_size by 30 */
    ObjectID oid;
    int vol_ix = 0;
    fds_uint32_t rank = 0;
    for (fds_uint32_t i = 1; i <= (rank_table_size + 30); ++i)
      {
	oid = ObjectID(i, i*i);

	/* will round-robin volumes */
	rank = rank_eng->rankAndInsertObject(oid, *(vols[vol_ix].voldesc));
	FDS_PLOG(test_log) << "object " << oid.ToHex() << " volume (prio=" << (vols[vol_ix].voldesc)->relativePrio << ") rank=" << rank;

	if ( (i % 15) == 0)
	  vol_ix = (vol_ix + 1) % vols_count;

      }

    if (!rank_eng->rankingInProgress()) 
      rank_eng->startObjRanking();

    /* wait for ranking to finish */
    while (rank_eng->rankingInProgress()) {
      sleep(1);
    }

    /* we should see 30 objects that need demotion */
    fds_uint32_t len = 7;
    fds_uint32_t count = 0;
    std::pair<ObjectID, ObjectRankEngine::rankOperType> chg_tbl[len];
    do {
      count = rank_eng->getDeltaChangeTblSegment(len, chg_tbl);
      if (count == 0) break;

      for (int i = 0; i < count; ++i)
	{
	  oid = chg_tbl[i].first;
	  if (chg_tbl[i].second == ObjectRankEngine::OBJ_RANK_PROMOTION)
	    FDS_PLOG(test_log) << "UNIT_TEST: chg table obj " << oid.ToHex() << " will be promoted";
	  else { 
	    FDS_PLOG(test_log) << "UNIT_TEST: chg table obj " << oid.ToHex() << " will be demoted"; 
            rank_eng->deleteObject(oid);
          }
	}      

    } while(count > 0);

    /* we should get lowest ranked objects in rank table in order of rank  */
    ObjectID objArray[len];
    int removed = 0;
    do {
      count = rank_eng->getNextTblSegment(len, objArray);
      if (count == 0) break;

      for (int i = 0; i < count; ++i) {
	oid = objArray[i];
	FDS_PLOG(test_log) << "UNIT_TEST: lowest rank obj: " << oid.ToHex() << "; will also delete";
	rank_eng->deleteObject(oid);
	++removed;
      }

    } while (count > 0);    

    /* add higher priority objects that will push out lower rank objects in the table   */

    for (fds_uint32_t i = 1; i <= (removed + 10); ++i)
      {
	oid = ObjectID(1000+i, i*i);

	rank = rank_eng->rankAndInsertObject(oid, *(vols[0].voldesc));
	FDS_PLOG(test_log) << "object " << oid.ToHex() << " volume (prio=" << (vols[0].voldesc)->relativePrio << ") rank=" << rank;

      }

    if (!rank_eng->rankingInProgress()) 
      rank_eng->startObjRanking();

    /* wait for ranking to finish */
    while (rank_eng->rankingInProgress()) {
      sleep(1);
    }

    FDS_PLOG(test_log) << "UNIT_TEST: after next round of ranking:";

    do {
      count = rank_eng->getDeltaChangeTblSegment(len, chg_tbl);
      if (count == 0) break;

      for (int i = 0; i < count; ++i)
	{
	  oid = chg_tbl[i].first;
	  if (chg_tbl[i].second == ObjectRankEngine::OBJ_RANK_PROMOTION)
	    FDS_PLOG(test_log) << "UNIT_TEST: chg table obj " << oid.ToHex() << " will be promoted";
	  else { 
	    FDS_PLOG(test_log) << "UNIT_TEST: chg table obj " << oid.ToHex() << " will be demoted"; 
          }
	}      
    } while (count > 0);

    do {
      count = rank_eng->getNextTblSegment(len, objArray);
      if (count == 0) break;

      for (int i = 0; i < count; ++i) {
	oid = objArray[i];
	FDS_PLOG(test_log) << "UNIT_TEST: lowest rank obj: " << oid.ToHex() << "; will also delete";
      }

    } while (count > 0);


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
  fds::ObjRankUnitTest unit_test(2000, 10);

  return unit_test.Run();
}
