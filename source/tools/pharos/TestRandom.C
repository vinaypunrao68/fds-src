// Tester for random number generators
#include <stdio.h>

#include "BaseRandom48.H"
#include "RandomBool.H"
#include "RandomUniform.H"

int main(int /*argc*/, char* /*argv*/[])
{
  printf("\n***** TestRandom starting.\n");

  BaseRandom* base = BaseRandom48::initialize();
  printf("TestRandom: Made base generator.\n");

  unsigned n_base = 20;
  printf("%u random int's and double's from the base generator:\n", n_base);
  i64    tot_ri = 0;
  double tot_rd = 0.0;
  for (unsigned i=0; i<n_base; ++i)
  {
    i32    ri = base->getI32();
    tot_ri += ri;
    double rd = base->getDouble();
    tot_rd += rd;
    printf("Random int=%10d double=%f\n", ri, rd);
  }
  printf("Mean int: %lld, mean double: %f.\n\n",
         tot_ri/n_base, tot_rd/(double)n_base);
  printf("Should be ~%u, ~0.5.\n", (unsigned) 1024*1024*1024*2);

  unsigned n_bool = 78;
  printf("%u random booleans with 33%% probability:\n", n_bool);
  RandomBool b(1. / 3.);
  unsigned c = 0;
  for (unsigned i=0; i<n_bool; ++i)
  {
    bool y = b.getBool();
    if (y)
      ++c;
    printf(y ? "T" : "F");
  }
  printf("\n");
  printf("%d (=%f%%) of the booleans were true.\n\n",
         c,
         100.*c/(double) n_bool);

  unsigned n_unid = 100;
  printf("%u uniform random doubles between 1.0 and 3.0:\n", n_unid);
  RandomUniform ud(1.0, 3.0);
  double tot_rud = 0.0;
  for (unsigned i=0; i<n_unid; ++i)
  {
    double rud = ud.getDouble();
    tot_rud += rud;
    printf("%f ", rud);
    if (i%8==7 || i==n_unid-1)
      printf("\n");
  }
  printf("Mean is %f\n", tot_rud / (double) n_unid);
  printf("\n");

  unsigned n_uniu = 200;
  printf("%u uniform random unsigneds between 100 and 300:\n", n_uniu);
  RandomUniform uu(100, 300);
  u32 tot_ruu = 0;
  for (unsigned i=0; i<n_uniu; ++i)
  {
    u32 ruu = uu.getU32();
    tot_ruu += ruu;
    printf("%3u ", ruu);
    if (i%18==17 || i==n_uniu-1)
      printf("\n");
  }
  printf("Mean is %f\n", (double) tot_ruu / (double) n_uniu);
  printf("\n");

  unsigned n_unil = 100;
  printf("%u uniform random longlongs between "
         "-1000000000000 and 1000000000000:\n", n_unil);
  RandomUniform ul(-1.e12, 1.e12);
  i64 tot_rul = 0;
  for (unsigned i=0; i<n_unil; ++i)
  {
    i64 rul = ul.getI64();
    tot_rul += rul;
    printf("%13Ld ", rul);
    if (i%5==4 || i==n_unil-1)
      printf("\n");
  }
  printf("Mean is %f\n", (double) tot_rul / (double) n_unil);
  printf("\n");

  printf("***** TestRandom done.\n");
  return 0;
}
