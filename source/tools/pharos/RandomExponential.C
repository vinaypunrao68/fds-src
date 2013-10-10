#include <math.h>

#include "RandomExponential.H"

RandomExponential::RandomExponential(double mean_in):
  mean(mean_in)
{
  // TODO: Debug output here
  assert(mean > 0.0);
}

RandomExponential::~RandomExponential()
{}

double RandomExponential::getDouble()
{
  return -mean * log(1. - base->getDouble() );
}

