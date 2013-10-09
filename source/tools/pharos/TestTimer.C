// Tester for the timer.
#include <stdio.h>

#include "Timer.H"

int main(int /*argc*/, char* /*argv*/[])
{
  printf("\n***** TestIO starting.\n");

  (void) clk();
  printf("Timer constructed.\n");

  clk()->initialize();
  printf("Timer initialized, time is now %.6f (should be ~0.0).\n",
	 now());

  double waittime = clk()->waitUntil(5.0);
  printf("Slept until 5.0, time is now %.6f (should be ~5.0), rc is %.6f (should be ~5.0).\n",
	 now(), waittime);

  clk()->sleep(5.0);
  printf("Slept for another 5.0, time is now %.6f (should be ~10.0).\n",
 now());

  printf("***** TestTimer done.\n");
}
