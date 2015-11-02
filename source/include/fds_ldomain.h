/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

/*
 * Basic include information for a Local Domain.
 */

#ifndef SOURCE_INCLUDE_FDS_LDOMAIN_H_
#define SOURCE_INCLUDE_FDS_LDOMAIN_H_

#include <string>

#include <fdsp/config_types_types.h>
#include <bits/shared_ptr.h>
#include <shared/fds_types.h>   // For fds_uint64_t.
#include <fdsp/FDSP_types.h>    // For FDSP_RegisterNodeType.

namespace fds {

using fds_ldomid_t = std::int32_t;

static constexpr fds_ldomid_t invalid_ldom_id{0};
static constexpr fds_ldomid_t master_ldom_id{1};

/**
 * The Local Domain class captures the details of a Local Domain.
 */
class LocalDomain final {
    public:
        LocalDomain();

        // Used to capture LocalDomain details from a source other than the ConfigDB, such as user input.
        LocalDomain(const std::string& name,
                    const std::string& site,
                    const std::shared_ptr<FDS_ProtocolInterface::FDSP_RegisterNodeType> omNode = nullptr);

        // Used to construct a LocalDomain instance when not all details are known.
        explicit LocalDomain(const std::string& name, fds_ldomid_t id = invalid_ldom_id);

        LocalDomain(const LocalDomain& localDomain);  // NOLINT

        ~LocalDomain() = default;

        bool operator==(const LocalDomain& rhs) const;
        bool operator!=(const LocalDomain& rhs) const;

        LocalDomain& operator=(const LocalDomain& localDomain);

        friend std::ostream& operator<<(std::ostream& out, const LocalDomain& localDomain);

        // Accessors
        void setName(const std::string& name) {
            this->name = name;
        }
        const std::string& getName() const {
            return this->name;
        }

        void setID(const fds_ldomid_t id) {
            this->id = id;
        }
        fds_ldomid_t getID() const {
            return this->id;
        }

        void setSite(const std::string& site) {
            this->site = site;
        }
        const std::string& getSite() const {
            return this->site;
        }

        void setCreateTime(const fds_uint64_t createTime) {
            this->createTime = createTime;
        }
        fds_uint64_t getCreateTime() const {
            return this->createTime;
        }

        void setCurrent(const bool current) {
            this->current = current;
        }
        bool getCurrent() const {
            return this->current;
        }

        void setOMNode(const std::shared_ptr<FDS_ProtocolInterface::FDSP_RegisterNodeType> omNode) {
            this->omNode = omNode;
        }
        std::shared_ptr<FDS_ProtocolInterface::FDSP_RegisterNodeType> getOMNode() const {
            return this->omNode;
        }

        // Serialization.
        std::string ToString();

        static void makeLocalDomain(LocalDomain& subscription, const apis::LocalDomainDescriptor& localDomainDesc);
        static void makeLocalDomainDescriptor(apis::LocalDomainDescriptor& localDomainDesc, const LocalDomain& localDomain);

        bool isCurrent() const {
            return getCurrent();
        }

        bool isMaster() const {
            return (this->id == master_ldom_id);
        }

    private:
        fds_ldomid_t            id;  // ID of the local domain. Unique within the global domain.

        std::string             name;  // Also constitutes a unique identifier within the global domain.

        std::string             site;  // Description of local domain location or usage. That's the intention, anyway.
        fds_uint64_t            createTime;
        bool                    current;  // 'true' if this instance represents the current local domain.
        std::shared_ptr<FDS_ProtocolInterface::FDSP_RegisterNodeType> omNode;  // When "current" is 'false', this is the connectivity
                                                                               // information for the OM node of this "remote" local domain.

};


}  // namespace fds

#endif  // SOURCE_INCLUDE_FDS_LDOMAIN_H_
