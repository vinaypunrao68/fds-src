#include <string>
#include <cstring>
#include <stdio.h>
#include <stdlib.h>

#include <util/PerfStat.h>

namespace fds {

boost::posix_time::ptime wait_next_io(const boost::posix_time::ptime& now, 
                                      const boost::posix_time::ptime& next_io, long iotime)
{
  boost::posix_time::ptime  ret_next_io;
  boost::posix_time::time_duration delay = next_io - now;
  long long delay_microsec = delay.total_microseconds();
  if (delay_microsec > 0) {
      boost::this_thread::sleep(boost::posix_time::microseconds(delay_microsec));
  }  
  ret_next_io = next_io + boost::posix_time::microseconds(iotime);
  return ret_next_io;    
}


int runMultivolTest(int slots, int sec_in_slot)
{
  Error err(ERR_OK);
  long iotime;
  boost::posix_time::ptime next_io;

  PerfStats* stats = new PerfStats(std::string("perf_test"), sec_in_slot);
  if (!stats) {
    std::cout << "Failed to create PerfStats object" << std::endl;
    return -1;
  }

  err = stats->enable();
  if (!err.ok()) {
    std::cout << "Failed to enable PerfStats" << std::endl;
    return -1;
  }
  /* enabling stats second time should not fail */
  err = stats->enable();
  if (!err.ok()) {
    std::cout << "Error (" << err.GetErrstr() <<")  when enabling perf stats second time" << std::endl;
    return -1;
  }

  std::cout << "Next 10 seconds, volume 1 =  500 IOPS " << std::endl;
  boost::posix_time::ptime now = boost::posix_time::microsec_clock::universal_time();
  boost::posix_time::ptime end_time = now + boost::posix_time::seconds(10);
  iotime = 2000;
  next_io = now + boost::posix_time::microseconds(iotime);
  while (now < end_time) {
      stats->recordIO(1, 50);
      next_io = wait_next_io(now, next_io, iotime);
      now = boost::posix_time::microsec_clock::universal_time();
 }

  std::cout << "Next 10 seconds, volume 2 = 200IOPS" << std::endl;
  now = boost::posix_time::microsec_clock::universal_time();
  end_time = now + boost::posix_time::seconds(10);
  iotime = 5000;
  next_io = now + boost::posix_time::microseconds(iotime);
  while (now < end_time) {
      stats->recordIO(2, 20);
      next_io = wait_next_io(now, next_io, iotime);
      now = boost::posix_time::microsec_clock::universal_time();
  }

  std::cout << "Next 10 seconds, volumes 1 and 2  doing 1000 IOPS" << std::endl;
  now = boost::posix_time::microsec_clock::universal_time();
  end_time = now + boost::posix_time::seconds(10);
  iotime = 1000;
  next_io = now + boost::posix_time::microseconds(iotime);
  while (now < end_time) {
      stats->recordIO(1, 33);
      stats->recordIO(2, 55);
      next_io = wait_next_io(now, next_io, iotime);
      now = boost::posix_time::microsec_clock::universal_time();
  }

  std::cout << "100 volumes, each doing iops with no sleep" << std::endl;
  now = boost::posix_time::microsec_clock::universal_time();
  end_time = now + boost::posix_time::seconds(10);
  while (now < end_time) {
     for (int i = 0; i < 1000; i++) {
        stats->recordIO(100+i, 10);
     }
     now = boost::posix_time::microsec_clock::universal_time();
  } 

  return 0;
} 

int runUnitTest(int slots, int sec_in_slot)
{
  std::ofstream statfile;
  statfile.open("perf_test.log", std::ios::out | std::ios::app );
  if (!statfile.is_open()) {
    std::cout << "Failed to open perf_test file" << std::endl;
    return -1;
  }

  StatHistory *hist = new StatHistory(0, slots, sec_in_slot);
  if (!hist) {
    std::cout << "Failed to create StatHistory" << std::endl;
    return -1;
  }  
  boost::posix_time::ptime start_time = boost::posix_time::second_clock::universal_time();
  long rel_seconds = 0;
  statfile << "Using history with " << slots << " slots, each slot is " << sec_in_slot << " second(s)";

  /* test 1 -- fill 1 I/O per slot */
  int delta = sec_in_slot;
  for (int i = 0; i < slots; ++i)
    {
      hist->addIo(rel_seconds, 200);
      rel_seconds += delta;
    }

  statfile <<"Expecting " << slots-1 << " stats, each 1 IOPS and 200microsec latency" << std::endl;
  hist->print(statfile, boost::posix_time::microsec_clock::local_time());

  /* test 2 -- fill every other slot, 2 IOs per slot */
  rel_seconds += sec_in_slot; //offset 
  delta = 2*sec_in_slot;
  for (int i = 0; i < slots; ++i)
    {
      hist->addIo(rel_seconds, 400);
      hist->addIo(rel_seconds, 200);
      rel_seconds += delta;
    }

  statfile << "Expecting " << slots-1 << " stats, every other slot is filled with 2 IOs and 300 microsec lat" << std::endl;
  hist->print(statfile, boost::posix_time::microsec_clock::local_time());

  /* test 3 -- fill slots with time intervals longer than history */
  rel_seconds += sec_in_slot;
  delta = (slots-2)*sec_in_slot;
  for (int i = 0; i < 5; ++i)
    {
      hist->addIo(rel_seconds, 999);
      rel_seconds += delta;
    }
  rel_seconds += sec_in_slot;
  hist->addIo(rel_seconds, 888); // this io will be discarded, since last slot is discarded

  statfile << "Expecting " << slots-1 << " stats, one slot is filled with 1 IO, lat = 999 microsec" << std::endl;
  hist->print(statfile, boost::posix_time::microsec_clock::local_time());

  /* test 4 -- add IOs out of order */
  rel_seconds += sec_in_slot;
  delta = sec_in_slot;
  int delta2 = slots*sec_in_slot;

  rel_seconds += delta;
  rel_seconds += delta;
  hist->addIo(rel_seconds, 500);
  rel_seconds -= delta;

  /* add io too far in the past */
  rel_seconds -= delta2;
  hist->addIo(rel_seconds, 789);
  rel_seconds += delta2;

  /* continue adding to current history */
  hist->addIo(rel_seconds, 300);
  rel_seconds += delta;
  hist->addIo(rel_seconds, 200);
  rel_seconds += delta;
  hist->addIo(rel_seconds, 999);
  rel_seconds -= delta;
  hist->addIo(rel_seconds, 300);
  rel_seconds -= delta;
  hist->addIo(rel_seconds, 1);
  rel_seconds += 5*delta;
  hist->addIo(rel_seconds, 5);
  rel_seconds+= delta;
  hist->addIo(rel_seconds, 6); //this will be discarded
  statfile << "Expecting 8 stats. A portion of stats should include:" << std::endl ;
  statfile << "2 IOs, lat=150.5;  3 IOs, lat=333.33; 1 IO, lat=999" 
	   << " then 0; 0; 1 IO, lat=5" << std::endl;
  hist->print(statfile, boost::posix_time::microsec_clock::local_time());

  /* test 5 -- fill in 4 IOs per second  */
  rel_seconds += sec_in_slot;
  delta = 1;

  for (int i = 0; i < (slots*sec_in_slot); ++i)
    {
      for (int k = 0; k < 4; ++k) {
	hist->addIo(rel_seconds, 100);
      }
      rel_seconds += delta;
    }

  statfile << "Expecting " << slots-1 << " stats, every slot is filled with "
	   << 4*sec_in_slot << " IOs, " << "IOPS=4 lat=100" << std::endl;
  hist->print(statfile, boost::posix_time::microsec_clock::local_time());

  return 0;
}

} /* namespace fds*/

void usage()
{
  std::cout << "perfstat_unit_test --slots=N --sec_in_slot=" << std::endl;
}

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
	usage();
	return -1;
      }
    }

  rc = fds::runUnitTest(slots, sec_in_slot);
  if (rc == 0) {
    rc = fds::runMultivolTest(slots, sec_in_slot);
  }
  if (rc == 0) {
    std::cout << "Perstat unit test PASSED" << std::endl;
  }
  else {
    std::cout << "Perfstat unit test FAILED" << std::endl;
  }


  return 0;
}
