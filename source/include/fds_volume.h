/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

/*
 * Basic include information for a volume.
 */

#ifndef SOURCE_INCLUDE_FDS_VOLUME_H_
#define SOURCE_INCLUDE_FDS_VOLUME_H_

#include <boost/date_time/posix_time/posix_time.hpp>

#include <string>

#include <fds_types.h>
#include <fds_err.h>
#include <fds_assert.h>
#include <fdsp/FDSP_types.h>
#include <boost/thread/thread.hpp>
#include <boost/lockfree/queue.hpp>
#include <boost/atomic.hpp>
#include <serialize.h>
#define FDS_MAX_VOLUME_POLICY  128
#define FDS_MAX_ARCHIVE_POLICY  128
using namespace FDS_ProtocolInterface;
using namespace boost;

#define FdsSysTaskQueueId 0xefffffff
#define FdsSysTaskPri 5

namespace fds {

    // typedef fds_uint64_t fds_volid_t;
    typedef boost::posix_time::ptime ptime;

    const fds_volid_t invalid_vol_id = 0;
    const fds_volid_t admin_vol_id = 0x8001;  /* not a real volume, using volume data struct to queue admin commands from AM to OM */

    /*
     * Basic descriptor class for a volume. This is intended
     * to be a cacheable/passable description of a volume instance.
     * The authoritative volume information is stored on the OM.
     */
    class VolumeDesc {
  public:
        /*
         * Basic ID information.
         */
        std::string            name;
        int                    tennantId;  // Tennant id that owns the volume
        int                    localDomainId;  // Local domain id that owns vol
        int                    globDomainId;
        fds_volid_t            volUUID;

        FDSP_VolType           volType;
        double                 capacity;  // Volume capacity is in MB
        double                 maxQuota;  // Quota % of capacity tho should alert
        int                    replicaCnt;  // Number of replicas reqd for this volume
        int                    writeQuorum;  // Quorum number of writes for success
        int                    readQuorum;  // This will be 1 for now
        FDSP_ConsisProtoType   consisProtocol;  // Read-Write consistency protocol
        // Other policies
        int                    volPolicyId;
        int                    archivePolicyId;
        FDSP_MediaPolicy       mediaPolicy;   // can change media policy
        int                    placementPolicy;  // Can change placement policy
        FDSP_AppWorkload       appWorkload;
        int                    backupVolume;  // UUID of backup volume

        /* policy info */
        double                 iops_min;
        double                 iops_max;
        int                    relativePrio;
        ptime                  tier_start_time;
        fds_uint32_t           tier_duration_sec;

        ptime ctime; /* Create time */

        /*
         * Constructors/destructors
         */
    
        VolumeDesc(const FDS_ProtocolInterface::FDSP_VolumeInfoType& volinfo,
                   fds_volid_t vol_uuid);

        VolumeDesc(const VolumeDesc& vdesc);
        VolumeDesc(const FDS_ProtocolInterface::FDSP_VolumeDescType& voldesc);
        /*
         * Used for testing where we don't have all of these fields.
         */
        VolumeDesc(const std::string& _name, fds_volid_t _uuid);

        VolumeDesc(const std::string& _name,
                   fds_volid_t _uuid,
                   fds_uint32_t _iops_min,
                   fds_uint32_t _iops_max,
                   fds_uint32_t _priority);

        ~VolumeDesc();

        void modifyPolicyInfo(fds_uint64_t _iops_min,
                              fds_uint64_t _iops_max,
                              fds_uint32_t _priority);

        std::string getName() const;
        fds_volid_t GetID() const;

        double getIopsMin() const;

        double getIopsMax() const;
        int getPriority() const;

        std::string ToString();

        bool operator==(const VolumeDesc &rhs) const;

        bool operator!=(const VolumeDesc &rhs) const;
        VolumeDesc & operator=(const VolumeDesc& volinfo);
        friend std::ostream& operator<<(std::ostream& out, const VolumeDesc& vol_desc);
    };

    

    enum FDS_VolType {
        FDS_VOL_S3_TYPE,
        FDS_VOL_BLKDEV_TYPE,
        FDS_VOL_BLKDEV_SSD_TYPE,
        FDS_VOL_BLKDEV_DISK_TYPE,
        FDS_VOL_BLKDEV_HYBRID_TYPE,
        FDS_VOL_BLKDEV_HYBRID_PREFCAP_TYPE
    };

