/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_FDSRANDOM_H_
#define SOURCE_INCLUDE_FDSRANDOM_H_

#include <random>
#include <fds_types.h>

namespace fds {

class RandNumGenerator {
  private:
    std::random_device rd;
    typedef std::linear_congruential_engine<
        fds_uint64_t, 3512401965023503517, 0, 9223372036854775807> lce;
    typedef std::mt19937_64 mte;

    mte generator;

  public:
    explicit RandNumGenerator(fds_uint64_t seed);
    ~RandNumGenerator();
    typedef boost::shared_ptr<RandNumGenerator> ptr;
    typedef boost::shared_ptr<const RandNumGenerator> const_ptr;

    /// Returns a random number to use for a seed
    /// Since this is slow, it should NOT be used to get
    /// random numbers
    static fds_uint64_t getRandSeed();

    fds_uint64_t genNum();
};

}  // namespace fds

#endif  // SOURCE_INCLUDE_FDSRANDOM_H_
