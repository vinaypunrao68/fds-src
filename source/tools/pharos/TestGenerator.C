// Tester for the generators
#include <assert.h>
#include <stdio.h>

#include "IO.H"
#include "Store.H"
#include "Generator.H"
#include "GeneratorConstSize.H"
#include "GeneratorAFAP.H"
#include "GeneratorRandom.H"
#include "GeneratorSeq.H"
#include "BaseRandom48.H"

#define TESTFILE "testfile"
int main(int /*argc*/, char* /*argv*/[])
{
  printf("\n***** TestGenerator starting.\n");

  (void) BaseRandom48::initialize(false);

  int status = system("dd if=/dev/zero of=" TESTFILE " bs=4096 count=4096");
  assert(status==0);
  printf("Test file %s created.\n", TESTFILE);
  
  // The store is readonly, as we will generate only reads
  Store s(TESTFILE, true, 0);
  s.initialize();

  GeneratorAFAP g1;
  GeneratorConstSize g2(g1, 1024, IO::Read);
  GeneratorSeq    g3s(g2, s);
  GeneratorRandom g3r(g2, s);

  const unsigned nio = 10;

  printf("%u sequential IOs over a 16MB file:\n", nio);
  for (unsigned i=0; i<nio; ++i) {
    IO io;
    g3s.make(io, 0.0, 0.0);

    assert(io.areParamsSet());

    printf("  "); io.print(); printf("\n");
  }
    
  printf("%u random IOs over a 16MB file:\n", nio);
  for (unsigned i=0; i<nio; ++i) {
    IO io;
    g3r.make(io, 0.0, 0.0);

    assert(io.areParamsSet());

    printf("  "); io.print(); printf("  (location is 0x%6.6Lx)\n", io.getLocation());
  }
    
  printf("***** TestGenerator done.\n");
}
