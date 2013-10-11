#include "Random.H"

Random::Random():
  base(BaseRandom::instance())
{
  // TODO: Debug message here
}

Random::~Random()
{}

i32 Random::doubleToI32()
{
  return (i32) getDouble();
}

u32 Random::doubleToU32()
{
  return (u32) getDouble();
}

i64 Random::doubleToI64()
{
  return (i64) getDouble();
}

u64 Random::doubleToU64()
{
  return (u64) getDouble();
}

double Random::i32ToDouble()
{
  return (double) getI32();
}

double Random::u32ToDouble()
{
  return (double) getU32();
}

double Random::i64ToDouble()
{
  return (double) getI64();
}

double Random::u64ToDouble()
{
  return (double) getU64();
}

