/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_PLATFORM_DOMAIN_CLUSTER_MAP_H_
#define SOURCE_INCLUDE_PLATFORM_DOMAIN_CLUSTER_MAP_H_

namespace fds
{
    // -------------------------------------------------------------------------------------
    // Node Inventory and Cluster Map
    // -------------------------------------------------------------------------------------
    class DomainClusterMap : public DomainContainer
    {
        public:
            typedef boost::intrusive_ptr<DomainClusterMap> pointer;

            virtual ~DomainClusterMap();
            explicit DomainClusterMap(char const *const name);
            DomainClusterMap(char const *const name, OmAgent::pointer master,
                             SmContainer::pointer sm, DmContainer::pointer dm,
                             AmContainer::pointer am, PmContainer::pointer pm,
                             OmContainer::pointer om);
    };

}  // namespace fds

#endif  // SOURCE_INCLUDE_PLATFORM_DOMAIN_CLUSTER_MAP_H_
