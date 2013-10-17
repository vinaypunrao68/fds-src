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
#include <fdsp/FDSP.h>
#include <boost/thread/thread.hpp>
#include <boost/lockfree/queue.hpp>
#include <boost/atomic.hpp>
#define FDS_MAX_VOLUME_POLICY  128
#define FDS_MAX_ARCHIVE_POLICY  128
using namespace FDS_ProtocolInterface;
using namespace boost;

namespace fds {

  // typedef fds_uint64_t fds_volid_t;
  typedef boost::posix_time::ptime ptime;

  const fds_volid_t invalid_vol_id = 0;

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
    double                 volUUID;

    FDSP_VolType           volType;
    double                 capacity;
    double                 maxQuota;  // Quota % of capacity tho should alert
    int                    replicaCnt;  // Number of replicas reqd for this volume
    int                    writeQuorum;  // Quorum number of writes for success
    int                    readQuorum;  // This will be 1 for now
    FDSP_ConsisProtoType   consisProtocol;  // Read-Write consistency protocol
    // Other policies
    int                    volPolicyId;
    int                    archivePolicyId;
    int                    placementPolicy;  // Can change placement policy
    FDSP_AppWorkload       appWorkload;
    int                    backupVolume;  // UUID of backup volume

    /* policy info */
    double                 iops_min;
    double                 iops_max;
    int                    relativePrio;

    ptime ctime; /* Create time */

    /*
     * Constructors/destructors
     */
    VolumeDesc(FDSP_VolumeInfoTypePtr&  volinfo)
     {
      name = volinfo->vol_name;
      tennantId = volinfo->tennantId;  
      localDomainId = volinfo->localDomainId;  
      globDomainId = volinfo->globDomainId;
      volUUID = volinfo->volUUID;
      volType = volinfo->volType;
      capacity = volinfo->capacity;
      maxQuota = volinfo->maxQuota; 
      replicaCnt = volinfo->replicaCnt; 
      writeQuorum = volinfo->writeQuorum; 
      readQuorum = volinfo->readQuorum;  
      consisProtocol = volinfo->consisProtocol; 
      volPolicyId = volinfo->volPolicyId;
      archivePolicyId = volinfo->archivePolicyId;
      placementPolicy = volinfo->placementPolicy;  
      appWorkload = volinfo->appWorkload;
      backupVolume = volinfo->backupVolume;
      /// FDSP_VolumeInfoTypePtr does not contain policy details 
      // but it is so far only called in orchMgr where policy
      // details are explicitly set after the construction, so 
      // just fill in zeros here
      iops_min = 0;
      iops_max = 0;
      relativePrio = 0;
      assert(volUUID != invalid_vol_id);
    }

    VolumeDesc(const VolumeDesc& vdesc) { 
      name = vdesc.name;
      tennantId = vdesc.tennantId;  
      localDomainId = vdesc.localDomainId;  
      globDomainId = vdesc.globDomainId;
      volUUID = vdesc.volUUID;
      volType = vdesc.volType;
      capacity = vdesc.capacity;
      maxQuota = vdesc.maxQuota; 
      replicaCnt = vdesc.replicaCnt; 
      writeQuorum = vdesc.writeQuorum; 
      readQuorum = vdesc.readQuorum;  
      consisProtocol = vdesc.consisProtocol; 
      volPolicyId = vdesc.volPolicyId;
      archivePolicyId = vdesc.archivePolicyId;
      placementPolicy = vdesc.placementPolicy;  
      appWorkload = vdesc.appWorkload;
      backupVolume = vdesc.backupVolume;
      iops_min = vdesc.iops_min;
      iops_max = vdesc.iops_max;
      relativePrio = vdesc.relativePrio;
      assert(volUUID != invalid_vol_id);
    }

