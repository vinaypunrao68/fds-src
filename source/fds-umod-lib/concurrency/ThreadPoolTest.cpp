/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>

#include <sched.h>
#include <iostream>  // NOLINT(*)
#include <ThreadPool.h>

namespace fds {
class TestUnitBase;

class TestArgBase
{
  public:
    explicit TestArgBase(int loop)
        : _targ_loop(loop), _targ_num_cpus(16) {}
    ~TestArgBase() {}

  private:
    friend class TestUnitBase;
    int                      _targ_num_cpus;
    int                      _targ_loop;
};

class TestUnitBase
{
  public:
    explicit TestUnitBase(TestArgBase &args);
    ~TestUnitBase();

    void tu_rec_loop(void);
    virtual void tu_exec(void) = 0;
    virtual void tu_report(void);

  private:
    TestArgBase             &_tu_args;
    int                     *_tu_cpu_loop;
    int                      _tu_sum_loop;
};

/** \TestUnitBase
 *
 */
TestUnitBase::TestUnitBase(TestArgBase &args)
    : _tu_args(args)
{
    int num_cpus = _tu_args._targ_num_cpus;

    _tu_cpu_loop = new int [num_cpus];
    memset(_tu_cpu_loop, 0, num_cpus * sizeof(int));
}

/** \~TestUnitBase
 *
 */
TestUnitBase::~TestUnitBase()
{
    delete [] _tu_cpu_loop;
}

/** \tu_rec_loop
 *
 */
void
TestUnitBase::tu_rec_loop(void)
{
    int cpu = sched_getcpu();

    // assert here.
    _tu_sum_loop++;
    _tu_cpu_loop[cpu]++;
}

/** \tu_report
 *
 */
void
TestUnitBase::tu_report(void)
{
}

/** \testunit_threadpool_fn
 *
 */
static void
testunit_threadpool_fn(TestUnitBase *test)
{
    test->tu_exec();
    test->tu_rec_loop();
}

/* -------------------------------------------------------------------------- */
class TestSuiteBase
{
  public:
    TestSuiteBase(int thpool_cnt, int test_cnt);
    ~TestSuiteBase();

    void tsuit_add_test(TestUnitBase *test);
    virtual void tsuit_run_tests(int loop);
    virtual void tsuit_report(void);

  private:
    int                      _ts_test_cnt;
    TestUnitBase           **_ts_tests;
    fds::fds_threadpool     *_ts_pool;
    boost::random::mt19937   _ts_gen;
};

/** \TestSuiteBase
 *
 */
TestSuiteBase::TestSuiteBase(int thpool_cnt, int test_cnt)
    : _ts_test_cnt(test_cnt)
{
    int i;

    _ts_tests = new TestUnitBase * [test_cnt];
    for (i = 0; i < test_cnt; i++) {
        _ts_tests[i] = 0;
    }
    _ts_pool = new fds::fds_threadpool(thpool_cnt);
}

/** \~TestSuiteBase
 *
 */
TestSuiteBase::~TestSuiteBase()
{
    _ts_pool->thp_join();
    for (int i = 0; i < _ts_test_cnt; i++) {
        if (_ts_tests[i] != 0) {
            delete _ts_tests[i];
        }
    }
    delete [] _ts_tests;
    delete _ts_pool;
}

/** \tsuit_add_test
 *
 */
void
TestSuiteBase::tsuit_add_test(TestUnitBase *test)
{
    for (int i = 0; i < _ts_test_cnt; i++) {
        if (_ts_tests[i] == 0) {
            _ts_tests[i] = test;
            break;
        }
    }
}

/** \tsuit_run_tests
 *
 */
void
TestSuiteBase::tsuit_run_tests(int loop)
{
    int           i, idx;
    TestUnitBase *test;

    boost::random::uniform_int_distribution<> dist(0, 100);
    for (i = 0; i < loop; i++) {
        idx  = dist(_ts_gen) % _ts_test_cnt;
        test = _ts_tests[idx];

        _ts_pool->schedule(testunit_threadpool_fn, test);
    }
    _ts_pool->thp_join();
}

/** \tsuit_report
 *
 */
void
TestSuiteBase::tsuit_report(void)
{
    int i;

    for (i = 0; i < _ts_test_cnt; i++) {
        if (_ts_tests[i] != NULL) {
            _ts_tests[i]->tu_report();
        }
    }
}

/* -------------------------------------------------------------------------- */
class TPoolTestData : public TestArgBase
{
  public:
    explicit TPoolTestData(int loop) : TestArgBase(loop) {
        tp_shared = 0;
    }
    ~TPoolTestData() {}

    int                      tp_shared;
};

class TPoolCpu : public TestUnitBase
{
  public:
    TPoolCpu(TPoolTestData &data) : TestUnitBase(data) {}
    ~TPoolCpu() {}

    void tu_exec() {}
};

class TPoolSysCall : public TestUnitBase
{
  public:
    TPoolSysCall(TPoolTestData &data) : TestUnitBase(data) {}
    ~TPoolSysCall() {}

    void tu_exec() {}
};

class TPoolEnqueue : public TestUnitBase
{
  public:
    TPoolEnqueue(TPoolTestData &data) : TestUnitBase(data) {}
    ~TPoolEnqueue() {}

    void tu_exec() {}
};

class TPoolConcurency : public TestUnitBase
{
  public:
    TPoolConcurency(TPoolTestData &data) : TestUnitBase(data) {}
    ~TPoolConcurency() {}

    void tu_exec() {}
};

}  // namespace fds

int
main(int argc, char* argv[])
{
    fds::TPoolTestData    tp_data(1000);
    fds::TestSuiteBase    tp_test(100, 4);

    tp_test.tsuit_add_test(new fds::TPoolCpu(tp_data));
    tp_test.tsuit_add_test(new fds::TPoolSysCall(tp_data));
    tp_test.tsuit_add_test(new fds::TPoolEnqueue(tp_data));
    tp_test.tsuit_add_test(new fds::TPoolConcurency(tp_data));

    tp_test.tsuit_run_tests(1000);
    tp_test.tsuit_report();
    return 0;
}
