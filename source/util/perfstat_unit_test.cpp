#include <iostream>
#include <string>
#include <cstring>
#include <stdio.h>
#include <stdlib.h>

#include "util/PerfStat.h"

namespace fds {

void dump(const IoStat& stat)
{
  std::cout << "STAT " << stat.getTimestamp() << "," 
	    << stat.getIos() << "," << stat.getAveLatency() << std::endl;
}

int dump_history(StatHistory* hist)
{
  IoStat *stats = NULL;
  int len = 0;

  int ret = hist->getStatsCopy(&stats, &len);
  if (ret != 0) {
    std::cout << "Failed to get copy of stat history" << std::endl;
    return ret;
  }

  for (int i = 0; i < len; ++i)
    {
      dump(stats[i]);
    }

  delete [] stats;

  return 0;
}

int runUnitTest(int slots, int sec_in_slot)
{
  StatHistory *hist = new StatHistory(slots, sec_in_slot);
  if (!hist) {
    std::cout << "Failed to create StatHistory" << std::endl;
    return -1;
  }  

  /* test 1 -- fill 1 I/O per slot */
  boost::posix_time::ptime ts = boost::posix_time::microsec_clock::local_time();
  boost::posix_time::time_duration delta = boost::posix_time::seconds(sec_in_slot);

  for (int i = 0; i < slots; ++i)
    {
      hist->addIo(ts, 200);
      ts += delta;
    }

  std::cout << "Expecting " << slots-1 << " stats, each 1 IOPS and 200microsec latency" << std::endl;
  dump_history(hist);

  /* test 2 -- fill every other slot, 2 IOs per slot */
  ts += boost::posix_time::seconds(sec_in_slot); // offset 
  delta = boost::posix_time::seconds(2*sec_in_slot);

  for (int i = 0; i < slots; ++i)
    {
      hist->addIo(ts, 400);
      hist->addIo(ts, 200);
      ts += delta;
    }

  std::cout << "Expecting " << slots-1 << " stats, every other slot is filled with 2 IOs and 300 microsec lat"
	    << std::endl;
  dump_history(hist);


  /* test 3 -- fill slots with time intervals longer than history */
  ts += boost::posix_time::seconds(sec_in_slot); // start this test with new slot 
  delta = boost::posix_time::seconds((slots-2) * sec_in_slot);
  for (int i = 0; i < 5; ++i)
    {
      hist->addIo(ts, 999);
      ts += delta;
    }
  ts+= boost::posix_time::seconds(sec_in_slot);
  hist->addIo(ts, 888); // this io will be discarded, since last slot is discarded

  std::cout << "Expecting " << slots-1 << " stats, one slot is filled with 1 IO, lat = 999 microsec" << std::endl;
  dump_history(hist);

  /* test 4 -- add IOs out of order */
  ts += boost::posix_time::seconds(sec_in_slot); // start this test with new slot
  delta = boost::posix_time::seconds(sec_in_slot);
  boost::posix_time::time_duration delta2 = boost::posix_time::seconds(slots*sec_in_slot);

  ts += delta;
  ts += delta;
  ts += delta;
  hist->addIo(ts, 500);
  ts -= delta;

  /* add io too far in the past */
  ts -= delta2;
  hist->addIo(ts, 789);
  ts += delta2;

  /* continue adding to current history */
  hist->addIo(ts, 300);
  ts += delta;
  hist->addIo(ts, 200);
  ts += delta;
  hist->addIo(ts, 999);
  ts -= delta;
  hist->addIo(ts, 300);
  ts -= delta;
  hist->addIo(ts, 1);
  ts += delta;
  ts += delta;
  ts += delta;
  ts += delta;
  ts += delta;
  hist->addIo(ts, 5);
  ts+= delta;
  hist->addIo(ts, 6); //this will be discarded
  std::cout << "Expecting " << slots-1 << " stats. A portion of stats should include:" << std::endl
	    << "2 IOs, lat=150.5;  3 IOs, lat=333.33; 1 IO, lat=999" 
	    <<" then 0; 0; 1 IO, lat=5" << std::endl;
  dump_history(hist);

  /* test 5 -- fill in 4 IOs per second  */
  ts += boost::posix_time::seconds(sec_in_slot); // start with next slot after last test
  delta = boost::posix_time::seconds(1);

  for (int i = 0; i < (slots*sec_in_slot); ++i)
    {
      for (int k = 0; k < 4; ++k) {
	hist->addIo(ts, 100);
      }
      ts += delta;
    }

  std::cout << "Expecting " << slots-1 << " stats, every other slot is filled with "
	    << 4*sec_in_slot << " IOs, lat=100" 
	    << std::endl;
  dump_history(hist);

  return 0;
}

} /* namespace fds*/


int main(int argc, char* argv[])
{
  int rc = 0;
  int slots = 10;
  int sec_in_slot = 1;

  for (int i = 1; i < argc; ++i)
    {
      if (strncmp(argv[i], "--slots=", 8) == 0) {
	slots = atoi(argv[i] + 8);
      } 
      else if (strncmp(argv[i], "--sec_in_slot=", 14) == 0) {
	sec_in_slot = atoi(argv[i] + 14);
      }
      else {
	std::cout << "Invalid argument " << argv[i] << std::endl;
	return -1;
      }
    }

  std::cout << "Will use " << slots << " slots" 
	    << " of length " << sec_in_slot << " second(s)" << std::endl; 

  rc = fds::runUnitTest(slots, sec_in_slot);
  if (rc == 0) {
    std::cout << "Perstat unit test PASSED" << std::endl;
  }
  else {
    std::cout << "Perfstat unit test FAILED" << std::endl;
  }


  return 0;
}
