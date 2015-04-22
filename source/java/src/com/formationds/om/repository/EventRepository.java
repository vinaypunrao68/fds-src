/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */
package com.formationds.om.repository;

import com.formationds.commons.crud.CRUDRepository;
import com.formationds.commons.events.FirebreakType;
import com.formationds.commons.model.Volume;
import com.formationds.commons.model.entity.Event;
import com.formationds.commons.model.entity.FirebreakEvent;
import com.formationds.commons.model.entity.UserActivityEvent;
import com.formationds.om.repository.query.QueryCriteria;

import java.util.List;

public interface EventRepository extends CRUDRepository<Event, Long> {


    /**
     *
     * @param queryCriteria the query criteria
     * @param tenantUsers the list of tenant users to search
     * @return the list of events matching the query criteria and belonging to the specified tenant users.
     */
    List<UserActivityEvent> queryTenantUsers( QueryCriteria queryCriteria, List<Long> tenantUsers );

    /**
     *
     * @param v the volume
     * @param type the type of firebreak
     *
     * @return the latest firebreak for the specified volume.s
     */
    FirebreakEvent findLatestFirebreak( Volume v, FirebreakType type );
}