    VolumeDesc(FDSP_VolumeDescTypePtr& voldesc)
      {
	name = voldesc->vol_name;
	tennantId = voldesc->tennantId;  
	localDomainId = voldesc->localDomainId;  
	globDomainId = voldesc->globDomainId;
	volUUID = voldesc->volUUID;
	volType = voldesc->volType;
	capacity = voldesc->capacity;
	maxQuota = voldesc->maxQuota;
	replicaCnt = voldesc->replicaCnt; 
	writeQuorum = voldesc->writeQuorum; 
	readQuorum = voldesc->readQuorum;  
	consisProtocol = voldesc->consisProtocol; 
	volPolicyId = voldesc->volPolicyId;
	archivePolicyId = voldesc->archivePolicyId;
	placementPolicy = voldesc->placementPolicy;  
	appWorkload = voldesc->appWorkload;
	backupVolume = voldesc->backupVolume;
	iops_min = voldesc->iops_min;
	iops_max = voldesc->iops_max;
	relativePrio = voldesc->rel_prio;
	assert(volUUID != invalid_vol_id);
      }

    /*
     * Used for testing where we don't have all of these fields.
     */
    VolumeDesc(const std::string& _name, fds_volid_t _uuid)
             : name(_name),
              volUUID(_uuid) {
        assert(_uuid != invalid_vol_id);

	tennantId = 0;
	localDomainId = 0;
	globDomainId = 0;
	volType = FDS_ProtocolInterface::FDSP_VOL_S3_TYPE;
	capacity = 0;
	maxQuota = 0;
	replicaCnt = 0;
	writeQuorum = 0;
	readQuorum = 0;
	consisProtocol = FDS_ProtocolInterface::FDSP_CONS_PROTO_STRONG;
	volPolicyId = 0;
	archivePolicyId = 0;
	placementPolicy = 0;
	appWorkload = FDS_ProtocolInterface::FDSP_APP_WKLD_TRANSACTION;
	backupVolume = 0;
	iops_min = 0;
	iops_max = 0;
	relativePrio = 0;
	assert(volUUID != invalid_vol_id);
    }

    ~VolumeDesc() {
    }

    std::string GetName() const {
      return name;
    }

    fds_volid_t GetID() const {
      return volUUID;
    }

    std::string ToString() {
      return (std::string("Vol<") + GetName() + std::string(":") + std::to_string(GetID()) + std::string(">"));  
    }

    bool operator==(const VolumeDesc &rhs) const {
      return (this->volUUID == rhs.volUUID );
    }

    bool operator!=(const VolumeDesc &rhs) const {
      return !(*this == rhs);
    }

    bool operator=(VolumeDesc &volinfo) {
      this->name = volinfo.name;
      this->tennantId = volinfo.tennantId;  
      this->localDomainId = volinfo.localDomainId;  
      this->globDomainId = volinfo.globDomainId;
      this->volUUID = volinfo.volUUID;
      this->volType = volinfo.volType;
      this->capacity = volinfo.capacity;
      this->maxQuota = volinfo.maxQuota; 
      this->replicaCnt = volinfo.replicaCnt; 
      this->writeQuorum = volinfo.writeQuorum; 
      this->readQuorum = volinfo.readQuorum;  
      this->consisProtocol = volinfo.consisProtocol; 
      this->volPolicyId = volinfo.volPolicyId;
      this->archivePolicyId = volinfo.archivePolicyId;
      this->placementPolicy = volinfo.placementPolicy;  
      this->appWorkload = volinfo.appWorkload;
      this->backupVolume = volinfo.backupVolume;
      return true;
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
    VolumeDesc *voldesc;
    fds_uint64_t   real_iops_max;
    fds_uint64_t   real_iops_min;
    
    FDS_Volume()
      {
      }
 
    FDS_Volume(const VolumeDesc& vol_desc)
      : FDS_Volume()
      {
	voldesc = new VolumeDesc(vol_desc);
      }
      
    ~FDS_Volume() {
       delete voldesc;
     }
  };

  class FDS_VolumePolicy {
 public:
    fds_uint32_t   volPolicyId;
    std::string    volPolicyName;
    fds_uint64_t   iops_max;
    fds_uint64_t   iops_min;
    fds_uint64_t   thruput;       // in MByte/sec
    fds_uint32_t   relativePrio;  // Relative priority

    FDS_VolumePolicy() {
    }
    ~FDS_VolumePolicy() {
    }

