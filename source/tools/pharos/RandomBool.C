#include "RandomBool.H"

RandomBool::RandomBool(double p_in):
  p(p_in),
  base(BaseRandom::instance())
{
  // TODO: Debug message here
}
RandomBool::~RandomBool()
{}

bool RandomBool::getBool()
{
  if (p <= 0.0)
    return false;
  if (p >= 1.0)
    return true;

  return (base->getDouble() < p);
}
