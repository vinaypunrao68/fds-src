#include <assert.h>
#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "BaseRandom48.H"
#include "RandomUniform.H"
#include "printe.H"

// This program is a dirty hack, as far as programming style is concerned.
// Only saving grace: it was written very quickly.

// ---------------------------------------------------------------------
// Utilities for printing to the output file

static FILE* f = NULL;

void p(const char* fmt, ...)
{
  va_list varargs;
  va_start(varargs, fmt);
  (void) vfprintf(f, fmt, varargs);
  va_end(varargs);
}

void P(const char* fmt, ...)
{
  va_list varargs;
  va_start(varargs, fmt);
  (void) vfprintf(f, fmt, varargs);
  (void) fprintf(f, "\n");
  va_end(varargs);
}

void P()
{
  (void) fprintf(f, "\n");
}

// ---------------------------------------------------------------------
// Utilities for generating the name of a zygaria device, and the name of
// output files:

static bool use_disk = false;

// TODO: This function is not reentrant - returning a pointer to a static
// variable is really ugly.
char const* zyg(unsigned pool, unsigned session) {
  static char buf[100];
  if (use_disk) {
    (void) snprintf(buf, 100, "/dev/hdk");
  }
  else {
    assert(session<10);
    assert(pool<4);
    (void) snprintf(buf, 100, "/dev/zygaria%c%u", 'a'+pool, session);
  }
  
  return buf;
}

char const* exp() {
  static char const exp41[] = "exp4-1";
  static char const exp42[] = "exp4-2";
  return (use_disk) ? exp42 : exp41;
}

char const* expfile() {
  static char const expfile41[] = "exp/exp4-1";
  static char const expfile42[] = "exp/exp4-2";
  return (use_disk) ? expfile42 : expfile41;
}

// ---------------------------------------------------------------------
// Utilities for calculating IO rates and locations

static const unsigned disk_capacity = 114473; // MB
static const unsigned disk_speed = 112; // IOps

// Function that calculated the IOps, given a nominal percentage of the disk.
// TODO: Make the overall disk speed adjustable.
// Usage: percent(10.) will give you 10% of the disk.
unsigned percent(double v)
{
  assert(v>0.);
  // The result is rounded DOWN, to make sure that sessions don't exceed the pool.
  return (int) ( v * disk_speed /100. );
}

unsigned location_random()
{
  return (unsigned) ( BaseRandom::instance()->getDouble() * (double) disk_capacity );
}

// ---------------------------------------------------------------------
// Stuff to track the small random jobs
typedef struct job_struct {
  unsigned pool; // What pool it is in
  unsigned start; // Start time
  unsigned duration;  // How long it runs
  unsigned iops; // IOps for this job
  unsigned total_iops; // Total IOps in the system at the time this job starts.
  unsigned session; // What session number (0...15) it will be run under.
} job;

