// Tester for the generators
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "Actuator.H"
#include "ActuatorAsync.H"
#include "BaseRandom48.H"
#include "Experiment.H"
#include "Generator.H"
#include "GeneratorAFAP.H"
#include "GeneratorConstSize.H"
#include "GeneratorRandomSize.H"
#include "GeneratorLocation.H"
#include "GeneratorPoisson.H"
#include "GeneratorPeriodic.H"
#include "GeneratorReplayTrace.H"
#include "GeneratorRandom.H"
#include "GeneratorSeqRun.H"
#include "GeneratorSeq.H"
#include "GeneratorUniform.H"
#include "IO.H"
#include "IOStat.H"
#include "Store.H"
#include "StreamSingleSync.H"
#include "StreamMultiSync.H"

void output_trace_statistics(const char* devname, const char* intrace_name, int start_sec, int n_run_sec, const char* outtrace_name);

void usage_exit(char const* progname)
{
  fprintf(stderr,
"Usage: %s [-q n] [-r x] [-p x] [-e p r e|n_ios] [-k k] [-d r|w] [-t tracename] [-s nnn] [-l intracename] ...\n"
"          <filename> <s,Snnn,r,Rnnn,sr> <n_secs|-n_io|-n_runs>  <sid> [name]\n"
"OR: [-c devicename intracename <start_sec> <n_sec> outname]" 
"Execute IOs against device <filename> for <n_secs> seconds or n_io IOs.\n"
"IOs will be sequential starting at the beginning (s), sequential starting\n"
"nnn MB into the device, random over the whole device (r), or random over\n"
"the first nnn MB of the device (Rnnn).\n"
"The process is associated with session <sid>\n"
" -- NOTE FDS: pass volume id for session id, only used to output in the stats file  \n"
"Option -q: Do up to n IOs in parallel using async IO (default n=1 sync).\n"
"Option -o: Do up to n IOs in parallel using sync IO (default n=1 sync) \n"
"Option -r: Do IOs at a fixed rate of x IOps (default is AFAP).\n"
"Option -p: Do IOs at a rate of x IOps, with poisson arrival times.\n"
"Optiom -e: Do IOs periodically with period p, rate r (disk-usec/sec) and n_ios per period or expected I/O time e (usec).\n"
"Option -k: Each IO is k KB size (default 1KB), if 0 then io size is random (in KB).\n"
"Option -t: Write a trace to file tracename.\n"
"Option -s: Seed the random number generator explicitly (default is ToD)\n"
"Option -c: Parse trace file intracename and output statistics to outname file. Does not execute IOs\n"
"Option -l: Replay trace file intracename.\n"
"If you specify [name], it will be printed on the result line.\n",
  progname);
  exit(1);
}

void usage_error(char const* progname, const char* fmt, ...)
{
  va_list varargs;
  va_start(varargs, fmt);
  (void) vfprintf(stderr, fmt, varargs);
  fprintf(stderr, "\n");
  va_end(varargs);

  usage_exit(progname);
}

