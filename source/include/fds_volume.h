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
#include <atomic>

#include <fds_error.h>
#include <fds_assert.h>
#include <fds_ptr.h>
#include <fds_value_type.h>
#include <has_state.h>
#include <boost/thread/thread.hpp>
#include <boost/lockfree/queue.hpp>
#include <boost/atomic.hpp>
#include <boost/io/ios_state.hpp>
#include <serialize.h>
#include <shared/fds-constants.h>
#include <fdsp/volumegroup_types.h>
#define FdsSysTaskQueueId fds_volid_t(0xefffffff)
#define FdsSysTaskPri 5

namespace fpi = FDS_ProtocolInterface;

namespace fds {

class FDS_IOType;

typedef boost::posix_time::ptime ptime;

using fds_volid_t = fds_value_type<uint64_t>;

static fds_volid_t const invalid_vol_id(0);
static constexpr fds_int64_t invalid_vol_token = -1;

/**
 * Basic descriptor class for a volume. This is intended
 * to be a cacheable/passable description of a volume instance.
 * The authoritative volume information is stored on the OM.
 */
class VolumeDesc : public HasState {
  public:
   /**
    * NOTE: prior to adding new fields here,
    * any of these that are needed for thrift interface should be updated
    * in toFdspDesc as well as the constructor that takes the thrift type
    */
    // Basic ID information.
    std::string            name;
    int                    tennantId;  // Tennant id that owns the volume
    int                    localDomainId;  // Local domain id that owns vol
    int                    globDomainId;
    fds_volid_t            volUUID;

    // Basic settings
    FDS_ProtocolInterface::FDSP_VolType volType;
    double                 capacity;  // Volume capacity is in MB
    double                 maxQuota;  // Quota % of capacity tho should alert
    int                    redundancyCnt;  // Number of redundant copies reqd for this volume for accessability
    fds_uint32_t           maxObjSizeInBytes;

    // Advanced settings
    int                    writeQuorum;  // Quorum number of writes for success
    int                    readQuorum;  // This will be 1 for now
    FDS_ProtocolInterface::FDSP_ConsisProtoType
    consisProtocol;  // Read-Write consistency protocol
    // Other policies
    int                    volPolicyId;
    int                    archivePolicyId;
    FDS_ProtocolInterface::FDSP_MediaPolicy mediaPolicy;   // can change media policy
    int                    placementPolicy;  // Can change placement policy
    FDS_ProtocolInterface::FDSP_AppWorkload appWorkload;
    int                    backupVolume;  // UUID of backup volume

    // QoS settings
    fds_int64_t            iops_assured;
    fds_int64_t            iops_throttle;
    int                    relativePrio;
    ptime                  tier_start_time;
    fds_uint32_t           tier_duration_sec;
    fds_uint64_t           createTime;

    /* Snapshot */
    bool                   fSnapshot = false;
    fds_volid_t            srcVolumeId = invalid_vol_id;
    fds_volid_t            lookupVolumeId = invalid_vol_id;
    fds_volid_t            qosQueueId = invalid_vol_id;
    // in seconds
    fds_uint64_t           contCommitlogRetention;
    // in seconds
    fds_uint64_t           timelineTime;
    // in millis

    // Async Replication
    bool primary {false}; // "true" if transactions against this volume are to be asynchronously replicated.
    bool replica {false}; // "true" if this volume is maintained with asynchronously replicated transactions.

    FDS_ProtocolInterface::IScsiTarget iscsiSettings;
    FDS_ProtocolInterface::NfsOption nfsSettings;

    FDS_ProtocolInterface::ResourceState     state;

    FDS_ProtocolInterface::VolumeGroupCoordinatorInfo   coordinator;

    /* Output from block device */
    char                   vol_blkdev[FDS_MAX_VOL_NAME];

    VolumeDesc(const FDS_ProtocolInterface::FDSP_VolumeDescType& voldesc,
               fds_volid_t vol_uuid);

    VolumeDesc(const VolumeDesc& vdesc);  // NOLINT
    VolumeDesc(const FDS_ProtocolInterface::FDSP_VolumeDescType& voldesc);  //NOLINT
    //  Used for testing where we don't have all of these fields.
    VolumeDesc(const std::string& _name, fds_volid_t _uuid);
    VolumeDesc(const std::string& _name,
               fds_volid_t _uuid,
               fds_int64_t _iops_assured,
               fds_int64_t _iops_throttle,
               fds_uint32_t _priority);
    ~VolumeDesc();

    void modifyPolicyInfo(fds_int64_t _iops_assured,
                          fds_int64_t _iops_throttle,
                          fds_uint32_t _priority);

    std::string getName() const;
    fds_volid_t GetID() const;

    fds_int64_t getIopsAssured() const;

    fds_int64_t getIopsThrottle() const;
    int getPriority() const;

    std::string ToString();
    void toFdspDesc(FDS_ProtocolInterface::FDSP_VolumeDescType& voldesc);