    enum FDS_AppWorkload {
        FDS_APP_WKLD_TRANSACTION,
        FDS_APP_WKLD_NOSQL,
        FDS_APP_WKLD_HDFS,
        FDS_APP_WKLD_JOURNAL_FILESYS,  // Ext3/ext4
        FDS_APP_WKLD_FILESYS,  // XFS, other
        FDS_APP_NATIVE_OBJS,  // Native object aka not going over http/rest apis
        FDS_APP_S3_OBJS,  // Amazon S3 style objects workload
        FDS_APP_AZURE_OBJS,  // Azure style objects workload
    };

    enum FDS_ConsisProtoType {
        FDS_CONS_PROTO_STRONG,
        FDS_CONS_PROTO_WEAK,
        FDS_CONS_PROTO_EVENTUAL
    };

    class FDS_Volume {
  public:
        VolumeDesc   *voldesc;
        fds_uint64_t  real_iops_max;
        fds_uint64_t  real_iops_min;
    
        FDS_Volume();
 
        FDS_Volume(const VolumeDesc& vol_desc);      
        ~FDS_Volume();
    };

    class FDS_VolumePolicy : public serialize::Serializable {
  public:
        fds_uint32_t   volPolicyId;
        std::string    volPolicyName;
        fds_uint64_t   iops_max;
        fds_uint64_t   iops_min;
        fds_uint64_t   thruput;       // in MByte/sec
        fds_uint32_t   relativePrio;  // Relative priority

        FDS_VolumePolicy();
        ~FDS_VolumePolicy();

        uint32_t virtual write(serialize::Serializer*  s) const;
        uint32_t virtual read(serialize::Deserializer* d);
        uint32_t virtual getEstimatedSize() const;
        friend std::ostream& operator<<(std::ostream& os, const FDS_VolumePolicy& policy);
    };

    class FDS_ArchivePolicy {
  public:
        fds_uint32_t   snapshotFreq;   // Number of minutes/hrs to take snapshot
        fds_uint32_t   archive2Cloud;  // Archive to cloud enabled
        fds_uint32_t   time2archive;   // TTL for unchanged objects to
        // archive to DR site

        FDS_ArchivePolicy();
        ~FDS_ArchivePolicy();
    };

    class FDS_ArchivePolicyTbl {
  private:
        FDS_ArchivePolicy  archivePolicy[FDS_MAX_ARCHIVE_POLICY];

  public:
        FDS_ArchivePolicyTbl();
        ~FDS_ArchivePolicyTbl();
        Error& AddArchivePolicy(const FDS_ArchivePolicy& archive_policy);
        void DeleteArchivePolicy(const FDS_ArchivePolicy& archive_policy);
        void DeleteArchivePolicy(fds_uint32_t archive_policy_id);
    };

    class FDS_VolumePolicyTbl {
  private:
        FDS_VolumePolicy  volPolicy[FDS_MAX_VOLUME_POLICY];

  public:
        FDS_VolumePolicyTbl();
        ~FDS_VolumePolicyTbl();
        Error& AddVolumePolicy(const FDS_VolumePolicy& vol_policy);
        void DeleteVolumePolicy(const FDS_VolumePolicy& vol_policy);
        FDS_VolumePolicy& GetVolumePolicy(fds_uint32_t vol_policy_id);
    };


    typedef enum {
        FDS_VOL_Q_INACTIVE,
        FDS_VOL_Q_SUSPENDED,
        FDS_VOL_Q_QUIESCING,
        FDS_VOL_Q_ACTIVE
    } VolumeQState;

    /* **********************************************
     *  FDS_VolumeQueue: VolumeQueue
     *
     **********************************************************/
    class FDS_VolumeQueue {
  public:

        boost::lockfree::queue<FDS_IOType*>  *volQueue;
        VolumeQState volQState;

        // Qos Parameters set for this volume/VolumeQueue
        fds_uint64_t  iops_max;
        fds_uint64_t iops_min;
        fds_uint32_t priority; // Relative priority

        // Ctor/dtor
        FDS_VolumeQueue(fds_uint32_t q_capacity, 
                        fds_uint64_t _iops_max, 
                        fds_uint64_t _iops_min, 
                        fds_uint32_t prio) ;

        ~FDS_VolumeQueue();

        void modifyQosParams(fds_uint64_t _iops_min,
                             fds_uint64_t _iops_max,
                             fds_uint32_t _prio);

        void activate();


        // Quiesce queued IOs on this queue & block any new IOs
        void  quiesceIOs();
        void   suspendIO();

        void   resumeIO();

        void   enqueueIO(FDS_IOType *io);
        FDS_IOType   *dequeueIO();
    };
}  // namespace fds
#endif  // SOURCE_INCLUDE_FDS_VOLUME_H_
