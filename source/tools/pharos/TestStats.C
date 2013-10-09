// Test program for the StatsMMMV class

#include <stdio.h>

#include "Stats.H"

int main(int /* argc */, char* /* argv */ [])
{
  printf("\n***** TestStats starting.\n");

  printf("Test 1: Stats on 1...5:\n");
  StatsMMMV s1 = StatsMMMV();
  for (double x = 1.0; x < 5.5; x += 1.0)
    s1.add(x);
  printf("    N    (should be 5.0): %u\n", s1.n_entries());
  printf("    Mean (should be 3.0): %f\n", s1.get_mean());
  printf("    Min  (should be 1.0): %f\n", s1.get_min());
  printf("    Max  (should be 5.0): %f\n", s1.get_max());

  printf("Test 2: Stats on -1000...1000:\n");
  StatsMMMV s2 = StatsMMMV();
  for (double x = -1000.0; x < 1000.5; x += 1.0)
    s2.add(x);
  printf("    N    (should be 2001): %u\n",    s2.n_entries());
  printf("    Mean (should be     0.0): %f\n", s2.get_mean());
  printf("    Min  (should be -1000.0): %f\n", s2.get_min());
  printf("    Max  (should be     1.0): %f\n", s2.get_max());

  printf("Copy and assign Stats object, both while filling and after:\n");
  StatsMMMV s3   = StatsMMMV();
  StatsMMMV s3aa = StatsMMMV();
  StatsMMMV s3ab = StatsMMMV();
  double x;
  for (x = -1000.0; x < 0.0; x += 1.0)
    s3.add(x);
  StatsMMMV s3ca = s3;
  s3aa = s3;
  for (; x < 1000.5; x += 1.0) {
    s3.add(x);
    s3aa.add(x);
    s3ca.add(x);
  }
  printf("    The following 5 lines shall be the same:\n");
  printf("    Original: N=%u Min=%f Max=%f Mean=%f\n",
	 s3.n_entries(), s3.get_min(), s3.get_max(), s3.get_mean());

  StatsMMMV s3cb = s3;
  s3ab = s3;
  printf("    Copy bef: N=%u Min=%f Max=%f Mean=%f\n",
	 s3cb.n_entries(), s3cb.get_min(), s3cb.get_max(), s3cb.get_mean());
  printf("    Copy aft: N=%u Min=%f Max=%f Mean=%f\n",
	 s3ca.n_entries(), s3ca.get_min(), s3ca.get_max(), s3ca.get_mean());
  printf("    Assign b: N=%u Min=%f Max=%f Mean=%f\n",
	 s3ab.n_entries(), s3ab.get_min(), s3ab.get_max(), s3ab.get_mean());
  printf("    Assign a: N=%u Min=%f Max=%f Mean=%f\n",
	 s3aa.n_entries(), s3aa.get_min(), s3aa.get_max(), s3aa.get_mean());

  printf("Test 4: Empty stats:\n");
  StatsMMMV s4 = StatsMMMV();
  printf("    N    (should be 0): %u\n", s4.n_entries());
  printf("    Mean (should be 0.0): %f\n", s4.get_mean());
  printf("    Min  (should be 0.0): %f\n", s4.get_min());
  printf("    Max  (should be 0.0): %f\n", s4.get_max());

  printf("Refillable Statistics:\n");
  StatsMMMV s5 = StatsMMMV(true);
  for (double x = 1.0; x < 5.5; x += 1.0)
    s5.add(x);
  printf("    Part 1:\n");
  printf("    N    (should be 5): %u\n", s5.n_entries());
  printf("    Mean (should be 3.0): %f\n", s5.get_mean());
  printf("    Min  (should be 1.0): %f\n", s5.get_min());
  printf("    Max  (should be 5.0): %f\n", s5.get_max());
  for (double x = -1.0; x > -5.5; x -= 1.0)
    s5.add(x);
  printf("    Part 2:\n");
  printf("    N    (should be 10): %u\n", s5.n_entries());
  printf("    Mean (should be 0.0): %f\n", s5.get_mean());
  printf("    Min  (should be -5.0): %f\n", s5.get_min());
  printf("    Max  (should be  5.0): %f\n", s5.get_max());

  printf("Non-refillable Statistics:\n");
  StatsMMMV s6 = StatsMMMV();
  s6.add(1.0);
  (void) s6.get_mean();
  printf("    You should get an error message now:\n");
  s6.add(1.0);

  printf("***** TestStats done.\n");
}
