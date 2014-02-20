/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <persistent_layer/dm_service.h>

namespace fds {

DmDiskQuery::DmDiskQuery()
{
}

DmDiskQuery::DmDiskQuery(const DmDomainID &domain, const DmNodeID &node)
{
}

DmDiskQuery::~DmDiskQuery()
{
}

DmSysQuery::DmSysQuery()
{
}

DmSysQueryOut::DmSysQueryOut()
{
}

DmModQuery::DmModQuery()
{
}

DmModQueryOut::DmModQueryOut()
{
}

DmQuery::DmQuery()
{
}

DmQuery::~DmQuery()
{
}

DmQuery  sgl_dm_query;

DmQuery &
DmQuery::dm_query()
{
    return sgl_dm_query;
}

void
DmQuery::dm_iops(int *min, int *max)
{
    *min  = 200;
    *max  = 4000;
}

void
DmQuery::dm_iops(const DmDiskQuery &query, int *min, int *max)
{
    *min  = 200;
    *max  = 4000;
}

/*
 * dmq_hdd_info
 * ------------
 */
DmDiskInfo *
DmQuery::dmq_hdd_info()
{
    DmDiskInfo *info = new DmDiskInfo;

    info->di_max_blks_cap = 0x80000000;
    info->di_used_blks    = 0x0;
    info->di_disk_type    = FDS_DISK_SATA;
    info->di_min_iops     = 100;
    info->di_max_iops     = 30000;
    info->di_min_latency  = 1000000 / info->di_max_iops;
    info->di_max_latency  = 1000000 / info->di_min_iops;
    return info;
}

/*
 * dmq_ssd_info
 * ------------
 */
DmDiskInfo *
DmQuery::dmq_ssd_info()
{
    DmDiskInfo *info = new DmDiskInfo;

    info->di_max_blks_cap = 0x10000000;
    info->di_used_blks    = 0x0;
    info->di_disk_type    = FDS_DISK_SSD;
    info->di_min_iops     = 1000;
    info->di_max_iops     = 300000;
    info->di_min_latency  = 1000000 / info->di_max_iops;
    info->di_max_latency  = 1000000 / info->di_min_iops;
    return info;
}

/*
 * dm_disk_query
 * -------------
 */
bool
DmQuery::dm_disk_query(const DmDiskQuery &query, DmDiskQueryOut *out)
{
    if (query.dmq_mask & fds::dmq_disk_info) {
        DmDiskInfo *hdd = dmq_hdd_info();
        DmDiskInfo *ssd = dmq_ssd_info();

        out->query_push(&hdd->di_chain);
        out->query_push(&ssd->di_chain);
        return true;
    }
    return false;
}

bool
DmQuery::dm_get_disk_params(const DmDiskQuery &query, DmDiskParams *p)
{
    return true;
}

bool
DmQuery::dm_set_disk_params(const DmDiskQuery &query, const DmDiskParams &p)
{
    return true;
}

bool
DmQuery::dm_sys_query(const DmSysQuery &query, DmSysQueryOut *out)
{
    return true;
}

bool
DmQuery::dm_get_sys_params(const DmSysQuery &query, DmSysParams *p)
{
    return true;
}

bool
DmQuery::dm_set_sys_params(const DmSysQuery &query, const DmSysParams &p)
{
    return true;
}

bool
DmQuery::dm_mod_query(const DmModQuery &query, DmModQueryOut *out)
{
    return true;
}

bool
DmQuery::dm_get_mod_params(const DmModQuery &query, DmModParams *p)
{
    return true;
}

bool
DmQuery::dm_set_mod_params(const DmModQuery &query, const DmModParams &p)
{
    return true;
}

}  // namespace fds
