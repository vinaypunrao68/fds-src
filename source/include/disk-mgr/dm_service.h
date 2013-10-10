#ifndef INCLUDE_DM_DM_SERVICE_H_
#define INCLUDE_DM_DM_SERVICE_H_

#include <shared/fds_types.h>
#include <fds_resource.h>

namespace fds {

class DmDomainID : public ResourceUUID
{
};

class DmNodeID : public ResourceUUID
{
};

class DmDiskID : public ResourceUUID
{
};

/* /////////////////////    DM Disk Query Data Sets    ////////////////////// */

class DmDiskQuery : public QueryIn
{
    const fds_uint32_t       dmq_disk_info = 0x00000001;

  public:
    DmDiskQuery();
    DmDiskQuery(const DmDomainID &domain, const DmNodeID &node);
    ~DmDiskQuery();

    fds_uint32_t             dmq_mask;
    fds_blk_t                dmq_blk_size;
    fds_disk_type_t          dmq_dsk_type;
};

class DmDiskInfo
{
  public:
    ChainLink<DmDiskInfo>    di_chain;
    fds_blk_t                di_max_blks_cap;
    fds_blk_t                di_used_blks;
    fds_disk_type_t          di_disk_type;

    DmDiskID                 di_disk_id;
    DmNodeID                 di_owner_node;
    DmDomainID               di_dm_domain;
};

class DmDiskQueryOut : public QueryOut
{
  public:
    DmDiskQueryOut();

  private:
};

class DmDiskParams
{
};

/* //////////////////    DM System Query Data Sets    /////////////////////// */

class DmSysQuery : public QueryIn
{
  public:
    DmSysQuery();
};

class DmSysQueryOut : public QueryOut
{
  public:
    DmSysQueryOut();
};

class DmSysParams
{
};

/* //////////////////    DM Module Query Data Sets    /////////////////////// */

class DmModQuery : public QueryIn
{
  public:
    DmModQuery();
};

class DmModQueryOut : QueryOut
{
  public:
    DmModQueryOut();
};

class DmModParams
{
};

/* //////////////////////    DM Query Manager    //////////////////////////// */

class DmQuery : protected QueryMgr
{
  public:
    ~DmQuery();

    /*
     * Usage:
     * const DmQuery &dm = DmQuery::dm_query();
     * dm.dm_iops(&min, &max);
     */
    static DmQuery &dm_query();

    static const int dm_blk_shift = 9;
    static const int dm_blk_size  = (1 << DmQuery::dm_blk_shift);
    static const int dm_blk_mask  = DmQuery::dm_blk_size - 1;

    void dm_iops(int *min, int *max);
    void dm_iops(const DmDiskQuery &query, int *min, int *max);

    bool dm_disk_query(const DmDiskQuery &query, DmDiskQueryOut *result);
    bool dm_get_disk_params(const DmDiskQuery &query, DmDiskParams *p);
    bool dm_set_disk_params(const DmDiskQuery &query, const DmDiskParams &p);

    bool dm_sys_query(const DmSysQuery &query, DmSysQueryOut *result);
    bool dm_get_sys_params(const DmSysQuery &query, DmSysParams *p);
    bool dm_set_sys_params(const DmSysQuery &query, const DmSysParams &p);

    bool dm_mod_query(const DmModQuery &query, DmModQueryOut *result);
    bool dm_get_mod_params(const DmModQuery &query, DmModParams *p);
    bool dm_set_mod_params(const DmModQuery &query, const DmModParams &p);

  // private: not yet
    DmQuery();
};

} // namespace fds

#endif /* INCLUDE_DM_DM_SERVICE_H_ */