    Error setPolicy(const char* data, fds_uint32_t len)
    {
      Error err(ERR_OK);
      int offset = 0;
      fds_uint32_t namelen = 0;
      fds_uint32_t minlen = getPolicyStringMinLength();
      if (len < minlen) {
	err = ERR_INVALID_ARG;
	return err;
      }

      memcpy(&volPolicyId, data + offset, sizeof(fds_uint32_t));
      offset += sizeof(fds_uint32_t);
      memcpy(&iops_max, data + offset, sizeof(fds_uint64_t));
      offset += sizeof(fds_uint64_t);
      memcpy(&iops_min, data + offset, sizeof(fds_uint64_t));
      offset += sizeof(fds_uint64_t);
      memcpy(&thruput, data + offset, sizeof(fds_uint64_t));
      offset += sizeof(fds_uint64_t);
      memcpy(&relativePrio, data + offset, sizeof(fds_uint32_t));
      offset += sizeof(fds_uint32_t);

      /* next is policy name length + policy name */
      memcpy(&namelen, data + offset, sizeof(fds_uint32_t));
      offset += sizeof(fds_uint32_t);
      if ((len-minlen) < namelen) {
	err = ERR_INVALID_ARG;
      } 
      else if (namelen > 0) {
	volPolicyName.assign(data + offset, namelen);
      }

      return err;
    }

    /* policy string length without policy name length but including policy string size */
    fds_uint32_t getPolicyStringMinLength() const 
    {
      int len = sizeof(volPolicyId) + 
	        sizeof(iops_max) + 
    	        sizeof(iops_min) + 
	        sizeof(thruput) + 
	        sizeof(relativePrio) + 
	        sizeof(fds_uint32_t);
					
      return len;
    }

    /* length of string representation of policy */
    fds_uint32_t getPolicyStringLength() const 
    {
      return (getPolicyStringMinLength() + volPolicyName.length());
    }

    std::string toString() const 
      {
	int offset = 0;
	fds_uint32_t length = getPolicyStringLength();
	fds_uint32_t namelen = volPolicyName.length();
	char str[length];

	memcpy(str, &volPolicyId, sizeof(fds_uint32_t));
	offset += sizeof(fds_uint32_t);
	memcpy(str + offset, &iops_max, sizeof(fds_uint64_t));
	offset += sizeof(fds_uint64_t);
	memcpy(str + offset, &iops_min, sizeof(fds_uint64_t));
	offset += sizeof(fds_uint64_t);
	memcpy(str + offset, &thruput, sizeof(fds_uint64_t));
	offset += sizeof(fds_uint64_t);
	memcpy(str + offset, &relativePrio, sizeof(fds_uint32_t));
	offset += sizeof(fds_uint32_t);
	memcpy(str + offset, &namelen, sizeof(fds_uint32_t));
	offset += sizeof(fds_uint32_t);
	if (namelen > 0) {
	  memcpy(str + offset, volPolicyName.data(), volPolicyName.length());
	}

	return std::string(str, length);
      }
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
                 fds_uint32_t prio) : 
                 iops_max(_iops_max), 
                 iops_min(_iops_min), 
                 priority(prio) {
    volQueue = new boost::lockfree::queue<FDS_IOType *> (q_capacity);
    volQState = FDS_VOL_Q_INACTIVE;
 }

~FDS_VolumeQueue() {
   delete volQueue;
 }

 void activate() {
   volQState = FDS_VOL_Q_ACTIVE;
 }


 // Quiesce queued IOs on this queue & block any new IOs
 void  quiesceIOs() {
   volQState = FDS_VOL_Q_QUIESCING;
 }

 void   suspendIO() { 
   volQState = FDS_VOL_Q_SUSPENDED;
 } 

 void   resumeIO() {
   volQState = FDS_VOL_Q_ACTIVE;
 }

 void   enqueueIO(FDS_IOType *io) { 
   if ( volQState == FDS_VOL_Q_ACTIVE) { 
      while(!volQueue->push(io));
   }
 }


 FDS_IOType   *dequeueIO() {

   FDS_IOType *io = NULL;
   if ( volQState == FDS_VOL_Q_ACTIVE || volQState == FDS_VOL_Q_QUIESCING ) { 
     volQueue->pop(io); 
     if (volQState == FDS_VOL_Q_QUIESCING && volQueue->empty())  {
       volQState = FDS_VOL_Q_SUSPENDED;
     }
     return io;
   }
  return NULL;
 }

};
}  // namespace fds
#endif  // SOURCE_INCLUDE_FDS_VOLUME_H_
