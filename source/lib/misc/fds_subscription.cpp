/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
#include <fds_subscription.h>
#include <fds_typedefs.h>

namespace fds {

// Used to capture subscription details from a source other than the ConfigDB, such as user input.
Subscription::Subscription(const std::string    name,
                           const std::int64_t   tenantID,
                           const std::int32_t   primaryDomainID,
                           const fds_volid_t    primaryVolumeID,
                           const std::int32_t   replicaDomainID,
                           const apis::SubscriptionType type,
                           const apis::SubscriptionScheduleType scheduleType,
                           const std::int64_t   intervalSize) {
    this->id = invalid_sub_id;
    this->state = FDS_ProtocolInterface::ResourceState::Unknown;

    this->name = name;
    this->tenantID = tenantID;
    this->primaryDomainID = primaryDomainID;
    this->primaryVolumeID = primaryVolumeID;
    this->replicaDomainID = replicaDomainID;
    this->createTime = util::getTimeStampMillis();
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

std::string Subscription::ToString() {
    return (std::string("Subscription<") + getName() +
            std::string(":") + std::to_string(getID()) +
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
    b += s->writeI64(id);
    b += s->writeI64(tenantID);
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

    b += d->readI64(tenantID);
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
    return 5*4 + 5*8 + 4 + name.length();
}

/**
 * Copy a Subscription instance into a SubscriptionDescriptor instance ready for Thrift.
 *
 * @param subscriptionDesc - output
 * @param subscription - input
 */
void Subscription::makeSubscriptionDescriptor(apis::SubscriptionDescriptor& subscriptionDesc, const Subscription& subscription) {
    subscriptionDesc.id = subscription.getID();
    subscriptionDesc.name = subscription.getName();
    subscriptionDesc.tenantID = subscription.getTenantID();
    subscriptionDesc.primaryDomainID = subscription.getPrimaryDomainID();
    subscriptionDesc.primaryVolumeID = static_cast<std::int64_t>(subscription.getPrimaryVolumeID().get());
    subscriptionDesc.replicaDomainID = subscription.getReplicaDomainID();
    subscriptionDesc.createTime = static_cast<std::int64_t>(subscription.getCreateTime());  // See FS-3365.
    subscriptionDesc.state = subscription.getState();
    subscriptionDesc.scheduleType = subscription.getScheduleType();
    subscriptionDesc.intervalSize = subscription.getIntervalSize();
}

/**
 * Copy a SubscriptionDescriptor instance into a Subscription instance ready for Condig DB or other uses.
 *
 * @param subscription - output
 * @param subscriptionDesc - input
 */
void Subscription::makeSubscription(Subscription& subscription, const apis::SubscriptionDescriptor& subscriptionDesc) {
    subscription.setID(subscriptionDesc.id);
    subscription.setName(subscriptionDesc.name);
    subscription.setTenantID(subscriptionDesc.tenantID);
    subscription.setPrimaryDomainID(subscriptionDesc.primaryDomainID);
    subscription.setPrimaryVolumeID(fds_volid_t(static_cast<uint64_t>(subscriptionDesc.primaryVolumeID)));
    subscription.setReplicaDomainID(subscriptionDesc.replicaDomainID);
    subscription.setCreateTime(static_cast<std::uint64_t>(subscriptionDesc.createTime));  // See FS-3365.
    subscription.setState(subscriptionDesc.state);
    subscription.setScheduleType(subscriptionDesc.scheduleType);
    subscription.setIntervalSize(subscriptionDesc.intervalSize);
}
}  // namespace fds
