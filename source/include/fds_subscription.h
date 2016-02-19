/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

/*
 * Basic include information for a subscription
 * used with asynchronous replication.
 */

#ifndef SOURCE_INCLUDE_FDS_SUBSCRIPTION_H_
#define SOURCE_INCLUDE_FDS_SUBSCRIPTION_H_

#include <string>

#include <fds_volume.h>
#include <has_state.h>

#include <fdsp/config_types_types.h>

namespace fds {

using fds_subid_t = std::int64_t;

static constexpr fds_subid_t invalid_sub_id{0};

/**
 * The Subscription class captures the details for asynchronously replicating
 * a volume from a "primary" local domain to a "replica" local domain. While
 * Subscription state is created at and persisted in the master domain, it
 * may be cached for convenience at local domains.
 */
class Subscription final : public HasState {
    public:
        Subscription() = default;

        // Used to capture subscription details from a source other than the ConfigDB, such as user input.
        Subscription(const std::string  name,
                     const std::int64_t tenantID,
                     const std::int32_t primaryDomainID,
                     const fds_volid_t  primaryVolumeID,
                     const std::int32_t replicaDomainID,
                     const fds::apis::SubscriptionType type,
                     const fds::apis::SubscriptionScheduleType scheduleType = fds::apis::SubscriptionScheduleType::NA,
                     const std::int64_t intervalSize = 0);

        // Used to construct a Subscription instance when not all details are known.
        explicit Subscription(const std::string& name, fds_subid_t id = invalid_sub_id);

        Subscription(const Subscription& subscription);  // NOLINT

        ~Subscription() = default;

        bool operator==(const Subscription& rhs) const;
        bool operator!=(const Subscription& rhs) const;

        Subscription & operator=(const Subscription& subscription);

        friend std::ostream& operator<<(std::ostream& out, const Subscription& subscription);

        // Accessors
        void setName(const std::string& name) {
            this->name = name;
        }
        const std::string& getName() const {
            return name;
        }

        void setID(const fds_subid_t id) {
            this->id = id;
        }
        fds_subid_t getID() const {
            return id;
        }

        void setTenantID(const std::int64_t tenantID) {
            this->tenantID = tenantID;
        }
        std::int64_t getTenantID() const {
            return tenantID;
        }

        void setPrimaryDomainID(const std::int32_t primaryDomainID) {
            this->primaryDomainID = primaryDomainID;
        }
        std::int32_t getPrimaryDomainID() const {
            return primaryDomainID;
        }

        void setPrimaryVolumeID(const fds_volid_t primaryVolumeID) {
            this->primaryVolumeID = primaryVolumeID;
        }
        fds_volid_t getPrimaryVolumeID() const {
            return primaryVolumeID;
        }

        void setReplicaDomainID(const std::int32_t replicaDomainID) {
            this->replicaDomainID = replicaDomainID;
        }
        std::int32_t getReplicaDomainID() const {
            return replicaDomainID;
        }

        void setCreateTime(const fds_uint64_t createTime) {
            this->createTime = createTime;
        }
        fds_uint64_t getCreateTime() const {
            return createTime;
        }

        void setState(const FDS_ProtocolInterface::ResourceState state) {
            this->state = state;
        }
        FDS_ProtocolInterface::ResourceState getState() const {
            return state;
        }

        void setType(const fds::apis::SubscriptionType type) {
            this->type = type;
        }
        fds::apis::SubscriptionType getType() const {
            return type;
        }

        void setScheduleType(const fds::apis::SubscriptionScheduleType scheduleType) {
            this->scheduleType = scheduleType;
        }
        fds::apis::SubscriptionScheduleType getScheduleType() const {
            return scheduleType;
        }

        void setIntervalSize(const std::int64_t intervalSize) {
            this->intervalSize = intervalSize;
        }
        std::int64_t getIntervalSize() const {
            return intervalSize;
        }

        // Serialization.
        std::string ToString();

        static void makeSubscription(Subscription& subscription, const apis::SubscriptionDescriptor& subscriptionDesc);
        static void makeSubscriptionDescriptor(apis::SubscriptionDescriptor& subscriptionDesc, const Subscription& subscription);

    private:
        // Basic ID information.
        fds_subid_t             id;  // ID of the subscription. Unique within the global domain.

        // The following 2 consititute a unique identifier.
        std::string             name;    // Unique for the tenant within global domain.
        std::int64_t            tenantID;  // Tenant id that owns the subscription

        // The following 3 constitute a unique identifier (because volume is unique within tenant).
        std::int32_t            primaryDomainID;  // ID of local domain that owns the primary volume
        fds_volid_t             primaryVolumeID;  // ID of primary volume. That is, ID of volume in the primary local domain
        std::int32_t            replicaDomainID;  // ID of local domain that owns the replica volume

        fds_uint64_t            createTime;
        FDS_ProtocolInterface::ResourceState     state;
        fds::apis::SubscriptionType type;   // Generally indicates whether the subscription is content or transaction based.
        fds::apis::SubscriptionScheduleType scheduleType;   // For content-based subscription types, the type of refresh scheduling policy. Also implies units. See the type definition.
        std::int64_t            intervalSize; // When scheduling, the "size" of the interval the expiration of which results in a primary snapshot pushed to the replica. Units according to scheduleType.

};


}  // namespace fds

#endif  // SOURCE_INCLUDE_FDS_SUBSCRIPTION_H_
