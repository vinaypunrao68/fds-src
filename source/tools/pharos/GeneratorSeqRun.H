// Set the location to all random, uniformly distributed within the whole
// capacity of the store.  The IOs must have their size set beforehand.

#ifndef _PHAROS_GeneratorSeqRun_H_INCLUDED_
#define _PHAROS_GeneratorSeqRun_H_INCLUDED_ 1

#include "GeneratorStackable.H"
#include "Store.H"
#include "RandomUniform.H"

class GeneratorSeqRun: public GeneratorStackable
{
public:
  GeneratorSeqRun(Generator& base_in, Store const& store_in, unsigned seq_run_length):
    GeneratorStackable(base_in),
    store_capacity(store_in.getCapacity()),
    random(0, store_capacity),
    //run_length(1),
    //remaining_ios_in_run(1), 
    last(0), last_finished(0)
    //rndFraction(0,1)
  {initseqrun(seq_run_length);}

  virtual ~GeneratorSeqRun()
  {}

  // Copy constructor and assignment operator are compiler-generated.

  virtual void make(IO& io, double start_time, double end_time);
  //void orig_make(IO& io, double start_time, double end_time);

  virtual bool is_start_time_required() const
  { return base.is_start_time_required(); }
  virtual bool is_end_time_required  () const
  { return base.is_end_time_required(); }
  virtual size_t largest_io() const
  { return base.largest_io(); }
  virtual void set_time_first_io(double start_time) 
  { base.set_time_first_io(start_time); }
  
  virtual int max_io_count() const 
  { return base.max_io_count(); } 

private:
  location_t store_capacity;
  RandomUniform random;
  unsigned run_length;
  unsigned remaining_ios_in_run; 
  location_t last;
  location_t last_finished;

  int numwrites;

  // temp -- ANNA
//  double percentSeq;
//  RandomUniform rndFraction;
//  int seqNum;
//  int rndNum;
//  int num;

  void initseqrun(unsigned rl){
	numwrites = rl;
 	run_length = rl; //10
  	remaining_ios_in_run = 0;

	// ANNA -- runlength here means % of sequential (1...100)
	/*percentSeq = (double)(rl+3) / (double)100;
	seqNum = 0;
	rndNum = 0;
	num = 0;*/
  }


};

#endif // _PHAROS_GeneratorRandom_H_INCLUDED_

