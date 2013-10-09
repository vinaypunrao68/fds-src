#include <assert.h>
#include <string.h>

#include "Experiment.H"

/* static */
Experiment* it = 0;

Experiment::Experiment(char const* name_in):
  name(name_in)
{}

char const* Experiment::getName() const
{
  return name;
}

bool Experiment::isNamePresent() const
{
  return ( name != NULL && strlen(name) > 0 );
}

Experiment* experiment()
{
  assert(it);
  return it;
}

Experiment* experiment(char const* name)
{
  assert(it == 0);
  it = new Experiment(name);
  return it;
}

