/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <fds_volume.h>
#include <string>
#include <queue>
#include <fds_typedefs.h>

namespace fds {

VolumeDesc::VolumeDesc(const fpi::FDSP_VolumeDescType& volinfo,
                       fds_volid_t vol_uuid) {
    name = volinfo.vol_name;
    tennantId = volinfo.tennantId;
    localDomainId = volinfo.localDomainId;
    volUUID = vol_uuid;
    volType = volinfo.volType;
    maxObjSizeInBytes = volinfo.maxObjSizeInBytes;
    capacity = volinfo.capacity;
    volPolicyId = volinfo.volPolicyId;
    placementPolicy = volinfo.placementPolicy;
    mediaPolicy = volinfo.mediaPolicy;
    iops_min = 0;
    iops_max = 0;
    iops_guarantee = 0;
    relativePrio = 0;
    if (volUUID == invalid_vol_id) {
        GLOGWARN << "volume id is invalid";
    }
    fSnapshot = volinfo.fSnapshot;
    contCommitlogRetention = volinfo.contCommitlogRetention;
    timelineTime = volinfo.timelineTime;
    createTime   = volinfo.createTime;
}

VolumeDesc::VolumeDesc(const VolumeDesc& vdesc) {
    name = vdesc.name;
    tennantId = vdesc.tennantId;
    localDomainId = vdesc.localDomainId;
    volUUID = vdesc.volUUID;
    volType = vdesc.volType;
    maxObjSizeInBytes = vdesc.maxObjSizeInBytes;
    capacity = vdesc.capacity;
    volPolicyId = vdesc.volPolicyId;
    placementPolicy = vdesc.placementPolicy;
    mediaPolicy = vdesc.mediaPolicy;
    iops_min = vdesc.iops_min;
    iops_max = vdesc.iops_max;
    iops_guarantee = vdesc.iops_guarantee;
    relativePrio = vdesc.relativePrio;
    fSnapshot = vdesc.fSnapshot;
    state = vdesc.state;
    contCommitlogRetention = vdesc.contCommitlogRetention;
    timelineTime = vdesc.timelineTime;
    if (volUUID == invalid_vol_id) {
        GLOGWARN << "volume id is invalid";
    }
}

VolumeDesc::VolumeDesc(const fpi::FDSP_VolumeDescType& voldesc) {
    name = voldesc.vol_name;
    tennantId = voldesc.tennantId;
    localDomainId = voldesc.localDomainId;
    volUUID = voldesc.volUUID;
    volType = voldesc.volType;
    maxObjSizeInBytes = voldesc.maxObjSizeInBytes;
    capacity = voldesc.capacity;
    volPolicyId = voldesc.volPolicyId;
    placementPolicy = voldesc.placementPolicy;
    mediaPolicy = voldesc.mediaPolicy;
    iops_min = voldesc.iops_min;
    iops_max = voldesc.iops_max;
    iops_guarantee = voldesc.iops_guarantee;
    relativePrio = voldesc.rel_prio;
    fSnapshot = voldesc.fSnapshot;
    state = voldesc.state;
    contCommitlogRetention = voldesc.contCommitlogRetention;
    timelineTime = voldesc.timelineTime;
    createTime  = voldesc.createTime;
    if (volUUID == invalid_vol_id) {
        GLOGWARN << "volume id is invalid";
    }
}

/*
 * Used for testing where we don't have all of these fields.
 */
VolumeDesc::VolumeDesc(const std::string& _name, fds_volid_t _uuid)
        : name(_name),
          volUUID(_uuid) {
    if (_uuid == invalid_vol_id) {
        GLOGWARN << "volume id is invalid";
    }

    tennantId = 0;
    localDomainId = 0;
    volType = fpi::FDSP_VOL_S3_TYPE;
    maxObjSizeInBytes = 0;
    capacity = 0;
    volPolicyId = 0;
    placementPolicy = 0;
    mediaPolicy = fpi::FDSP_MEDIA_POLICY_HDD;
    iops_min = 0;
    iops_max = 0;
    iops_guarantee = 0;
    fSnapshot = false;
    relativePrio = 0;
    contCommitlogRetention = 0;
    timelineTime = 0;
    if (volUUID == invalid_vol_id) {
        GLOGWARN << "volume id is invalid";
    }
}

VolumeDesc::VolumeDesc(const std::string& _name,
                       fds_volid_t _uuid,
                       fds_uint32_t _iops_min,
                       fds_uint32_t _iops_max,
                       fds_uint32_t _priority)
        : name(_name),
          volUUID(_uuid),
          iops_min(_iops_min),
          iops_max(_iops_max),
          relativePrio(_priority) {
    if (volUUID == invalid_vol_id) {
        GLOGWARN << "volume id is invalid";
    }

    tennantId = 0;
    localDomainId = 0;
    volType = fpi::FDSP_VOL_S3_TYPE;
    maxObjSizeInBytes = 0;
    capacity = 0;
    volPolicyId = 0;
    placementPolicy = 0;
    fSnapshot = false;
    mediaPolicy = fpi::FDSP_MEDIA_POLICY_HDD;
    contCommitlogRetention = 0;
    timelineTime = 0;
    createTime = 0;
}

VolumeDesc::~VolumeDesc() {
}

void VolumeDesc::modifyPolicyInfo(fds_uint64_t _iops_min,
                                  fds_uint64_t _iops_max,
                                  fds_uint32_t _priority)
{
    iops_min = _iops_min;
    iops_max = _iops_max;
    relativePrio = _priority;
}

std::string VolumeDesc::getName() const {
    return name;
}

fds_volid_t VolumeDesc::GetID() const {
    return volUUID;
}

double VolumeDesc::getIopsMin() const {
    return iops_min;
}

double VolumeDesc::getIopsMax() const {
    return iops_max;
}

int VolumeDesc::getPriority() const {
    return relativePrio;
}

std::string VolumeDesc::ToString() {
    return (std::string("Vol<") + getName() +
            std::string(":") + std::to_string(GetID()) +
            std::string(">"));
}

bool VolumeDesc::operator==(const VolumeDesc &rhs) const {
    return (this->volUUID == rhs.volUUID);
}

bool VolumeDesc::operator!=(const VolumeDesc &rhs) const {
    return !(*this == rhs);
}

VolumeDesc& VolumeDesc::operator=(const VolumeDesc& volinfo) {
    this->name = volinfo.name;
    this->tennantId = volinfo.tennantId;
    this->localDomainId = volinfo.localDomainId;
    this->volUUID = volinfo.volUUID;
    this->volType = volinfo.volType;
    this->maxObjSizeInBytes = volinfo.maxObjSizeInBytes;
    this->capacity = volinfo.capacity;

    this->volPolicyId = volinfo.volPolicyId;
    this->placementPolicy = volinfo.placementPolicy;
    this->mediaPolicy = volinfo.mediaPolicy;
    this->fSnapshot = volinfo.fSnapshot;
    this->contCommitlogRetention = volinfo.contCommitlogRetention;
    this->timelineTime = volinfo.timelineTime;
    this->createTime = volinfo.createTime;
    return *this;
}

bool VolumeDesc::isSnapshot() const { return fSnapshot; }

bool VolumeDesc::isClone() const {
    return srcVolumeId > invalid_vol_id && !fSnapshot;
}

fds_volid_t VolumeDesc::getSrcVolumeId() const {
    return srcVolumeId;
}

fds_volid_t VolumeDesc::getLookupVolumeId() const {
    return lookupVolumeId;
}

std::ostream& operator<<(std::ostream& os, const VolumeDesc& vol) {
    return os << "["
              << " uuid:" << vol.volUUID
              << " name:" << vol.name
              << " tenant:" << vol.tennantId
              << " localdomain:" <<vol.localDomainId
              << " type:" << vol.volType
              << " max.obj.size.bytes:" << vol.maxObjSizeInBytes
              << " capacity:" << vol.capacity
              << " vol.policy.id:" << vol.volPolicyId
              << " media.policy:" << vol.mediaPolicy
              << " placement.policy:" << vol.placementPolicy
              << " iops.min:" << vol.iops_min
              << " iops.max:" << vol.iops_max
              << " iops.guarantee:" << vol.iops_guarantee
              << " rel.prio:" << vol.relativePrio
              << " isSnapshot:" << vol.fSnapshot
              << " state:" << vol.getState()
              << " qosQueueId:" << vol.contCommitlogRetention
              << " timelineTime:" << vol.timelineTime
              << " createTime:" << vol.createTime
              << " statename:" << fpi::_ResourceState_VALUES_TO_NAMES.find(vol.getState())->second
              << " ]";
}

/************************************************************************************/

FDS_Volume::FDS_Volume()
{
}

FDS_Volume::FDS_Volume(const VolumeDesc& vol_desc)
        : FDS_Volume()
{
    voldesc = new VolumeDesc(vol_desc);
}

FDS_Volume::~FDS_Volume() {
    delete voldesc;
}


FDS_VolumePolicy::FDS_VolumePolicy() {
}

FDS_VolumePolicy::~FDS_VolumePolicy() {
}

uint32_t FDS_VolumePolicy::write(serialize::Serializer* s) const {
    uint32_t b = 0;
    b += s->writeI32(volPolicyId);
    b += s->writeI64(iops_max);
    b += s->writeI64(iops_min);
    b += s->writeI64(thruput);
    b += s->writeI32(relativePrio);
    b += s->writeString(volPolicyName);
    return b;
}

uint32_t FDS_VolumePolicy::read(serialize::Deserializer* d) {
    uint32_t b = 0;
    b += d->readI32(volPolicyId);
    b += d->readI64(iops_max);
    b += d->readI64(iops_min);
    b += d->readI64(thruput);
    b += d->readI32(relativePrio);
    b += d->readString(volPolicyName);
    return b;
}

uint32_t FDS_VolumePolicy::getEstimatedSize() const {
    return 2*4 + 3*8 + 4 + volPolicyName.length();
}

std::ostream& operator<<(std::ostream& os, const FDS_VolumePolicy& policy) {
    os << "["
       << " id:" << policy.volPolicyId
       << " name:" << policy.volPolicyName
       << " iops.max:" << policy.iops_max
       << " iops.min:" << policy.iops_min
       << " thruput:" << policy.thruput
       << " relative.prio:" << policy.relativePrio
       << " ]";
    return os;
}

/***********************************************************************************/

FDS_VolumeQueue::FDS_VolumeQueue(fds_uint32_t q_capacity,
                                 fds_uint64_t _iops_max,
                                 fds_uint64_t _iops_min,
                                 fds_uint32_t prio) :
        iops_max(_iops_max),
        iops_min(_iops_min),
        priority(prio),
        count_(0) {
    volQueue = new boost::lockfree::queue<FDS_IOType *> (q_capacity);
    volQState = FDS_VOL_Q_INACTIVE;
}

FDS_VolumeQueue::~FDS_VolumeQueue() {
    delete volQueue;
}

void FDS_VolumeQueue::modifyQosParams(fds_uint64_t _iops_min,
                                      fds_uint64_t _iops_max,
                                      fds_uint32_t _prio)
{
    iops_max = _iops_max;
    iops_min = _iops_min;
    priority = _prio;
}

void FDS_VolumeQueue::activate() {
    volQState = FDS_VOL_Q_ACTIVE;
}

void FDS_VolumeQueue::stopDequeue() {
    volQState = FDS_VOL_Q_STOP_DEQUEUE;
}


// Quiesce queued IOs on this queue & block any new IOs
void  FDS_VolumeQueue::quiesceIOs() {
    volQState = FDS_VOL_Q_QUIESCING;
}

void   FDS_VolumeQueue::suspendIO() {
    volQState = FDS_VOL_Q_SUSPENDED;
}

void   FDS_VolumeQueue::resumeIO() {
    volQState = FDS_VOL_Q_ACTIVE;
}

void FDS_VolumeQueue::enqueueIO(FDS_IOType *io) {
    if (volQState == FDS_VOL_Q_ACTIVE || volQState == FDS_VOL_Q_STOP_DEQUEUE) {
        while (!volQueue->push(io)){}
        ++count_;
    }
}

FDS_IOType   *FDS_VolumeQueue::dequeueIO() {
    FDS_IOType *io = NULL;
    if ( volQState == FDS_VOL_Q_ACTIVE || volQState == FDS_VOL_Q_QUIESCING ) {
        volQueue->pop(io);
        if (volQState == FDS_VOL_Q_QUIESCING && volQueue->empty())  {
            volQState = FDS_VOL_Q_SUSPENDED;
        }
        --count_;
        return io;
    }
    return NULL;
}

}  // namespace fds
