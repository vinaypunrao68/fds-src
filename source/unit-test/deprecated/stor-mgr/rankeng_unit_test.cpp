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
#include "ObjStats.h"

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
    ObjStatsTracker *obj_stats = new ObjStatsTracker(test_log);
    ObjectRankEngine* rank_eng = new ObjectRankEngine("sm", rank_table_size, NULL, obj_stats, test_log);
    if (!rank_eng) {
      FDS_PLOG(test_log) << "basic_ranking: failed to create ObjectRankEngine";
      return 1;
    }    
    fds_uint32_t len = 7;
    fds_uint32_t count = 0;
    ObjectID objArray[len];

    /* lets have 10 volumes, priorities 1 to 10 */
    FDS_Volume vols[vols_count];
    for (int i = 1; i <= vols_count; i++)
      {
	vols[i-1].voldesc = new VolumeDesc(std::string("vol_") + std::to_string(i), i);
	(vols[i-1].voldesc)->iops_min = 0;
	(vols[i-1].voldesc)->relativePrio = i;
      }

    /* lets insert # objects > rank_table_size by 5 */
    ObjectID oid;
    int vol_ix = 1;
    fds_uint32_t rank = 0;
    for (fds_uint32_t i = 1; i <= (rank_table_size + 5); ++i)
      {
	oid = ObjectID(i, i*i);

	/* will round-robin volumes */
	rank = rank_eng->rankAndInsertObject(oid, *(vols[vol_ix].voldesc));
	FDS_PLOG(test_log) << "object " << oid.ToHex() << " volume (prio=" << (vols[vol_ix].voldesc)->relativePrio << ") rank=" << rank;

	if ( (i % 5) == 0)
	  vol_ix = (vol_ix + 1) % vols_count;
      }

    /* lets do ranking first */
    if (!rank_eng->rankingInProgress()) 
      rank_eng->startObjRanking();

    /* wait for ranking to finish */
    while (rank_eng->rankingInProgress()) {
      sleep(1);
    }

    /* simulate some accesses to different objects (on disk, that rank table does not know about) */
    vol_ix = 1;
    for (fds_uint32_t k = 0; k < 100; ++k)
      {
	for (fds_uint32_t i = 1; i <= 5; ++i)
	  {
	    oid = ObjectID(2000+i, i);

	    /* access this object 100 times */
      	    obj_stats->updateIOpathStats(vol_ix, oid);
	  }
	/* each object gets accessed once per 100 ms = 10iops */
	usleep(100000);
      }

    /* should add promotions for hot objs to a change list */
    rank_eng->analyzeStats();


    /* we should see 10 objects that need demotion, and 5 objs that need promotion */
    count = 0;
    ObjectRankEngine::chg_table_entry_t chg_tbl[len];
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

    } while(count > 0);
    
    /* we should get lowest ranked objects in rank table in order of rank  */
       int removed = 0;
      do {
      count = rank_eng->getNextTblSegment(len, objArray);
      if (count == 0) break;

      for (int i = 0; i < count; ++i) {
	oid = objArray[i];
	FDS_PLOG(test_log) << "UNIT_TEST: lowest rank obj: " << oid.ToHex() << "; will also delete";
    
	/* for unit test -- lest delete those low-rank objects (as if migrator took them and actually removed from ssd */ 
    	rank_eng->deleteObject(oid);
	++removed;
      }

    } while (count > 0);    

    FDS_PLOG(test_log) << "UNIT_TEST: removed " << removed << " lowest ranked objs from the rank table";
    FDS_PLOG(test_log) << "UNIT_TEST: tail_rank = " << rank_eng->getTblTailRank();

    /* add higher priority objects that will push out lower rank objects in the table   */
    FDS_PLOG(test_log) << "UNIT_TEST: will add " << (removed + 10) << " high prio vol objects too rank engine";
    for (fds_uint32_t i = 1; i <= (removed + 5); ++i)
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

    /* add hot objects while we have cached insertions and deletions to the table, some of the objs are dup from last hot obj   */
    vol_ix = 1;
    for (fds_uint32_t k = 0; k < 100; ++k)
      {
	for (fds_uint32_t i = 1; i <= 5; ++i)
	  {
	    oid = ObjectID(2000+3+i, i+3);

	    /* access this object 100 times */
      	    obj_stats->updateIOpathStats(vol_ix, oid);
	  }
	/* each object gets accessed once per 100 ms = 10iops */
	usleep(100000);
      }

    /* first wait to see how ranking engine builds a change list based on what it gets from stats */
    rank_eng->analyzeStats();

    /* do ranking -- now we have cached insertions and deletions and hot obj promotions */

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


    delete rank_eng;

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
  fds::ObjRankUnitTest unit_test(40, 10);

  return unit_test.Run();
}
