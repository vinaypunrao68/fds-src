#include <string>
#include <cstring>
#include <stdio.h>
#include <stdlib.h>

#include <fds_typedefs.h>
#include <util/timeutils.h>
#include <fds_process.h>
#include <PerfHistory.h>
#include <lib/StatsCollector.h>

namespace fds {

//
// all times are in nanoseconds
//
fds_uint64_t wait_next_io(fds_uint64_t now, fds_uint64_t next_io, long iotime)
{
    fds_uint64_t delay_micro = 0;
    if (next_io > now) delay_micro = (next_io - now) / 1000;
    if (delay_micro > 0) {
        boost::this_thread::sleep(boost::posix_time::microseconds(delay_micro));
    }
    return (next_io + iotime*1000);
}


int runMultivolTest(int slots, int sec_in_slot)
{
  Error err(ERR_OK);
  long iotime;
  fds_uint64_t next_io;
  fds_uint64_t nanos_in_sec = 1000000000;
  fds_uint64_t nanos_in_micro = 1000;

  StatsCollector* stats = StatsCollector::singleton();
  stats->enableQosStats("AM");

  std::cout << "Next 10 seconds, volume 1 =  200 IOPS " << std::endl;
  fds_uint64_t now = util::getTimeStampNanos();
  fds_uint64_t end_time = now + 10 * nanos_in_sec;
  iotime = 2000;
  next_io = now + iotime * nanos_in_micro;
  while (now < end_time) {
      stats->recordEvent(fds_volid_t(1), now, STAT_AM_PUT_OBJ, 50);
      next_io = wait_next_io(now, next_io, iotime);
      now = util::getTimeStampNanos();
 }

  std::cout << "Next 10 seconds, volume 2 = 200IOPS" << std::endl;
  now = util::getTimeStampNanos();
  end_time = now + 10 * nanos_in_sec;
  iotime = 5000;
  next_io = now + iotime * nanos_in_micro;
  while (now < end_time) {
      stats->recordEvent(fds_volid_t(2), now, STAT_AM_PUT_OBJ, 20);
      next_io = wait_next_io(now, next_io, iotime);
      now = util::getTimeStampNanos();
  }

  std::cout << "Next 10 seconds, volumes 1 and 2  doing 1000 IOPS" << std::endl;
  now = util::getTimeStampNanos();
  end_time = now + 10 * nanos_in_sec;
  iotime = 1000;
  next_io = now + iotime * nanos_in_micro;
  while (now < end_time) {
      stats->recordEvent(fds_volid_t(1), now, STAT_AM_PUT_OBJ, 33);
      stats->recordEvent(fds_volid_t(2), now, STAT_AM_PUT_OBJ, 55);
      next_io = wait_next_io(now, next_io, iotime);
      now = util::getTimeStampNanos();
  }

  std::cout << "100 volumes, each doing iops with no sleep" << std::endl;
  now = util::getTimeStampNanos();
  end_time = now + 10 * nanos_in_sec;
  while (now < end_time) {
     for (int i = 0; i < 1000; i++) {
         stats->recordEvent(fds_volid_t(100+i), now, STAT_AM_PUT_OBJ, 10);
     }
     now = util::getTimeStampNanos();
  } 

  return 0;
} 


void printSlot(const std::string& name,
               FdsStatType evt,
               const StatSlot& slot) {
    std::cout << name << " Ev/sec " << slot.getEventsPerSec(evt) << " total "
              << slot.getTotal(evt) << " average " << slot.getAverage(evt)
              << std::endl;
}

int statSlotTest(int sec_in_slot)
{
    fds_uint64_t rel_sec = 0;
    StatSlot slot;
    slot.reset(rel_sec, sec_in_slot);

    // add couple of counters
    GenericCounter c1, c2;
    fds_uint32_t cnt = 10;
    for (fds_uint32_t i = 0; i < cnt; ++i) {
        c1.add(5);
        c2.add(i+1);
    }

    std::cout << "Reading counters directly "
              << "c1: " << c1 << " average = " << c1.average()
              << "; c2: " << c2 << " average = " << c2.average() << std::endl;

    // add to slot
    slot.add(STAT_AM_PUT_OBJ, c1);
    slot.add(STAT_AM_GET_OBJ, c2);

    std::cout << slot << std::endl;
    printSlot("PUT_IO", STAT_AM_PUT_OBJ, slot);
    printSlot("GET_IO", STAT_AM_GET_OBJ, slot);

    // new slot
    StatSlot newslot;
    newslot.reset(rel_sec, sec_in_slot);
    for (fds_uint32_t i = 0; i < cnt; ++i) {
        newslot.add(STAT_AM_PUT_OBJ, 5);
        newslot.add(STAT_AM_GET_OBJ, i+1);
    }
    std::cout << newslot << std::endl;
    printSlot("PUT_IO", STAT_AM_PUT_OBJ, newslot);
    printSlot("GET_IO", STAT_AM_GET_OBJ, newslot);

    // add old and new slot
    newslot += slot;
    std::cout << "Sum of old and new slot " << newslot << std::endl;
    printSlot("PUT_IO", STAT_AM_PUT_OBJ, newslot);
    printSlot("GET_IO", STAT_AM_GET_OBJ, newslot);

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

  fds_uint64_t start_time = util::getTimeStampNanos();
  fds_uint64_t sec_in_slot64 = sec_in_slot;
  fds_uint64_t nanos_in_slot = sec_in_slot64 * 1000000000;
  VolumePerfHistory *hist = new VolumePerfHistory(fds_volid_t(1), start_time,
                                                  slots, sec_in_slot);
  if (!hist) {
    std::cout << "Failed to create VolumePerfHistory" << std::endl;
    return -1;
  }  
  long rel_seconds = 0;
  statfile << "Using history with " << slots << " slots, each slot is "
           << sec_in_slot << " second(s)" << std::endl;

  // test 1 -- fill 1 I/O per slot
  fds_uint64_t ts = start_time;
  fds_uint64_t delta = nanos_in_slot;
  for (int i = 0; i < slots; ++i)
    {
        hist->recordEvent(ts, STAT_AM_PUT_OBJ, 200);
        ts += delta;
    }

  statfile << "Expecting " << slots-1
           << " stats, each 1 IOPS and 200 microsec latency" << std::endl;
  statfile << *hist << std::endl;

  /* test 2 -- fill every other slot, 2 IOs per slot */
  ts += nanos_in_slot;
  delta = 2 * nanos_in_slot;
  for (int i = 0; i < slots; ++i)
    {
        hist->recordEvent(ts, STAT_AM_PUT_OBJ, 400);
        hist->recordEvent(ts, STAT_AM_PUT_OBJ, 200);
        ts += delta;
    }

  statfile << "Expecting " << slots-1
           << " stats, every other slot is filled with 2 IOs and 300 micros lat"
           << std::endl;
  statfile << *hist << std::endl;

  /* test 3 -- fill slots with time intervals longer than history */
  ts += nanos_in_slot;
  delta = (slots-2) * nanos_in_slot;
  for (int i = 0; i < 5; ++i)
    {
        hist->recordEvent(ts, STAT_AM_PUT_OBJ, 999);
        ts += delta;
    }
  ts += nanos_in_slot;
  hist->recordEvent(ts, STAT_AM_PUT_OBJ, 888);   // this last slot will be discarded

  statfile << "Expecting " << slots-1
           << " stats, one slot is filled with 1 IO, lat = 999 microsec"
           << std::endl;
  statfile << *hist << std::endl;

  /* test 4 -- add IOs out of order */
  ts += nanos_in_slot;
  delta = nanos_in_slot;
  int delta2 = slots * nanos_in_slot;

  ts += delta;
  ts += delta;
  hist->recordEvent(ts, STAT_AM_PUT_OBJ, 500);
  ts -= delta;

  /* add io too far in the past */
  ts -= delta2;
  hist->recordEvent(ts, STAT_AM_PUT_OBJ, 789);
  ts += delta2;

  /* continue adding to current history */
  hist->recordEvent(ts, STAT_AM_PUT_OBJ, 300);
  ts += delta;
  hist->recordEvent(ts, STAT_AM_PUT_OBJ, 200);
  ts += delta;
  hist->recordEvent(ts, STAT_AM_PUT_OBJ, 999);
  ts -= delta;
  hist->recordEvent(ts, STAT_AM_PUT_OBJ, 300);
  ts -= delta;
  hist->recordEvent(ts, STAT_AM_PUT_OBJ, 1);
  ts += 5*delta;
  hist->recordEvent(ts, STAT_AM_PUT_OBJ, 5);
  ts += delta;
  hist->recordEvent(ts, STAT_AM_PUT_OBJ, 6);  // will be discarded

  statfile << "Expecting 8 stats. A portion of stats should include:"
           << std::endl ;
  statfile << "2 IOs, lat=150.5;  3 IOs, lat=333.33; 1 IO, lat=999" 
	   << " then 0; 0; 1 IO, lat=5" << std::endl;
  statfile << *hist;

  /* test 5 -- fill in 4 IOs per second  */
  ts += nanos_in_slot;
  delta = 1000000000;

  for (int i = 0; i < (slots*sec_in_slot); ++i)
    {
      for (int k = 0; k < 10; ++k) {
          hist->recordEvent(ts, STAT_AM_PUT_OBJ, 100);
      }
      for (int k = 0; k < 40; ++k) {
          hist->recordEvent(ts, STAT_AM_GET_OBJ, 5);
      }
      ts += delta;
    }

  statfile << "Expecting " << slots-1 << " stats, every slot is filled with "
	   << 4*sec_in_slot << " IOs, " << "IOPS=10 lat=100" << std::endl;
  statfile << *hist << std::endl;

  boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time();
  hist->print(statfile, now, 0);

  std::cout << "Testing converting to FDSP and back" << std::endl;
  fpi::VolStatList fdsp_stats;
  fds_uint64_t rel_sec = hist->toFdspPayload(fdsp_stats, 0);

  fds_uint64_t recv_start_time = start_time + nanos_in_slot;
  VolumePerfHistory *recv_hist = new VolumePerfHistory(fds_volid_t(1), recv_start_time,
                                                       slots, sec_in_slot);
  if (!hist) {
    std::cout << "Failed to create VolumePerfHistory" << std::endl;
    return -1;
  }

  Error err = recv_hist->mergeSlots(fdsp_stats);
  statfile << "Sent history: " << *hist << std::endl;
  statfile << "Received history: " << *recv_hist << " " << err << std::endl;
  statfile << "Sent another history" << std::endl;
  err = recv_hist->mergeSlots(fdsp_stats);
  statfile << "Merged history: " << *recv_hist << " " << err << std::endl;

  VolumePerfHistory::ptr snap = hist->getSnapshot();
  statfile << "Snap history: " << *snap;


  std::cout << "Testing converting to list of slots and back" << std::endl;
  std::vector<StatSlot> slot_list;
  rel_sec = hist->toSlotList(slot_list, 0);

  recv_start_time = start_time + nanos_in_slot;
  VolumePerfHistory *other_hist = new VolumePerfHistory(fds_volid_t(1), recv_start_time,
                                                       slots, sec_in_slot);
  if (!hist) {
    std::cout << "Failed to create VolumePerfHistory" << std::endl;
    return -1;
  }

  other_hist->mergeSlots(slot_list);
  statfile << "Source history: " << *hist << std::endl;
  statfile << "Destination history: " << *other_hist << std::endl;
  statfile << "Add source history again" << std::endl;
  other_hist->mergeSlots(slot_list);
  statfile << "Merged history: " << *other_hist << std::endl;


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

  rc = fds::statSlotTest(sec_in_slot);
  std::cout << "runUnitTest" << std::endl;
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
