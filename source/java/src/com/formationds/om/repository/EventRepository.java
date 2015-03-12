/**
 * Copyright (c) 2014 Formation Data Systems.  All rights reserved.
 */

package com.formationds.om.repository;

import com.formationds.commons.events.FirebreakType;
import com.formationds.commons.model.Volume;
import com.formationds.commons.model.entity.Event;
import com.formationds.commons.model.entity.FirebreakEvent;
import com.formationds.commons.model.entity.UserActivityEvent;
import com.formationds.om.repository.query.QueryCriteria;
import com.formationds.om.repository.query.builder.CriteriaQueryBuilder;

import javax.persistence.EntityManager;
import javax.persistence.criteria.Expression;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public interface EventRepository {

    public static class EventCriteriaQueryBuilder extends CriteriaQueryBuilder<Event> {

        // TODO: how to support multiple contexts (category and severity)?
        private static final String CONTEXT = "category";
        private static final String TIMESTAMP = "initialTimestamp";

        EventCriteriaQueryBuilder(EntityManager em) {
            super(em, TIMESTAMP, CONTEXT);
        }
    }

    public static class UserEventCriteriaQueryBuilder extends CriteriaQueryBuilder<UserActivityEvent> {

        // TODO: how to support multiple contexts (category and severity)?
        private static final String CONTEXT = "category";
        private static final String TIMESTAMP = "initialTimestamp";
        static final String USER_ID = "userId";

        UserEventCriteriaQueryBuilder(EntityManager em) {
            super(em, TIMESTAMP, CONTEXT);
        }

        protected UserEventCriteriaQueryBuilder usersIn(List<Long> in) {
            final Expression<String> expression = from.get( USER_ID );
            super.and(expression.in(in.toArray(new Long[in.size()])));
            return this;
        }
    }

    public static class FirebreakEventCriteriaQueryBuilder extends CriteriaQueryBuilder<FirebreakEvent> {

        // TODO: how to support multiple contexts (category and severity)?
        private static final String CONTEXT = "category";
        private static final String TIMESTAMP = "initialTimestamp";
        static final String VOLID = "volumeId";
        static final String VOLNAME = "volumeName";
        static final String FBTYPE = "firebreakType";

        FirebreakEventCriteriaQueryBuilder(EntityManager em) {
            super(em, TIMESTAMP, CONTEXT);
        }

        protected FirebreakEventCriteriaQueryBuilder volumeById(String v) {
            final Expression<?> expression = from.get( VOLID );
            super.and( cb.equal(expression, v) );
            return this;
        }

        protected FirebreakEventCriteriaQueryBuilder volumeByName(String v) {
            final Expression<?> expression = from.get( VOLNAME );
            super.and( cb.equal(expression, v) );
            return this;
        }

        protected FirebreakEventCriteriaQueryBuilder volumeByFBType(FirebreakType t) {
            final Expression<?> expression = from.get( FBTYPE );
            super.and( cb.equal(expression, t) );
            return this;
        }

        protected FirebreakEventCriteriaQueryBuilder volumesById(String... in) {
            return volumesById((in != null ? Arrays.asList( in ) : new ArrayList<>()));
        }

        protected FirebreakEventCriteriaQueryBuilder volumesById(List<String> in) {
            final Expression<String> expression = from.get( VOLID );
            super.and(expression.in(in.toArray(new String[in.size()])));
            return this;
        }

        protected FirebreakEventCriteriaQueryBuilder volumesByName(String... in) {
            return volumesByName((in != null ? Arrays.asList(in) : new ArrayList<>()));
        }

        protected FirebreakEventCriteriaQueryBuilder volumesByName(List<String> in) {
            final Expression<String> expression = from.get( VOLNAME );
            super.and(expression.in(in.toArray(new String[in.size()])));
            return this;
        }

        protected FirebreakEventCriteriaQueryBuilder fbType(FirebreakType type) {
            final Expression<String> expression = from.get( FBTYPE );
            super.and( cb.equal(expression, type) );
            return this;
        }
    }

    /**
     * Persist the event in the underlying storage
     * @param event the event
     * @return the event, with any auto-generated data filled in (id etc)
     */
    Event save( Event event );

    /**
     * Persist the list of events
     *
     * @param events the list of events
     *
     * @return the list per
     */
    List<Event> save( Event... events );

    List<? extends Event> query( QueryCriteria queryCriteria );

    List<? extends Event> queryTenantUsers( QueryCriteria queryCriteria, List<Long> tenantUsers );

    FirebreakEvent findLatestFirebreak( Volume v, FirebreakType type );
}
