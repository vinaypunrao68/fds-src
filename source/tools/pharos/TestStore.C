// Tester for the IO objects.
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "Store.H"
#include "Timer.H"

#define TESTFILE "testfile"
int main(int /*argc*/, char* /*argv*/[])
{
  printf("\n***** TestStore starting.\n");

  clk()->initialize();
  printf("Timer initialized.\n");

  int status = system("dd if=/dev/zero of=" TESTFILE " bs=4096 count=4096");
  assert(status==0);
  printf("Test file %s created at %.6f.\n", TESTFILE, now());
  
  // The store is readonly as we will generate only reads
  Store s(TESTFILE, true, 0);
  s.initialize();
  printf("Store created, capacity is %Lu (should be %u)\n",
	 s.getCapacity(), 4096*4096);
  s.close();
  printf("Store closed\n");

  printf("***** TestStore done.\n");
}
