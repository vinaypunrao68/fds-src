#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "ActuatorAsync.H"
#include "Experiment.H"

#define TESTFILE "testfile"
#define TESTTRACE "testtrace"

int main(int /*argc*/, char* /*argv*/[])
{
  printf("\n***** TestActuator starting.\n");

  clk()->initialize();
  printf("Timer initialized.\n");

  int status = system("dd if=/dev/zero of=" TESTFILE " bs=4096 count=4096");
  assert(status==0);
  printf("Test file %s created at %.6f.\n", TESTFILE, now());

  // TODO: We need to initialize the experiment name, even if we don't have
  // one.  Fix that in Experiment.C
  (void) experiment("");
  
  // The store is readonly, as we will do only reads
  Store s(TESTFILE, true, 0);
  s.initialize();

  ActuatorAsync a(10, s, TESTTRACE);
  printf("Actuator initialized.\n");
  a.initialize(1024);

  printf("Please ignore warning from IOStat: "
	 "zygaria stats not found in buffer.\n");

  for (location_t l=0; l<4096*4096; l += 1024) {
    IO io(IO::Now, l, 1024, IO::Read);
    IOStat ios(io);
    a.doIO(ios);
  }
  printf("All IOs done at %.6lf.\n", now());

  a.close();
  s.close();
  printf("Everything closed\n");

  printf("***** TestActuator done.\n");
}
