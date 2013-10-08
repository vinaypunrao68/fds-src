#include <dm/dm_service.h>

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

DmDiskQueryOut::DmDiskQueryOut()
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

const DmQuery  sgl_dm_query;

const DmQuery &
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

bool
DmQuery::dm_disk_query(const DmDiskQuery &query, DmDiskQueryOut *out)
{
    return true;
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

} // namespace fds
