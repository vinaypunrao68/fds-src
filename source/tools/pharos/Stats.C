#include <stdio.h> // TODO: Temporary

#include "Stats.H"
#include "printe.H"

// =====================================================================
Stats::Stats():
  refillable(false),
  reported(false),
  n(0)
{}

Stats::Stats(bool can_refill):
  refillable(can_refill),
  reported(false),
  n(0)
{}

Stats::Stats(Stats const& old):
  refillable (old.refillable),
  reported   (old.reported),
  n          (old.n)
{}

Stats const& Stats::operator=(Stats const& rhs)
{
  if (this != &rhs) {
    refillable = rhs.refillable;
    reported   = rhs.reported;
    n          = rhs.n;
  }
  return *this;
}

Stats::~Stats()
{}

// bool Stats::is_nonempty() const
// Implemented inline in the header file

unsigned Stats::n_entries()
{
  reported = true;
  return n;
}

void Stats::count_entry()
{
  if (reported && !refillable) {
    // TODO: Assert or throw here
    printe("Stats counting after reporting on non-refill.\n");
  }

  ++n;
}

void Stats::now_reporting()
{
  reported = true;
}

// =====================================================================
StatsMMMV::StatsMMMV():
  Stats(),
  tot(0.0),
  totsq(0.0),
  min( 99999999.), // TODO: Something better here
  max(-99999999.)
{}

StatsMMMV::StatsMMMV(bool can_refill):
  Stats(can_refill),
  tot(0.0),
  totsq(0.0),
  min( 99999999.), // TODO: Something better here
  max(-99999999.)
{}

StatsMMMV::StatsMMMV(StatsMMMV const& old):
  Stats(old),
  tot  (old.tot),
  totsq(old.totsq),
  min  (old.min),
  max  (old.max)
{}

StatsMMMV const& StatsMMMV::operator=(StatsMMMV const& rhs)
{
  if (this != &rhs) {
    Stats::operator=(rhs);
    tot   = rhs.tot;
    totsq = rhs.totsq;
    min   = rhs.min;
    max   = rhs.max;
  }
  return *this;
}

StatsMMMV::~StatsMMMV()
{}

void StatsMMMV::add(double v)
{
  count_entry();

  tot += v;
  totsq += v*v;
  if (v < min) min = v;
  if (v > max) max = v;
}

double StatsMMMV::get_min()
{
  now_reporting();
  return is_nonempty() ? min : 0.0;
}

double StatsMMMV::get_max()
{
  now_reporting();
  return is_nonempty() ? max : 0.0;
}

double StatsMMMV::get_mean()
{
  now_reporting();
  return is_nonempty() ? tot / double(get_n()) : 0.0;
}

double StatsMMMV::get_var()
{
  now_reporting();
  return 0.0; // TODO: Add variance calculation here
}

