/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_PERSISTENT_LAYER_DM_SERVICE_H_
#define SOURCE_INCLUDE_PERSISTENT_LAYER_DM_SERVICE_H_

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

/////////////////////    DM Disk Query Data Sets    //////////////////////

const fds_uint32_t       dmq_disk_info = 0x00000001;

class DmDiskQuery : public QueryIn
{
  public:
    DmDiskQuery();
    DmDiskQuery(const DmDomainID &domain, const DmNodeID &node);
    ~DmDiskQuery();

    fds_uint32_t             dmq_mask;
    fds_blk_t                dmq_blk_size;
    fds_disk_type_t          dmq_dsk_type;
};

class DmDiskInfo;

class DmDiskQueryOut : public QueryOut<DmDiskInfo>
{
  public:
    DmDiskQueryOut() : QueryOut<DmDiskInfo>() {}
    ~DmDiskQueryOut() {}

    /*
     * Standard QueryOut iterations to process query results.
     *
     * DmDiskInfo     *info;
     * DmDiskQueryOut  q_out;
     *
     * while ((info = q_out.query_pop()) != nullptr) {
     *     ... work with info
     *     delete info;
     * }
     * for (q_out.query_iter_reset();
     *      q_out.query_iter_term() != false; q_out.query_iter_next()) {
     *     info = q_out.query_iter_current();
     *     ...
     * }
     */
  private:
};

class DmDiskInfo
{
  public:
    DmDiskInfo() : di_chain(this) {}

    fds_blk_t                di_max_blks_cap;
    fds_blk_t                di_used_blks;
    fds_disk_type_t          di_disk_type;
    fds_uint32_t             di_min_iops;     // based on given dmq_blk_size.
    fds_uint32_t             di_max_iops;
    fds_uint32_t             di_min_latency;  // 1/iops number in micro-sec.
    fds_uint32_t             di_max_latency;
    ChainLink                di_chain;
};

class DmDiskParams
{
};

//////////////////    DM System Query Data Sets    ///////////////////////

class DmSysQuery : public QueryIn
{
  public:
    DmSysQuery();
};

class DmSysQueryOut
{
  public:
    DmSysQueryOut();
};

class DmSysParams
{
};

//////////////////    DM Module Query Data Sets    ///////////////////////

class DmModQuery : public QueryIn
{
  public:
    DmModQuery();
};

class DmModQueryOut
{
  public:
    DmModQueryOut();
};

class DmModParams
{
};

//////////////////////    DM Query Manager    ////////////////////////////

class DmQuery : protected QueryMgr
{
  public:
    ~DmQuery();
    DmQuery();
    static DmQuery &dm_query();

    static const int dm_blk_shift = 9;
    static const int dm_blk_size  = (1 << DmQuery::dm_blk_shift);
    static const int dm_blk_mask  = DmQuery::dm_blk_size - 1;

    void dm_iops(int *min, int *max);
    void dm_iops(const DmDiskQuery &query, int *min, int *max);

    bool dm_get_disk_params(const DmDiskQuery &query, DmDiskParams *p);
    bool dm_set_disk_params(const DmDiskQuery &query, const DmDiskParams &p);

    bool dm_sys_query(const DmSysQuery &query, DmSysQueryOut *result);
    bool dm_get_sys_params(const DmSysQuery &query, DmSysParams *p);
    bool dm_set_sys_params(const DmSysQuery &query, const DmSysParams &p);

    bool dm_mod_query(const DmModQuery &query, DmModQueryOut *result);
    bool dm_get_mod_params(const DmModQuery &query, DmModParams *p);
    bool dm_set_mod_params(const DmModQuery &query, const DmModParams &p);
};

}  // namespace fds
#endif  // SOURCE_INCLUDE_PERSISTENT_LAYER_DM_SERVICE_H_
