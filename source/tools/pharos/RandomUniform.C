#include "RandomUniform.H"

RandomUniform::RandomUniform(double l_in, double u_in):
  l(l_in),
  u(u_in),
  u_minus_l(u_in-l_in)
{
  // TODO: Debug output here
  // TODO: assert(u_minus_l > 0);
}

RandomUniform::~RandomUniform()
{}

double RandomUniform::getDouble()
{
  return l + base->getDouble() * u_minus_l;
}

