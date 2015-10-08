/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
#include <fds_subscription.h>
#include <queue>
#include <fds_typedefs.h>

namespace fds {

Subscription::Subscription(const std::string    name,
                           const fds_subid_t    id,
                           const int            tenantID,
                           const int            primaryDomainID,
                           const fds_volid_t    primaryVolumeID,
                           const int            replicaDomainID,
                           const apis::SubscriptionType type,
                           const apis::SubscriptionScheduleType scheduleType,
                           const fds_uint64_t   intervalSize) {
    if (id == invalid_sub_id) {
        GLOGWARN << "Subscription id is invalid.";
    }
    this->name = name;
    this->id = id;
    this->tenantID = tenantID;
    this->primaryDomainID = primaryDomainID;
    this->primaryVolumeID = primaryVolumeID;
    this->replicaDomainID = replicaDomainID;
    this->createTime = util::getTimeStampMillis();
    this->state = FDS_ProtocolInterface::ResourceState::Created;
    this->type = type;
    this->scheduleType = scheduleType;
    this->intervalSize = intervalSize;
}

Subscription::Subscription(const Subscription& subscription) {
    this->name = subscription.name;
    this->id = subscription.id;
    this->tenantID = subscription.tenantID;
    this->primaryDomainID = subscription.primaryDomainID;
    this->primaryVolumeID = subscription.primaryVolumeID;
    this->replicaDomainID = subscription.replicaDomainID;
    this->createTime = subscription.createTime;
    this->state = subscription.state;
    this->type = subscription.type;
    this->scheduleType = subscription.scheduleType;
    this->intervalSize = subscription.intervalSize;
}

/*
 * Used for initializing an instance where we don't have all member variable values.
 */
Subscription::Subscription(const std::string& _name, fds_subid_t _id)
        : name(_name),
          id(_id) {
    if (_id == invalid_vol_id) {
        GLOGWARN << "subscription id is invalid";
    }

    this->tenantID = 0;
    this->primaryDomainID = 0;
    this->primaryVolumeID = 0;
    this->replicaDomainID = 0;
    this->createTime = 0;
    this->state = FDS_ProtocolInterface::ResourceState::Unknown;
    this->type = apis::SubscriptionType::TRANSACTION_NO_LOG;
    this->scheduleType = apis::SubscriptionScheduleType::NA;
    this->intervalSize = 0;
}

Subscription::~Subscription() {
}

std::string Subscription::ToString() {
    return (std::string("Subscription<") + getName() +
            std::string(":") + std::to_string(getID().get()) +
            std::string(">"));
}

bool Subscription::operator==(const Subscription &rhs) const {
    return (this->id == rhs.id);
}

bool Subscription::operator!=(const Subscription &rhs) const {
    return !(*this == rhs);
}

Subscription& Subscription::operator=(const Subscription& rhs) {
    this->name = rhs.name;
    this->id = rhs.id;
    this->tenantID = rhs.tenantID;
    this->primaryDomainID = rhs.primaryDomainID;
    this->primaryVolumeID = rhs.primaryVolumeID;
    this->replicaDomainID = rhs.replicaDomainID;
    this->createTime = rhs.createTime;
    this->state = rhs.state;
    this->type = rhs.type;
    this->scheduleType = rhs.scheduleType;
    this->intervalSize = rhs.intervalSize;
    return *this;
}

std::ostream& operator<<(std::ostream& os, const Subscription&subscription) {
    return os << "["
              << " name:" << subscription.name
              << " id:" << subscription.id
              << " tenant:" << subscription.tenantID
              << " primaryDomain:" << subscription.primaryDomainID
              << " primaryVolume:" << subscription.primaryVolumeID
              << " replicaDomain:" << subscription.replicaDomainID
              << " createTime:" << subscription.createTime
              << " state:" << subscription.state
              << " stateName:" << fpi::_ResourceState_VALUES_TO_NAMES.find(subscription.state)->second
              << " type:" << subscription.type
              << " typeName:" << apis::_SubscriptionType_VALUES_TO_NAMES.find(subscription.type)->second
              << " scheduleType:" << subscription.scheduleType
              << " scheduleTypeName:" << apis::_SubscriptionScheduleType_VALUES_TO_NAMES.find(subscription.scheduleType)->second
              << " intervalSize:" << subscription.intervalSize
              << " ]";
}

uint32_t Subscription::write(serialize::Serializer* s) const {
    uint32_t b = 0;
    b += s->writeI64(id.get());
    b += s->writeI32(tenantID);
    b += s->writeI32(primaryDomainID);
    b += s->writeI64(primaryVolumeID.get());
    b += s->writeI32(replicaDomainID);
    b += s->writeI64(createTime);
    b += s->writeI32(state);
    b += s->writeI32(type);
    b += s->writeI32(scheduleType);
    b += s->writeI64(intervalSize);
    b += s->writeString(name);
    return b;
}

uint32_t Subscription::read(serialize::Deserializer* d) {
    uint32_t b = 0;

    fds_uint64_t _id;
    b += d->readI64(_id);
    id = fds_subid_t(_id);

    b += d->readI32(tenantID);
    b += d->readI32(primaryDomainID);

    b += d->readI64(_id);
    primaryVolumeID = fds_volid_t(_id);

    b += d->readI32(replicaDomainID);
    b += d->readI64(createTime);

    int intVal;
    b += d->readI32(intVal);
    state = FDS_ProtocolInterface::ResourceState(intVal);

    b += d->readI32(intVal);
    type = apis::SubscriptionType(intVal);

    b += d->readI32(intVal);
    scheduleType = apis::SubscriptionScheduleType(intVal);

    b += d->readI64(intervalSize);
    b += d->readString(name);

    return b;
}

uint32_t Subscription::getEstimatedSize() const {
    return 6*4 + 4*8 + 4 + name.length();
}
}  // namespace fds
