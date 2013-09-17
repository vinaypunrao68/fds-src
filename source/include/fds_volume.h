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

#include "include/fds_types.h"
#include "include/fds_err.h"
#define FDS_MAX_VOLUME_POLICY  128
#define FDS_MAX_ARCHIVE_POLICY  128

namespace fds {

  typedef fds_uint64_t fds_volid_t;
  typedef boost::posix_time::ptime ptime;

  const fds_volid_t invalid_vol_id = 0;

  /*
   * Basic descriptor class for a volume. This is intended
   * to be a cacheable/passable description of a volume instance.
   * The authoritative volume information is stored on the OM.
   */
  class VolumeDesc {
 private:
    /*
     * Basic ID information.
     */
    std::string name;
    fds_volid_t uuid;

    fds_uint64_t parent_ld; /* TODO: Fill with parent local domain ID */

    /* TODO: Put permission info here. */
    /* TODO: Put some SLA info here. */
    /* TODO: Volume type */

    ptime ctime; /* Create time */

 public:
    /*
     * Constructors/destructors
     */
    VolumeDesc(const std::string& _name, fds_volid_t _uuid)
        : name(_name),
        uuid(_uuid),
        parent_ld(0) {
      assert(_uuid != invalid_vol_id);
    }
    ~VolumeDesc() {
    }

    std::string GetName() const {
      return name;
    }

    fds_volid_t GetID() const {
      return uuid;
    }

    std::string ToString() {
      return (std::string("Vol<") + GetName() + std::string(":") + std::to_string(GetID()) + std::string(">"));  
    }

    bool operator==(const VolumeDesc &rhs) const {
      return (this->uuid == rhs.uuid);
    }

    bool operator!=(const VolumeDesc &rhs) const {
      return !(*this == rhs);
    }
  };

  inline std::ostream& operator<<(std::ostream& out,
                                  const VolumeDesc& vol_desc) {
    return out << "Vol<" << vol_desc.GetName() << ":"
               << vol_desc.GetID() << ">";
  }

  enum FDS_VolType {
    FDS_VOL_S3_TYPE,
    FDS_VOL_BLKDEV_TYPE,
    FDS_VOL_BLKDEV_SSD_TYPE
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
    std::string         vol_name;
    fds_uint32_t        tennantId;  // Tennant id that owns the volume
    fds_uint32_t        localDomainId;  // Local domain id that owns vol
    fds_uint32_t        globDomainId;
    fds_volid_t         volUUID;
    FDS_VolType         volType;

    fds_uint64_t        capacity;
    fds_uint64_t        maxQuota;  // Quota % of capacity tho should alert

    fds_uint32_t        replicaCnt;  // Number of replicas reqd for this volume
    fds_uint32_t        writeQuorum;  // Quorum number of writes for success
    fds_uint32_t        readQuorum;  // This will be 1 for now
    FDS_ConsisProtoType consisProtocol;  // Read-Write consistency protocol

    fds_uint32_t        volPolicyId;
    fds_uint32_t        archivePolicyId;
    fds_uint32_t        placementPolicy;  // Can change placement policy
    FDS_AppWorkload     appWorkload;

    fds_volid_t         backupVolume;  // UUID of backup volume

    FDS_Volume();
    ~FDS_Volume();
  };

  class FDS_VolumePolicy {
 public:
    fds_uint32_t   volPolicyId;
    std::string    volPolicyName;
    fds_uint64_t   iops_max;
    fds_uint64_t   iops_min;
    fds_uint64_t   thruput;       // in MByte/sec
    fds_uint32_t   relativePrio;  // Relative priority

    FDS_VolumePolicy();
    ~FDS_VolumePolicy();
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
}  // namespace fds
#endif  // SOURCE_INCLUDE_FDS_VOLUME_H_