int main(int argc, char* argv[])
{
  // ======================================================================
  // Check and decode the command line arguments:
  // TODO: This is terribly pedestrian, and needs to be cleaned up.
  char const* progname = argv[0];

  // Results of command line decoding
  unsigned async_q = 0; // >0 means use async IO.
  unsigned sync_out = 0; // >0 means use parallel sync IO
  double rate_uniform = -1.0; // If >0: IO rate shall be these many IOps,
  double rate_poisson = -1.0; // uniform or poisson distributed
  int seq_run_length = 1; // sequential runlength
  unsigned io_size = 1; // In KB
  char const* tracename = NULL; // If non-null, write trace file here.
  bool use_random_seed = false; // Use random_seed ...
  int random_seed = 0;          // ... to initialize the random generator.
  long rate_periodic = -1;
  unsigned long period = 0;
  long exp_dt = 0;
  const char* intrace_name = NULL;  
  IO::dir_t io_direction = IO::Read;

  // Process options, and count how many argv's we have used up:
  int iarg=1;
  while (iarg<argc &&
         argv[iarg][0] == '-') {

    if (strcmp(argv[iarg], "-c") == 0) {
      const char* outtrace_name = NULL;
      const char* devname = NULL;
      int n_sec = 0;
      int start_sec = 0;
      
      ++iarg;
      if (argc <= iarg+2)
		usage_error(progname, "Not enough arguments after -c");
	  devname = argv[iarg];
		
	  ++iarg;
	  intrace_name = argv[iarg];
	  
	  ++iarg;
	  start_sec = atoi(argv[iarg]);
	  
	  ++iarg;
	  n_sec = atoi(argv[iarg]);
	
	  ++iarg;
	  outtrace_name = argv[iarg];
      ++iarg;
      
      // we do not expect any more arguments, just get statistics from trace and return
      output_trace_statistics(devname, intrace_name, start_sec, n_sec, outtrace_name);
      
      return 0;
    }
    


    if (strcmp(argv[iarg], "-q") == 0) {
      ++iarg;
      if (argc <= iarg+1)
	usage_error(progname, "Not enough arguments after -q");
      if (atoi(argv[iarg]) <= 1)
	usage_error(progname, "Argument %s to -q makes no sense, has to be >1", argv[iarg]);
      async_q = (unsigned) atoi(argv[iarg]);
      ++iarg;
    }

    if (strcmp(argv[iarg], "-o") == 0) {
       ++iarg;
       if (argc <= iarg+1)
          usage_error(progname, "Not enough arguments after -o");
       if (atoi(argv[iarg]) < 1)
          usage_error(progname, "Argument %s to -o makes no sense, has to be >0", argv[iarg]);
       sync_out = (unsigned) atoi(argv[iarg]);
       ++iarg;
    }

    if (strcmp(argv[iarg], "-r") == 0) {
      //if (rate_poisson > 0.0 || seq_run_length >= 1)
      //  usage_error(progname, "Can't use -p, -r and -sr options together.");
      ++iarg;
      if (argc <= iarg+1)
        usage_error(progname, "Not enough arguments after -r");
      rate_uniform = atof(argv[iarg]);
      if (rate_uniform <= 0.0)
        usage_error(progname, "Rate %s after -r is not positive", argv[iarg]);
      ++iarg;
    }    

    if (strcmp(argv[iarg], "-sr") == 0) {
      //if (rate_poisson > 0.0 || rate_uniform > 0.0)
      //  usage_error(progname, "Can't use -p, -r and -sr options together.");
      ++iarg;    
      if (argc <= iarg+1)
        usage_error(progname, "Not enough arguments after -sr");
      seq_run_length = atoi(argv[iarg]);
      if (seq_run_length == 1)
        printf("Choosing runlength 1 is the same as random, next time use random !\n");
      else if (seq_run_length < 1)
        usage_error(progname, "Minimal sequential runlength has to be 1, %s does not work", argv[iarg]);
        
       /*printf("seq_run_length = %d", seq_run_length);
      */
      ++iarg;
    }

    if (strcmp(argv[iarg], "-p") == 0) {
     // if (rate_uniform > 0.0 || seq_run_length >= 1)
     //   usage_error(progname, "Can't use -p, -r and -sr options together.");
      ++iarg;
      if (argc <= iarg+1)
        usage_error(progname, "Not enough arguments after -p");
      rate_poisson = atof(argv[iarg]);
      if (rate_poisson <= 0.0)
        usage_error(progname, "Rate %s after -p is not positive", argv[iarg]);
      ++iarg;
    }
    
    else if (strcmp(argv[iarg], "-e") == 0) {
    	if (rate_uniform > 0.0 || rate_poisson > 0.0)
    		usage_error(progname, "Can't use -p, -r, -sr, and -e together.\n");
    	++iarg;
    	if (argc <= iarg+3)
    		usage_error(progname, "Not enough arguments after -e\n");
    	if (atoi(argv[iarg]) <= 0)
    		usage_error(progname, "Period should be >= 0.\n");    		
    	period = atoi(argv[iarg]);
    	++iarg;
    	if (atoi(argv[iarg]) <= 0)
    		usage_error(progname, "Rate should be >= 0.\n");        	
    	rate_periodic = atoi(argv[iarg]); 
    	++iarg;
	   	exp_dt = atoi(argv[iarg]); //if <0 then its number of IOs per period
    	++iarg;
    }

    else if (strcmp(argv[iarg], "-k") == 0) {
      ++iarg;
      if (argc <= iarg+1)
        usage_error(progname, "Not enough arguments after -k");
      if (atoi(argv[iarg]) < 0)
        usage_error(progname, "IO size %s makes no sense.\n", argv[iarg]);
      io_size = atoi(argv[iarg]); //if 0, then iosize is random
      ++iarg;
    }

   else if (strcmp(argv[iarg], "-d") == 0) {
      ++iarg;
      if (argc <= iarg+1)
        usage_error(progname, "Not enough arguments after -d");

      if (strcmp(argv[iarg], "w") == 0)
	{
	   io_direction = IO::Write;
	   printf("doing writes\n");
	}
      else 
	{
	   printf("doing reads\n");
	}
      ++iarg;
    }

    else if (strcmp(argv[iarg], "-t") == 0) {
      ++iarg;
      if (argc <= iarg+1)
        usage_error(progname, "Not enough arguments after -t");
      tracename = argv[iarg];
      ++iarg;
    }
    
    else if (strcmp(argv[iarg], "-l") == 0) {
      
      ++iarg;
      if (argc <= iarg+1)
		usage_error(progname, "Not enough arguments after -c");
	  intrace_name = argv[iarg];
      ++iarg;
    }  
  
    else if (strcmp(argv[iarg], "-s") == 0) {
      ++iarg;
      if (argc <= iarg+1)
        usage_error(progname, "Not enough arguments after -s");
      random_seed = atoi(argv[iarg]);
      use_random_seed = true;
      ++iarg;
    }

    else
      usage_error(progname, "Option %s unknown.", argv[iarg]);
  }

  if (argc-iarg < 3)
    usage_error(progname, "Not enough arguments");

  char const* filename = argv[iarg];
  ++iarg;

  bool random = false; // True means the user requested "r"
  bool seq_runs = false;
  unsigned random_restricted = 0; // >0 means the user requested "Rnnn"
  unsigned seq_start = 0; // >0 means user requested "Snnn"
  // maybe do some more checks here that some clown does not combine 
  // sequential run paramters with purely sequential or random I/O 
  
  if (strcmp(argv[iarg], "sr") == 0) {
	seq_runs = true;  	
    // It is stupid that we have to tell pharos twice about sequential runs,
    // this needs to be fixed in the next incarnation
  }
  else if (strcmp(argv[iarg], "s") == 0) {
    // Nothing to do, this is the default setting
  }
  else if (argv[iarg][0] == 'S' && strlen(argv[iarg]) > 1) {
    if (atoi(&argv[iarg][1]) <= 0)
      usage_error(progname, "Sequential starting point %s not positive.",
		  argv[iarg]);
    seq_start = (unsigned) atoi(&argv[iarg][1]);
  }
  else if (strcmp(argv[iarg], "r") == 0) {
    random = true;
  }
  else if (argv[iarg][0] == 'R' && strlen(argv[iarg]) > 1) {
    if (atoi(&argv[iarg][1]) <= 0)
      usage_error(progname, "Random restriction value %s not positive.",
		  argv[iarg]);
    random_restricted = (unsigned) atoi(&argv[iarg][1]);
    random = true;
  }
  else {
    usage_error(progname, "Spatial distribution %s unknown", argv[iarg]);
  }
  ++iarg;

  int n_secs = atoi(argv[iarg]);
  ++iarg;
  
  int sid = atoi(argv[iarg]);
  ++iarg;

  char const* experiment_name = NULL;
  if (argc-iarg > 1)
    usage_error(progname, "Too many arguments");
  if (argc-iarg == 1)
    experiment_name = argv[iarg];

  if (n_secs > 0)
    printf("The plan is to execute %u seconds of ", n_secs);
  else
    printf("The plan is to execute %u IOs of ", -n_secs);
    
    
  if (seq_runs)
  	printf("sequential runs consisting of length %d I/O's", seq_run_length);
  else if (random)
    if (random_restricted == 0)
      printf("random (first %uMB)", random_restricted);
    else
      printf("random");
  else
    if (seq_start > 0)
      printf("sequential (starting at %uMB)", seq_start);
    else
      printf("sequential");
  printf(" IOs against %s.\n", filename);
  if (tracename)
    printf("And write a trace to %s.\n", tracename);

  printf("IO size is %uKB", io_size);

  if (io_direction == IO::Write)
	printf(", all writes. \n");
  else
	printf(", all reads. \n");

  if (async_q > 0)
    printf("Execute up to %u asynchronous IOs (AIO) in parallel.\n", async_q);

  if (sync_out > 0)
    printf("Execute up to %u syncronous IOs in parallel. \n", sync_out);

  if (rate_uniform > 0.0)
    printf("IOs at a uniform rate of %lf IOps.\n", rate_uniform);
  else if (rate_poisson > 0.0)
    printf("IOs at a rate of %lf IOps with poisson arrival times.\n", rate_poisson);
  else if (rate_periodic > 0) {
  	if (exp_dt >= 0)
  		printf("IOs at a rate of %ld disk-usec/sec with period %ld usec, assuming %ld access time\n", rate_periodic, period, exp_dt);
  	else
		printf("IOs at a rate of %ld disk-usec/sec with period %ld usec, %ld IOs per period\n", rate_periodic, period, -exp_dt);  	
  }
  else
    printf("IOs as fast as possible.\n");

  if (use_random_seed)
    printf("Random number generator seeded at %d\n", random_seed);
  else
    printf("Random number generator starting at random seed.\n");

  if (experiment_name)
    printf("The experiment name is \"%s\".\n", experiment_name);

  // ======================================================================
  // Initialize stuff:
  if (use_random_seed)
    (void) BaseRandom48::initialize(random_seed);
  else
    (void) BaseRandom48::initialize();

  if (experiment_name)
    (void) experiment(experiment_name);
  else
    (void) experiment("");

  clk()->initialize();

  // For now, the store is readonly, as we do only reads.
  Store s(filename, false, sid);
  s.initialize();
  printf("Capacity of %s is %Lu bytes = %LuKB.\n", 
	 filename, s.getCapacity(), s.getCapacity() / 1024);
	 
	 
  if (intrace_name)
  {
  	// we are replaying trace
    Generator* gen_all = NULL;
  	gen_all = new GeneratorReplayTrace(intrace_name, s.getCapacity());
  	
  	Actuator* a = NULL; // Will be set in the following if statement
  	if (async_q > 0)
  	  a = new ActuatorAsync(async_q, s, tracename, sid);
  	else
  	  a = new Actuator(s, tracename, sid);
  	a->initialize(*gen_all);  	
  	
  	// Execute the IOs:
  	Stream* stream = NULL; // Will be set in the following if statement.
        if (sync_out > 0) {
    	  if (n_secs > 0) 
  	    stream = new StreamMultiSync(sync_out, *a, *gen_all, (double) n_secs);
  	  else
  	    stream = new StreamMultiSync(sync_out, *a, *gen_all, (unsigned) -n_secs, true);
        }
        else {
    	  if (n_secs > 0) 
  	    stream = new StreamSingleSync(*a, *gen_all, (double) n_secs);
  	  else
  	    stream = new StreamSingleSync(*a, *gen_all, (unsigned) -n_secs, true);
	}
	stream->run();  
	
	a->close();
    s.close();
    
    return 0;
  }

  Generator* gen_time = NULL; // Will be set in the following if statement
  //if(seq_runs)
  // && seq_run_length > 1)
  //  gen_time = new GeneratorSeqRuns(seq_run_length); 	
  if (rate_uniform > 0.0)
    gen_time = new GeneratorUniform(rate_uniform);
  else if (rate_poisson > 0.0)
    gen_time = new GeneratorPoisson(rate_poisson);
  else if (rate_periodic > 0) {
  	if (exp_dt >= 0)
  		gen_time = new GeneratorPeriodic(period, rate_periodic, exp_dt);
  	else
  		gen_time = new GeneratorPeriodic(period, rate_periodic, (int) -exp_dt);  	
  }
  else
    gen_time = new GeneratorAFAP;

  Generator* gen_size_dir = NULL;
  if (io_size > 0)
  {
       gen_size_dir = new GeneratorConstSize(*gen_time, io_size * 1024, io_direction);
  }
  else
  {
       //random size
       gen_size_dir = new GeneratorRandomSize(*gen_time, io_direction);
  }

  Generator* gen_loc = NULL; // Will be set in the following if statement
  if (seq_runs)
       gen_loc = new GeneratorSeqRun(*gen_size_dir, s, (unsigned)seq_run_length);  
  else if (random) {
    if (random_restricted > 0) {
      assert((location_t) random_restricted *1024*1024 + 1024 < 
	     s.getCapacity());
      Random* r = new RandomUniform(0.0,
				    (double) random_restricted *1024.*1024.);
      gen_loc = new GeneratorLocation(*gen_size_dir, s, r);
    }
    else {
      gen_loc = new GeneratorRandom(*gen_size_dir, s);
    }
  }
  else { // sequential
    if (seq_start > 0) {
      /* HACK -- for our current tests, we overloading seq_start
      * to mean restrict sequential over first NNN MB rather than
      * start sequential from NNN MB -- if we want to change behavior
      * back, need to modify GeneratorSeq code */
      location_t seq_start_location = seq_start *1024*1024;
      assert(seq_start_location < s.getCapacity());
      gen_loc = new GeneratorSeq(*gen_size_dir, s, seq_start_location);
    }
    else {
      gen_loc = new GeneratorSeq(*gen_size_dir, s);
    }
  }

  // If no tracing is requested, tracename=NULL, and we create an Actuator
  // without tracing by passing in NULL as the second argument.
  Actuator* a = NULL; // Will be set in the following if statement
  if (async_q > 0)
    a = new ActuatorAsync(async_q, s, tracename, sid);
  else
    a = new Actuator(s, tracename, sid);
  a->initialize(*gen_loc);

  // ======================================================================
  // Execute the IOs:
  Stream* stream = NULL; // Will be set in the following if statement.
  if (sync_out > 0)
  {
     if (n_secs > 0)
       stream = new StreamMultiSync(sync_out, *a, *gen_loc, (double) n_secs);
     else
       stream = new StreamMultiSync(sync_out, *a, *gen_loc, (unsigned) -n_secs, true);
  }
  else {
     if (n_secs > 0) 
       stream = new StreamSingleSync(*a, *gen_loc, (double) n_secs);
     else
       stream = new StreamSingleSync(*a, *gen_loc, (unsigned) -n_secs, true);
  }

  stream->run();

  // ======================================================================
  // Finish:
  // The statistics were printed by the stream.

  a->close();
  s.close();
  
  // TODO: Should delete the objects that were created by new above.
}


