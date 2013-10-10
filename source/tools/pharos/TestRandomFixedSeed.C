// Tester for random number generators
// Makes sure that the setting of the seed works fine:
#include <stdio.h>
#include <assert.h>

#include "BaseRandom48.H"

int main(int /*argc*/, char* /*argv*/[])
{
  const unsigned n = 20;

  // This should be the correct answer:
  int results[n] = {
    -1174814498,
    -1093310885,
    -2001964313,
    1459408524,
    1093215683,
    -595981729,
    -138256187,
    -1628585113,
    -965768777,
    9183618,
    1035959202,
    828944673,
    -1572338055,
    1974625394,
    -414565882,
    -1260931050,
    -875684983,
    1269110876,
    -1346075421,
    1592554004 };

  printf("\n***** TestRandomFixedSeed starting.\n");

  int seed = 19610826; // Ralph's birthday
  BaseRandom* base = BaseRandom48::initialize(seed);
  printf("TestRandom: Made base generator with seed %d.\n", seed);

  printf("%u random int's from the base generator:\n", n);
  for (unsigned i=0; i<n; ++i)
  {
    i32 ri = base->getI32();
    printf("Random int=%10d=%10d\n", ri, results[i]);
    assert(ri == results[i]);
  }
  printf("***** TestRandom done.\n");
  return 0;
}
