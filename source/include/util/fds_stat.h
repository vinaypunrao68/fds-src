/*
 * Copyright 2013 by Formation Data Systems, Inc.
 */
#ifndef INCLUDE_UTIL_FDS_STAT_H_
#define INCLUDE_UTIL_FDS_STAT_H_

#include <time.h>
#include <fds_module.h>
#include <shared/fds_types.h>

namespace fds {

typedef enum
{
    STAT_NGX                 = 0,
    STAT_FDSN                = 1,
    STAT_MAX_MODS            = 2
} stat_mod_e;

const int STAT_HISTGRAM      = 32;
const int CLOCKS_PER_USEC    = (CLOCKS_PER_SEC / 1000000);

typedef struct stat_decode stat_decode_t;
struct stat_decode
{
    int                      stat_point;
    const char              *stat_name;
};

class StatRec
{
  public:
    StatRec();
    ~StatRec();

    int                      stat_hist_cnt;
    int                      stat_hist_idx;
    int                      stat_reqs;
    int                      stat_cpu_switch;
    int                     *stat_req_hist;
    int                      stat_histgram[STAT_HISTGRAM];
    fds_uint64_t             stat_prev_period;
};

class StatMod
{
  public:
    StatMod();
    ~StatMod();

    int                      stat_pts;
    int                      stat_hist_us;
    StatRec                 *stat_rec;
    const stat_decode_t     *stat_decode;
};

class StatModule : public Module
{
  public:
    fds_bool_t               stat_on;

    StatModule(char const *const name);
    virtual ~StatModule();

    virtual int  mod_init(SysParams const *const param);
    virtual void mod_startup();
    virtual void mod_shutdown();

    int  stat_out(stat_mod_e mod, char *buf, int len);
    void stat_reg_mod(stat_mod_e mod, const stat_decode_t decode[]);

    void stat_set_hist(stat_mod_e mod, int seclong);
    void stat_sampling();
    void stat_record(stat_mod_e mod, int pt, fds_uint64_t start, fds_uint64_t);

    inline void stat_enable(fds_bool_t enable) {
        stat_on = enable;
    }
    inline int stat_tck_per_us() {
        return stat_cpu_mhz;
    }
  private:
    int                      stat_cpu_mhz;
    StatMod                  stat_mod[STAT_MAX_MODS];

    int  stat_rec_out(StatMod *stat, int point, char **buf, int *len);
    void stat_sampling_mod(stat_mod_e mod);
};

extern StatModule gl_fds_stat;

/*
 * fds_tck_per_us
 * --------------
 * Return number of ticks per micro second.
 */
static inline int fds_tck_per_us()
{
    return gl_fds_stat.stat_tck_per_us();
}

/*
 * fds_rdtsc
 * ---------
 * Return the current CPU's ticks.  Need to move this to arch dep header file.
 */
static inline volatile fds_uint64_t fds_rdtsc()
{
//    return (fds_uint64_t)clock();
#ifdef __i386__
    register fds_uint64_t tsc asm("eax");
    asm volatile(".byte 15, 49" : : : "eax", "edx");
    return tsc;
#else
    register fds_uint64_t hi, lo;

    asm volatile("cpuid; rdtscp" : "=a"(lo), "=d"(hi));
    return ((hi << 32) | lo);
#endif
}

/*
 * fds_stat_record
 * ---------------
 * Record the stat at the module and recording point.
 */
static inline void
fds_stat_record(stat_mod_e mod, int point, fds_uint64_t start, fds_uint64_t end)
{
    if (gl_fds_stat.stat_on) {
        gl_fds_stat.stat_record(mod, point, start, end);
    }
}

} // namespace fds

#endif /* INCLUDE_UTIL_FDS_STAT_H_ */
