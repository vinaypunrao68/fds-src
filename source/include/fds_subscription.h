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
#include <atomic>

#include <fds_error.h>
#include <fds_assert.h>
#include <fds_ptr.h>
#include <fds_value_type.h>
#include <fds_volume.h>
#include <has_state.h>

#include <boost/date_time/posix_time/posix_time.hpp>

#include <boost/thread/thread.hpp>
#include <boost/lockfree/queue.hpp>
#include <boost/atomic.hpp>
#include <boost/io/ios_state.hpp>
#include <serialize.h>
#include <fdsp/config_types_types.h>

namespace fds {

using fds_subid_t = fds_value_type<uint64_t>;

static fds_subid_t const invalid_sub_id(0);

/**
 * Basic class for a subscription. This is intended
 * to be a cachable/passable description of a subscription instance.
 * The authoritative subscription information is stored on the OM.
 */
class Subscription : public HasState,
                     public serialize::Serializable {
    public:
        Subscription() = delete;

        Subscription(const std::string  name,
                     const fds_subid_t  id,
                     const int          tenantID,
                     const int          primaryDomainID,
                     const fds_volid_t  primaryVolumeID,
                     const int          replicaDomainID,
                     const fds::apis::SubscriptionType type,
                     const fds::apis::SubscriptionScheduleType scheduleType = fds::apis::SubscriptionScheduleType::NA,
                     const fds_uint64_t intervalSize = 0);

        Subscription(const std::string& name, fds_subid_t id);

        Subscription(const Subscription& subscription);  // NOLINT

        ~Subscription();

        bool operator==(const Subscription& rhs) const;
        bool operator!=(const Subscription& rhs) const;

        Subscription & operator=(const Subscription& subscription);

        friend std::ostream& operator<<(std::ostream& out, const Subscription& subscription);

        // Accessors
        void setName(const std::string name) {
            this->name = name;
        }
        std::string getName() const {
            return name;
        }

        void setID(const fds_subid_t id) {
            this->id = id;
        }
        fds_subid_t getID() const {
            return id;
        }

        void setTenantID(const int tenantID) {
            this->tenantID = tenantID;
        }
        int getTenantID() const {
            return tenantID;
        }

        void setPrimaryDomainID(const int primaryDomainID) {
            this->primaryDomainID = primaryDomainID;
        }
        int getPrimaryDomainID() const {
            return primaryDomainID;
        }

        void setPrimaryVolumeID(const fds_volid_t primaryVolumeID) {
            this->primaryVolumeID = primaryVolumeID;
        }
        fds_volid_t getPrimaryVolumeID() const {
            return primaryVolumeID;
        }

        void setReplicaDomainID(const int replicaDomainID) {
            this->replicaDomainID = replicaDomainID;
        }
        int getReplicaDomainID() const {
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

        void setIntervalSize(const fds_uint64_t intervalSize) {
            this->intervalSize = intervalSize;
        }
        fds_uint64_t getIntervalSize() const {
            return intervalSize;
        }

        // Serialization.
        std::string ToString();

        uint32_t virtual write(serialize::Serializer*  s) const;
        uint32_t virtual read(serialize::Deserializer* d);
        uint32_t virtual getEstimatedSize() const;

    private:
        // Basic ID information.
        std::string             name;    // Unique within the global domain.
        fds_subid_t             id;  // ID of the subscription. Unique within the global domain.
        int                     tenantID;  // Tenant id that owns the subscription

        // The following 3 constitute a unique identifier.
        int                     primaryDomainID;  // ID of local domain that owns the primary volume
        fds_volid_t             primaryVolumeID;  // ID of primary volume. That is, ID of volume in the primary local domain
        int                     replicaDomainID;  // ID of local domain that owns the replica volume

        // Basic settings
        fds_uint64_t            createTime;
        FDS_ProtocolInterface::ResourceState     state;
        fds::apis::SubscriptionType type;   // Generally indicates whether the subscription is content or transaction based.
        fds::apis::SubscriptionScheduleType scheduleType;   // For content-based subscription types, the type of refresh scheduling policy. Also implies units. See the type definition.
        fds_uint64_t            intervalSize; // When scheduling, the "size" of the interval the expiration of which results in a primary snapshot pushed to the replica. Units according to scheduleType.

};


}  // namespace fds

#endif  // SOURCE_INCLUDE_FDS_SUBSCRIPTION_H_
