#include <assert.h>

#include "GeneratorReplayTrace.H"
#include "Timer.H"
#include <stdio.h>
#include <string.h>

GeneratorReplayTrace::GeneratorReplayTrace(const char* tracefname, location_t in_store_capacity):
store_capacity(in_store_capacity)
{	
	char fline[200]; 
    unsigned int disk_id;
	char operation[2];
	unsigned long long offset;
	int block_size;
	long io_size;
	float enqueue_time;
	long dispatch_time, completion_time;
	float sec_completed;
	int i = 0;
	double trace_start_time = 0;
	bool bFirstIO = true;
	bool bRead = true;
  
	
	largest_io_size = 0;
	
	FILE* tempFile;
	
	traceFile = fopen(tracefname, "r");
  	if (!traceFile) {
      	printf("Unable to open tracefile %s\n", tracefname);
      	return;
  	}
	
	while (fgets(fline, 200, traceFile) != NULL)
	{
		recCount++;
	}
	fseek(traceFile, 0, SEEK_SET);
	
	trace_buf = new trace_io_t[recCount];
	if (!trace_buf) {
		printf("Failed to allocate trace buffer\n");
		return;
	}
	
	i = 0;
	while (fgets(fline, 200, traceFile) != NULL)
	{
		sscanf(fline, "%d,%1s,%11llu,%5d,%6ld,%f,%6ld,%6ld,%f", 
			&disk_id, &operation, &offset, &block_size, &io_size, &enqueue_time, &dispatch_time, &completion_time, &sec_completed);
	
		if (bFirstIO)
		{
			trace_start_time = (double)enqueue_time;
			bFirstIO = false;
		}
		
		bRead = true;
		if ((strcmp(operation, "W") == 0) || (strcmp(operation, "w") == 0)) {
			bRead = false;
		}
	
		if (io_size > largest_io_size)
			largest_io_size = io_size;
	
		if (i < recCount) {
			trace_buf[i].offset = offset;
			trace_buf[i].io_size = io_size;
			trace_buf[i].bRead = bRead;
			trace_buf[i].start_time = (double)enqueue_time - trace_start_time;
		}
		i++;
	}
	
	printf("Largest IO size %ld Bytes\n", largest_io_size);
	
	next_io_index = 0;
	start_sec = 0.0;
}


void GeneratorReplayTrace::make(IO& io, double, double)
{
	location_t next;
	
	assert(next_io_index < recCount);
		
	io.setTime(start_sec+trace_buf[next_io_index].start_time);	
    io.setSize(trace_buf[next_io_index].io_size);
    
    if (trace_buf[next_io_index].bRead)
    	io.setDir(IO::Read);	
    else {
    	io.setDir(IO::Write);	
    }
      
    next = trace_buf[next_io_index].offset;
    if ((next + io.getSize()) > store_capacity) {
		printf("WARNING: offset > store_capacity, changing offset\n");
		next = store_capacity - io.getSize();
    }
    io.setLocation(next);
    
    next_io_index++;	

}
