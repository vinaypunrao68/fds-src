// Tester for the IO objects.
#include <stdio.h>

#include "IOStat.H"

int main(int /*argc*/, char* /*argv*/[])
{
  printf("\n***** TestIO starting.\n");

  printf("Size of location_t is %d, should be 8.\n", sizeof(location_t));
  if (sizeof(location_t) != 8)
      printf("*** You should really compile it differently.\n");

  clk()->initialize();
  printf("Timer initialized.\n");

  IO io1(IO::Now, 8192, 512, IO::Read);
  printf("IO1 constructed: ");
  io1.print();
  printf(" (should be 512@8192 now).\n");

  IO io2(321.0, 12345, 54321, IO::Write);
  printf("IO2 constructed: ");
  io2.print();
  printf("(should be 54321@12345 time 321).\n");

  IOStat io1s(io1);
  clk()->sleep(1.0);
  io1s.setStarted();
  printf("IO1 started at time %.6f (should be ~1.0).\n",
	 now());

  clk()->sleep(1.0);
  char* buffer[512];
  io1s.setKernel(buffer);
  printf("IO1 back from kernel shortly before %.6f (should be ~2.0).\n",
	 now());

  clk()->sleep(1.0);
  io1s.setStopped();
  printf("IO1 stopped shortly before %.6f.\n", now());

  printf("\n");
  printf("IO statistics:\n");
  printf("Usermode start:         %.6f\n", io1s.get_U_Start());
  printf("Kernelmode start:       %.6f\n", io1s.get_K_Start());
  printf("Device start:           %.6f\n", io1s.get_D_Start());
  printf("Device stop:            %.6f\n", io1s.get_D_Stop());
  printf("Usermode stop:          %.6f\n", io1s.get_U_Stop());

  printf("***** TestIO done.\n");
}