// ---------------------------------------------------------------------
int main(int argc, char* argv[])
{
  // Parameters.  TODO: Make them adjustable.

  // Overall duration of the experiment:
  unsigned exp_duration = 1000;  
  // Number of little jobs to run per pool:
  unsigned n_jobs = 100;
  // How many IOps for each of the random job pools:
  unsigned random_pool_iops = percent(28.5);
  // Minimum IOps for a random job pool (maximum is random_pool_iops):
  unsigned min_iops = 2;
  // Min and max duration of a random job:
  unsigned min_duration = 10;
  unsigned max_duration = 100;

  // The parameters of the background pool and the multimedia pool are below:

  // After a job finishes, we wait a few seconds before we release the IO rate,
  // in case Pharos runs a little slow.  This allows for the setparms
  // to set the rate back down to zero before some other random job uses it.
  unsigned time_slop = 2;

  // We don't allow the random jobs to use all of the IO rate of the random
  // pool; it seems that rounding errors prevent the sum of all the random jobs
  // being exactly equal to the pool.  So we leave this much safety margin:
  unsigned iops_slop = 1;

  // Decode options:
  // -d: Use the real disk /dev/hdk instead of /dev/zygaria<p><s>
  // -c: Use IO counts instead of times for the random jobs
  // -sn: Use n as the seed for the random number generator.
  // -s: Use a random seed for the random number generator.
  bool use_iocounts=false;
  bool use_random_seed=false;
  int seed=19610826;
  bool seed_set=false;

  for (int iarg=1; iarg<argc; ++iarg) {
    if (strcmp(argv[iarg], "-d") == 0) {
      use_disk = true;
      printf("Using real disk /dev/hdk instead of /dev/zygaria<p><s>.\n");
    }
    else if (strcmp(argv[iarg], "-c") == 0) {
      use_iocounts = true;
      printf("Using fixed IO counts instead of fixed times for the random jobs.\n");
    }
    else if (strcmp(argv[iarg], "-s") == 0) {
      if (strlen(argv[iarg]) == 2) {
	use_random_seed=true;
	printf("Using random seed for the random number generator.\n");
      }
      else {
	seed = atoi(&argv[iarg][2]);
	printf("Using seed %d for the random number generator.\n", seed);	
	seed_set = true;
      }
    }
    else {
      fprintf(stderr, "Option %s unknown.\n", argv[iarg]);
      exit(1);
    }
  }

  f = fopen(expfile(), "w");
  if (!f)
    printx("Unable to create output file %s: %s\n",
	    expfile(), strerror(errno));

  P("#!/bin/csh");
  P("# Auto-generated script for zygaria macro performance experiments");
  P("# This script is intended to live in zygaria/exp");
  P();

  P("########################################################################");
  P("# Initialization");
  P();

  // Seed the random number generator:
  if (use_random_seed) {
    (void) BaseRandom48::initialize();
    printf("GenerateMacroExperiments starting with random seed.\n");
    P("# Random number generated starting with random seed.\n");
  }
  else {
    (void) BaseRandom48::initialize(seed);
    if (seed_set) {
      printf("GenerateMacroExperiments starting with seed parameter %d.\n", seed);
      P("# Random number generated starting with seed parameter %d.\n", seed);
    }
    else {
      printf("GenerateMacroExperiments starting with default seed %d.\n", seed);
      P("# Random number generated starting with default seed %d.\n", seed);
    }
  }

  if (!use_disk) {
    P("# Initialize the zygaria kernel driver.  Relies on a script named zyginit.");
    P("./zyginit");
    P();
  }

  P("set Setparms=../kernel/setparms");
  P("set Pharos=../pharos/PharosSync");
  P("# All pharos runs write their chatty output to a log file:");
  P("set Pharlog=results/%s.pharos.log", exp());
  P();

  if (!use_disk) {
    P("########################################################################");
    P("# Configure the pools/sessions that can be configured statically.");
    P();

    P("# Pool a: Single sequential background workload.");
    P("# Pool has 10%% of the disk reserved, 25%% as the limit.");
    P("$Setparms %s pool %u %u", zyg(0,0), percent(10.), percent(25.));
    P("# Only one session, so values are the same as pool.");
    P("$Setparms %s session %u %u", zyg(0,0), percent(10.), percent(25.));
    P();

    // Officially, the media pool has only 30% reserve.  We give it 33%, so if
    // the workload is running at exactly 30%, it is always a little behind the
    // deadline, and gets run with high priority.
    P("# Pool b: Three media streams.");
    P("# Pool has 33%% reserve, 35%% limit.");
    P("$Setparms %s pool %u %u", zyg(1,0), percent(33.), percent(35.));
    P("# Three sessions have one third of the pool each:");
    for (unsigned i=0; i<3; ++i)
      P("$Setparms %s session %u %u", zyg(1,i), i, percent(33./3.), percent(35./3.));
    P();

    P("# Pools c+d: Random apps live in here.  We'll create the sessions later.");
    P("# Each has 28.5% reserve, no limit.");
    P("$Setparms %s pool %u 10000", zyg(2,0), random_pool_iops);
    P("$Setparms %s pool %u 10000", zyg(3,0), random_pool_iops);
    P();
  }

  P("########################################################################");
  P("# Start the workloads that run the whole length.");
  P();

  P("# Pool a: One random generator, AFAP.");
  P("$Pharos -t results/%s.a %s r %u a >> $Pharlog &",
    exp(), zyg(0,0), exp_duration);
  P();

  // Pick three random numbers that shall be no closer than 5% of the
  // disk capacity from each other, and from zero (where the workload for pool A
  // starts):
  unsigned media_start[3];
  for (unsigned i=0; i<3; ++i) {
    bool accepted = false;
    while (!accepted) {
      media_start[i] = location_random();
      accepted = true; // Innocent until proven guilty
      if (media_start[i] / (double) disk_capacity < 0.05)
	accepted = false;
      for (unsigned j=0; accepted && j<i; ++j) {
	double distance = fabs((double)media_start[i] - (double)media_start[j]);
	if (distance / (double)disk_capacity < 0.05)
	  accepted = false;
      }
    }
  }

  P("# Pool b: Three generators, one for each session, starting at random places:");
  for (int i=0; i<3; ++i)
    P("$Pharos -t results/%s.b.%u -r %u %s S%u %u b >> $Pharlog &",
      exp(), i, percent(30./3.), zyg(1,i), media_start[i], exp_duration);
  P();

  // ---------------------------------------------------------------------
  // Just for cross-check: Calculate and print the mean IOps expected in the
  // random pools:
  double mean = n_jobs * 
		(min_duration + max_duration + 1) /2. / exp_duration *
		(min_iops + random_pool_iops +1) /2.;
  printf("Check: Mean IOps in random pools is %f out of %u allowed.\n",
	 mean, random_pool_iops);

  // Now generate zillions of little random jobs for the two random pools:
  RandomUniform r_start(0., (double) exp_duration);
  RandomUniform r_duration((double) min_duration, (double) (max_duration+1) );
  RandomUniform r_iops((double) min_iops, (double) (random_pool_iops+1));

  job** jobs = new job*[2];
  assert(jobs != NULL);
  for (unsigned i_pool=0; i_pool<2; ++i_pool) {
    jobs[i_pool] = new job[n_jobs];
    assert(jobs[i_pool] != NULL);
  }

  P("########################################################################");
  P("# Time series output (for Richard) and debugging (for Ralph):");
  for (unsigned i_pool=0; i_pool<2; ++i_pool) {
    // Alias for all the jobs for this pool, for convenience:
    job* pjobs = jobs[i_pool];

    for (unsigned i=0; i<n_jobs; ++i) {
      bool accepted = false;
      unsigned c_tries = 0;
      while (!accepted) {
	assert (c_tries < 10000);

	pjobs[i].pool = i_pool;
	pjobs[i].start = r_start.getU32();
	pjobs[i].duration = r_duration.getU32();
	pjobs[i].iops = r_iops.getU32();

	accepted = true; // Innocent until proven guilty

	// Check trivial invariants:
	assert(pjobs[i].duration >= min_duration);
	assert(pjobs[i].duration <= max_duration);
	assert(pjobs[i].iops >= min_iops);
	assert(pjobs[i].iops <= random_pool_iops);

	// Reject this job if it ends after the whole experiment duration:
	if (pjobs[i].start + pjobs[i].duration +time_slop > exp_duration)
	  accepted = false;

	// Calculate the total IOps at the time this new job turns on; make sure it
	// doesn't exceed the reserve of the pool:
	pjobs[i].total_iops = pjobs[i].iops;
	for (unsigned j=0; accepted && j<i; ++j) {
	  if (pjobs[j].start <= pjobs[i].start &&
	      pjobs[j].start+pjobs[j].duration+time_slop > pjobs[i].start) {
	    pjobs[i].total_iops += pjobs[j].iops;
	    // P("# Overlap 1  %u:%u+%u@%u and %u:%u+%u@%u -> %u",
	    //   i, pjobs[i].start, pjobs[i].duration, pjobs[i].iops,
	    //   j, pjobs[j].start, pjobs[j].duration, pjobs[j].iops,
	    //   pjobs[i].total_iops);
	    if (pjobs[i].total_iops > random_pool_iops - iops_slop)
	      accepted = false;
	  }
	}

	// Calculate the total IOps at the turnon time of any other job that
	// starts while this new job is on:
	for (unsigned j=0; accepted && j<i; ++j) {
	  if (pjobs[j].start >= pjobs[i].start &&
	      pjobs[j].start < pjobs[i].start + pjobs[i].duration +time_slop) {
	    // P("# Overlap 2  %u:%u+%u@%u and %u:%u+%u@%u -> %u",
	    //   i, pjobs[i].start, pjobs[i].duration, pjobs[i].iops,
	    //   j, pjobs[j].start, pjobs[j].duration, pjobs[j].iops,
	    //   pjobs[j].total_iops + pjobs[i].iops);
	    if (pjobs[j].total_iops + pjobs[i].iops > random_pool_iops -iops_slop)
	      accepted = false;
	  }
	}
        ++c_tries;
      }
      if (accepted) {
	// Update all the jobs that come on while this job is running, and
	// increment their total_iops at turnon time:
	for (unsigned j=0; j<i; ++j) {
	  if (pjobs[j].start >= pjobs[i].start &&
	      pjobs[j].start < pjobs[i].start + pjobs[i].duration) {
	    pjobs[j].total_iops += pjobs[i].iops;
	    assert(pjobs[j].total_iops <= random_pool_iops);
	  }
	}
      }
    }

    // Now output the time series for Richard's benefit.  While we are at it,
    // assign sessions to each job.  Make sure at any given time there are no
    // more than 10 jobs running:
    unsigned i_session = 0;
    P("# Time series for pool %c: time iops (sessions...)", 'c'+i_pool);
    for (unsigned t=0; t<exp_duration; ++t) {
      unsigned t_iops = 0;
      unsigned sessions[10];
      unsigned n_sessions = 0;
      for (unsigned i=0; i<n_jobs; ++i) {
	if (pjobs[i].start <= t && 
	    pjobs[i].start + pjobs[i].duration > t) {
	  t_iops += pjobs[i].iops;

	  if (pjobs[i].start == t) {
  	    pjobs[i].session = i_session;
	    ++i_session;
	    if (i_session >= 10)
	      i_session = 0;
	  }

          sessions[n_sessions] = pjobs[i].session;
	  ++n_sessions;
	  assert(n_sessions < 10);
	}
      }
      p("# %u: %u (", t, t_iops);
      assert(t_iops <= random_pool_iops);
      for (unsigned i=0; i<n_sessions; ++i) {
	if (i>0)
  	  p(" %u", sessions[i]);
	else
  	  p("%u", sessions[i]);
      }
      P(")");
    }
    P();

    // For debugging, print out the resulting jobs
    #if 1
    P("# Debugging: All jobs for pool %c:", 'c'+i_pool);
    for (unsigned i=0; i<n_jobs; ++i)
      P("# Job no. %u Start %u Duration %u IOps %u Session %u", i, 
	pjobs[i].start, pjobs[i].duration, pjobs[i].iops, pjobs[i].session);
    P();
    #endif
  }

  // OK, now we have two lists of jobs.  Now turn them into one global time
  // series.  While we are at it, calculate for each entry in the time series
  // which jobs are starting at that time:
  P("# Time series for all pools: time iops (pool:sessions@iops...)");
  P("# Only times where a job starts are listed.");
  job*** starting = new job**[exp_duration];
  for (unsigned t=0; t<exp_duration; ++t) {
    job* temp[2*10];
    unsigned n_temp = 0;
    for (unsigned i_pool=0; i_pool<2; ++i_pool) {
      for (unsigned i_job=0; i_job<n_jobs; ++i_job) {
	if (jobs[i_pool][i_job].start == t) {
	  temp[n_temp] = & (jobs[i_pool][i_job] );
	  ++n_temp;
	  assert(n_temp < 2*10);
	}
      }
    }

    if (n_temp > 0) {
      starting[t] = new job*[n_temp+1];
      for (unsigned i=0; i<n_temp; ++i)
	starting[t][i] = temp[i];
      starting[t][n_temp] = NULL;
    }
    else {
      starting[t] = NULL;
    }
  }

  // We walk the newly created data structure again.  This is a test.
  for (unsigned t=0; t<exp_duration; ++t) {
    if (starting[t] != NULL) {
      unsigned t_iops = 0;
      for (job** pj=starting[t]; *pj!=NULL; ++pj) {
	job* j = *pj;
	t_iops += j->iops;
      }
      p("# %u %u (", t, t_iops);
      for (job** pj=starting[t]; *pj!=NULL; ++pj) {
	job* j = *pj;
	if (pj!=starting[t])
	  p(" ");
	p("%c:%u@%u", 'c'+j->pool, j->session, j->iops);
      }
      P(")");
    }
  }
  P();

  // Now finally, we output the series of commands:
  P("########################################################################");
  P("# Start the random workloads:");
  P("echo -n \"Time = 0 (at starting) is \"; date");
  unsigned last_time = 0;
  for (unsigned t=0; t<exp_duration; ++t) {
    if (starting[t] != NULL) {
      if (t>last_time) {
	P("sleep %u", t - last_time);
	P("echo -n \"It shall now be time = %u \"; date", t);
	last_time = t;
      }
      for (job** pj=starting[t]; *pj!=NULL; ++pj) {
	job* j = *pj;
	if (use_iocounts) {	  
	  unsigned iocount = j->duration * j->iops;
	  if (!use_disk)
  	    P("( $Setparms %s session %u 10000; \\",
	      zyg(j->pool+2, j->session), j->iops);
	  P("  $Pharos -t results/%s.%c.%u.%u %s r -%u %c >> $Pharlog%s",
	    exp(), 'c'+j->pool, j->session, t, zyg(j->pool+2, j->session),
	    iocount, 'c'+j->pool,
	    use_disk ? " &" : "; \\");
	  if (!use_disk)
	    P("  $Setparms %s session 0 10000 ) &",
	      zyg(j->pool+2, j->session));
	}
	else {
	  if (!use_disk)
	    P("( $Setparms %s session %u 10000; \\",
	      zyg(j->pool+2, j->session), j->iops);
	  P("  $Pharos -t results/%s.%c.%u.%u %s r %u %c >> $Pharlog%s",
	    exp(), 'c'+j->pool, j->session, t, zyg(j->pool+2, j->session),
	    j->duration, 'c'+j->pool,
	    use_disk ? " &" : "; \\");
	  if (!use_disk)
	    P("  $Setparms %s session 0 10000 ) &",
	      zyg(j->pool+2, j->session));
	  else
	    P("  &");
	}
      }
    }
  }
  P();

  P("########################################################################");
  P("# Done");
  P("wait");
  if (!use_disk) {
    P("rmmod zygaria");
  }

  fclose(f);

  // Change the resulting script to be executable:
  struct stat sb;
  int status = stat(expfile(), &sb);
  if (status != 0)
    printx("Unable to stat %s: %s\n", expfile(), strerror(errno));

  status = chmod(expfile(), sb.st_mode | S_IXUSR | S_IXGRP | S_IXOTH);
  if (status != 0)
    printx("Unable to chmod %s: %s\n", expfile(), strerror(errno));

  return 0;
}
