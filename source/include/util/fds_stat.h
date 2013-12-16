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
    int                     *stat_req_hist;
    int                      stat_histgram[STAT_HISTGRAM];
    clock_t                  stat_prev_period;
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
    void stat_record(stat_mod_e mod, int point, clock_t start, clock_t end);

    inline void stat_enable(fds_bool_t enable) {
        stat_on = enable;
    }
  private:
    int                      stat_cpu_mhz;
    StatMod                  stat_mod[STAT_MAX_MODS];

    void stat_sampling_mod(stat_mod_e mod);
};

extern StatModule gl_fds_stat;

//static inline volatile fds_uint64_t

/*
 * fds_stat_record
 * ---------------
 * Record the stat at the module and recording point.
 */
static inline void
fds_stat_record(stat_mod_e mod, int point, clock_t start, clock_t end)
{
    if (gl_fds_stat.stat_on && (end >= start)) {
        gl_fds_stat.stat_record(mod, point, start, end);
    }
}

} // namespace fds

#endif /* INCLUDE_UTIL_FDS_STAT_H_ */
