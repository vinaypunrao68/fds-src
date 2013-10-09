#include <assert.h>
#include "GeneratorSeqRun.H"

void GeneratorSeqRun::make(IO& io, double start_time, double end_time)
//void GeneratorSeqRun::orig_make(IO& io, double start_time, double end_time)
{
  location_t next;
  base.make(io, start_time, end_time);

  if (remaining_ios_in_run == 0) { 

  	remaining_ios_in_run = run_length;
   	next = random.getU64();
        next = next - next % ( io.getSize() * (unsigned long long)run_length);
        last = next;

	// sequential
	//next = last_finished;
	//last = next;

  
  }
  else{
	next = last_finished;
	last = next;
  	//last += io.getSize();
    	//next = last;

	//overwrite direction
	//if (remaining_ios_in_run <= numwrites)
	//	io.setDir(IO::Write);
  }
  remaining_ios_in_run--;
  assert(next + io.getSize() <= store_capacity);
  io.setLocation(next);
  last_finished = next + io.getSize();

  //printf("next %Lu last_finished %Lu\n", next, last_finished);	
}

// random run :) 
/*void GeneratorSeqRun::make(IO& io, double start_time, double end_time)
{
  location_t next;

  if (run_length == 20) {
	orig_make(io, start_time, end_time);
  	return;
  }

  base.make(io, start_time, end_time);

// temp -- hardcoded run

  if (remaining_ios_in_run == 0) { 

	if (run_length == 2)	
	  run_length = 3;
	else if (run_length == 3)
	  run_length = 2;	

  	remaining_ios_in_run = run_length;
	
	//sequential
	next = last_finished;
	last = next;  
  }
  else{
	// continuing random run
   	next = random.getU64();
    	next = next - next % ( io.getSize() * (unsigned long long)run_length);
    	last = next;
  }
  remaining_ios_in_run--;
  assert(next + io.getSize() <= store_capacity);
  io.setLocation(next);
  last_finished = next + io.getSize();

  //printf("next %Lu last_finished %Lu\n", next, last_finished);	
}
*/


//ANNA -- temp version
// generate % sequential pattern
/*void GeneratorSeqRun::make(IO& io, double start_time, double end_time)
{
  location_t next;
  double whatisnextio;
  base.make(io, start_time, end_time);

  // decide whether next IO should be sequential or random
  whatisnextio = rndFraction.getDouble();
  if (whatisnextio <= percentSeq) {
	// do sequential IO
	next = last_finished;
	last = next;
	seqNum++;
  }
  else {
	// do random IO
   	next = random.getU64();
    	next = next - next % ( io.getSize() * (unsigned long long)run_length); // reasonable for some sequential run
    	last = next;	
	rndNum++;
  }

  num++;
//  if ((num % 100) == 0) {
//	printf("seq %d rnd %d\n", seqNum, rndNum);
//	seqNum = 0;
//	rndNum = 0;
//  }


  assert(next + io.getSize() <= store_capacity);
  io.setLocation(next);
  last_finished = next + io.getSize();

  //printf("next %Lu last_finished %Lu\n", next, last_finished);	
}
*/
