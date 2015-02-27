/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sched.h>
#include <iostream>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <concurrency/ThreadPool.h>

using namespace std;

namespace fds {
class TestUnitBase;
class TestSuiteBase;
boost::random::mt19937       gl_seed_gen;

class TestArgBase
{
  public:
    explicit TestArgBase(int loop) : _targ_loop(loop), _targ_num_cpus(16) {}
    ~TestArgBase() {}

  private:
    friend class             TestUnitBase;
    int                      _targ_loop;
    int                      _targ_num_cpus;
};

class TestUnitBase
{
  public:
    TestUnitBase(TestArgBase &args, const char *name);
    virtual ~TestUnitBase();

    void tu_rec_loop(void);
    virtual void tu_exec(void) = 0;
    virtual void tu_report(void);
    virtual int  tu_rand_loop(void);

  private:
    friend class             TestSuiteBase;
    TestArgBase             &_tu_args;
    TestSuiteBase           *_tu_suite;
    const char              *_tu_name;
    int                     *_tu_cpu_loop;
    int                      _tu_sum_loop;
};

/** \TestUnitBase
 * --------------
 *
 */
TestUnitBase::TestUnitBase(TestArgBase &args, const char *name)
    : _tu_args(args), _tu_suite(0), _tu_name(name)
{
    int num_cpus = _tu_args._targ_num_cpus;

    _tu_cpu_loop = new int [num_cpus];
    memset(_tu_cpu_loop, 0, num_cpus * sizeof(int));
}

/** \~TestUnitBase
 * ---------------
 *
 */
TestUnitBase::~TestUnitBase()
{
    delete [] _tu_cpu_loop;
}

/** \tu_rec_loop
 * -------------
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

/** \tu_rand_loop
 * --------------
 * Return random number [0, loop] specified by the test argument.
 */
int
TestUnitBase::tu_rand_loop(void)
{
    boost::random::uniform_int_distribution<> dist(0, _tu_args._targ_loop);
    return dist(gl_seed_gen);
}

/** \tu_report
 * -----------
 *
 */
void
TestUnitBase::tu_report(void)
{
    cout << _tu_name << ": all loops: " << _tu_sum_loop << endl;
    for (int i = 0; i < _tu_args._targ_num_cpus; i++) {
        if (_tu_cpu_loop[i] != 0) {
            cout << "\tCPU " << i << " loops: " << _tu_cpu_loop[i] << endl;
        }
    }
}

/** \testunit_threadpool_fn
 * ------------------------
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
};

/** \TestSuiteBase
 * ---------------
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
 * ----------------
 *
 */
TestSuiteBase::~TestSuiteBase()
{
    sleep(1);
    delete _ts_pool;

    for (int i = 0; i < _ts_test_cnt; i++) {
        if (_ts_tests[i] != 0) {
            delete _ts_tests[i];
        }
    }
    delete [] _ts_tests;
}

/** \tsuit_add_test
 * ----------------
 *
 */
void
TestSuiteBase::tsuit_add_test(TestUnitBase *test)
{
    for (int i = 0; i < _ts_test_cnt; i++) {
        if (_ts_tests[i] == 0) {
            _ts_tests[i]    = test;
            test->_tu_suite = this;
            break;
        }
    }
}

/** \tsuit_run_tests
 * -----------------
 *
 */
void
TestSuiteBase::tsuit_run_tests(int loop)
{
    int           i, idx;
    TestUnitBase *test;
    boost::random::uniform_int_distribution<> dist(0, 100);

    for (i = 0; i < loop; i++) {
        idx  = dist(gl_seed_gen) % _ts_test_cnt;
        test = _ts_tests[idx];

        _ts_pool->schedule(testunit_threadpool_fn, test);
    }
}

/** \tsuit_report
 * --------------
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
    TPoolCpu(TPoolTestData &data) : TestUnitBase(data, "CPU Test") {}
    ~TPoolCpu() {}
    void tu_exec();

  private:
    boost::random::mt19937   _rand_gen;
};

/** \TPoolCpu::tu_exec
 * -------------------
 */
void
TPoolCpu::tu_exec(void)
{
    int i, n, loop;
    boost::random::uniform_int_distribution<> dist(1, 1 << 30);

    loop = tu_rand_loop();
    for (i = 0; i < loop; i++) {
        TestUnitBase::tu_rec_loop();

        /* Just burn CPU cycle to generate random number. */
        // n = dist(gl_seed_gen);
        n = random();

        /* TODO: We should excercise boost API here. */
    }
}

class TPoolSysCall : public TestUnitBase
{
  public:
    TPoolSysCall(TPoolTestData &data) : TestUnitBase(data, "SysCall Test") {}
    ~TPoolSysCall() {}

    void tu_exec();
};

/** \TPoolSysCall::tu_exec
 * -----------------------
 */
void
TPoolSysCall::tu_exec(void)
{
    int i, fd, loop;

    loop = tu_rand_loop();
    for (int i = 0; i < loop; i++) {
        TestUnitBase::tu_rec_loop();

        fd = open("/dev/null", O_DIRECT);
        close(fd);
    }
}

class TPoolEnqueue : public TestUnitBase
{
  public:
    TPoolEnqueue(TPoolTestData &data) : TestUnitBase(data, "Queue Test") {}
    ~TPoolEnqueue() {}

    void tu_exec();
};

/** \TPoolEnqueue::tu_exec
 * -----------------------
 */
void
TPoolEnqueue::tu_exec(void)
{
}

class TPoolConcurency : public TestUnitBase
{
  public:
    TPoolConcurency(TPoolTestData &data) : TestUnitBase(data, "Concurency") {}
    ~TPoolConcurency() {}

    void tu_exec();
};

/** \TPoolConcurency::tu_exec
 * --------------------------
 */
void
TPoolConcurency::tu_exec(void)
{
}


}  // namespace fds

int
main(int argc, char* argv[])
{
    fds::TPoolTestData    tp_data(2000);
    fds::TestSuiteBase    tp_test(100, 4);

    tp_test.tsuit_add_test(new fds::TPoolCpu(tp_data));
    tp_test.tsuit_add_test(new fds::TPoolSysCall(tp_data));
    tp_test.tsuit_add_test(new fds::TPoolEnqueue(tp_data));
    tp_test.tsuit_add_test(new fds::TPoolConcurency(tp_data));

    tp_test.tsuit_run_tests(1000);
    tp_test.tsuit_report();
    sleep(200);
    return 0;
}
