/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_PLATFORM_DOMAIN_RESOURCES_H_
#define SOURCE_INCLUDE_PLATFORM_DOMAIN_RESOURCES_H_

#include "dlt.h"

namespace fds
{
    // -------------------------------------------------------------------------------------
    // Common Platform Resources
    // -------------------------------------------------------------------------------------
    class DomainResources
    {
        public:
            typedef boost::intrusive_ptr<DomainResources> pointer;
            typedef boost::intrusive_ptr<const DomainResources> const_ptr;

            virtual ~DomainResources();
            explicit DomainResources(char const *const name);

        protected:
            ResourceUUID            drs_tent_id;
            ResourceUUID            drs_domain_id;

            const DLT              *drs_dlt;
            DLTManager              drs_dltMgr;
            float                   drs_cur_throttle_lvl;

            // Will use new DMT
            int                     drs_dmt_version;
            fpi::Node_Table_Type    drs_dmt;

        private:
            INTRUSIVE_PTR_DEFS(DomainResources, rs_refcnt);
    };
}  // namespace fds

#endif  // SOURCE_INCLUDE_PLATFORM_DOMAIN_RESOURCES_H_