    bool operator==(const VolumeDesc &rhs) const;

    bool operator!=(const VolumeDesc &rhs) const;
    VolumeDesc & operator=(const VolumeDesc& voldesc);

    bool isSnapshot() const;
    bool isClone() const;
    fds_volid_t getSrcVolumeId() const;
    fds_volid_t getLookupVolumeId() const;
    bool isSystemVolume() const;

    friend std::ostream& operator<<(std::ostream& out, const VolumeDesc& vol_desc);
    FDS_ProtocolInterface::ResourceState getState() const {
        return state;
    }

    void setState(FDS_ProtocolInterface::ResourceState state) {
        this->state = state;
    }

    inline fpi::VolumeGroupCoordinatorInfo getCoordinatorInfo() const {
        return coordinator;
    }
    inline fpi::SvcUuid getCoordinatorId() const {
        return coordinator.id;
    }
    inline int32_t getCoordinatorVersion() const {
        return coordinator.version;
    }
    inline void setCoordinatorId(fpi::SvcUuid id) {
        coordinator.id = id;
    }
    inline void setCoordinatorVersion(int32_t version) {
        coordinator.version = version;
    }
    inline bool isCoordinatorSet() const {
        return coordinator.id.svc_uuid != 0;
    }
};

/**
 * Basic volume descriptor class
 * TODO(Andrew): Combine with VolumeDesc...we
 * don't need both.
 */
class FDS_Volume {
  public:
    VolumeDesc   *voldesc;
    fds_int64_t  real_iops_throttle;
    fds_int64_t  real_iops_assured;

    FDS_Volume();
    explicit FDS_Volume(const VolumeDesc& vol_desc);
    inline bool isSystemVolume() {
        if (voldesc)
            return voldesc->isSystemVolume();
        else
            return false;
    }
    ~FDS_Volume();
};

/**
 * Volume QoS policy settings.
 * TODO(Andrew): Rename to something QoS specific since
 * that's what these really are...
 */

class FDS_VolumePolicy : public serialize::Serializable {
  public:
    fds_uint32_t   volPolicyId;
    std::string    volPolicyName;
    fds_int64_t    iops_throttle;
    fds_int64_t    iops_assured;
    fds_uint64_t   thruput;       // in MByte/sec
    fds_uint32_t   relativePrio;  // Relative priority

    FDS_VolumePolicy();
    ~FDS_VolumePolicy();

    uint32_t virtual write(serialize::Serializer*  s) const;
    uint32_t virtual read(serialize::Deserializer* d);
    uint32_t virtual getEstimatedSize() const;
    friend std::ostream& operator<<(std::ostream& os, const FDS_VolumePolicy& policy);
};


typedef enum {
        FDS_VOL_Q_INACTIVE,
        FDS_VOL_Q_SUSPENDED,
        FDS_VOL_Q_QUIESCING,
        FDS_VOL_Q_ACTIVE,
        FDS_VOL_Q_STOP_DEQUEUE
}VolumeQState;

/* **********************************************
 *  FDS_VolumeQueue: VolumeQueue
 *
 **********************************************************/
class FDS_VolumeQueue {
  public:
        boost::lockfree::queue<FDS_IOType*>  *volQueue;  // NOLINT
        VolumeQState volQState;

        // Qos Parameters set for this volume/VolumeQueue
        fds_int64_t iops_throttle;
        fds_int64_t iops_assured;
        fds_uint32_t priority;  // Relative priority

        // Ctor/dtor
        FDS_VolumeQueue(fds_uint32_t q_capacity,
                        fds_int64_t _iops_throttle,
                        fds_int64_t _iops_assured,
                        fds_uint32_t prio);

        ~FDS_VolumeQueue();

        void modifyQosParams(fds_int64_t _iops_assured,
                             fds_int64_t _iops_throttle,
                             fds_uint32_t _prio);

        void activate();
        void stopDequeue();


        // Quiesce queued IOs on this queue & block any new IOs
        void  quiesceIOs();
        void   suspendIO();

        void   resumeIO();

        inline fds_uint32_t count() const {
            return count_;
        }

        Error enqueueIO(FDS_IOType *io);
        FDS_IOType   *dequeueIO();

  private:
        std::atomic<fds_uint32_t> count_;

        INTRUSIVE_PTR_DEFS(FDS_VolumeQueue, refcnt_);
    };

// Define streaming operator for volume ids
inline std::ostream& operator<<(std::ostream& out, const fds_volid_t& vol_id) {
    boost::io::ios_flags_saver ifs(out);
    return out << std::dec << vol_id.get();
}

}  // namespace fds

// Define a hash function for fds_volid_t
namespace std {
    template<>
    struct hash<fds::fds_volid_t> {
        std::size_t operator()(fds::fds_volid_t const& val) const
            { return std::hash<fds_uint64_t>()(val.get()); }
    };
}  // namespace std

#endif  // SOURCE_INCLUDE_FDS_VOLUME_H_
