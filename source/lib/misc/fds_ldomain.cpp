/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
#include <fds_ldomain.h>
#include <fds_typedefs.h>
#include <include/fdsp_utils.h>

namespace fds {

LocalDomain::LocalDomain() {
    this->id = invalid_ldom_id;

    this->name = "local";
    this->site = "local";
    this->createTime = util::getTimeStampMillis();
    this->current = false;
    // No OM nodes even though we set current to 'false'.
}

// Used to capture local domain details from a source other than the ConfigDB, such as user input.
LocalDomain::LocalDomain(const std::string& name,
                         const std::string& site,
                         const std::vector<fpi::FDSP_RegisterNodeType>& omNodes) {
    this->id = invalid_ldom_id;

    this->name = name;
    this->site = site;
    this->createTime = util::getTimeStampMillis();
    this->current = false;
    this->setOMNodes(omNodes);
}

LocalDomain::LocalDomain(const LocalDomain& localDomain) {
    this->name = localDomain.name;
    this->id = localDomain.id;
    this->site = localDomain.site;
    this->createTime = localDomain.createTime;
    this->current = localDomain.current;
    this->setOMNodes(localDomain.getOMNodes());
}

/*
 * Used for initializing an instance where we don't have all member variable values.
 */
LocalDomain::LocalDomain(const std::string& _name, fds_ldomid_t _id)
        : name(_name),
          id(_id) {
    this->site = "";
    this->createTime = 0;
    this->current = false;
    // No OM nodes even though we set current to 'false'.
}

std::string LocalDomain::ToString() {
    return (std::string("LocalDomain<") + getName() +
            std::string(":") + std::to_string(getID()) +
            std::string(">"));
}

bool LocalDomain::operator==(const LocalDomain &rhs) const {
    return (this->id == rhs.id);
}

bool LocalDomain::operator!=(const LocalDomain &rhs) const {
    return !(*this == rhs);
}

LocalDomain& LocalDomain::operator=(const LocalDomain& rhs) {
    this->name = rhs.name;
    this->id = rhs.id;
    this->site = rhs.site;
    this->createTime = rhs.createTime;
    this->current = rhs.current;
    this->setOMNodes(rhs.getOMNodes());

    return *this;
}

std::ostream& operator<<(std::ostream& os, const LocalDomain& localDomain) {
    return os << "["
              << " name:" << localDomain.name
              << " id:" << localDomain.id
              << " site:" << localDomain.site
              << " createTime:" << localDomain.createTime
              << " current:" << localDomain.current
              << " ]";
}

/**
 * Copy a LocalDomain instance into a LocalDomainDescriptor instance ready for Thrift.
 *
 * @param localDomainDesc - output
 * @param localDomain - input
 */
void LocalDomain::makeLocalDomainDescriptor(apis::LocalDomainDescriptor& localDomainDesc, const LocalDomain& localDomain) {
    localDomainDesc.id = localDomain.getID();
    localDomainDesc.name = localDomain.getName();
    localDomainDesc.site = localDomain.getSite();
    localDomainDesc.createTime = static_cast<std::int64_t>(localDomain.getCreateTime());  // See FS-3365.
    localDomainDesc.current = localDomain.getCurrent();

    auto newOMNodes(localDomain.getOMNodes());
    localDomainDesc.omNodes.swap(newOMNodes);
}

/**
 * Copy a LocalDomainDescriptor instance into a LocalDomain instance ready for Config DB or other uses.
 *
 * @param localDomain - output
 * @param localDomainDesc - input
 */
void LocalDomain::makeLocalDomain(LocalDomain& localDomain, const apis::LocalDomainDescriptor& localDomainDesc) {
    localDomain.setID(localDomainDesc.id);
    localDomain.setName(localDomainDesc.name);
    localDomain.setSite(localDomainDesc.site);
    localDomain.setCreateTime(static_cast<std::uint64_t>(localDomainDesc.createTime));  // See FS-3365.
    localDomain.setCurrent(localDomainDesc.current);
    localDomain.setOMNodes(localDomainDesc.omNodes);
}
}  // namespace fds
