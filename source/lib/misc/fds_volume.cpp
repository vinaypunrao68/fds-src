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
    iops_assured = 0;
    iops_throttle = 0;
    relativePrio = 0;
    if (volUUID == invalid_vol_id) {
        GLOGWARN << "volume id is invalid";
    }
    fSnapshot = volinfo.fSnapshot;
    srcVolumeId = volinfo.srcVolumeId;
    contCommitlogRetention = volinfo.contCommitlogRetention;
    timelineTime = volinfo.timelineTime;
    createTime   = volinfo.createTime;
    state   = volinfo.state;
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
    iops_assured = vdesc.iops_assured;
    iops_throttle = vdesc.iops_throttle;
    relativePrio = vdesc.relativePrio;
    fSnapshot = vdesc.fSnapshot;
    srcVolumeId = vdesc.srcVolumeId;
    state = vdesc.state;
    contCommitlogRetention = vdesc.contCommitlogRetention;
    timelineTime = vdesc.timelineTime;
    if (volUUID == invalid_vol_id) {
        GLOGWARN << "volume id is invalid";
    }
}

// NOTE: counterpart of outputting toFdspDesc
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
    iops_assured = voldesc.iops_assured;
    iops_throttle = voldesc.iops_throttle;
    relativePrio = voldesc.rel_prio;
    fSnapshot = voldesc.fSnapshot;
    srcVolumeId = voldesc.srcVolumeId;
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
    iops_assured = 0;
    iops_throttle = 0;
    fSnapshot = false;
    srcVolumeId = invalid_vol_id;
    relativePrio = 0;
    contCommitlogRetention = 0;
    timelineTime = 0;
    if (volUUID == invalid_vol_id) {
        GLOGWARN << "volume id is invalid";
    }
}

VolumeDesc::VolumeDesc(const std::string& _name,
                       fds_volid_t _uuid,
                       fds_int64_t _iops_assured,
                       fds_int64_t _iops_throttle,
                       fds_uint32_t _priority)
        : name(_name),
          volUUID(_uuid),
          iops_assured(_iops_assured),
          iops_throttle(_iops_throttle),
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
    srcVolumeId = invalid_vol_id;
    mediaPolicy = fpi::FDSP_MEDIA_POLICY_HDD;
    contCommitlogRetention = 0;
    timelineTime = 0;
    createTime = 0;
}

VolumeDesc::~VolumeDesc() {
}

void VolumeDesc::modifyPolicyInfo(fds_int64_t _iops_assured,
                                  fds_int64_t _iops_throttle,
                                  fds_uint32_t _priority)
{
    iops_assured = _iops_assured;
    iops_throttle = _iops_throttle;
    relativePrio = _priority;
}

std::string VolumeDesc::getName() const {
    return name;
}

fds_volid_t VolumeDesc::GetID() const {
    return volUUID;
}

fds_int64_t VolumeDesc::getIopsAssured() const {
    return iops_assured;
}

fds_int64_t VolumeDesc::getIopsThrottle() const {
    return iops_throttle;
}

int VolumeDesc::getPriority() const {
    return relativePrio;
}

std::string VolumeDesc::ToString() {
    return (std::string("Vol<") + getName() +
            std::string(":") + std::to_string(GetID().get()) +
            std::string(">"));
}

void VolumeDesc::toFdspDesc(FDS_ProtocolInterface::FDSP_VolumeDescType& voldesc) {
    voldesc.vol_name = name;
    voldesc.tennantId = tennantId;
    voldesc.localDomainId = localDomainId;
    voldesc.volUUID = volUUID.v;
    voldesc.volType = volType;
    voldesc.maxObjSizeInBytes = maxObjSizeInBytes;
    voldesc.capacity = capacity;
    voldesc.volPolicyId = volPolicyId;
    voldesc.placementPolicy = placementPolicy;
    voldesc.mediaPolicy = mediaPolicy;
    voldesc.iops_assured = iops_assured;
    voldesc.iops_throttle = iops_throttle;
    voldesc.rel_prio = relativePrio;
    voldesc.fSnapshot = fSnapshot;
    voldesc.state = state;
    voldesc.contCommitlogRetention = contCommitlogRetention;
    voldesc.srcVolumeId = srcVolumeId.v;
    voldesc.timelineTime = timelineTime;
    voldesc.createTime = createTime;
    voldesc.state = state;
}

bool VolumeDesc::operator==(const VolumeDesc &rhs) const {
    return (this->volUUID == rhs.volUUID);
}

bool VolumeDesc::operator!=(const VolumeDesc &rhs) const {
    return !(*this == rhs);
}

VolumeDesc& VolumeDesc::operator=(const VolumeDesc& volinfo) {
    if (this != &volinfo) {
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
        this->iops_assured = volinfo.iops_assured;
        this->iops_throttle = volinfo.iops_throttle;
        this->relativePrio = volinfo.relativePrio;
        this->fSnapshot = volinfo.fSnapshot;
        this->srcVolumeId = volinfo.srcVolumeId;
        this->contCommitlogRetention = volinfo.contCommitlogRetention;
        this->timelineTime = volinfo.timelineTime;
        this->state = volinfo.state;
    }
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

bool VolumeDesc::isSystemVolume() const {
    return 0 == name.compare(0,7,"SYSTEM_",0,7);
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
              << " iops.assured:" << vol.iops_assured
              << " iops.throttle:" << vol.iops_throttle
              << " rel.prio:" << vol.relativePrio
              << " isSnapshot:" << vol.fSnapshot
              << " srcVolumeId:" << vol.srcVolumeId
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
    b += s->writeI64(iops_throttle);
    b += s->writeI64(iops_assured);
    b += s->writeI64(thruput);
    b += s->writeI32(relativePrio);
    b += s->writeString(volPolicyName);
    return b;
}

uint32_t FDS_VolumePolicy::read(serialize::Deserializer* d) {
    uint32_t b = 0;
    b += d->readI32(volPolicyId);
    b += d->readI64(iops_throttle);
    b += d->readI64(iops_assured);
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
       << " iops.throttle:" << policy.iops_throttle
       << " iops.assured:" << policy.iops_assured
       << " thruput:" << policy.thruput
       << " relative.prio:" << policy.relativePrio
       << " ]";
    return os;
}

/***********************************************************************************/

FDS_VolumeQueue::FDS_VolumeQueue(fds_uint32_t q_capacity,
                                 fds_int64_t _iops_throttle,
                                 fds_int64_t _iops_assured,
                                 fds_uint32_t prio) :
        iops_throttle(_iops_throttle),
        iops_assured(_iops_assured),
        priority(prio),
        count_(0) {
    volQueue = new boost::lockfree::queue<FDS_IOType *> (q_capacity);
    volQState = FDS_VOL_Q_INACTIVE;
}

FDS_VolumeQueue::~FDS_VolumeQueue() {
    if (volQueue) {
        delete volQueue;
    }
}

void FDS_VolumeQueue::modifyQosParams(fds_int64_t _iops_assured,
                                      fds_int64_t _iops_throttle,
                                      fds_uint32_t _prio)
{
    iops_throttle = _iops_throttle;
    iops_assured = _iops_assured;
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

Error FDS_VolumeQueue::enqueueIO(FDS_IOType *io) {
    if (volQState == FDS_VOL_Q_ACTIVE || volQState == FDS_VOL_Q_STOP_DEQUEUE) {
        while (!volQueue->push(io)){}
        ++count_;
	return ERR_OK;
    }
    return ERR_NOT_READY;
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
