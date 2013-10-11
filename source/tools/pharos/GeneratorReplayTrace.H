#ifndef _PHAROS_GeneratorReplayTrace_H_INCLUDED_
#define _PHAROS_GeneratorReplayTrace_H_INCLUDED_ 1

#include "Generator.H"

typedef struct trace_io{
	location_t offset;
	size_t io_size;
	bool bRead;
	double start_time;
} trace_io_t;
 
class GeneratorReplayTrace: public Generator
{
public:
    GeneratorReplayTrace(const char* tracefname, location_t in_store_capacity);
    

  virtual ~GeneratorReplayTrace() {
  	if (traceFile) (void) fclose(traceFile);
  }

  // Copy constructor and assignment operator are compiler-generated.

  virtual void make(IO& io, double start_time, double end_time);

  virtual bool is_start_time_required() const { return false; }
  virtual bool is_end_time_required  () const { return false; }

  virtual size_t largest_io() const { return largest_io_size; }
  
  virtual void set_time_first_io(double start_time) 
  	{ 		 
  		start_sec = start_time; 
  	}
  	
  virtual int max_io_count() const 
  { return recCount; }   	

private:
  FILE* traceFile;
  trace_io_t* trace_buf;
  int recCount;

  double start_sec; 
  location_t store_capacity;
  
  size_t largest_io_size;
  int next_io_index;
  
};


#endif /*_PHAROS_GeneratorReplayTrace_H_INCLUDED_*/