/*
 * Parses input trace, calculates statistics, and outputs to outtrace_name
 * Trace must be sorted in IO completion time order
 * 
 * \param intrace_name input trace name.
 *        Expected format:
 * 		  <disk ID>,<R/W>,<offset>,<block size>,<IO size>,<enqueue time>,<dispatch time>,<completion time>
 * 
 * \param outtrace_name output trace name.
 * 	     Format:
 * 		 <time in seconds>\t<number of IOs completed>
 */
void output_trace_statistics(const char* devname, const char* intrace_name, int start_sec, int n_run_sec, const char* outtrace_name)
{
	int nios = 0;
	int nsec = 1;
	float cur_start_time = 0.0; 
	float cur_end_time = 1.0;	
	bool bfirst = true;
	char fline[200]; 
	unsigned int disk_id;
	char operation[2];
	unsigned long long offset;
	int block_size;
	long io_size;
	float enqueue_time;
	long dispatch_time, completion_time;
	float sec_completed, sec_dispatched;
	location_t store_capacity;
	bool bExceedCapacity = false;
	
	float last_io_completed = 0.0;
	float io_time_sec;
	long io_time;
	float total_sec_iotime, ave_iotime;
	int io_num;
	int nios_10sec;
	float latency, latency_1s, latency_10s;
	
	char trace_name_tput[256];
	char trace_name_iotime[256];
	char trace_name_avesec_iotime[256];
	char trace_name_10sec_tput[256];
	sprintf(trace_name_tput, "%s-thruput", outtrace_name);
	sprintf(trace_name_iotime, "%s-iotime", outtrace_name);
	sprintf(trace_name_avesec_iotime, "%s-1sec-iotime", outtrace_name);
	sprintf(trace_name_10sec_tput, "%s-10sec-thruput", outtrace_name);

	
	Store s(devname, true, 0);
  	s.initialize();
  	store_capacity = s.getCapacity();
	s.close();
	
	FILE* inf = fopen(intrace_name, "r");
  	if (!inf) {
      	printf("Unable to open tracefile %s\n", intrace_name);
      	return;
  	}
      	
    FILE* fileThruput = fopen(trace_name_tput, "w");
  	if (!fileThruput) {
      	printf("Unable to open tracefile %s\n", trace_name_tput);
      	(void) fclose(inf);
      	return;
  	}
  	
    FILE* fileIOtime = fopen(trace_name_iotime, "w");
  	if (!fileIOtime) {
      	printf("Unable to open tracefile %s\n", trace_name_iotime);
      	(void) fclose(inf);
      	(void) fclose(fileThruput);
      	return;
  	}  	

    FILE* fileSecIOtime = fopen(trace_name_avesec_iotime, "w");
  	if (!fileSecIOtime) {
      	printf("Unable to open tracefile %s\n", trace_name_avesec_iotime);
      	(void) fclose(inf);
      	(void) fclose(fileThruput);
      	(void) fclose(fileIOtime);
      	return;
  	}  	
  	
    FILE* file10SecThruput = fopen(trace_name_10sec_tput, "w");
  	if (!file10SecThruput) {
      	printf("Unable to open tracefile %s\n", trace_name_10sec_tput);
      	(void) fclose(inf);
      	(void) fclose(fileThruput);
      	(void) fclose(fileIOtime);
      	(void) fclose(fileSecIOtime);
      	return;
  	}    	
	
	// parse records
	io_num = 0;
	total_sec_iotime = 0.0;
	nios_10sec = 0;
	latency_1s = 0;
	latency_10s = 0;
	while (fgets(fline, 200, inf) != NULL)
	{
		sec_completed = 0.0;
		sscanf(fline, "%d,%1s,%11llu,%5d,%6ld,%f,%ld,%ld,%f", 
			&disk_id, &operation, &offset, &block_size, &io_size, &enqueue_time, &dispatch_time, &completion_time, &sec_completed);

		if (sec_completed < (float)start_sec)	
			continue;
			
		if (bfirst) {
			cur_start_time = enqueue_time;
			cur_end_time = cur_start_time + 1.0;
			bfirst = false;
		}
		
		if (!bExceedCapacity && offset >= store_capacity)
		{
			// so we just print it once
			printf("WARNING disk %d: offset %llu > store capacity %Lu\n", disk_id, offset, store_capacity);
			bExceedCapacity = true;
		}
		
		// time completed since start of trace
		//sec_completed = enqueue_time + (double)completion_time/1000000.0;
		sec_dispatched =  enqueue_time + (double)dispatch_time/1000000.0;
		latency = sec_completed - enqueue_time;
		
		io_time_sec = sec_completed - sec_dispatched;
		if (io_time_sec < 0)
			printf("%d: %f (io_time) = %f (completed) - %f (dispatched) \n", io_num+1, io_time_sec, sec_completed, sec_dispatched);
			
		if (io_time_sec > (sec_completed - last_io_completed))
			io_time_sec = sec_completed - last_io_completed;
		io_time = (long)(io_time_sec * 1000000.0);
		last_io_completed = sec_completed;	
		
		io_num++;
		fprintf(fileIOtime, "%d\t%ld\t%f\t%f\t%f\n", io_num, io_time, io_time/1000.0, latency, latency*1000.0);		

		if (sec_completed <= cur_end_time)
		{
			nios++;
			total_sec_iotime += io_time_sec;
			latency_1s += latency;
		}
		else 
		{			
			nios_10sec += nios;
			latency_10s += latency_1s;			
			
			if ((nsec % 10) == 0)
			{
				if (nios_10sec > 0) {
					latency_10s = (latency_10s / (float) nios_10sec) * 1000.0; //in ms
				}
				
				fprintf(file10SecThruput, "%d\t%d\t%.3f\n", nsec, (int)(nios_10sec / 10.0), latency_10s);
				nios_10sec = 0;
				latency_10s = 0;
			}
		
			//finished with statistics for this second
			if (nios > 0) {
				latency_1s = (latency_1s / (float) nios) * 1000.0; //ms
			}
			fprintf(fileThruput, "%d\t%d\t%.3f\n", nsec, nios, latency_1s);
			
			ave_iotime = total_sec_iotime / (float)nios;
			fprintf(fileSecIOtime, "%d\t%ld\t%f\n", nsec, (long)(ave_iotime * 1000000.0), ave_iotime * 1000.0);			
			
			cur_start_time = cur_end_time;
			cur_end_time += 1.0;
			nsec++;
			while (sec_completed > cur_end_time) {
				fprintf(fileThruput, "%d\t%d\t%.3f\n", nsec, 0, 0.0);
				fprintf(fileSecIOtime, "%d\t%ld\t%f\n", nsec, 0, 0.0);			
				cur_start_time = cur_end_time;
				cur_end_time += 1.0;
				nsec++;
			}
			
			nios = 1;
			total_sec_iotime = io_time_sec;
			latency_1s = latency;
		}
		
		if (nsec > n_run_sec)
			break;
		
	
			
		//printf("IO %s %d enqueued %f completed %f\n", operation, io_size, enqueue_time, enqueue_time + (double)completion_time/1000000.0);
	}	
	
	if (!bExceedCapacity)
		printf("disk %d: all IOs < %s capacity\n", disk_id, devname);

	(void) fclose(file10SecThruput);	
	(void) fclose(fileSecIOtime);	
	(void) fclose(fileIOtime);
	(void) fclose(fileThruput);
	(void) fclose(inf);
}     
      
