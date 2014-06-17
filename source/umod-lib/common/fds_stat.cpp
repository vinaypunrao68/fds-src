/*
 * Copyright 2013 by Formation Data Systems, Inc.
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <fds_assert.h>
#include <util/fds_stat.h>
#include <util/timeutils.h>
#include <shared/bitutil.h>

namespace fds {

StatModule gl_fds_stat("FDS Stat");

StatRec::StatRec()
{
    stat_hist_cnt    = 0;
    stat_hist_idx    = 0;
    stat_reqs        = 0;
    stat_req_hist    = NULL;
    stat_prev_period = 0;
    stat_cpu_switch  = 0;
    memset(stat_histgram, 0, sizeof(int) * STAT_HISTGRAM);
}

StatRec::~StatRec()
{
    if (stat_req_hist != NULL) {
        delete [] stat_req_hist;
    }
}

StatMod::StatMod()
    : stat_pts(0), stat_hist_us(0), stat_rec(NULL), stat_decode(NULL)
{
    // Keep request/sec history.
    stat_hist_us = 1000000;
}

StatMod::~StatMod()
{
    if (stat_rec != NULL) {
        delete [] stat_rec;
    }
}

StatModule::~StatModule() {}
StatModule::StatModule(char const *const name)
    : Module(name), stat_on(false) {}

// mod_init
// --------
//
int
StatModule::mod_init(SysParams const *const param)
{
    return fds::util::getCpuSpeedHz();
}

// mod_startup
// -----------
//
void
StatModule::mod_startup()
{
    stat_enable(true);
    stat_set_hist(STAT_NGX, 3600);
}

// mod_shutdown
// ------------
//
void
StatModule::mod_shutdown()
{
    stat_enable(false);
}

void
StatModule::stat_reg_mod(stat_mod_e mod, const stat_decode_t decode[])
{
    int      i;
    StatMod *stat;

    stat = &stat_mod[mod];
    fds_verify(stat->stat_pts == 0);

    for (i = 0; decode[i].stat_name != NULL; i++) {
        fds_verify(decode[i].stat_point == i);
    }
    stat->stat_pts    = i;
    stat->stat_rec    = new StatRec [stat->stat_pts];
    stat->stat_decode = decode;
}


// stat_set_hist
// -------------
//
void
StatModule::stat_set_hist(stat_mod_e mod, int seclong)
{
    int      *hist, *old, i;
    StatMod  *stat;
    StatRec  *rec;

    stat = &stat_mod[mod];
    for (i = 0; i < stat->stat_pts; i++) {
        rec = &stat->stat_rec[i];
        if (rec->stat_hist_cnt < seclong) {
            hist = new int [seclong];
            old  = rec->stat_req_hist;
            rec->stat_req_hist = hist;
            if (old != NULL) {
                delete [] old;
            }
        } else {
            fds_verify(rec->stat_req_hist != NULL);
        }
        rec->stat_hist_idx = 0;
        rec->stat_hist_cnt = seclong;
    }
}

// stat_sampling
// -------------
//
void
StatModule::stat_sampling()
{
    int i;

    for (i = 0; i < STAT_MAX_MODS; i++) {
        stat_sampling_mod((stat_mod_e)i);
    }
}

// stat_record
// -----------
//
void
StatModule::stat_record(stat_mod_e   mod,
                        int          point,
                        fds_uint64_t start,
                        fds_uint64_t end)
{
    int       dif, idx;
    StatRec  *rec;
    StatMod  *stat;

    stat = &stat_mod[mod];
    rec  = &stat->stat_rec[point];
    if (end >= start) {
        dif  = (int)((end - start) / stat_cpu_mhz);
        idx  = bit_hibit(dif);
        if (idx != 0) {
            idx--;
        }
        // Don't care if we lost some updates here.
        rec->stat_histgram[idx]++;
    } else {
        rec->stat_cpu_switch++;
    }
    rec->stat_reqs++;
    stat_sampling_mod(mod);
}

// stat_sampling_mod
// -----------------
//
void
StatModule::stat_sampling_mod(stat_mod_e mod)
{
}

static char const *const stat_time_fmt[] =
{
    "         [0...1] us: ",
    "         [2...4] us: ",
    "         [4...8] us: ",
    "        [8...16] us: ",
    "       [16...32] us: ",
    "       [32...64] us: ",
    "      [64...128] us: ",
    "     [128...256] us: ",
    "     [256...512] us: ",
    "    [512...1024] us: ",
    "         [1...2] ms: ",
    "         [2...4] ms: ",
    "         [4...8] ms: ",
    "        [8...16] ms: ",
    "       [16...32] ms: ",
    "       [32...64] ms: ",
    "      [64...128] ms: ",
    "     [128...256] ms: ",
    "     [256...512] ms: ",
    "    [512...1024] ms: ",
    "         [1...2]  s: ",
    "         [2...4]  s: ",
    "         [4...8]  s: ",
    "        [8...16]  s: ",
    "       [16...32]  s: ",
    "       [32...64]  s: ",
    "   [1.06...2.13]  m: ",
    "   [2.13...4.27]  m: ",
    "   [4.27...8.53]  m: ",
    "  [8.53...17.10]  m: ",
    " [17.10...34.13]  m: ",
    " [34.13...68.27]  m: ",
    NULL
};

// stat_out
// --------
// Return the length that would be consumed to store the full output.
//
int
StatModule::stat_out(stat_mod_e mod, char *buf, int len)
{
    int             i, j, used;
    StatMod        *stat;

    used = 0;
    if (mod == STAT_MAX_MODS) {
        for (i = 0; i < STAT_MAX_MODS; i++) {
            stat = &stat_mod[i];
            if (stat == NULL) {
                continue;
            }
            for (j = 0; j < stat->stat_pts; j++) {
                used += stat_rec_out(stat, j, &buf, &len);
            }
        }
    } else {
        stat = &stat_mod[mod];
        for (i = 0; i < stat->stat_pts; i++) {
            used += stat_rec_out(stat, i, &buf, &len);
        }
    }
    return used;
}

// stat_rec_out
// ------------
//
int
StatModule::stat_rec_out(StatMod *stat, int point, char **buf, int *len)
{
    int             i, save, cnt, req_sum, off;
    float           pct, pct_sum;
    StatRec        *rec;
    // stat_decode_t  *dcode;

    req_sum = 0;
    pct_sum = 0;
    save    = *len;
    rec     = stat->stat_rec + point;

    for (i = 0; i < STAT_HISTGRAM; i++) {
        req_sum += rec->stat_histgram[i];
    }
    if (req_sum == 0) {
        return 0;
    }
    off = snprintf(*buf, *len, "\n<<<<<<[  %s  ]>>>>>>\n",
                   stat->stat_decode[point].stat_name);
    *len -= off;
    *buf += off;

    for (i = 0; i < STAT_HISTGRAM; i++) {
        cnt = rec->stat_histgram[i];
        if (cnt == 0) {
            continue;
        }
        pct = (cnt * 100) / req_sum;
        pct_sum += pct;
        off = snprintf(*buf, *len, "%6.2f%% %s %6u\n",
                       pct, stat_time_fmt[i], cnt);
        *len -= off;
        *buf += off;
    }
    off = snprintf(*buf, *len,
                   "%6.2f%%            Total   :  %6u   cpu switch: %d\n",
                   pct_sum, req_sum, rec->stat_cpu_switch);
    *len -= off;
    *buf += off;
    return (save - *len);
}

} // namespace fds
