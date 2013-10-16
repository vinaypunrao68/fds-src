#include <assert.h>
#include <vector>
#include <thread>

#include "StreamMultiSync.H"
#include "Experiment.H"

void StreamMultiSync::run()
{
  start_time = now();
  assert(how_many == 0 || how_long < 0.0);

  stathist->setStartTime(start_time);
  g.set_time_first_io(start_time);

  std::vector<std::thread *> threads;
  for (unsigned int i = 0; i < out_io_count; ++i)
  {
     std::thread* new_thread = new std::thread(start_dispatch_thread, this);
     threads.push_back(new_thread);
  }
  printf("Started %d threads, one for each outstanding IO\n", out_io_count);

  for (unsigned int i = 0; i < out_io_count; ++i)
  {
     threads[i]->join();
  }


  if (experiment()->isNamePresent())
     printf("Experiment result for %s:", experiment()->getName());
  else
     printf("Experiment result: ");
  double runtime = now() - start_time;
  unsigned long c_io = atomic_load(&io_count);
  assert(runtime > 0.0);
  printf("%lu IOs in %.6lf seconds, rate is %.2lf IOps\n",
         c_io, runtime, (double)c_io/runtime);
}

void StreamMultiSync::start_dispatch_thread(StreamMultiSync *self)
{
   assert(self);
   self->dispatch_io_thread();
}

void StreamMultiSync::dispatch_io_thread()
{
   if (how_long > 0.0) {
      dispatch_io_thread_howlong();
   }
   else {
      dispatch_io_thread_howmany();
   }
}

void StreamMultiSync::dispatch_io_thread_howmany()
{
  unsigned long thread_ios = 0;
  unsigned long c_io = atomic_fetch_add(&io_count, (unsigned long)1);
  double end_time;
  /* stats */
  long cur_sec_since_start = 0;
  fds::IoStat cur_stat;  /* we update stat for current second locally, 
                          * and add it to stat history when next second starts -- this reduces locking overhead */
  cur_stat.reset(cur_sec_since_start);

  printf("dispatch_io_thread - start\n");

  while ((c_io + 1) <= how_many)
  {
    /* dispatch IO*/
    IO io;
    write_lock.lock();
    g.make(io, 0.0, 0.0);
    write_lock.unlock();
    IOStat ios(io);
    a.doIO(ios);

     /* io finished */
     end_time = now();
     double latency = 0.1; //ios.get_U_Stop() - ios.get_U_Start();
     long sec_slot = (end_time - start_time);
     if (sec_slot != cur_sec_since_start)
     {
         assert(sec_slot > cur_sec_since_start);
         recordStat(cur_stat, end_time);

         /* start next slot */
         cur_sec_since_start = sec_slot;
         cur_stat.reset(cur_sec_since_start);
     }
     cur_stat.add((long)(latency*1000000.0));

     c_io = atomic_fetch_add(&io_count, (unsigned long)1);
     thread_ios++;

     /* also stop if generator cannot generate more IOs */
     if ((max_io_count > 0) && ((c_io+1) > max_io_count))
         break;
  }

  /* sub last io since it was over the limit */
  c_io = atomic_fetch_sub(&io_count, (unsigned long)1);
  printf("dispatch_io_thread - dispatched %lu ios -- end \n", thread_ios); 
}

void StreamMultiSync::dispatch_io_thread_howlong()
{
  unsigned long thread_ios = 0;
  unsigned long c_io;
  double end_time;
  /* stats */
  long cur_sec_since_start = 0;
  fds::IoStat cur_stat;  /* we update stat for current second locally, 
                          * and add it to stat history when next second starts -- this reduces locking overhead */
  cur_stat.reset(cur_sec_since_start);

  while (now() < (start_time + how_long)) 
  {
     /* dispatch io */
     IO io;
     write_lock.lock();
     g.make(io, 0.0, 0.0);
     write_lock.unlock();
     IOStat ios(io);
     a.doIO(ios);

     /* io finished */
     end_time = now();
     double latency = 0.1; //ios.get_U_Stop() - ios.get_U_Start();
     long sec_slot = (end_time - start_time);
     if (sec_slot != cur_sec_since_start)
     {
         assert(sec_slot > cur_sec_since_start);
         recordStat(cur_stat, end_time);

         /* start next slot */
         cur_sec_since_start = sec_slot;
         cur_stat.reset(cur_sec_since_start);
     }
     cur_stat.add((long)(latency*1000000.0));

     c_io = atomic_fetch_add(&io_count, (unsigned long)1);
     c_io++;
     thread_ios++; 

     if ((max_io_count > 0) && (c_io > max_io_count))
        break;
  }

  printf("dispatch_io_thread - dispatched %d ios -- end \n", thread_ios);
}

void StreamMultiSync::recordStat(const fds::IoStat& iostat, double now)
{
  write_lock.lock();
  stathist->addStat(iostat);
  if (now > next_print_time)
    {
      stathist->print(a.getTracefile(), now);
      next_print_time += (double)PHAROS_PRINT_INTERVAL;
    }
  write_lock.unlock();
}




