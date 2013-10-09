#ifndef _STATS_H_INCLUDED_
#define _STATS_H_INCLUDED_ 1

// Comment here

// =====================================================================
// This is an abstract Base Class
class Stats
{
public:
  // Default constructor makes a Stats that can't be added to after
  // it has been reported.
  Stats();
  Stats(bool can_refill);
  Stats(Stats const& old);
  virtual ~Stats();
  Stats const& operator=(Stats const& rhs);

  void can_refill(bool v);

  virtual void add(double v) = 0;

  bool is_nonempty() const
  {
    return ( n > 0 );
  }

  // The following functions cause state switch from filling to reporting
  unsigned n_entries(); // TODO: Change name to get_ or report_
  // There will be many more reporting functions for concrete classes here.

protected:
  // Derived classes have to call this for each entry:
  void count_entry();

  // Derived classes have to call this when reporting
  void now_reporting();

  unsigned get_n() const
  {
    return n;
  }

private:
  bool refillable;
  bool reported; // False: We're filling; True: We're reporting
  unsigned n;

};

// =====================================================================
// Concrete: Does min/max, mean, variance
class StatsMMMV: public Stats
{
public:
  StatsMMMV();
  StatsMMMV(bool can_refill);
  StatsMMMV(StatsMMMV const& old);
  virtual ~StatsMMMV();
  StatsMMMV const& operator=(StatsMMMV const& rhs);

  virtual void add(double v);

  double get_min();
  double get_max();
  double get_mean();
  double get_var();

private:
  double tot;
  double totsq;
  double min;
  double max;
};


#endif
